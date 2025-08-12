// OLED.c
#include "OLED.h"
#include "OLED_Font.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_i2c.h"

static I2C_HandleTypeDef *oled_i2c;
#define OLED_ADDR 0x78

static void OLED_WriteCommand(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd};
    HAL_I2C_Master_Transmit(oled_i2c, OLED_ADDR, data, 2, HAL_MAX_DELAY);
}

static void OLED_WriteData(uint8_t dat)
{
    uint8_t data[2] = {0x40, dat};
    HAL_I2C_Master_Transmit(oled_i2c, OLED_ADDR, data, 2, HAL_MAX_DELAY);
}

static void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	X+=2;
    OLED_WriteCommand(0xB0 | Y);
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));
    OLED_WriteCommand(0x00 | (X & 0x0F));
}

void OLED_Clear(void)
{  
    for (uint8_t j = 0; j < 8; j++)
    {
        OLED_SetCursor(j, 0);
        for (uint8_t i = 0; i < 128; i++)
        {
            OLED_WriteData(0x00);
        }
    }
}

void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{      
    OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);
    for (uint8_t i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i]);
    }
    OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);
    for (uint8_t i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);
    }
}

void OLED_ShowString(uint8_t Line, uint8_t Column, const char *String)
{
    for (uint8_t i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}


static uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
        Result *= X;
    return Result;
}

void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    for (uint8_t i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint32_t absNum = (Number >= 0) ? Number : -Number;
    OLED_ShowChar(Line, Column, (Number >= 0) ? '+' : '-');
    for (uint8_t i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i + 1, absNum / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t digit;
    for (uint8_t i = 0; i < Length; i++)
    {
        digit = Number / OLED_Pow(16, Length - i - 1) % 16;
        OLED_ShowChar(Line, Column + i, digit < 10 ? digit + '0' : digit - 10 + 'A');
    }
}

void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    for (uint8_t i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
    }
}

void OLED_Init(I2C_HandleTypeDef *hi2c)
{
    oled_i2c = hi2c;
    HAL_Delay(100);

    OLED_WriteCommand(0xAE);
    OLED_WriteCommand(0xD5);
    OLED_WriteCommand(0x80);
    OLED_WriteCommand(0xA8);
    OLED_WriteCommand(0x3F);
    OLED_WriteCommand(0xD3);
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0x40);
    OLED_WriteCommand(0xA1);
    OLED_WriteCommand(0xC8);
    OLED_WriteCommand(0xDA);
    OLED_WriteCommand(0x12);
    OLED_WriteCommand(0x81);
    OLED_WriteCommand(0xCF);
    OLED_WriteCommand(0xD9);
    OLED_WriteCommand(0xF1);
    OLED_WriteCommand(0xDB);
    OLED_WriteCommand(0x30);
    OLED_WriteCommand(0xA4);
    OLED_WriteCommand(0xA6);
    OLED_WriteCommand(0x8D);
    OLED_WriteCommand(0x14);
    OLED_WriteCommand(0xAF);

    OLED_Clear();
}

void OLED_UpdateScore(const char *result, uint32_t score, uint32_t combo)
{
    OLED_Clear();
    OLED_ShowString(1, 1, "Result:");
    OLED_ShowString(1, 9, result);         
    
    OLED_ShowString(2, 1, "Score:");
    OLED_ShowNum(2, 8, score, 5);  

    OLED_ShowString(3, 1, "Combo:");
    OLED_ShowNum(3, 8, combo, 3); 
}

void OLED_ShowSummary(uint32_t score, uint32_t max_combo, uint32_t miss_count)
{
    OLED_Clear();
    OLED_ShowString(1, 1, "GAME OVER");
    OLED_ShowString(2, 1, "Score:");
    OLED_ShowNum(2, 8, score, 5);

    OLED_ShowString(3, 1, "MaxCombo:");
    OLED_ShowNum(3, 11, max_combo, 3);

    OLED_ShowString(4, 1, "Miss:");
    OLED_ShowNum(4, 7, miss_count, 3);
}

void OLED_ShowSongInfo(const char* name, const char* artist, uint16_t bpm, const char* diff)
{
    OLED_Clear();
    OLED_ShowString(1, 1, name);         
    OLED_ShowString(2, 1, artist);   
    OLED_ShowString(3, 1, "BPM:");
    OLED_ShowNum(3, 6, bpm, 3);

    OLED_ShowString(4, 1, "Diff:");
    OLED_ShowString(4, 7, diff); 
}

void OLED_ShowError(const char* line1, const char* line2)
{
    OLED_Clear();
    OLED_ShowString(1, 1, "ERROR!");
    OLED_ShowString(2, 1, line1);
    OLED_ShowString(3, 1, line2);

}

void OLED_ShowChar_8x8(uint8_t Line, uint8_t Column, char Char)
{
    if (Char < ' ' || Char > '~') return;    // out of font range
    uint8_t idx = Char - ' ';
    // set page (Line−1) and column pixel = (Column−1)*8
    OLED_SetCursor(Line - 1, (Column - 1) * 8);
    // send 8 bytes for 8 columns
    for (uint8_t i = 0; i < 8; i++) {
        OLED_WriteData(FontData_8x8[idx * 8 + i]);
    }
}
void OLED_ShowString_8x8(uint8_t Line, uint8_t Column, const char *String)
{
    uint8_t col = Column;
    while (*String) {
        OLED_ShowChar_8x8(Line, col++, *String++);
    }
}
