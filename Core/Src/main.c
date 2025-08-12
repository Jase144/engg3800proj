/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
#include "main.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "OLED.h"
#include "SD_card.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void myprintf(const char *fmt, ...);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void myprintf(const char *fmt, ...)
{
  static char buffer[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  int len = strlen(buffer);
  HAL_UART_Transmit(&huart2, (uint8_t *)buffer, len, -1);
}

typedef struct
{
  char name[65];
  char artist[65];
  int bpm;
  char difficulty[12]; // N/A, EASY, MEDIUM, HARD, EXPERT, GIGA
  int offset_ms;       // 0..1000
} SongInfo;

static void trim_crlf(char *s)
{
  size_t n = strlen(s);
  while (n && (s[n - 1] == '\r' || s[n - 1] == '\n'))
  {
    s[--n] = 0;
  }
}

static int ends_with_case(const char *s, const char *ext)
{
  size_t ls = strlen(s), le = strlen(ext);
  if (ls < le)
    return 0;
  for (size_t i = 0; i < le; i++)
  {
    char a = s[ls - le + i], b = ext[i];
    if (a >= 'A' && a <= 'Z')
      a += 32;
    if (b >= 'A' && b <= 'Z')
      b += 32;
    if (a != b)
      return 0;
  }
  return 1;
}

static void replace_ext(char *path, const char *newext)
{
  char *dot = NULL;
  for (char *p = path; *p; ++p)
    if (*p == '.')
      dot = p;
  if (!dot)
    return;
  strcpy(dot, newext);
}

// 0:/xxx.tsq
static FRESULT parse_tsq_header(const char *tsq_path, SongInfo *info)
{
  FIL f;
  FRESULT fr;
  memset(info, 0, sizeof(*info));
  strcpy(info->difficulty, "N/A");
  info->offset_ms = 0;

  fr = f_open(&f, tsq_path, FA_READ | FA_OPEN_EXISTING);
  if (fr != FR_OK)
    return fr;

  char line[160];
  for (;;)
  {
    TCHAR *r = f_gets((TCHAR *)line, sizeof(line), &f);
    if (!r)
      break;
    trim_crlf(line);
    if (strcmp(line, "---") == 0)
      break; // end of header

    if (strncmp(line, "Name:", 5) == 0)
    {
      strncpy(info->name, line + 5, sizeof(info->name) - 1);
      while (info->name[0] == ' ')
        memmove(info->name, info->name + 1, strlen(info->name));
    }
    else if (strncmp(line, "Artist:", 7) == 0)
    {
      strncpy(info->artist, line + 7, sizeof(info->artist) - 1);
      while (info->artist[0] == ' ')
        memmove(info->artist, info->artist + 1, strlen(info->artist));
    }
    else if (strncmp(line, "BPM:", 4) == 0)
    {
      info->bpm = atoi(line + 4);
    }
    else if (strncmp(line, "Difficulty:", 11) == 0)
    {
      strncpy(info->difficulty, line + 11, sizeof(info->difficulty) - 1);
      while (info->difficulty[0] == ' ')
        memmove(info->difficulty, info->difficulty + 1, strlen(info->difficulty));
    }
    else if (strncmp(line, "Offset:", 7) == 0)
    {
      info->offset_ms = atoi(line + 7);
    }
    //
  }
  f_close(&f);
  return FR_OK;
}

// read wav header
static FRESULT parse_wav_info(const char *wav_path,
                              uint16_t *channels, uint32_t *sr,
                              uint16_t *bits, uint32_t *duration_ms)
{
  FIL f;
  FRESULT fr;
  *channels = 0;
  *sr = 0;
  *bits = 0;
  *duration_ms = 0;

  fr = f_open(&f, wav_path, FA_READ | FA_OPEN_EXISTING);
  if (fr != FR_OK)
    return fr;

  // RIFF(4) + size(4) + WAVE(4)
  BYTE hdr[12];
  UINT br = 0;
  fr = f_read(&f, hdr, 12, &br);
  if (fr != FR_OK || br != 12 || memcmp(hdr, "RIFF", 4) || memcmp(hdr + 8, "WAVE", 4))
  {
    f_close(&f);
    return FR_INT_ERR;
  }

  uint32_t data_size = 0;
  for (;;)
  {
    BYTE chdr[8];
    br = 0;
    fr = f_read(&f, chdr, 8, &br);
    if (fr != FR_OK)
    {
      f_close(&f);
      return fr;
    }
    if (br != 8)
    {
      break;
    } // EOF

    uint32_t sz = chdr[4] | (chdr[5] << 8) | (chdr[6] << 16) | (chdr[7] << 24);

    if (!memcmp(chdr, "fmt ", 4))
    {
      BYTE fmt[32];
      UINT toread = (sz > sizeof(fmt)) ? sizeof(fmt) : sz;
      fr = f_read(&f, fmt, toread, &br);
      if (fr != FR_OK)
      {
        f_close(&f);
        return fr;
      }
      if (br < 16)
      {
        f_close(&f);
        return FR_INT_ERR;
      }
      uint16_t audioFmt = fmt[0] | (fmt[1] << 8);
      *channels = fmt[2] | (fmt[3] << 8);
      *sr = fmt[4] | (fmt[5] << 8) | (fmt[6] << 16) | (fmt[7] << 24);
      *bits = fmt[14] | (fmt[15] << 8);
      if (sz > toread)
        f_lseek(&f, f_tell(&f) + (sz - toread));
      if (audioFmt != 1)
      {
      }
    }
    else if (!memcmp(chdr, "data", 4))
    {
      data_size = sz;
      f_lseek(&f, f_tell(&f) + sz);
    }
    else
    {
      f_lseek(&f, f_tell(&f) + sz);
    }
    if (sz & 1)
      f_lseek(&f, f_tell(&f) + 1);
  }

  f_close(&f);
  if (*sr && *channels && *bits && data_size)
  {
    uint32_t bytes_per_sec = (*sr) * (*channels) * ((*bits) / 8);
    *duration_ms = (bytes_per_sec) ? (uint32_t)((1000ULL * data_size) / bytes_per_sec) : 0;
    return FR_OK;
  }
  return FR_INT_ERR;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_FATFS_Init();
  MX_SPI2_Init();
  MX_USART2_UART_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */

  OLED_Init(&hi2c2);

  myprintf("\r\n~ tsq + wav info reader ~\r\n");
  HAL_Delay(800);

  FATFS FatFs;
  FRESULT fres;
  fres = f_mount(&FatFs, "0:", 1);
  if (fres != FR_OK)
  {
    myprintf("f_mount error (%d)\r\n", (int)fres);
    while (1)
      ;
  }

  // 2) liat all the file .tsq
  DIR dir;
  FILINFO fno;
  fres = f_opendir(&dir, "0:/");
  if (fres != FR_OK)
  {
    myprintf("f_opendir error (%d)\r\n", (int)fres);
    while (1)
      ;
  }

  myprintf("Scanning 0:/ for .tsq ...\r\n");
  for (;;)
  {
    fres = f_readdir(&dir, &fno);
    if (fres != FR_OK)
    {
      myprintf("f_readdir error (%d)\r\n", (int)fres);
      break;
    }
    if (fno.fname[0] == 0)
      break; // end of dir

    // if the file is a .tsq
    if (!(fno.fattrib & AM_DIR) && ends_with_case(fno.fname, ".tsq"))
    {
      char tsq_path[128] = "0:/";
      strncat(tsq_path, fno.fname, sizeof(tsq_path) - 4);

      SongInfo si;
      FRESULT fr1 = parse_tsq_header(tsq_path, &si);
      if (fr1 != FR_OK)
      {
        myprintf("TSQ parse fail (%d): %s\r\n", (int)fr1, tsq_path);
        continue;
      }

      // print the song info
      myprintf("\r\n[Sequence]\r\n  File: %s\r\n  Name: %s\r\n  Artist: %s\r\n  BPM: %d\r\n  Difficulty: %s\r\n  Offset: %d ms\r\n",
               tsq_path, si.name[0] ? si.name : "(N/A)", si.artist[0] ? si.artist : "(N/A)",
               si.bpm, si.difficulty, si.offset_ms);

      // 3) wav info
      char wav_path[128];
      strncpy(wav_path, tsq_path, sizeof(wav_path) - 1);
      wav_path[sizeof(wav_path) - 1] = 0;
      replace_ext(wav_path, ".wav");
      uint16_t ch = 0, bits = 0;
      uint32_t sr = 0, dur_ms = 0;
      FRESULT fr2 = parse_wav_info(wav_path, &ch, &sr, &bits, &dur_ms);
      if (fr2 == FR_OK)
      {
        uint32_t mm = dur_ms / 60000U;
        uint32_t ss = (dur_ms % 60000U) / 1000U;
        myprintf("[Audio]\r\n  File: %s\r\n  %u ch, %lu Hz, %u-bit\r\n  Length: %02lu:%02lu\r\n",
                 wav_path, (unsigned)ch, (unsigned long)sr, (unsigned)bits,
                 (unsigned long)mm, (unsigned long)ss);
      }
      else
      {
        myprintf("[Audio]\r\n  File: %s\r\n  open/parse failed (%d)\r\n", wav_path, (int)fr2);
      }
    }
  }
  f_closedir(&dir);

  // 4) unmount the filesystem
  f_mount(NULL, "0:", 0);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
