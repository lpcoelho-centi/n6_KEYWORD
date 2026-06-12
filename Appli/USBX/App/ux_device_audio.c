/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ux_device_audio.c
  * @author  MCD Application Team
  * @brief   USBX Device Audio applicative source file
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
#include "ux_device_audio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ux_device_descriptors.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define AUDIO_VOLUME_MIN      ((SHORT)-23040) /* -90 dB */
#define AUDIO_VOLUME_MAX      ((SHORT)0)
#define AUDIO_VOLUME_RES      ((USHORT)256)   /* 1 dB */
#define AUDIO_VOLUME_DEFAULT  ((SHORT)0)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static UX_DEVICE_CLASS_AUDIO20_CONTROL audio_control[1];

static volatile UINT audio_device_active;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/**
  * @brief  USBD_AUDIO_Activate
  *         This function is called when insertion of a Audio device.
  * @param  audio_instance: Pointer to the audio class instance.
  * @retval none
  */
VOID USBD_AUDIO_Activate(VOID *audio_instance)
{
  /* USER CODE BEGIN USBD_AUDIO_Activate */
  UX_PARAMETER_NOT_USED(audio_instance);
  audio_device_active = 1U;
  /* USER CODE END USBD_AUDIO_Activate */

  return;
}

/**
  * @brief  USBD_AUDIO_Deactivate
  *         This function is called when extraction of a Audio device.
  * @param  audio_instance: Pointer to the audio class instance.
  * @retval none
  */
VOID USBD_AUDIO_Deactivate(VOID *audio_instance)
{
  /* USER CODE BEGIN USBD_AUDIO_Deactivate */
  UX_PARAMETER_NOT_USED(audio_instance);
  audio_device_active = 0U;
  /* USER CODE END USBD_AUDIO_Deactivate */

  return;
}

/**
  * @brief  USBD_AUDIO_ControlProcess
  *         This function is invoked to manage the UAC class requests.
  * @param  audio_instance: Pointer to Audio class stream instance.
  * @param  transfer: Pointer to the transfer request.
  * @retval status
  */
UINT USBD_AUDIO_ControlProcess(UX_DEVICE_CLASS_AUDIO *audio_instance,
                               UX_SLAVE_TRANSFER *transfer)
{
  UINT status  = UX_SUCCESS;

  /* USER CODE BEGIN USBD_AUDIO_ControlProcess */
  UX_DEVICE_CLASS_AUDIO20_CONTROL_GROUP group;

  group.ux_device_class_audio20_control_group_controls_nb = 1U;
  group.ux_device_class_audio20_control_group_controls = audio_control;

  status = ux_device_class_audio20_control_process(audio_instance,
                                                  transfer,
                                                  &group);
  /* USER CODE END USBD_AUDIO_ControlProcess */

  return status;
}

/**
  * @brief  USBD_AUDIO_SetControlValues
  *         This function is invoked to Set audio control values.
  * @param  none
  * @retval none
  */
VOID USBD_AUDIO_SetControlValues(VOID)
{
  /* USER CODE BEGIN USBD_AUDIO_SetControlValues */
  audio_control[0].ux_device_class_audio20_control_cs_id =
    USBD_AUDIO_RECORD_CLOCK_SOURCE_ID;

  audio_control[0].ux_device_class_audio20_control_sampling_frequency =
      USBD_AUDIO_RECORD_DEFAULT_FREQ;

  audio_control[0].ux_device_class_audio20_control_fu_id =
      USBD_AUDIO_RECORD_FEATURE_UNIT_ID;

  audio_control[0].ux_device_class_audio20_control_mute[0] = 0U;
  audio_control[0].ux_device_class_audio20_control_volume_min[0] = AUDIO_VOLUME_MIN;
  audio_control[0].ux_device_class_audio20_control_volume_max[0] = AUDIO_VOLUME_MAX;
  audio_control[0].ux_device_class_audio20_control_volume_res[0] = AUDIO_VOLUME_RES;
  audio_control[0].ux_device_class_audio20_control_volume[0] = AUDIO_VOLUME_DEFAULT;
  /* USER CODE END USBD_AUDIO_SetControlValues */

  return;
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
