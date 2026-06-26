/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ux_device_audio_record.c
  * @author  MCD Application Team
  * @brief   USBX Device Video applicative source file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "ux_device_audio_record.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "ux_device_descriptors.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define I2S_SLOTS_PER_FRAME             2U
#define USB_AUDIO_BYTES_PER_SAMPLE      3U
#define USB_AUDIO_HS_SAMPLES_PER_PACKET (USBD_AUDIO_RECORD_DEFAULT_FREQ / 8000U)
#define USB_AUDIO_FS_SAMPLES_PER_PACKET (USBD_AUDIO_RECORD_DEFAULT_FREQ / 1000U)
#define USB_AUDIO_MAX_SAMPLES_PER_PACKET USB_AUDIO_FS_SAMPLES_PER_PACKET
#define USB_AUDIO_MAX_PACKET_BYTES      (USB_AUDIO_MAX_SAMPLES_PER_PACKET * USB_AUDIO_BYTES_PER_SAMPLE)
#define USB_AUDIO_FIFO_SAMPLE_COUNT     256U
#define USB_AUDIO_PREBUFFER_PACKETS     16U
#define USB_AUDIO_QUEUE_TARGET_PACKETS  16U
#define USB_AUDIO_FIFO_HIGH_WATERMARK   ((USB_AUDIO_FIFO_SAMPLE_COUNT * 3U) / 4U)
#define USB_AUDIO_FIFO_CRITICAL_WATERMARK ((USB_AUDIO_FIFO_SAMPLE_COUNT * 7U) / 8U)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static UX_DEVICE_CLASS_AUDIO_STREAM *record_stream = UX_NULL;
static UCHAR record_packet[USB_AUDIO_MAX_PACKET_BYTES] = {0};
static UINT record_stream_active = 0U;
static UINT transmission_started = 0U;
static ULONG queued_packets = 0U;
static uint32_t last_sample = 0U;
static uint32_t audio_fifo[USB_AUDIO_FIFO_SAMPLE_COUNT] = {0};
static volatile ULONG audio_fifo_head = 0U;
static volatile ULONG audio_fifo_tail = 0U;
static volatile ULONG audio_fifo_count = 0U;

static volatile ULONG frames_sent = 0U;
static volatile ULONG zero_length_frames = 0U;
static volatile ULONG fifo_overruns = 0U;
static volatile ULONG fifo_underruns = 0U;
static volatile ULONG usb_commit_errors = 0U;
static volatile ULONG usb_committed_packets = 0U;
static volatile ULONG usb_push_calls = 0U;
static volatile ULONG usb_stream_changes = 0U;
volatile ULONG g_usb_audio_fifo_level = 0U;
volatile ULONG g_usb_audio_fifo_underruns = 0U;
volatile ULONG g_usb_audio_fifo_overruns = 0U;
volatile ULONG g_usb_audio_commit_errors = 0U;
volatile ULONG g_usb_audio_committed_packets = 0U;
volatile ULONG g_usb_audio_last_commit_status = 0U;
volatile ULONG g_usb_audio_last_packet_length = 0U;
volatile ULONG g_usb_audio_last_max_length = 0U;
volatile ULONG g_usb_audio_last_start_status = 0U;
volatile ULONG g_usb_audio_transmission_started = 0U;
volatile ULONG g_usb_audio_push_calls = 0U;
volatile ULONG g_usb_audio_stream_changes = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static ULONG SamplesPerPacket(void);
static ULONG MaxSamplesPerPacket(void);
static ULONG SamplesForNextPacket(void);
static UINT ServiceRecordStream(ULONG target_queued_packets);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  USBD_AUDIO_RecordingStreamChange
  *         This function is invoked to inform application that the
  *         alternate setting are changed.
  * @param  audio_record_stream: Pointer to audio recording class stream instance.
  * @param  alternate_setting: interface alternate setting.
  * @retval none
  */
VOID USBD_AUDIO_RecordingStreamChange(UX_DEVICE_CLASS_AUDIO_STREAM *audio_record_stream,
                                      ULONG alternate_setting)
{
  /* USER CODE BEGIN USBD_AUDIO_RecordingStreamChange */
  record_stream = audio_record_stream;
  record_stream_active = (alternate_setting != 0U);
  transmission_started = 0U;
  queued_packets = 0U;
  last_sample = 0U;
  audio_fifo_head = 0U;
  audio_fifo_tail = 0U;
  audio_fifo_count = 0U;
  usb_stream_changes++;
  g_usb_audio_stream_changes = usb_stream_changes;
  g_usb_audio_transmission_started = 0U;
  /* USER CODE END USBD_AUDIO_RecordingStreamChange */

  return;
}

/**
  * @brief  USBD_AUDIO_RecordingStreamFrameDone
  *         This function is invoked whenever a USB packet (audio frame) is received
  *         from the host.
  * @param  audio_record_stream: Pointer to audio recodring class stream instance.
  * @param  length: transfer length.
  * @retval none
  */
VOID USBD_AUDIO_RecordingStreamFrameDone(UX_DEVICE_CLASS_AUDIO_STREAM *audio_record_stream,
                                        ULONG length)
{
  /* USER CODE BEGIN USBD_AUDIO_RecordingStreamFrameDone */
  if ((record_stream == UX_NULL) ||
      (audio_record_stream != record_stream) ||
      (record_stream_active == 0U))
  {
    return;
  }

  frames_sent++;
  if (queued_packets > 0U)
  {
    queued_packets--;
  }

  if (length == 0U)
  {
    zero_length_frames++;
  }

  (void)ServiceRecordStream(USB_AUDIO_QUEUE_TARGET_PACKETS);
  /* USER CODE END USBD_AUDIO_RecordingStreamFrameDone */

  return;
}

/**
  * @brief  USBD_AUDIO_RecordingStreamGetMaxFrameBufferNumber
  *         This function is invoked to Set audio recording stream max Frame buffer number.
  * @param  none
  * @retval max frame buffer number
  */
ULONG USBD_AUDIO_RecordingStreamGetMaxFrameBufferNumber(VOID)
{
  ULONG max_frame_buffer_number = 0U;

  /* USER CODE BEGIN USBD_AUDIO_RecordingStreamGetMaxFrameBufferNumber */
  max_frame_buffer_number = 32U;
  /* USER CODE END USBD_AUDIO_RecordingStreamGetMaxFrameBufferNumber */

  return max_frame_buffer_number;
}

/**
  * @brief  USBD_AUDIO_RecordingStreamGetMaxFrameBufferSize
  *         This function is invoked to Set audio recording stream max Frame buffer size.
  * @param  none
  * @retval max frame buffer size
  */
ULONG USBD_AUDIO_RecordingStreamGetMaxFrameBufferSize(VOID)
{
  ULONG max_frame_buffer_size = 0U;

  /* USER CODE BEGIN USBD_AUDIO_RecordingStreamGetMaxFrameBufferSize */
  if (USBD_AUDIO_RECORD_EPIN_FS_MPS > USBD_AUDIO_RECORD_EPIN_HS_MPS)
  {
    max_frame_buffer_size = USBD_AUDIO_RECORD_EPIN_FS_MPS;
  }
  else
  {
    max_frame_buffer_size = USBD_AUDIO_RECORD_EPIN_HS_MPS;
  }
  /* USER CODE END USBD_AUDIO_RecordingStreamGetMaxFrameBufferSize */

  return max_frame_buffer_size;
}

/* USER CODE BEGIN 1 */
static inline ULONG SamplesPerPacket(void)
{
  if ((record_stream != UX_NULL) &&
      (ux_device_class_audio_speed_get(record_stream) == UX_HIGH_SPEED_DEVICE))
  {
    return USB_AUDIO_HS_SAMPLES_PER_PACKET;
  }

  return USB_AUDIO_FS_SAMPLES_PER_PACKET;
}

static inline ULONG MaxSamplesPerPacket(void)
{
  ULONG max_packet_bytes;

  if ((record_stream != UX_NULL) &&
      (ux_device_class_audio_speed_get(record_stream) == UX_HIGH_SPEED_DEVICE))
  {
    max_packet_bytes = USBD_AUDIO_RECORD_EPIN_HS_MPS;
  }
  else
  {
    max_packet_bytes = USBD_AUDIO_RECORD_EPIN_FS_MPS;
  }

  return max_packet_bytes / USB_AUDIO_BYTES_PER_SAMPLE;
}

static inline ULONG AudioFifoLevel(void)
{
  ULONG level;

  __disable_irq();
  level = audio_fifo_count;
  __enable_irq();

  return level;
}

static inline void AudioFifoPush(uint32_t sample)
{
  __disable_irq();
  if (audio_fifo_count == USB_AUDIO_FIFO_SAMPLE_COUNT)
  {
    audio_fifo_tail++;
    if (audio_fifo_tail == USB_AUDIO_FIFO_SAMPLE_COUNT)
    {
      audio_fifo_tail = 0U;
    }
    audio_fifo_count--;
    fifo_overruns++;
  }

  audio_fifo[audio_fifo_head] = sample;
  audio_fifo_head++;
  if (audio_fifo_head == USB_AUDIO_FIFO_SAMPLE_COUNT)
  {
    audio_fifo_head = 0U;
  }
  audio_fifo_count++;
  g_usb_audio_fifo_level = audio_fifo_count;
  g_usb_audio_fifo_overruns = fifo_overruns;
  __enable_irq();
}

static inline uint32_t AudioFifoPopOrRepeat(void)
{
  uint32_t sample;

  __disable_irq();
  if (audio_fifo_count > 0U)
  {
    sample = audio_fifo[audio_fifo_tail];
    audio_fifo_tail++;
    if (audio_fifo_tail == USB_AUDIO_FIFO_SAMPLE_COUNT)
    {
      audio_fifo_tail = 0U;
    }
    audio_fifo_count--;
    last_sample = sample;
    g_usb_audio_fifo_level = audio_fifo_count;
  }
  else
  {
    sample = last_sample;
    fifo_underruns++;
    g_usb_audio_fifo_underruns = fifo_underruns;
  }
  __enable_irq();

  return sample;
}

static inline ULONG SamplesForNextPacket(void)
{
  ULONG fifo_level;
  ULONG samples_per_packet;
  ULONG max_samples_per_packet;

  fifo_level = AudioFifoLevel();
  samples_per_packet = SamplesPerPacket();
  max_samples_per_packet = MaxSamplesPerPacket();

  if ((fifo_level > USB_AUDIO_FIFO_HIGH_WATERMARK) &&
      (samples_per_packet < max_samples_per_packet))
  {
    samples_per_packet++;
  }

  if ((fifo_level > USB_AUDIO_FIFO_CRITICAL_WATERMARK) &&
      (samples_per_packet < max_samples_per_packet))
  {
    samples_per_packet++;
  }

  return samples_per_packet;
}

static UINT CommitRecordPacket(ULONG samples_per_packet)
{
  UCHAR *usb_frame;
  ULONG max_length;
  ULONG length;
  ULONG sample_index;
  UINT status;

  length = samples_per_packet * USB_AUDIO_BYTES_PER_SAMPLE;

  status = ux_device_class_audio_write_frame_get(record_stream,
                                                  &usb_frame,
                                                  &max_length);
  g_usb_audio_last_commit_status = status;
  g_usb_audio_last_packet_length = length;
  g_usb_audio_last_max_length = max_length;
  if ((status != UX_SUCCESS) || (max_length < length))
  {
    usb_commit_errors++;
    g_usb_audio_commit_errors = usb_commit_errors;
    return status;
  }

  for (sample_index = 0U; sample_index < samples_per_packet; sample_index++)
  {
    uint32_t sample;
    ULONG offset;

    sample = AudioFifoPopOrRepeat() & 0x00FFFFFFU;
    offset = sample_index * USB_AUDIO_BYTES_PER_SAMPLE;

    record_packet[offset]      = (UCHAR)(sample);
    record_packet[offset + 1U] = (UCHAR)(sample >> 8);
    record_packet[offset + 2U] = (UCHAR)(sample >> 16);
  }

  memcpy(usb_frame, record_packet, length);

  status = ux_device_class_audio_write_frame_commit(record_stream, length);
  g_usb_audio_last_commit_status = status;
  if (status != UX_SUCCESS)
  {
    usb_commit_errors++;
    g_usb_audio_commit_errors = usb_commit_errors;
    return status;
  }

  queued_packets++;
  usb_committed_packets++;
  g_usb_audio_committed_packets = usb_committed_packets;

  return status;
}

static UINT ServiceRecordStream(ULONG target_queued_packets)
{
  UINT status = UX_SUCCESS;

  if ((record_stream_active == 0U) || (record_stream == UX_NULL))
  {
    return UX_SUCCESS;
  }

  if ((transmission_started == 0U) &&
      (AudioFifoLevel() < (SamplesPerPacket() * USB_AUDIO_PREBUFFER_PACKETS)))
  {
    return UX_SUCCESS;
  }

  while (queued_packets < target_queued_packets)
  {
    ULONG samples_per_packet;

    samples_per_packet = SamplesForNextPacket();
    if (AudioFifoLevel() < samples_per_packet)
    {
      break;
    }

    status = CommitRecordPacket(samples_per_packet);
    if (status != UX_SUCCESS)
    {
      return status;
    }
  }

  if ((transmission_started == 0U) &&
      (queued_packets >= USB_AUDIO_PREBUFFER_PACKETS))
  {
    transmission_started = 1U;
    g_usb_audio_transmission_started = 1U;

    status = ux_device_class_audio_transmission_start(record_stream);
    g_usb_audio_last_start_status = status;

    if (status != UX_SUCCESS)
    {
      transmission_started = 0U;
      g_usb_audio_transmission_started = 0U;
    }
  }

  return status;
}

UINT USBD_AUDIO_RecordingPushLeft(const uint32_t *stereo_slots,
                                   ULONG frame_count)
{
  ULONG frame_index;

  if ((record_stream_active == 0U) || (record_stream == UX_NULL))
  {
    return UX_SUCCESS;
  }

  usb_push_calls++;
  g_usb_audio_push_calls = usb_push_calls;

  for (frame_index = 0U; frame_index < frame_count; frame_index++)
  {
    uint32_t sample;

    sample = stereo_slots[frame_index * I2S_SLOTS_PER_FRAME] & 0x00FFFFFFU; // Left channel only
    AudioFifoPush(sample);
  }

  if (transmission_started == 0U)
  {
    return ServiceRecordStream(USB_AUDIO_QUEUE_TARGET_PACKETS);
  }

  return UX_SUCCESS;
}
/* USER CODE END 1 */
