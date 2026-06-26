/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_threadx.c
  * @author  MCD Application Team
  * @brief   ThreadX applicative file
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
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_threadx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include "ux_device_audio_record.h"
#include "app_x-cube-ai.h"
#include "kws_audio.h"
#include "npu_init.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */


#define I2S_RX_FRAMES_PER_HALF          64U
#define I2S_RX_HALF_FREE                0U
#define I2S_RX_HALF_READY               1U
#define I2S_RX_HALF_PROCESSING          2U

#define ACQUISITION_THREAD_STACK_SIZE   2048U
#define ACQUISITION_THREAD_PRIORITY     6U

#define AI_THREAD_STACK_SIZE            16384U
#define AI_THREAD_PRIORITY              10U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static TX_THREAD acquisition_thread;
static TX_SEMAPHORE acquisition_semaphore;
static ULONG acquisition_thread_stack[ACQUISITION_THREAD_STACK_SIZE / sizeof(ULONG)];

static TX_THREAD ai_thread;
static TX_SEMAPHORE ai_semaphore;
static ULONG ai_thread_stack[AI_THREAD_STACK_SIZE / sizeof(ULONG)];

static uint32_t i2s_rx_slots[2][2U * I2S_RX_FRAMES_PER_HALF] __NON_CACHEABLE;
static volatile uint8_t i2s_rx_half_state[2] =
{
  I2S_RX_HALF_FREE,
  I2S_RX_HALF_FREE
};
static volatile uint32_t i2s_rx_overrun_count;

extern I2S_HandleTypeDef hi2s1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static VOID AcquisitionThreadEntry(ULONG initial_input);
static VOID AiThreadEntry(ULONG initial_input);
static HAL_StatusTypeDef StartI2sReceive(void);
static int32_t ClaimReadyI2sHalf(void);
static void ReleaseProcessedI2sHalf(uint32_t half_index);

/* USER CODE END PFP */

/**
  * @brief  Application ThreadX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT App_ThreadX_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;

  /* USER CODE BEGIN App_ThreadX_MEM_POOL */
  (void)memory_ptr;

  /* USER CODE END App_ThreadX_MEM_POOL */

  /* USER CODE BEGIN App_ThreadX_Init */
  ret = tx_semaphore_create(&acquisition_semaphore, "i2s acquisition", 0U);
  if (ret != TX_SUCCESS)
  {
    return ret;
  }

  ret = tx_semaphore_create(&ai_semaphore, "ai inference", 0U);
  if (ret != TX_SUCCESS)
  {
    return ret;
  }

  ret = tx_thread_create(&acquisition_thread,
                         "i2s acquisition",
                         AcquisitionThreadEntry,
                         0U,
                         acquisition_thread_stack,
                         sizeof(acquisition_thread_stack),
                         ACQUISITION_THREAD_PRIORITY,
                         ACQUISITION_THREAD_PRIORITY,
                         TX_NO_TIME_SLICE,
                         TX_AUTO_START);
  if (ret != TX_SUCCESS)
  {
    return ret;
  }

  ret = tx_thread_create(&ai_thread,
                         "keyword spotting",
                         AiThreadEntry,
                         0U,
                         ai_thread_stack,
                         sizeof(ai_thread_stack),
                         AI_THREAD_PRIORITY,
                         AI_THREAD_PRIORITY,
                         TX_NO_TIME_SLICE,
                         TX_AUTO_START);
  if (ret != TX_SUCCESS)
  {
    return ret;
  }
  /* USER CODE END App_ThreadX_Init */

  return ret;
}

  /**
  * @brief  Function that implements the kernel's initialization.
  * @param  None
  * @retval None
  */
void MX_ThreadX_Init(void)
{
  /* USER CODE BEGIN Before_Kernel_Start */

  /* USER CODE END Before_Kernel_Start */

  tx_kernel_enter();

  /* USER CODE BEGIN Kernel_Start_Error */

  /* USER CODE END Kernel_Start_Error */
}

/* USER CODE BEGIN 1 */

static VOID AcquisitionThreadEntry(ULONG initial_input)
{
  int32_t ready_half_index;

  (void)initial_input;

  KwsAudio_Init();

  if (StartI2sReceive() != HAL_OK)
  {
    Error_Handler();
  }

  for (;;)
  {
    (void)tx_semaphore_get(&acquisition_semaphore, TX_WAIT_FOREVER);
    bool inference_ready = false;

    while ((ready_half_index = ClaimReadyI2sHalf()) >= 0)
    {
      (void)USBD_AUDIO_RecordingPushLeft(i2s_rx_slots[ready_half_index],
                                         I2S_RX_FRAMES_PER_HALF);  


      inference_ready = KwsAudio_PushI2s(i2s_rx_slots[ready_half_index],
                                              I2S_RX_FRAMES_PER_HALF);

      ReleaseProcessedI2sHalf((uint32_t)ready_half_index);
      
      if (inference_ready)
      {
        (void)tx_semaphore_put(&ai_semaphore);
      }
    }
  }
}

static VOID AiThreadEntry(ULONG initial_input)
{
  (void)initial_input;

  aiPreInitialize();
  if (aiInit() != 0)
  {
    HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
    return;
  }

  for (;;)
  {
    (void)tx_semaphore_get(&ai_semaphore, TX_WAIT_FOREVER);

    if (KwsAudio_RunInference() != 0)
    {
      HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
    }
    else
    {
      // HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
    }
  }
}

static HAL_StatusTypeDef StartI2sReceive(void)
{
  return HAL_I2S_Receive_DMA(&hi2s1,
                             (uint16_t *)i2s_rx_slots,
                             2U * I2S_RX_FRAMES_PER_HALF * 2U);
}

void PublishReadyI2sHalf(uint32_t half_index)
{
  UINT interrupt_posture;

  if (half_index >= 2U)
  {
    return;
  }

  interrupt_posture = tx_interrupt_control(TX_INT_DISABLE);
  if (i2s_rx_half_state[half_index] != I2S_RX_HALF_FREE)
  {
    i2s_rx_overrun_count++;
  }
  i2s_rx_half_state[half_index] = I2S_RX_HALF_READY;
  tx_interrupt_control(interrupt_posture);

  (void)tx_semaphore_put(&acquisition_semaphore);
}

static int32_t ClaimReadyI2sHalf(void)
{
  int32_t ready_half_index = -1;
  UINT interrupt_posture;

  interrupt_posture = tx_interrupt_control(TX_INT_DISABLE);
  for (uint32_t half_index = 0U; half_index < 2U; half_index++)
  {
    if (i2s_rx_half_state[half_index] == I2S_RX_HALF_READY)
    {
      i2s_rx_half_state[half_index] = I2S_RX_HALF_PROCESSING;
      ready_half_index = (int32_t)half_index;
      break;
    }
  }
  tx_interrupt_control(interrupt_posture);

  return ready_half_index;
}

static void ReleaseProcessedI2sHalf(uint32_t half_index)
{
  UINT interrupt_posture;

  if (half_index >= 2U)
  {
    return;
  }

  interrupt_posture = tx_interrupt_control(TX_INT_DISABLE);
  i2s_rx_half_state[half_index] = I2S_RX_HALF_FREE;
  tx_interrupt_control(interrupt_posture);
}

/* USER CODE END 1 */
