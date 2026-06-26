#ifndef KWS_AUDIO_H
#define KWS_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "app_x-cube-ai.h"

#define KWS_SAMPLE_RATE_HZ      16000U
#define KWS_MEL_BINS            64U
#define KWS_TIME_FRAMES         96U
#define KWS_CLASS_COUNT         AI_NETWORK_OUTPUT_SIZE

/* These variables are intentionally visible in the debugger. */
extern volatile uint32_t g_kws_inference_count;
extern volatile uint32_t g_kws_class_index;
extern volatile float g_kws_confidence;
extern volatile float g_kws_scores[KWS_CLASS_COUNT];
extern volatile uint32_t g_kws_error_count;

void KwsAudio_Init(void);
bool KwsAudio_PushI2s(const uint32_t *stereo_slots, uint32_t frame_count);
int KwsAudio_RunInference(void);

#ifdef __cplusplus
}
#endif

#endif /* KWS_AUDIO_H */
