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
#define USB_AUDIO_HS_SAMPLES_PER_PACKET 6U
#define USB_AUDIO_FS_SAMPLES_PER_PACKET 48U
#define USB_AUDIO_MAX_PACKET_BYTES_HS   18U
#define USB_AUDIO_MAX_PACKET_BYTES_FS   144U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static UX_DEVICE_CLASS_AUDIO_STREAM *record_stream = UX_NULL;
static UCHAR record_packet[USB_AUDIO_MAX_PACKET_BYTES_HS] = {0};
static ULONG record_packet_samples = 0U;
static UINT record_stream_active = 0U;
static UINT transmission_started = 0U;

static volatile ULONG frames_sent = 0U;
static volatile ULONG zero_length_frames = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

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
  record_packet_samples = 0U;
  transmission_started = 0U;
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

  if (length == 0U)
  {
    zero_length_frames++;
  }
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
  max_frame_buffer_number = 16U;
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
  max_frame_buffer_size = USBD_AUDIO_RECORD_EPIN_HS_MPS;
  /* USER CODE END USBD_AUDIO_RecordingStreamGetMaxFrameBufferSize */

  return max_frame_buffer_size;
}

/* USER CODE BEGIN 1 */
static UINT CommitRecordPacket(void)
{
  UCHAR *usb_frame;
  ULONG max_length;
  ULONG length;
  UINT status;

  length = record_packet_samples * USB_AUDIO_BYTES_PER_SAMPLE;

  status = ux_device_class_audio_write_frame_get(record_stream,
                                                  &usb_frame,
                                                  &max_length);
  if ((status != UX_SUCCESS) || (max_length < length))
  {
    return status;
  }

  memcpy(usb_frame, record_packet, length);

  status = ux_device_class_audio_write_frame_commit(record_stream, length);
  if (status != UX_SUCCESS)
  {
    return status;
  }

  record_packet_samples = 0U;

  if (transmission_started == 0U)
  {
    status = ux_device_class_audio_transmission_start(record_stream);

    if (status == UX_SUCCESS)
    {
      transmission_started = 1U;
    }
  }

  return status;
}

UINT USBD_AUDIO_RecordingWriteLeft(const uint32_t *stereo_slots,
                                   ULONG frame_count)
{
  ULONG frame_index;
  UINT status;

  if ((record_stream_active == 0U) || (record_stream == UX_NULL))
  {
    return UX_SUCCESS;
  }

  for (frame_index = 0U; frame_index < frame_count; frame_index++)
  {
    uint32_t sample;
    ULONG offset;

    if (record_packet_samples == USB_AUDIO_HS_SAMPLES_PER_PACKET)
    {
      status = CommitRecordPacket();

      if (status != UX_SUCCESS)
      {
        return status;
      }
    }

    sample = stereo_slots[frame_index * I2S_SLOTS_PER_FRAME] & 0x00FFFFFFU;
    offset = record_packet_samples * USB_AUDIO_BYTES_PER_SAMPLE;

    record_packet[offset]      = (UCHAR)(sample);
    record_packet[offset + 1U] = (UCHAR)(sample >> 8);
    record_packet[offset + 2U] = (UCHAR)(sample >> 16);

    record_packet_samples++;

  }

  if (record_packet_samples == USB_AUDIO_HS_SAMPLES_PER_PACKET)
  {
    return CommitRecordPacket();
  }

  return UX_SUCCESS;
}
/* USER CODE END 1 */
