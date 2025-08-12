/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body ******************************************************************************
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "intern_sysclock.h"
#include "periph_64ledmatrix.h"
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

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* Configs  */
// Pins used
#define PINDEC_LEDARR_DIN 0 // PA0 - output pin connecting to RGB array DIN

// General pin masks
#define TWO_BIT_PIN_MASK(mask, pinNumber) (mask << pinNumber * 2)
#define ONE_BIT_PIN_MASK(mask, pinNumber) (mask << pinNumber)

/* GPIO Pin config masks */
#define PINHIGH_GPIO_MODER 0x03 // 0b11 since each pin has 2 bit MODER reg
#define PINHIGH_GPIO_OTYPER 0x01 // 0b1 since each pin has 1 bit OTYPER reg
#define PINHIGH_GPIO_OSPEEDR 0x03
#define PINHIGH_GPIO_PUPDR 0x03

#define PINSET_GPIO_MODER_DIN 0x02 // 0b10 for alternate function mode - pwm
#define PINSET_GPIO_OTYPER_DIN 0x00 // 0b1 for output push-pull
#define PINSET_GPIO_OSPEEDR_DIN 0x02 // 0b10 for high speed (max speed: 0b11)
#define PINSET_GPIO_PUPDR_DIN 0x00 // ob01 to enable pull-up resistor

#define PORTSET_GPIO_DIN(maskType, value) (maskType(value, PINDEC_LEDARR_DIN))

/* PWM timer config masks */
#define ALIAS_PWM_TIMER TIM2 // TIM2 is used as the PWM timer

// CR1 configs
#define PINSET_PWM_CR1_ARPE_DIN 1 // enable auto-reload preload
#define BITPOS_PWM_CR1_ARPE_DIN 7
#define PINSET_PWM_CR1_CEN_DIN 1
#define BITPOS_PWM_CR1_CEN_DIN 0

// CCMR1 configs
#define PINSET_PWM_CCMR1_OC1M_DIN 0x06 // 0b0110 for PWM Mode 2
#define BITPOS_PWM_CCMR1_OC1M_DIN 4 // starts at bit 4
#define PINSET_PWM_CCMR1_OC1PE_DIN 1 // register gets preload when written and writes to the ARR at each update event
#define BITPOS_PWM_CCMR1_OC1PE_DIN 3 // starts at bit 4

//PSC configs
#define PINSET_PWM_PSC_DIN 0 // prescaler value - CK_CNT = f/(PSC + 1)

//ARR configs
#define PINSET_PWM_ARR_DIN 71 // max value the counter reaches

// CCR1 configs
#define PINSET_PWM_CCR1_DIN 48 // ticks when output toggles - e.g. for PWM mode 1, at 48 ticks, it goes from high to low

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
  opSystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_DMA_Init();
  /* USER CODE BEGIN 2 */
  reg_64ledmatrix_init_external();

  //transferComplete = 1;
  //reg_64ledmatrix_senddata(0);
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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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
