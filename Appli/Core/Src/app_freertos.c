/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : FreeRTOS applicative file
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
#include "app_freertos.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"
#include "app_usbx.h"
#include "stdbool.h"
#include "ux_device_audio_record.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define I2S_RX_FRAMES_PER_HALF    6U

#define I2S_RX_HALF_FREE          0U
#define I2S_RX_HALF_READY         1U  
#define I2S_RX_HALF_PROCESSING    2U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static volatile bool i2s_rx_overrun = false;

static uint32_t i2s_rx_slots[2][2 * I2S_RX_FRAMES_PER_HALF] __NON_CACHEABLE;

static volatile uint8_t i2s_rx_half_state[2] =
{
  I2S_RX_HALF_FREE,
  I2S_RX_HALF_FREE
};

extern I2S_HandleTypeDef hi2s1;

volatile uint32_t g_AcquisitionTaskStackHwmWords = 0;
volatile uint32_t g_MLTaskStackHwmWords = 0;
/* USER CODE END Variables */
/* Definitions for AcquisitionTask */
osThreadId_t AcquisitionTaskHandle;
uint32_t AcquisitionTask_Buffer[ 256 ];
osStaticThreadDef_t AcquisitionTask_ControlBlock;
const osThreadAttr_t AcquisitionTask_attributes = {
  .name = "AcquisitionTask",
  .stack_mem = &AcquisitionTask_Buffer[0],
  .stack_size = sizeof(AcquisitionTask_Buffer),
  .cb_mem = &AcquisitionTask_ControlBlock,
  .cb_size = sizeof(AcquisitionTask_ControlBlock),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for MLTask */
osThreadId_t MLTaskHandle;
uint32_t MLTask_Buffer[ 4096 ];
osStaticThreadDef_t MLTask_ControlBlock;
const osThreadAttr_t MLTask_attributes = {
  .name = "MLTask",
  .stack_mem = &MLTask_Buffer[0],
  .stack_size = sizeof(MLTask_Buffer),
  .cb_mem = &MLTask_ControlBlock,
  .cb_size = sizeof(MLTask_ControlBlock),
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static HAL_StatusTypeDef StartI2sReceive(void);
static int32_t ClaimReadyI2sHalf(void);
void PublishReadyI2sHalf(uint32_t half_index);
/* USER CODE END FunctionPrototypes */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
  /* creation of AcquisitionTask */
  AcquisitionTaskHandle = osThreadNew(StartAcquisitionTask, NULL, &AcquisitionTask_attributes);

  /* creation of MLTask */
  MLTaskHandle = osThreadNew(StartMLTask, NULL, &MLTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  g_AcquisitionTaskStackHwmWords = uxTaskGetStackHighWaterMark(AcquisitionTaskHandle);
  g_MLTaskStackHwmWords = uxTaskGetStackHighWaterMark(MLTaskHandle);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}
/* USER CODE BEGIN Header_StartAcquisitionTask */
/**
* @brief Function implementing the AcquisitionTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartAcquisitionTask */
void StartAcquisitionTask(void *argument)
{
  /* USER CODE BEGIN AcquisitionTask */
  if (StartI2sReceive() != HAL_OK)
  {
    Error_Handler();
  }
  /* Infinite loop */
  for(;;)
  {
    int32_t ready_half_index = ClaimReadyI2sHalf();
    if(i2s_rx_overrun)
    {
      i2s_rx_overrun = false;
      HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET); 
    }

    if (ready_half_index >= 0)
    {  
    
    (void)USBD_AUDIO_RecordingWriteLeft(
          i2s_rx_slots[ready_half_index],
          I2S_RX_FRAMES_PER_HALF);
      
      i2s_rx_half_state[ready_half_index] = I2S_RX_HALF_FREE;
    }
    ux_system_tasks_run();
  }
  /* USER CODE END AcquisitionTask */
}

/* USER CODE BEGIN Header_StartMLTask */
/**
* @brief Function implementing the MLTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartMLTask */
void StartMLTask(void *argument)
{
  /* USER CODE BEGIN MLTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END MLTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

static HAL_StatusTypeDef StartI2sReceive(void)
{
  return HAL_I2S_Receive_DMA(&hi2s1,
                             (uint16_t *)i2s_rx_slots,
                             2U * I2S_RX_FRAMES_PER_HALF * 2U );
}

    static int32_t ClaimReadyI2sHalf(void)
{
  __disable_irq();
  for (uint32_t half_index = 0U; half_index < 2; half_index++)
  {
    if (i2s_rx_half_state[half_index] == I2S_RX_HALF_READY)
    {
      i2s_rx_half_state[half_index] = I2S_RX_HALF_PROCESSING;
      __enable_irq();
      return (int32_t)half_index;
    }
  }
  __enable_irq();
  return -1;
}

void PublishReadyI2sHalf(uint32_t half_index)
{
  if (i2s_rx_half_state[half_index] != I2S_RX_HALF_FREE)
  {
    i2s_rx_overrun = true;
  }
  i2s_rx_half_state[half_index] = I2S_RX_HALF_READY;
}
/* USER CODE END Application */

