// source code for tim2 DMA
#include "stm32l433xx.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_rcc.h"
#include "intern_tim2dma.h"
#include "stm32l4xx_hal_dma.h"
uint8_t arr[195] = {0};
void reg_tim2dma_initdma_external(void) {

    __HAL_RCC_DMA1_CLK_ENABLE();
    DMA1_Channel2->CCR |= (0x01 << DMA_CCR_DIR_Pos);
    DMA1_Channel2->CCR &= ~(0x03 << DMA_CCR_PSIZE_Pos);
    DMA1_Channel2->CCR &= ~(0x03 << DMA_CCR_MSIZE_Pos);
    DMA1_Channel2->CCR |= (0x01 << DMA_CCR_MINC_Pos);
    TIM2->CR2 |= (1 << TIM_CR2_CCDS_Pos);
    DMA1_Channel2->CNDTR = 195;
    for (uint8_t i = 0; i < 193; i++) {
        if (i > 100) {
            arr[i] = 0;
        } else {
            arr[i] = 0;
        }
    }

    arr[192] = 21;
    arr[193] = 0;
    arr[194] = 21;
    DMA1_Channel2->CMAR = (uint32_t)&arr;
    DMA1_Channel2->CPAR =(uint32_t)&(TIM2->DMAR);
    // Set DMAMUX1 Channel 2 request to TIM2 update (request 8)

    TIM2->DCR |= (0x02 < TIM_DCR_DBL_Pos) | (0xE < TIM_DCR_DBA_Pos);
    TIM2->DIER |= (1 << TIM_DIER_UDE_Pos);

}
