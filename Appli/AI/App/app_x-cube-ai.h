/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __APP_AI_H
#define __APP_AI_H
#ifdef __cplusplus
extern "C" {
#endif
/**
  ******************************************************************************
  * @file    app_x-cube-ai.h
  * @author  STM32Cube AI Studio C code generator
  * @brief   AI entry function definitions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stai.h"
#include "ai_datatypes_defines.h"
#include "ll_aton_runtime.h"
#include "stai_network.h"
#include "user_init.h"

/* IO buffers ----------------------------------------------------------------*/

void STM32CubeAI_Studio_AI_Init(void);
void STM32CubeAI_Studio_AI_Process(void);
void STM32CubeAI_Studio_AI_Deinit(void);

/* Function prototypes -------------------------------------------------------*/
int acquire_and_process_data(void);
int post_process(void);
stai_return_code aiRun(void);

/* USER CODE BEGIN includes */
#include <stdint.h>

#define AI_NETWORK_INPUT_SIZE   STAI_NETWORK_IN_1_SIZE
#define AI_NETWORK_OUTPUT_SIZE  STAI_NETWORK_OUT_1_SIZE

extern volatile stai_return_code g_ai_last_error;

int aiInit(void);
int aiDeinit(void);
int aiRunFeatures(const int8_t features[AI_NETWORK_INPUT_SIZE],
                  float scores[AI_NETWORK_OUTPUT_SIZE],
                  uint32_t *class_index);
/* USER CODE END includes */


#ifdef __cplusplus
}
#endif
#endif /*__STMicroelectronics_ST_EDGE_AI_4.0.1-20581 7ed50de05_H */
