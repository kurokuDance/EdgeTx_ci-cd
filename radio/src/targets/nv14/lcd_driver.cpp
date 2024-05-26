/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "stm32_hal_ll.h"
#include "stm32_hal.h"
#include "opentx_types.h"
#include "dma2d.h"
#include "hal.h"
#include "delays_driver.h"
#include "debug.h"
#include "lcd.h"
#include "lcd_driver.h"

static LTDC_HandleTypeDef hltdc;
static void* initialFrameBuffer = nullptr;

static volatile uint8_t _frame_addr_reloaded = 0;

static void startLcdRefresh(lv_disp_drv_t *disp_drv, uint16_t *buffer,
                            const rect_t &copy_area)
{
  (void)disp_drv;
  (void)copy_area;

  LTDC_Layer1->CFBAR &= ~(LTDC_LxCFBAR_CFBADD);
  LTDC_Layer1->CFBAR = (uint32_t)buffer;
  // reload shadow registers on vertical blank
  _frame_addr_reloaded = 0;
  LTDC->SRCR = LTDC_SRCR_VBR;
  __HAL_LTDC_ENABLE_IT(&hltdc, LTDC_IT_LI);

  // wait for reload
  // TODO: replace through some smarter mechanism without busy wait
  while(_frame_addr_reloaded == 0);
}

lcdSpiInitFucPtr lcdInitFunction;
lcdSpiInitFucPtr lcdOffFunction;
lcdSpiInitFucPtr lcdOnFunction;
uint32_t lcdPixelClock;

volatile uint8_t LCD_ReadBuffer[24] = { 0, 0 };

static void LCD_Delay(void) {
  volatile unsigned int i;

  for (i = 0; i < 20; i++) {
    ;
  }
}

enum ENUM_IO_SPEED
{
    IO_SPEED_LOW,
    IO_SPEED_MID,
    IO_SPEED_QUICK,
    IO_SPEED_HIGH
};

enum ENUM_IO_MODE
{
    IO_MODE_INPUT,
    IO_MODE_OUTPUT,
    IO_MODE_ALTERNATE,
    IO_MODE_ANALOG
};

static void LCD_AF_GPIOConfig(void) {
  LL_GPIO_InitTypeDef GPIO_InitStructure;
  LL_GPIO_StructInit(&GPIO_InitStructure);
  /*
   -----------------------------------------------------------------------------
   LCD_CLK <-> PG.07 | LCD_HSYNC <-> PI.12 | LCD_R3 <-> PJ.02 | LCD_G5 <-> PK.00
   | LCD VSYNC <-> PI.13 | LCD_R4 <-> PJ.03 | LCD_G6 <-> PK.01
   |                     | LCD_R5 <-> PJ.04 | LCD_G7 <-> PK.02
   |                     | LCD_R6 <-> PJ.05 | LCD_B4 <-> PK.03
   |                     | LCD_R7 <-> PJ.06 | LCD_B5 <-> PK.04
   |                     | LCD_G2 <-> PJ.09 | LCD_B6 <-> PK.05
   |                     | LCD_G3 <-> PJ.10 | LCD_B7 <-> PK.06
   |                     | LCD_G4 <-> PJ.11 | LCD_DE <-> PK.07
   |                     | LCD_B3 <-> PJ.15 |
   */

  // GPIOG configuration
  GPIO_InitStructure.Pin        = LL_GPIO_PIN_7;
  GPIO_InitStructure.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStructure.Mode       = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStructure.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStructure.Pull       = LL_GPIO_PULL_NO;
  GPIO_InitStructure.Alternate  = LL_GPIO_AF_14; // AF LTDC
  LL_GPIO_Init(GPIOG, &GPIO_InitStructure);

  // GPIOI configuration
  GPIO_InitStructure.Pin        = LL_GPIO_PIN_12 | LL_GPIO_PIN_13;
  LL_GPIO_Init(GPIOI, &GPIO_InitStructure);

  // GPIOJ configuration
  GPIO_InitStructure.Pin = LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_6 | LL_GPIO_PIN_9 | LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_15;
  LL_GPIO_Init(GPIOJ, &GPIO_InitStructure);

  // GPIOK configuration
  GPIO_InitStructure.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_6 | LL_GPIO_PIN_7;
  LL_GPIO_Init(GPIOK, &GPIO_InitStructure);
}

static void lcdSpiConfig(void) {
  LL_GPIO_InitTypeDef GPIO_InitStructure;
  LL_GPIO_StructInit(&GPIO_InitStructure);

  GPIO_InitStructure.Pin        = LCD_SPI_SCK_GPIO_PIN | LCD_SPI_MOSI_GPIO_PIN;
  GPIO_InitStructure.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStructure.Mode       = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStructure.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStructure.Pull       = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LCD_SPI_GPIO, &GPIO_InitStructure);

  GPIO_InitStructure.Pin        = LCD_SPI_CS_GPIO_PIN;
  GPIO_InitStructure.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStructure.Mode       = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStructure.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStructure.Pull       = LL_GPIO_PULL_UP;
  LL_GPIO_Init(LCD_SPI_GPIO, &GPIO_InitStructure);

  GPIO_InitStructure.Pin        = LCD_NRST_GPIO_PIN;
  LL_GPIO_Init(LCD_NRST_GPIO, &GPIO_InitStructure);

  /* Set the chip select pin aways low */
  LCD_CS_LOW();
}

void lcdDelay() {
  delay_01us(1);
}

unsigned char LCD_ReadByteOnFallingEdge(void) {
  unsigned int i;
  unsigned char ReceiveData = 0;

  LCD_MOSI_HIGH();
  LCD_MOSI_AS_INPUT();

  for (i = 0; i < 8; i++) {
    LCD_DELAY();
    LCD_SCK_HIGH();
    LCD_DELAY();
    LCD_DELAY();
    ReceiveData <<= 1;

    LCD_SCK_LOW();
    LCD_DELAY();
    LCD_DELAY();
    if (LCD_READ_DATA_PIN()) {
      ReceiveData |= 0x01;
    }
  }

  LCD_MOSI_AS_OUTPUT();

  return (ReceiveData);
}

static void lcdWriteByte(uint8_t data_enable, uint8_t byte) {
  LCD_SCK_LOW();
  lcdDelay();

  if (data_enable) {
    LCD_MOSI_HIGH();
  } else {
    LCD_MOSI_LOW();
  }

  LCD_SCK_HIGH();
  lcdDelay();

  for (int i = 0; i < 8; i++) {
    LCD_SCK_LOW();
    lcdDelay();

    if (byte & 0x80) {
      LCD_MOSI_HIGH();
    } else {
      LCD_MOSI_LOW();
    }

    LCD_SCK_HIGH();
    byte <<= 1;

    lcdDelay();
  }

  LCD_SCK_LOW();
}

unsigned char LCD_ReadByte(void) {
  unsigned int i;
  unsigned char ReceiveData = 0;

  LCD_MOSI_HIGH();
  LCD_MOSI_AS_INPUT();
  for (i = 0; i < 8; i++) {
    LCD_SCK_LOW();
    LCD_DELAY();
    LCD_DELAY();
    ReceiveData <<= 1;
    LCD_SCK_HIGH();
    LCD_DELAY();
    LCD_DELAY();
    if (LCD_READ_DATA_PIN()) {
      ReceiveData |= 0x01;
    }
  }
  LCD_SCK_LOW();
  LCD_MOSI_AS_OUTPUT();
  return (ReceiveData);
}

unsigned char LCD_ReadRegister(unsigned char Register) {
  unsigned char ReadData = 0;

  lcdWriteByte(0, Register);
  LCD_DELAY();
  LCD_DELAY();
  ReadData = LCD_ReadByte();
  return (ReadData);
}

void lcdWriteCommand(uint8_t command) {
  lcdWriteByte(0, command);
}

void lcdWriteData(uint8_t data) {
  lcdWriteByte(1, data);
}

void LCD_HX8357D_Init(void) {
#if 0
  lcdWriteCommand(0x11);
  delay_ms(200);

  lcdWriteCommand(0xB9);
  lcdWriteData(0xFF);
  lcdWriteData(0x83);
  lcdWriteData(0x57);

  lcdWriteCommand(0xB1);
  lcdWriteData(0x00);
  lcdWriteData(0x14);
  lcdWriteData(0x1C);
  lcdWriteData(0x1C);
  lcdWriteData(0xC7);
  lcdWriteData(0x21);

  lcdWriteCommand(0xB3);
  lcdWriteData(0x83);
  lcdWriteData(0x00);
  lcdWriteData(0x06);
  lcdWriteData(0x06);

  lcdWriteCommand(0xB4);
  lcdWriteData(0x11);
  lcdWriteData(0x40);
  lcdWriteData(0x00);
  lcdWriteData(0x2A);
  lcdWriteData(0x2A);
  lcdWriteData(0x20);
  lcdWriteData(0x4E);

  lcdWriteCommand(0xB5);
  lcdWriteData(0x03);
  lcdWriteData(0x03);

  lcdWriteCommand(0xB6);
  lcdWriteData(0x38);

  lcdWriteCommand(0xC0);
  lcdWriteData(0x24);
  lcdWriteData(0x24);
  lcdWriteData(0x00);
  lcdWriteData(0x10);
  lcdWriteData(0xc8);
  lcdWriteData(0x08);

  lcdWriteCommand(0xC2);
  lcdWriteData(0x00);
  lcdWriteData(0x08);
  lcdWriteData(0x04);

  lcdWriteCommand(0xCC);
  lcdWriteData(0x00);

//GAMMA 2.5"
  lcdWriteCommand(0xE0);
  lcdWriteData(0x00);
  lcdWriteData(0x06);
  lcdWriteData(0x0D);
  lcdWriteData(0x18);
  lcdWriteData(0x23);
  lcdWriteData(0x3B);
  lcdWriteData(0x45);
  lcdWriteData(0x4D);
  lcdWriteData(0x4D);
  lcdWriteData(0x46);
  lcdWriteData(0x40);
  lcdWriteData(0x37);
  lcdWriteData(0x34);
  lcdWriteData(0x2F);
  lcdWriteData(0x2B);
  lcdWriteData(0x21);
  lcdWriteData(0x00);
  lcdWriteData(0x06);
  lcdWriteData(0x0D);
  lcdWriteData(0x18);
  lcdWriteData(0x23);
  lcdWriteData(0x3B);
  lcdWriteData(0x45);
  lcdWriteData(0x4D);
  lcdWriteData(0x4D);
  lcdWriteData(0x46);
  lcdWriteData(0x40);
  lcdWriteData(0x37);
  lcdWriteData(0x34);
  lcdWriteData(0x2F);
  lcdWriteData(0x2B);
  lcdWriteData(0x21);
  lcdWriteData(0x00);
  lcdWriteData(0x01);

  lcdWriteCommand(0x3A);
  lcdWriteData(0x66);

  lcdWriteCommand(0x36);
  lcdWriteData(0x08);

  lcdWriteCommand(0x29);
  delay_ms(10);
#else
  delay_ms(50);
  lcdWriteCommand(0xB9); //EXTC
  lcdWriteData(0xFF); //EXTC
  lcdWriteData(0x83); //EXTC
  lcdWriteData(0x57); //EXTC
  delay_ms(5);

  lcdWriteCommand(0x3A);
  lcdWriteData(0x65); //262k

  lcdWriteCommand(0xB3); //COLOR FORMAT
  lcdWriteData(0x83); //SDO_EN,BYPASS,EPF[1:0],0,0,RM,DM  //43

  lcdWriteCommand(0xB6); //
  lcdWriteData(0x5a); //VCOMDC

  lcdWriteCommand(0x35); // TE ON
  lcdWriteData(0x01);

  lcdWriteCommand(0xB0);
  lcdWriteData(0x68); //70Hz

  lcdWriteCommand(0xCC); // Set Panel
  lcdWriteData(0x00); //

  lcdWriteCommand(0xB1); //
  lcdWriteData(0x00); //
  lcdWriteData(0x11); //BT
  lcdWriteData(0x1C); //VSPR
  lcdWriteData(0x1C); //VSNR
  lcdWriteData(0x83); //AP
  lcdWriteData(0x48); //FS  0xAA

  lcdWriteCommand(0xB4); //
  lcdWriteData(0x02); //NW
  lcdWriteData(0x40); //RTN
  lcdWriteData(0x00); //DIV
  lcdWriteData(0x2A); //DUM
  lcdWriteData(0x2A); //DUM
  lcdWriteData(0x0D); //GDON
  lcdWriteData(0x78); //GDOFF  0x4F
  lcdWriteCommand(0xC0); //STBA
  lcdWriteData(0x50); //OPON
  lcdWriteData(0x50); //OPON
  lcdWriteData(0x01); //
  lcdWriteData(0x3C); //
  lcdWriteData(0x1E); //
  lcdWriteData(0x08); //GEN

  /*
   lcdWriteCommand(0xE0); //
   lcdWriteData(0x02); //1
   lcdWriteData(0x06); //2
   lcdWriteData(0x09); //3
   lcdWriteData(0x1C); //4
   lcdWriteData(0x27); //5
   lcdWriteData(0x3C); //6
   lcdWriteData(0x48); //7
   lcdWriteData(0x50); //8
   lcdWriteData(0x49); //9
   lcdWriteData(0x42); //10
   lcdWriteData(0x3E); //11
   lcdWriteData(0x35); //12
   lcdWriteData(0x31); //13
   lcdWriteData(0x2A); //14
   lcdWriteData(0x28); //15
   lcdWriteData(0x03); //16
   lcdWriteData(0x02); //17 v1
   lcdWriteData(0x06); //18
   lcdWriteData(0x09); //19
   lcdWriteData(0x1C); //20
   lcdWriteData(0x27); //21
   lcdWriteData(0x3C); //22
   lcdWriteData(0x48); //23
   lcdWriteData(0x50); //24
   lcdWriteData(0x49); //25
   lcdWriteData(0x42); //26
   lcdWriteData(0x3E); //27
   lcdWriteData(0x35); //28
   lcdWriteData(0x31); //29
   lcdWriteData(0x2A); //30
   lcdWriteData(0x28); //31
   lcdWriteData(0x03); //32
   lcdWriteData(0x44); //33
   lcdWriteData(0x01); //34
   */
  lcdWriteCommand(0xE0);
  lcdWriteData(0x00);
  lcdWriteData(0x06);
  lcdWriteData(0x0D);
  lcdWriteData(0x18);
  lcdWriteData(0x23);
  lcdWriteData(0x3B);
  lcdWriteData(0x45);
  lcdWriteData(0x4D);
  lcdWriteData(0x4D);
  lcdWriteData(0x46);
  lcdWriteData(0x40);
  lcdWriteData(0x37);
  lcdWriteData(0x34);
  lcdWriteData(0x2F);
  lcdWriteData(0x2B);
  lcdWriteData(0x21);
  lcdWriteData(0x00);
  lcdWriteData(0x06);
  lcdWriteData(0x0D);
  lcdWriteData(0x18);
  lcdWriteData(0x23);
  lcdWriteData(0x3B);
  lcdWriteData(0x45);
  lcdWriteData(0x4D);
  lcdWriteData(0x4D);
  lcdWriteData(0x46);
  lcdWriteData(0x40);
  lcdWriteData(0x37);
  lcdWriteData(0x34);
  lcdWriteData(0x2F);
  lcdWriteData(0x2B);
  lcdWriteData(0x21);
  lcdWriteData(0x00);
  lcdWriteData(0x01);
  lcdWriteCommand(0x36);
  lcdWriteData(0x18);

  lcdWriteCommand(0x11); // SLPOUT
  delay_ms(200);

  lcdWriteCommand(0x29); // Display On
  delay_ms(25);
  lcdWriteCommand(0x2C);

#endif

}

void LCD_HX8357D_On(void) {
  lcdWriteCommand(0x29);
  lcdWriteCommand(0x22);
}

void LCD_HX8357D_Off(void) {
  lcdWriteCommand(0x22);
  lcdWriteCommand(0x28);
}

unsigned int LCD_HX8357D_ReadID(void) {
  int ID = 0;

  return (ID);
}

void LCD_ILI9481_Init(void) {
  lcdWriteCommand(0x11);
  delay_ms(120);

  lcdWriteCommand(0xE4);
  lcdWriteData(0x0A);

  lcdWriteCommand(0xF0);
  lcdWriteData(0x01);

  lcdWriteCommand(0xF3);
  lcdWriteData(0x02);
  lcdWriteData(0x1A);

  lcdWriteCommand(0xD0);
  lcdWriteData(0x07);
  lcdWriteData(0x42);
  lcdWriteData(0x1B);

  lcdWriteCommand(0xD1);
  lcdWriteData(0x00);
  lcdWriteData(0x00); //04
  lcdWriteData(0x1A);

  lcdWriteCommand(0xD2);
  lcdWriteData(0x01);
  lcdWriteData(0x00); //11

  lcdWriteCommand(0xC0);
  lcdWriteData(0x10);
  lcdWriteData(0x3B); //
  lcdWriteData(0x00); //
  lcdWriteData(0x02);
  lcdWriteData(0x11);

  lcdWriteCommand(0xC5);
  lcdWriteData(0x03);

  lcdWriteCommand(0xC8);
  lcdWriteData(0x00);
  lcdWriteData(0x01);
  lcdWriteData(0x47);
  lcdWriteData(0x60);
  lcdWriteData(0x04);
  lcdWriteData(0x16);
  lcdWriteData(0x03);
  lcdWriteData(0x67);
  lcdWriteData(0x67);
  lcdWriteData(0x06);
  lcdWriteData(0x0F);
  lcdWriteData(0x00);

  lcdWriteCommand(0x36);
  lcdWriteData(0x08);

  lcdWriteCommand(0x3A);
  lcdWriteData(0x66); //0x55=65k color, 0x66=262k color.

  lcdWriteCommand(0x2A);
  lcdWriteData(0x00);
  lcdWriteData(0x00);
  lcdWriteData(0x01);
  lcdWriteData(0x3F);

  lcdWriteCommand(0x2B);
  lcdWriteData(0x00);
  lcdWriteData(0x00);
  lcdWriteData(0x01);
  lcdWriteData(0xE0);

  lcdWriteCommand(0xB4);
  lcdWriteData(0x11);

  lcdWriteCommand(0xc6);
  lcdWriteData(0x82);

  delay_ms(120);

  lcdWriteCommand(0x21);
  lcdWriteCommand(0x29);
  lcdWriteCommand(0x2C);

}

void LCD_ILI9481_On(void) {
  lcdWriteCommand(0x29);
}

void LCD_ILI9481_Off(void) {
  lcdWriteCommand(0x28);
}

unsigned int LCD_ILI9481_ReadID(void) {
#if 1
  /* Have a issue here */
  return 0;
#else
  int ID = 0;
  int Data;


  lcdWriteByte(0, 0xBF);

  Data = LCD_ReadByteOnFallingEdge();
  Data = LCD_ReadByteOnFallingEdge();
  ID = LCD_ReadByteOnFallingEdge();
  ID <<= 8;
  ID |= LCD_ReadByteOnFallingEdge();
  Data = LCD_ReadByteOnFallingEdge();
  Data = LCD_ReadByteOnFallingEdge();

  LCD_DELAY();
  LCD_DELAY();
  LCD_DELAY();

  lcdWriteCommand(0xC6);
  lcdWriteData(0x82);
  //lcdWriteData( 0x9b );
  return (ID);
#endif
}

void LCD_ILI9486_On(void) {
  lcdWriteCommand(0x29);
}

void LCD_ILI9486_Init(void) {
  lcdWriteCommand(0XFB);
  lcdWriteData(0x00);

  lcdWriteCommand(0xf2);
  lcdWriteData(0x18);
  lcdWriteData(0xa3);
  lcdWriteData(0x12);
  lcdWriteData(0x02);
  lcdWriteData(0xb2);
  lcdWriteData(0x12);
  lcdWriteData(0xff);
  lcdWriteData(0x13);
  lcdWriteData(0x00);
  lcdWriteCommand(0xf1);
  lcdWriteData(0x36);
  lcdWriteData(0x04);
  lcdWriteData(0x00);
  lcdWriteData(0x3c);
  lcdWriteData(0x0f);
  lcdWriteData(0x8f);
  lcdWriteCommand(0xf8);
  lcdWriteData(0x21);
  lcdWriteData(0x04);
  lcdWriteCommand(0xf9);
  lcdWriteData(0x00);
  lcdWriteData(0x08);
  lcdWriteCommand(0x36);
  lcdWriteData(0x18);
  lcdWriteCommand(0x3a);
  lcdWriteData(0x65);
  lcdWriteCommand(0xc0);
  lcdWriteData(0x0f);
  lcdWriteData(0x0f);
  lcdWriteCommand(0xc1);
  lcdWriteData(0x41);

  lcdWriteCommand(0xc5);
  lcdWriteData(0x00);
  lcdWriteData(0x27);
  lcdWriteData(0x80);
  lcdWriteCommand(0xb6);
  lcdWriteData(0xb2);
  lcdWriteData(0x42);
  lcdWriteData(0x3b);
  lcdWriteCommand(0xb1);
  lcdWriteData(0xb0);
  lcdWriteData(0x11);
  lcdWriteCommand(0xb4);
  lcdWriteData(0x02);
  lcdWriteCommand(0xb7);
  lcdWriteData(0xC6);

  lcdWriteCommand(0xe0);
  lcdWriteData(0x0f);
  lcdWriteData(0x1C);
  lcdWriteData(0x18);
  lcdWriteData(0x0B);
  lcdWriteData(0x0D);
  lcdWriteData(0x06);
  lcdWriteData(0x48);
  lcdWriteData(0x87);
  lcdWriteData(0x3A);
  lcdWriteData(0x09);
  lcdWriteData(0x15);
  lcdWriteData(0x08);
  lcdWriteData(0x0D);
  lcdWriteData(0x04);
  lcdWriteData(0x00);

  lcdWriteCommand(0xe1);
  lcdWriteData(0x0f);
  lcdWriteData(0x37);
  lcdWriteData(0x34);
  lcdWriteData(0x0A);
  lcdWriteData(0x0B);
  lcdWriteData(0x03);
  lcdWriteData(0x4B);
  lcdWriteData(0x31);
  lcdWriteData(0x39);
  lcdWriteData(0x03);
  lcdWriteData(0x0F);
  lcdWriteData(0x03);
  lcdWriteData(0x22);
  lcdWriteData(0x1D);
  lcdWriteData(0x00);

  lcdWriteCommand(0x21);
  lcdWriteCommand(0x11);
  delay_ms(120);
  lcdWriteCommand(0x28);

  LCD_ILI9486_On();
}

void LCD_ILI9486_Off(void) {
  lcdWriteCommand(0x28);
}

unsigned int LCD_ILI9486_ReadID(void) {
  int ID = 0;

  lcdWriteCommand(0XF7);
  lcdWriteData(0xA9);
  lcdWriteData(0x51);
  lcdWriteData(0x2C);
  lcdWriteData(0x82);
  lcdWriteCommand(0XB0);
  lcdWriteData(0X80);

  lcdWriteCommand(0XFB);
  lcdWriteData(0x10 | 0x00);
  ID = LCD_ReadRegister(0xd3);

  lcdWriteCommand(0XFB);
  lcdWriteData(0x10 | 0x01);
  ID = LCD_ReadRegister(0xd3);

  lcdWriteCommand(0XFB);
  lcdWriteData(0x10 | 0x02);
  ID = LCD_ReadRegister(0xd3);
  ID <<= 8;
  lcdWriteCommand(0XFB);
  lcdWriteData(0x10 | 0x03);
  ID |= LCD_ReadRegister(0xd3);

  lcdWriteCommand(0XFB);
  lcdWriteData(0x00);

  return (ID);
}

void LCD_ILI9488_On(void) {
  lcdWriteCommand(0x29);
  lcdWriteCommand(0x23); //all pixels on
}

void LCD_ILI9488_Init(void) {
  lcdWriteCommand(0XFB);
  lcdWriteData(0x00);

  lcdWriteCommand(0XF7);
  lcdWriteData(0xA9);
  lcdWriteData(0x51);
  lcdWriteData(0x2C);
  lcdWriteData(0x82);

  lcdWriteCommand(0xC0);
  lcdWriteData(0x11);
  lcdWriteData(0x09);

  lcdWriteCommand(0xC1);
  lcdWriteData(0x41);

  lcdWriteCommand(0XC5);
  lcdWriteData(0x00);
  lcdWriteData(0x0A);
  lcdWriteData(0x80);

  lcdWriteCommand(0xB1);
  lcdWriteData(0xB0);
  lcdWriteData(0x11);

  lcdWriteCommand(0xB4);
  lcdWriteData(0x02);

  lcdWriteCommand(0xB6);
  lcdWriteData(0x30);
  lcdWriteData(0x02);

  lcdWriteCommand(0xB7);
  lcdWriteData(0xc6);

  lcdWriteCommand(0xBE);
  lcdWriteData(0x00);
  lcdWriteData(0x04);

  lcdWriteCommand(0xE9);
  lcdWriteData(0x00);

  lcdWriteCommand(0x36);
  lcdWriteData(0x08);

  lcdWriteCommand(0x3A);
  lcdWriteData(0x65);

  lcdWriteCommand(0xE0);
  lcdWriteData(0x00);
  lcdWriteData(0x07);
  lcdWriteData(0x10);
  lcdWriteData(0x09);
  lcdWriteData(0x17);
  lcdWriteData(0x0B);
  lcdWriteData(0x41);
  lcdWriteData(0x89);
  lcdWriteData(0x4B);
  lcdWriteData(0x0A);
  lcdWriteData(0x0C);
  lcdWriteData(0x0E);
  lcdWriteData(0x18);
  lcdWriteData(0x1B);
  lcdWriteData(0x0F);

  lcdWriteCommand(0XE1);
  lcdWriteData(0x00);
  lcdWriteData(0x17);
  lcdWriteData(0x1A);
  lcdWriteData(0x04);
  lcdWriteData(0x0E);
  lcdWriteData(0x06);
  lcdWriteData(0x2F);
  lcdWriteData(0x45);
  lcdWriteData(0x43);
  lcdWriteData(0x02);
  lcdWriteData(0x0A);
  lcdWriteData(0x09);
  lcdWriteData(0x32);
  lcdWriteData(0x36);
  lcdWriteData(0x0F);

  lcdWriteCommand(0x11);
  delay_ms(120);
  lcdWriteCommand(0x28);

  LCD_ILI9488_On();
}

void LCD_ILI9488_Off(void) {
  lcdWriteCommand(0x22); //all pixels off
  lcdWriteCommand(0x28);
}

void LCD_ILI9488_ReadDevice(void) {
  int Index = 0;
  int Parameter = 0x80;

#if 1

#if 1
  lcdWriteCommand(0XF7);
  lcdWriteData(0xA9);
  lcdWriteData(0x51);
  lcdWriteData(0x2C);
  lcdWriteData(0x82);

  lcdWriteCommand(0XB0);
  lcdWriteData(0X80);

#endif
  lcdWriteCommand(0XFB);
  lcdWriteData(Parameter | 0x00);
  LCD_ReadBuffer[Index++] = LCD_ReadRegister(0xd3);

  //lcdWriteCommand(0X2E);
  lcdWriteCommand(0XFB);
  lcdWriteData(Parameter | 0x01);        //Parameter2=0X88
  LCD_ReadBuffer[Index++] = LCD_ReadRegister(0xd3);

  lcdWriteCommand(0XFB);
  lcdWriteData(Parameter | 0x02);        //Parameter2=0X88
  LCD_ReadBuffer[Index++] = LCD_ReadRegister(0xd3);

  lcdWriteCommand(0XFB);
  lcdWriteData(Parameter | 0x03);        //Parameter2=0X88
  LCD_ReadBuffer[Index++] = LCD_ReadRegister(0xd3);
#endif

#if 0
  lcdWriteCommand( 0XFB );
  lcdWriteData( Parameter|0x00 );        //Parameter3=0X94
  LCD_ReadBuffer[Index++] = LCD_ReadRegister( 0xd3 );
  lcdWriteData( Parameter|0x01 );//Parameter3=0X94
  LCD_ReadBuffer[Index++] = LCD_ReadRegister( 0xd3 );
  lcdWriteCommand( 0XFB );
  lcdWriteData( Parameter|0x02 );//Parameter3=0X94
  LCD_ReadBuffer[Index++] = LCD_ReadRegister( 0xd3 );

  lcdWriteCommand( 0XFB );
  lcdWriteData( Parameter|0x03 );//Parameter4=0X88
  LCD_ReadBuffer[Index++] = LCD_ReadRegister( 0xd3 );
#else
  //lcdWriteCommand( 0xd0 );
  //lcdWriteData( Parameter|0x03 );        //Parameter4=0X88
  //LCD_ReadBuffer[Index++] = LCD_ReadRegister( 0xd0 );
#endif
}

unsigned int LCD_ILI9488_ReadID(void) {
  int ID = 0;

  lcdWriteCommand(0XF7);
  lcdWriteData(0xA9);
  lcdWriteData(0x51);
  lcdWriteData(0x2C);
  lcdWriteData(0x82);
  lcdWriteCommand(0XB0);
  lcdWriteData(0X80);

  lcdWriteCommand(0XFB);
  lcdWriteData(0x80 | 0x00);
  ID = LCD_ReadRegister(0xd3);

  lcdWriteCommand(0XFB);
  lcdWriteData(0x80 | 0x01);
  ID = LCD_ReadRegister(0xd3);

  lcdWriteCommand(0XFB);
  lcdWriteData(0x80 | 0x02);
  ID = LCD_ReadRegister(0xd3);
  ID <<= 8;

  lcdWriteCommand(0XFB);
  lcdWriteData(0x80 | 0x03);
  ID |= LCD_ReadRegister(0xd3);

  lcdWriteCommand(0XFB);
  lcdWriteData(0x00);
  return (ID);
}

void LCD_ST7796S_On(void) {
  lcdWriteCommand(0x29);
}

void LCD_ST7796S_Init(void) {
  delay_ms(120);

  lcdWriteCommand( 0x11 );
  delay_ms(120);
  lcdWriteCommand( 0xF0 );
  lcdWriteData( 0xC3 );

  lcdWriteCommand( 0xF0 );
  lcdWriteData( 0x96 );

  lcdWriteCommand( 0x36 );
  lcdWriteData( 0x88 );

  lcdWriteCommand( 0x3A );
  lcdWriteData( 0x66 );

  //SET RGB STRAT
  lcdWriteCommand (0xB0 );   //SET HS VS DE CLK 上升还是下降有效
  lcdWriteData( 0x80 );

  lcdWriteCommand( 0xB4 );
  lcdWriteData( 0x01 );

  lcdWriteCommand( 0xB6 );
  lcdWriteData( 0x20 );
  lcdWriteData( 0x02 );
  lcdWriteData( 0x3B );
  //SET RGB END

  lcdWriteCommand( 0xB7);
  lcdWriteData( 0xC6);

  lcdWriteCommand( 0xB9 );
  lcdWriteData( 0x02 );
  lcdWriteData( 0xE0 );

  lcdWriteCommand( 0xC0 );
  lcdWriteData( 0x80 );
  lcdWriteData( 0x65 );

  lcdWriteCommand( 0xC1 );
  lcdWriteData( 0x0D );

  lcdWriteCommand( 0xC2 );
  lcdWriteData( 0xA7 );

  lcdWriteCommand( 0xC5 );
  lcdWriteData( 0x14 );

  lcdWriteCommand( 0xE8 );
  lcdWriteData( 0x40 );
  lcdWriteData( 0x8A );
  lcdWriteData( 0x00 );
  lcdWriteData( 0x00 );
  lcdWriteData( 0x29 );
  lcdWriteData( 0x19 );
  lcdWriteData( 0xA5 );
  lcdWriteData( 0x33 );

  lcdWriteCommand( 0xE0 );
  lcdWriteData( 0xD0 );
  lcdWriteData( 0x00 );
  lcdWriteData( 0x04 );
  lcdWriteData( 0x05 );
  lcdWriteData( 0x04 );
  lcdWriteData( 0x21 );
  lcdWriteData( 0x25 );
  lcdWriteData( 0x43 );
  lcdWriteData( 0x3F );
  lcdWriteData( 0x37 );
  lcdWriteData( 0x13 );
  lcdWriteData( 0x13 );
  lcdWriteData( 0x29 );
  lcdWriteData( 0x32 );

  lcdWriteCommand( 0xE1 );
  lcdWriteData( 0xD0 );
  lcdWriteData( 0x04 );
  lcdWriteData( 0x06 );
  lcdWriteData( 0x09 );
  lcdWriteData( 0x06 );
  lcdWriteData( 0x03 );
  lcdWriteData( 0x25 );
  lcdWriteData( 0x32 );
  lcdWriteData( 0x3E );
  lcdWriteData( 0x18 );
  lcdWriteData( 0x15 );
  lcdWriteData( 0x15 );
  lcdWriteData( 0x2B );
  lcdWriteData( 0x30 );

  lcdWriteCommand( 0xF0 );
  lcdWriteData( 0x3C );

  lcdWriteCommand( 0xF0 );
  lcdWriteData( 0x69 );

  delay_ms(120);

  lcdWriteCommand( 0x21 );

  LCD_ST7796S_On();
}

void LCD_ST7796S_Off(void) {
  lcdWriteCommand(0x28);
}

unsigned int LCD_ST7796S_ReadID(void) {
  unsigned int ID = 0;

  lcdWriteCommand( 0XF0 );
  lcdWriteData( 0XC3 );
  lcdWriteCommand( 0XF0 );
  lcdWriteData( 0X96 );

  lcdWriteCommand( 0XB0 );
  lcdWriteData( 0X80 );

  lcdWriteCommand( 0XD3 );

  LCD_MOSI_AS_INPUT();
  LCD_SCK_LOW();
  LCD_DELAY();
  LCD_DELAY();
  LCD_SCK_HIGH();
  LCD_DELAY();
  LCD_DELAY();

  LCD_ReadByte();
  ID += (uint16_t)(LCD_ReadByte())<<8;
  ID += LCD_ReadByte();

   return (ID);
 }

static void lcdReset() {
  LCD_NRST_HIGH();
  delay_ms(1);

  LCD_NRST_LOW(); // RESET();
  delay_ms(100);

  LCD_NRST_HIGH();
  delay_ms(100);
}

void LCD_Init_LTDC() {
  hltdc.Instance = LTDC;

  /* Configure PLLSAI prescalers for LCD */
  /* PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz */
  /* PLLSAI_VCO Output = PLLSAI_VCO Input * lcdPixelclock * 16 = XX Mhz */
  /* PLLLCDCLK = PLLSAI_VCO Output/PLL_LTDC = PLLSAI_VCO/4 = YY Mhz */
  /* LTDC clock frequency = PLLLCDCLK / RCC_PLLSAIDivR = YY/4 = lcdPixelClock Mhz */
  uint32_t clock = (lcdPixelClock*16) / 1000000; // clock*16 in MHz
  RCC_PeriphCLKInitTypeDef clkConfig;
  clkConfig.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  clkConfig.PLLSAI.PLLSAIN = clock;
  clkConfig.PLLSAI.PLLSAIR = 4;
  clkConfig.PLLSAIDivQ = 6;
  clkConfig.PLLSAIDivR = RCC_PLLSAIDIVR_4;
  HAL_RCCEx_PeriphCLKConfig(&clkConfig);

  /* LTDC Configuration *********************************************************/
  /* Polarity configuration */
  /* Initialize the horizontal synchronization polarity as active low */
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  /* Initialize the vertical synchronization polarity as active low */
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  /* Initialize the data enable polarity as active low */
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  /* Initialize the pixel clock polarity as input pixel clock */
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;

  /* Configure R,G,B component values for LCD background color */
  hltdc.Init.Backcolor.Red = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Blue = 0;

  /* Configure horizontal synchronization width */
  hltdc.Init.HorizontalSync = HSW;
  /* Configure vertical synchronization height */
  hltdc.Init.VerticalSync = VSH;
  /* Configure accumulated horizontal back porch */
  hltdc.Init.AccumulatedHBP = HBP;
  /* Configure accumulated vertical back porch */
  hltdc.Init.AccumulatedVBP = VBP;
  /* Configure accumulated active width */
  hltdc.Init.AccumulatedActiveW = LCD_PHYS_W + HBP;
  /* Configure accumulated active height */
  hltdc.Init.AccumulatedActiveH = LCD_PHYS_H + VBP;
  /* Configure total width */
  hltdc.Init.TotalWidth = LCD_PHYS_W + HBP + HFP;
  /* Configure total height */
  hltdc.Init.TotalHeigh = LCD_PHYS_H + VBP + VFP;

  HAL_LTDC_Init(&hltdc);

  // Configure IRQ (line)
  NVIC_SetPriority(LTDC_IRQn, LTDC_IRQ_PRIO);
  NVIC_EnableIRQ(LTDC_IRQn);

  // Trigger on last line
  HAL_LTDC_ProgramLineEvent(&hltdc, LCD_PHYS_H);
  __HAL_LTDC_ENABLE_IT(&hltdc, LTDC_IT_LI);

#if 0
  DMA2D_ITConfig(DMA2D_CR_TCIE, ENABLE);
  NVIC_InitStructure.NVIC_IRQChannel = DMA2D_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = DMA_SCREEN_IRQ_PRIO;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; /* Not used as 4 bits are used for the pr     e-emption priority. */;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init( &NVIC_InitStructure );
  DMA2D->IFCR = (unsigned long)DMA2D_IFSR_CTCIF;
#endif
}

void LCD_LayerInit() {
  auto& layer = hltdc.LayerCfg[0];

  /* Windowing configuration */
  layer.WindowX0 = 0;
  layer.WindowX1 = LCD_PHYS_W;
  layer.WindowY0 = 0;
  layer.WindowY1 = LCD_PHYS_H;

  /* Pixel Format configuration*/
  layer.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;

  /* Alpha constant (255 totally opaque) */
  layer.Alpha = 255;

  /* Default Color configuration (configure A,R,G,B component values) */
  layer.Backcolor.Blue = 0;
  layer.Backcolor.Green = 0;
  layer.Backcolor.Red = 0;
  layer.Alpha0 = 0;

  /* Configure blending factors */
  layer.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  layer.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;

  layer.ImageWidth = LCD_PHYS_W;
  layer.ImageHeight = LCD_PHYS_H;

  /* Start Address configuration : the LCD Frame buffer is defined on SDRAM w/ Offset */
  layer.FBStartAdress = (intptr_t)initialFrameBuffer;

  /* Initialize LTDC layer 1 */
  HAL_LTDC_ConfigLayer(&hltdc, &hltdc.LayerCfg[0], 0);

  /* dithering activation */
  HAL_LTDC_EnableDither(&hltdc);
}

extern "C"
void lcdSetInitalFrameBuffer(void* fbAddress)
{
  initialFrameBuffer = fbAddress;
}

const char* boardLcdType = "";

extern "C"
void lcdInit(void)
{
  /* Configure the LCD SPI+RESET pins */
  lcdSpiConfig();

  /* Reset the LCD --------------------------------------------------------*/
  lcdReset();

  /* Configure the LCD Control pins */
  LCD_AF_GPIOConfig();

  /* Send LCD initialization commands */
  if (LCD_ILI9481_ReadID() == LCD_ILI9481_ID) {
    TRACE("LCD INIT: ILI9481");
    boardLcdType = "ILI9481";
    lcdInitFunction = LCD_ILI9481_Init;
    lcdOffFunction = LCD_ILI9481_Off;
    lcdOnFunction = LCD_ILI9481_On;
    lcdPixelClock = 12000000;
  } else if (LCD_ILI9486_ReadID() == LCD_ILI9486_ID) {
    TRACE("LCD INIT: ILI9486");
    boardLcdType = "ILI9486";
    lcdInitFunction = LCD_ILI9486_Init;
    lcdOffFunction = LCD_ILI9486_Off;
    lcdOnFunction = LCD_ILI9486_On;
    lcdPixelClock = 12000000;
  } else if (LCD_ILI9488_ReadID() == LCD_ILI9488_ID) {
    TRACE("LCD INIT: ILI9488");
    boardLcdType = "ILI9488";
    lcdInitFunction = LCD_ILI9488_Init;
    lcdOffFunction = LCD_ILI9488_Off;
    lcdOnFunction = LCD_ILI9488_On;
    lcdPixelClock = 12000000;
  } else if (LCD_HX8357D_ReadID() == LCD_HX8357D_ID) {
    TRACE("LCD INIT: HX8357D");
    boardLcdType = "HX8357D";
    lcdInitFunction = LCD_HX8357D_Init;
    lcdOffFunction = LCD_HX8357D_Off;
    lcdOnFunction = LCD_HX8357D_On;
    lcdPixelClock = 12000000;
  } else { /* if (LCD_ST7796S_ReadID() == LCD_ST7796S_ID) { */ // ST7796S detection is unreliable
    TRACE("LCD INIT (default): ST7796S");
    boardLcdType = "ST7796S";
    lcdInitFunction = LCD_ST7796S_Init;
    lcdOffFunction = LCD_ST7796S_Off;
    lcdOnFunction = LCD_ST7796S_On;
    lcdPixelClock = 10000000;
/*  } else {
    TRACE("LCD INIT: unknown LCD controller");
    boardLcdType = "unknown";*/
  }

  lcdInitFunction();

  LCD_Init_LTDC();
  LCD_LayerInit();

  // Enable LCD display
  __HAL_LTDC_ENABLE(&hltdc);

  lcdSetFlushCb(startLcdRefresh);
}

extern "C" void LTDC_IRQHandler(void)
{
  // clear interrupt flag
  __HAL_LTDC_CLEAR_FLAG(&hltdc, LTDC_FLAG_LI);
  __HAL_LTDC_DISABLE_IT(&hltdc, LTDC_IT_LI);
  _frame_addr_reloaded = 1;
}
