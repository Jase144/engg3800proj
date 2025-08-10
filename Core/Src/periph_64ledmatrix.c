// led matrix src code

#include "stm32l4xx.h"
#include "stm32l4xx_hal.h"
#include "periph_64ledmatrix.h"
#include "stm32l4xx_hal_dma.h"
#include "stm32l4xx_hal_rcc.h"
#include "intern_tim2dma.h"

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

/* function decs */
void reg_64ledmatrix_init_external(void);
void reg_64ledmatrix_initgpio_internal(void);
void reg_64ledmatrix_inittim2_internal(void);

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

void ledmatrix64_lib_error_handler(void);


void reg_64ledmatrix_init_external(void) {


    // GPIO config - each instruction does both clear and set the pins
    reg_64ledmatrix_initgpio_internal();

    __HAL_RCC_TIM2_CLK_ENABLE();


    reg_64ledmatrix_inittim2_internal();
}


void reg_64ledmatrix_initgpio_internal(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIOA->MODER = ((GPIOA->MODER & ~PORTSET_GPIO_DIN(TWO_BIT_PIN_MASK,
                                                     PINHIGH_GPIO_MODER)) |
                    PORTSET_GPIO_DIN(TWO_BIT_PIN_MASK, PINSET_GPIO_MODER_DIN));

    GPIOA->OSPEEDR = ((GPIOA->OSPEEDR & ~PORTSET_GPIO_DIN(TWO_BIT_PIN_MASK,
                                                     PINHIGH_GPIO_OSPEEDR)) |
                    PORTSET_GPIO_DIN(TWO_BIT_PIN_MASK, PINSET_GPIO_OSPEEDR_DIN));

    GPIOA->PUPDR = ((GPIOA->PUPDR & ~PORTSET_GPIO_DIN(TWO_BIT_PIN_MASK,
                                                     PINHIGH_GPIO_PUPDR)) |
                    PORTSET_GPIO_DIN(TWO_BIT_PIN_MASK, PINSET_GPIO_PUPDR_DIN));

    GPIOA->OTYPER = ((GPIOA->OTYPER & ~PORTSET_GPIO_DIN(ONE_BIT_PIN_MASK,
                                                     PINHIGH_GPIO_OTYPER)) |
                    PORTSET_GPIO_DIN(TWO_BIT_PIN_MASK, PINSET_GPIO_OTYPER_DIN));

    GPIOA -> AFR[0] |= (1 << 0*4);

}

uint32_t ccr1_values[4] = {21, 21, 21, 14};
void reg_64ledmatrix_inittim2_internal(void) {
    DMA_HandleTypeDef hdma_tim2_up;
    TIM_HandleTypeDef tim2Struct;
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};
    /* USER CODE BEGIN TIM2_Init 1 */

    /* USER CODE END TIM2_Init 1 */

    __HAL_RCC_DMA1_CLK_ENABLE();

    tim2Struct.Instance = TIM2;
    tim2Struct.Init.Prescaler = 0;
    tim2Struct.Init.CounterMode = TIM_COUNTERMODE_UP;
    tim2Struct.Init.Period = 21;
    tim2Struct.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tim2Struct.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    /*
    if (HAL_TIM_Base_Init(&tim2Struct) != HAL_OK)
    {
      ledmatrix64_lib_error_handler();
    }*/
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    /*
    if (HAL_TIM_ConfigClockSource(&tim2Struct, &sClockSourceConfig) != HAL_OK)
    {
      ledmatrix64_lib_error_handler();
    }
    if (HAL_TIM_PWM_Init(&tim2Struct) != HAL_OK)
    {
      ledmatrix64_lib_error_handler();
    }*/
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    /*
    if (HAL_TIMEx_MasterConfigSynchronization(&tim2Struct, &sMasterConfig) != HAL_OK)
    {
      ledmatrix64_lib_error_handler();
    }*/
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 14;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    /*
    if (HAL_TIM_PWM_ConfigChannel(&tim2Struct, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
      ledmatrix64_lib_error_handler();
    }*/

    /* USER CODE BEGIN TIM2_Init 2 */
    //reg_tim2dma_initdma_external();


    //DMA1_Channel2->CCR |= (0x01 << DMA_CCR_EN_Pos);

    //TIM2->CR2 |= (1 << TIM_CR2_CCDS_Pos);
    //TIM2->DCR |= (0x02 < TIM_DCR_DBL_Pos) | (0xE < TIM_DCR_DBA_Pos);
    //TIM2->DIER |= (1 << TIM_DIER_UDE_Pos);
    /* USER CODE END TIM2_Init 2 */
    /*
    __HAL_LINKDMA(&tim2Struct, hdma[TIM_DMA_ID_UPDATE], hdma_tim2_up);

     __HAL_RCC_DMA1_CLK_ENABLE();

    hdma_tim2_up.Instance = DMA1_Channel5;  // Example channel
    hdma_tim2_up.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_tim2_up.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tim2_up.Init.MemInc = DMA_MINC_ENABLE;
    hdma_tim2_up.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_tim2_up.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_tim2_up.Init.Mode = DMA_NORMAL;
    hdma_tim2_up.Init.Priority = DMA_PRIORITY_LOW;
    HAL_DMA_Init(&hdma_tim2_up);

    // DMAMUX: set request ID 25 for TIM2_UP
    DMA1_CSELR->CSELR = (DMA1_CSELR->CSELR & ~(0xF << (4 * (4)))) | (25 << (4 * (4)));
    HAL_DMA_Start(&hdma_tim2_up, (uint32_t)ccr1_values, (uint32_t)&TIM2->CCR1, 4);
    HAL_TIM_MspPostInit(&tim2Struct);
    HAL_TIM_PWM_Start(&tim2Struct, TIM_CHANNEL_1);
    __HAL_TIM_ENABLE_DMA(&tim2Struct, TIM_DMA_UPDATE);
*/



//__HAL_TIM_ENABLE_DMA(&tim2Struct, TIM_DMA_CC1);

//__HAL_LINKDMA(&tim2Struct, hdma[TIM_DMA_ID_UPDATE], hdma_tim2_up);

// DMA config
hdma_tim2_up.Instance = DMA1_Channel2;
hdma_tim2_up.Init.Direction = DMA_MEMORY_TO_PERIPH;
hdma_tim2_up.Init.PeriphInc = DMA_PINC_DISABLE;
hdma_tim2_up.Init.MemInc = DMA_MINC_ENABLE;
hdma_tim2_up.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
hdma_tim2_up.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
hdma_tim2_up.Init.Mode = DMA_CIRCULAR; // stop after last
hdma_tim2_up.Init.Priority = DMA_PRIORITY_LOW;
HAL_DMA_Init(&hdma_tim2_up);

// DMAMUX: TIM2_UP request = 25
DMA1_CSELR->CSELR = (DMA1_CSELR->CSELR & ~(0xF << (4 * 1))) | (57 << (4 * 1));

// Start DMA

// Start PWM + DMA
__HAL_LINKDMA(&tim2Struct, hdma[TIM_DMA_ID_CC1], hdma_tim2_up);

//DMA1_Channel2->CCR |= (0x01 << DMA_CCR_EN_Pos);
__HAL_TIM_ENABLE_DMA(&tim2Struct, TIM_DMA_UPDATE);

HAL_DMA_Start(&hdma_tim2_up,
              (uint32_t)ccr1_values,
              (uint32_t)&TIM2->CCR1,
              sizeof(ccr1_values) / sizeof(uint16_t));
//__HAL_TIM_ENABLE_DMA(&tim2Struct, TIM_DMA_UPDATE);

HAL_TIM_Base_Init(&tim2Struct);
//HAL_TIM_ConfigClockSource(&tim2Struct, &sClockSourceConfig);
HAL_TIM_PWM_Init(&tim2Struct);
//HAL_TIMEx_MasterConfigSynchronization(&tim2Struct, &sMasterConfig);
HAL_TIM_PWM_ConfigChannel(&tim2Struct, &sConfigOC, TIM_CHANNEL_1);
HAL_TIM_MspPostInit(&tim2Struct);
HAL_TIM_PWM_Start(&tim2Struct, TIM_CHANNEL_1);

}

void ledmatrix64_lib_error_handler(void)
{
    /* USER CODE BEGIN ledmatrix64_lib_error_handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END ledmatrix64_lib_error_handler_Debug */
}

void reg_64ledmatrix_senddata() {

}

