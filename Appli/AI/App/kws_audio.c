/**
  ******************************************************************************
  * @file    kws_audio.c
  * @brief   Streaming YAMNet-compatible log-mel frontend for keyword spotting.
  *
  * The generated model accepts one int8 [1,64,96,1] tensor.  The frontend
  * reads the 16 kHz, 24-bit I2S left channel and produces 96
  * log-mel frames (25 ms window, 10 ms step, 64 bands, 125..7500 Hz).
  ******************************************************************************
  */

#include "kws_audio.h"
#include "main.h"
#include "arm_math.h"

#include <math.h>
#include <string.h>

#define KWS_I2S_SLOTS_PER_FRAME 2U
#define KWS_FFT_SIZE            512U
#define KWS_WINDOW_SAMPLES      400U
#define KWS_HOP_SAMPLES         160U
#define KWS_WINDOW_SAMPLE_COUNT (((KWS_TIME_FRAMES - 1U) * KWS_HOP_SAMPLES) + KWS_WINDOW_SAMPLES)
#define KWS_INFERENCE_HOP       7680U
#define KWS_FFT_BIN_COUNT       ((KWS_FFT_SIZE / 2U) + 1U)
#define KWS_MEL_COEFFICIENT_CAPACITY 512U
#define KWS_MIN_FREQUENCY_HZ    125.0f
#define KWS_MAX_FREQUENCY_HZ    7500.0f
#define KWS_LOG_OFFSET          0.001f
#define KWS_INPUT_SCALE         STAI_NETWORK_IN_1_SCALES
#define KWS_INPUT_ZERO_POINT    44

static int16_t audio_ring[KWS_WINDOW_SAMPLE_COUNT];
static int16_t audio_window[KWS_WINDOW_SAMPLE_COUNT];
static uint32_t audio_write_index;
static uint32_t samples_since_inference;

static arm_rfft_fast_instance_f32 fft_instance;
static float fft_input[KWS_FFT_SIZE];
static float fft_packed_output[KWS_FFT_SIZE];
static float fft_magnitude[KWS_FFT_BIN_COUNT];
static float hann_window[KWS_WINDOW_SAMPLES];
static float mel_edges[KWS_MEL_BINS + 2U];
static float fft_bin_mel[KWS_FFT_BIN_COUNT];
static float mel_coefficients[KWS_MEL_COEFFICIENT_CAPACITY];
static uint16_t mel_first_bin[KWS_MEL_BINS];
static uint16_t mel_bin_count[KWS_MEL_BINS];
static uint16_t mel_coefficient_offset[KWS_MEL_BINS];
static int8_t quantized_features[AI_NETWORK_INPUT_SIZE] __attribute__((aligned(32)));

volatile uint32_t g_kws_inference_count;
volatile uint32_t g_kws_class_index;
volatile float g_kws_confidence;
volatile float g_kws_scores[KWS_CLASS_COUNT];
volatile uint32_t g_kws_error_count;

static void BuildLogMelFeatures(void);

static inline int32_t SignExtend24(uint32_t sample)
{
  return ((int32_t)(sample << 8U)) >> 8U;
}

static inline float HertzToMel(float hertz)
{
  return 1127.0f * logf(1.0f + (hertz / 700.0f));
}

void KwsAudio_Init(void)
{
  const float two_pi = 6.2831853071795864769f;
  const float min_mel = HertzToMel(KWS_MIN_FREQUENCY_HZ);
  const float max_mel = HertzToMel(KWS_MAX_FREQUENCY_HZ);
  uint32_t coefficient_count = 0U;

  memset(audio_ring, 0, sizeof(audio_ring));
  (void)arm_rfft_fast_init_512_f32(&fft_instance);

  for (uint32_t index = 0U; index < KWS_WINDOW_SAMPLES; index++)
  {
    hann_window[index] = 0.5f -
      (0.5f * cosf((two_pi * (float)index) / (float)KWS_WINDOW_SAMPLES));
  }

  for (uint32_t index = 0U; index < (KWS_MEL_BINS + 2U); index++)
  {
    const float ratio = (float)index / (float)(KWS_MEL_BINS + 1U);
    mel_edges[index] = min_mel + ((max_mel - min_mel) * ratio);
  }

  for (uint32_t bin = 0U; bin < KWS_FFT_BIN_COUNT; bin++)
  {
    fft_bin_mel[bin] = HertzToMel((float)bin *
                                  ((float)KWS_SAMPLE_RATE_HZ / (float)KWS_FFT_SIZE));
  }

  for (uint32_t mel = 0U; mel < KWS_MEL_BINS; mel++)
  {
    const float left_mel = mel_edges[mel];
    const float center_mel = mel_edges[mel + 1U];
    const float right_mel = mel_edges[mel + 2U];
    uint32_t first_bin = 0U;
    uint32_t last_bin;

    while ((first_bin < KWS_FFT_BIN_COUNT) &&
           (fft_bin_mel[first_bin] < left_mel))
    {
      first_bin++;
    }

    last_bin = first_bin;
    while ((last_bin < KWS_FFT_BIN_COUNT) &&
           (fft_bin_mel[last_bin] <= right_mel))
    {
      last_bin++;
    }

    mel_first_bin[mel] = (uint16_t)first_bin;
    mel_bin_count[mel] = (uint16_t)(last_bin - first_bin);
    mel_coefficient_offset[mel] = (uint16_t)coefficient_count;

    for (uint32_t bin = first_bin; bin < last_bin; bin++)
    {
      const float frequency_mel = fft_bin_mel[bin];
      float weight = 0.0f;

      if (frequency_mel < center_mel)
      {
        weight = (frequency_mel - left_mel) / (center_mel - left_mel);
      }
      else
      {
        weight = (right_mel - frequency_mel) / (right_mel - center_mel);
      }

      if (coefficient_count < KWS_MEL_COEFFICIENT_CAPACITY)
      {
        mel_coefficients[coefficient_count] = weight;
      }
      coefficient_count++;
    }
  }

}

bool KwsAudio_PushI2s(const uint32_t *stereo_slots, uint32_t frame_count)
{
  bool inference_ready = false;

  for (uint32_t frame = 0U; frame < frame_count; frame++)
  {
    int32_t pcm24 = SignExtend24(stereo_slots[frame * KWS_I2S_SLOTS_PER_FRAME]); // Left channel only

    audio_ring[audio_write_index] = (int16_t)pcm24;
    audio_write_index = (audio_write_index + 1U) % KWS_WINDOW_SAMPLE_COUNT;

    samples_since_inference++;
    if ((audio_write_index == 0U) &&
        (samples_since_inference >= KWS_INFERENCE_HOP))
    {
      samples_since_inference = 0U;
      inference_ready = true;
    }
  }

  return inference_ready;
}

static int8_t QuantizeFeature(float value)
{
  const float input_scale[] = KWS_INPUT_SCALE;
  const float scaled = (value / input_scale[0]) + (float)KWS_INPUT_ZERO_POINT;
  int32_t quantized = (int32_t)(scaled + ((scaled >= 0.0f) ? 0.5f : -0.5f));

  if (quantized > 127)
  {
    quantized = 127;
  }
  else if (quantized < -128)
  {
    quantized = -128;
  }
  return (int8_t)quantized;
}

static void BuildLogMelFeatures(void)
{
  const float pcm_scale = 1.0f / 32768.0f;

  for (uint32_t frame = 0U; frame < KWS_TIME_FRAMES; frame++)
  {
    const uint32_t frame_start = frame * KWS_HOP_SAMPLES;

    for (uint32_t index = 0U; index < KWS_WINDOW_SAMPLES; index++)
    {
      fft_input[index] = ((float)audio_window[frame_start + index] * pcm_scale) *
                         hann_window[index];
    }
    for (uint32_t index = KWS_WINDOW_SAMPLES; index < KWS_FFT_SIZE; index++)
    {
      fft_input[index] = 0.0f;
    }

    arm_rfft_fast_f32(&fft_instance, fft_input, fft_packed_output, 0U);

    /* CMSIS stores DC and Nyquist separately, then complex bins as real/imag pairs. */
    fft_magnitude[0] = fabsf(fft_packed_output[0]);
    arm_cmplx_mag_f32(&fft_packed_output[2U],
                      &fft_magnitude[1U],
                      (KWS_FFT_SIZE / 2U) - 1U);
    fft_magnitude[KWS_FFT_SIZE / 2U] = fabsf(fft_packed_output[1]);

    for (uint32_t mel = 0U; mel < KWS_MEL_BINS; mel++)
    {
      float energy = 0.0f;
      arm_dot_prod_f32(&fft_magnitude[mel_first_bin[mel]],
                       &mel_coefficients[mel_coefficient_offset[mel]],
                       mel_bin_count[mel],
                       &energy);

      /* Model layout is [mel][time], matching [1,64,96,1]. */
      quantized_features[(mel * KWS_TIME_FRAMES) + frame] =
        QuantizeFeature(logf(energy + KWS_LOG_OFFSET));
    }
  }
}

int KwsAudio_RunInference(void)
{
  float scores[KWS_CLASS_COUNT];
  uint32_t class_index;

  BuildLogMelFeatures();
  
  if (aiRunFeatures(quantized_features, scores, &class_index) != 0)
  {
    return -1;
  }

  for (uint32_t index = 0U; index < KWS_CLASS_COUNT; index++)
  {
    g_kws_scores[index] = scores[index];
  }
  g_kws_class_index = class_index;
  g_kws_confidence = scores[class_index];
  return 0;
}
