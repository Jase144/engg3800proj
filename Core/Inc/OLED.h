// OLED.h
#ifndef __OLED_H
#define __OLED_H

#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_i2c.h"

void OLED_Init(I2C_HandleTypeDef *hi2c);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, const char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_UpdateScore(const char *result, uint32_t score, uint32_t combo);
void OLED_ShowSongInfo(const char* name, const char* artist, uint16_t bpm, const char* diff);
void OLED_ShowSummary(uint32_t score, uint32_t max_combo, uint32_t miss_count);
void OLED_ShowChar_8x8(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString_8x8(uint8_t Line, uint8_t Column, const char *String);

#endif
