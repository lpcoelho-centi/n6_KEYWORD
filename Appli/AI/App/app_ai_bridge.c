#include "app_x-cube-ai.h"

#include <string.h>
#include "main.h"

extern stai_network network_context[STAI_NETWORK_CONTEXT_SIZE];

volatile stai_return_code g_ai_last_error = STAI_SUCCESS;

int aiRunFeatures(const int8_t features[AI_NETWORK_INPUT_SIZE],
                  float scores[AI_NETWORK_OUTPUT_SIZE],
                  uint32_t *class_index)
{
  stai_ptr inputs[STAI_NETWORK_IN_NUM];
  stai_ptr outputs[STAI_NETWORK_OUT_NUM];
  stai_size input_count = 0U;
  stai_size output_count = 0U;
  uint32_t best_index = 0U;

  if ((features == NULL) || (scores == NULL) || (class_index == NULL))
  {
    return -1;
  }
  

  g_ai_last_error = stai_network_get_inputs(network_context,
                                            inputs,
                                            &input_count);
  if ((g_ai_last_error != STAI_SUCCESS) ||
      (input_count != STAI_NETWORK_IN_NUM) ||
      (inputs[0] == NULL))
  {
    return -1;
  }

  g_ai_last_error = stai_network_get_outputs(network_context,
                                             outputs,
                                             &output_count);
  if ((g_ai_last_error != STAI_SUCCESS) ||
      (output_count != STAI_NETWORK_OUT_NUM) ||
      (outputs[0] == NULL))
  {
    return -1;
  }

  memcpy(inputs[0], features, AI_NETWORK_INPUT_SIZE);
  HAL_GPIO_TogglePin(ML_GPIO_Port, ML_Pin);
  g_ai_last_error = stai_network_run(network_context, STAI_MODE_SYNC);
  if (g_ai_last_error != STAI_SUCCESS)
  {
    g_ai_last_error = stai_network_get_error(network_context);
    return -1;
  }
  HAL_GPIO_TogglePin(ML_GPIO_Port, ML_Pin);
  memcpy(scores, outputs[0], sizeof(float) * AI_NETWORK_OUTPUT_SIZE);
  for (uint32_t index = 1U; index < AI_NETWORK_OUTPUT_SIZE; index++)
  {
    if (scores[index] > scores[best_index])
    {
      best_index = index;
    }
  }

  *class_index = best_index;
  return 0;
}
