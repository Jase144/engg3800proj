// source code for tim2 DMA
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_rcc.h"
#include "intern_tim2dma.h"

void reg_tim2dma_initdma_external(void) {
    //TIM2->CR2 |= (1 << TIM_CR2_CCDS_Pos);
    //TIM2->DIER |= (1 << TIM_DIER_UDE_Pos);

    //DMA->CCR &= ~(0x02 << DMA_CCR_PSIZE_Pos);
    //DMA->CCR |= (0x01 << DMA_CCR_DIR_Pos);
    //DMA->
    //DMA->CCR |= (0x01 << DMA_CCR_EN_Pos);

}
