#include <stdio.h>

extern "C" {
#include "air105.h"
#include "air105_otp.h"
#include "air105_spi.h"
#include "delay.h"
#include "sysc.h"
#include "uart.h"
#include "crasharalho.h"
}

// On GPIO 6
#define LCD_ENABLE_BIT GPIO_Pin_13
#define LCD_CMD_DATA_BIT GPIO_Pin_15
// On GPIO 0
#define EXT_FLASH_ENABLE_BIT GPIO_Pin_5


#define P15_ON   GPIO_SetBits(GPIOG, GPIO_Pin_15)
#define P15_OFF  GPIO_ResetBits(GPIOG, GPIO_Pin_15)
#define P14_OFF  GPIO_ResetBits(GPIOG, GPIO_Pin_14)
#define P14_ON   GPIO_SetBits(GPIOG, GPIO_Pin_14)
#define P13_OFF  GPIO_ResetBits(GPIOG, GPIO_Pin_13)
#define P13_ON   GPIO_SetBits(GPIOG, GPIO_Pin_13)
#define WAIT_SPI_BUSY while (SPI_IsBusy(LCD_SPI))

#define RGB888_TO_RGB565(r, g, b) ( \
    (((r) & 0xF8) << 8) |  /* 5 bits of red, shifted to bits 15-11 */ \
    (((g) & 0xFC) << 3) |  /* 6 bits of green, shifted to bits 10-5 */ \
    (((b) & 0xF8) >> 3)    /* 5 bits of blue, shifted to bits 4-0 */ \
)

void SendCMD(SPI_TypeDef *LCD_SPI, uint8_t cmd) {
    P14_OFF;
    SPI_SendData(LCD_SPI, cmd);
    WAIT_SPI_BUSY;
    P14_ON;
}

void SendCMDParam(SPI_TypeDef *LCD_SPI, uint8_t cmdParam) {
    P15_ON;
    P14_OFF;
    SPI_SendData(LCD_SPI, cmdParam);
    WAIT_SPI_BUSY;
    P14_ON;
}

int main(void) {
    SYSCTRL->CG_CTRL1 = SYSCTRL_APBPeriph_ALL;
	SYSCTRL->SOFT_RST1 = SYSCTRL_APBPeriph_ALL;
    SYSCTRL->SOFT_RST2 &= ~SYSCTRL_USB_RESET;
    SYSCTRL->LOCK_R |= SYSCTRL_USB_RESET;
    SYSCTRL_PLLConfig(SYSCTRL_PLL_168MHz);
    SYSCTRL_PLLDivConfig(1);
    SYSCTRL_HCLKConfig(0);
    SYSCTRL_PCLKConfig(1);

    // Enable JTAG
    GPIO_InitTypeDef gpio;
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 |  GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 ;
    gpio.GPIO_Remap = GPIO_Remap_0;
    GPIO_Init(GPIOC, &gpio);

    gpio.GPIO_Pin =  GPIO_Pin_8 | GPIO_Pin_10; // CLK / MOSI
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Remap = GPIO_Remap_0;
    GPIO_Init(GPIOD, &gpio);

    gpio.GPIO_Pin =  GPIO_Pin_11; // MISO
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Remap = GPIO_Remap_0;
    GPIO_Init(GPIOD, &gpio);

    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Pin = LCD_ENABLE_BIT | LCD_CMD_DATA_BIT | EXT_FLASH_ENABLE_BIT;
    gpio.GPIO_Remap = GPIO_Remap_1;
    GPIO_Init(GPIOA, &gpio);

    //SYSCTRL_APBPeriphClockCmd(SYSCTRL_APBPeriph_UART3,ENABLE);
    GPIO_PinRemapConfig(GPIOA, GPIO_Pin_0 | GPIO_Pin_1,GPIO_Remap_0);
    SYSCTRL_APBPeriphResetCmd(SYSCTRL_APBPeriph_UART0, DISABLE);

    UART_InitTypeDef uart_init;
    uart_init.UART_BaudRate = 115200;
    uart_init.UART_WordLength = UART_WordLength_8b;
    uart_init.UART_StopBits = UART_StopBits_1;
    uart_init.UART_Parity = UART_Parity_No;
    UART_Init(UART0, &uart_init);
    Delay_Init();

    //Delay_ms(2000);
    printf("USART Init\r\n");
    printf("CHIP ID: %04x\r\n", SYSCTRL->CHIP_ID);

    SYSCTRL_APBPeriphClockCmd(SYSCTRL_APBPeriph_SPI3,ENABLE);
    SYSCTRL_APBPeriphResetCmd(SYSCTRL_APBPeriph_SPI3,ENABLE);

    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_15 | GPIO_Pin_14 | GPIO_Pin_13 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_2;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Remap = GPIO_Remap_1;
    GPIO_Init(GPIOG, &gpio);

    #define LCD_SPI SPIM3

    GPIO->ALT[5] = 0x01150000;
    GPIO_ResetBits(GPIOF, GPIO_Pin_8);
    GPIO_ResetBits(GPIOF, GPIO_Pin_9);
    GPIO_ResetBits(GPIOF, GPIO_Pin_10);
    GPIO_ResetBits(GPIOF, GPIO_Pin_12);

    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_8;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Remap = GPIO_Remap_0;
    GPIO_Init(GPIOH, &gpio);
    GPIO_SetBits(GPIOH, GPIO_Pin_8);

    GPIO_ResetBits(GPIOG, GPIO_Pin_3); // SPEAKER
    GPIO_SetBits(GPIOG, GPIO_Pin_4);
    // GPIO_ResetBits(GPIOG, GPIO_Pin_2);
    //GPIO_SetBits(GPIOC, GPIO_Pin_2);
    //GPIO_SetBits(GPIOC, GPIO_Pin_11);

    // Configure SPI for LCD ST7735
    SPI_InitTypeDef spi_init;
    SPI_StructInit(&spi_init);
    spi_init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi_init.SPI_Mode = SPI_Mode_Master;
    spi_init.SPI_DataSize = SPI_DataSize_8b;

    SPI_Init(LCD_SPI, &spi_init);
    SPI_Cmd(LCD_SPI, ENABLE);
    // Hopefully the right SPI
    #define LCD_SPI SPIM3
    GPIO_ResetBits(GPIOA, EXT_FLASH_ENABLE_BIT); // Enable Flash
    Delay_ms(10);
    GPIO_SetBits(GPIOA, EXT_FLASH_ENABLE_BIT); // Disable Flash
    Delay_ms(10);

    GPIO_SetBits(GPIOG, LCD_ENABLE_BIT);    // Enable LCD
    GPIO_ResetBits(GPIOG, LCD_ENABLE_BIT);
    GPIO_SetBits(GPIOG, LCD_ENABLE_BIT);    // Enable LCD

    P13_ON;
    P13_OFF;
    P13_ON;
    P15_OFF;

    {
        // 0x11 SLEEP OUT
        SendCMD(LCD_SPI, 0x11);                // Sleep out command
        Delay_ms(120); // Sleep out delay
    }

    {
        // 0xB1 0x05 0x3C 0x3C FRMCTR1
        SendCMD(LCD_SPI, 0xB1);       // FRMCTR1
        SendCMDParam(LCD_SPI, 0x05);
        SendCMDParam(LCD_SPI, 0x3C);
        SendCMDParam(LCD_SPI, 0x3C);
        P15_OFF;
    }

    {
        // 0xB2 0x05 0x3C 0x3C FRMCTR2
        SendCMD(LCD_SPI, 0xB2);                // FRMCTR2
        SendCMDParam(LCD_SPI, 0x05);
        SendCMDParam(LCD_SPI, 0x3C);
        SendCMDParam(LCD_SPI, 0x3C);
        P15_OFF;
    }

    {
        // 0xB3 0x05 0x3C 0x3C 0x05 0x3C 0x3C  FRMCTR3
        SendCMD(LCD_SPI, 0xB3);                // FRMCTR3
        SendCMDParam(LCD_SPI, 0x05);
        SendCMDParam(LCD_SPI, 0x3C);
        SendCMDParam(LCD_SPI, 0x3C);
        SendCMDParam(LCD_SPI, 0x05);
        SendCMDParam(LCD_SPI, 0x3C);
        SendCMDParam(LCD_SPI, 0x3C);
        P15_OFF;
    }

    {
        // 0xB4 0x03 INVCTR
        SendCMD(LCD_SPI, 0xB4);                // INVCTR
        SendCMDParam(LCD_SPI, 0x03);
        P15_OFF;
    }

    {
        // C0 0x28 0x08 0x04 PWCTR1
        SendCMD(LCD_SPI, 0xC0);                // PWCTR1
        SendCMDParam(LCD_SPI, 0x28);
        SendCMDParam(LCD_SPI, 0x08);
        SendCMDParam(LCD_SPI, 0x04);
        P15_OFF;
    }

    {
        // 0xC1 0xC0 PWCTR2
        SendCMD(LCD_SPI, 0xC1);     // PWCTR2
        SendCMDParam(LCD_SPI, 0xC0);
        P15_OFF;
    }

    {
        // 0xC2 0x0D 0x00 PWCTR3
        SendCMD(LCD_SPI, 0xC2);                // PWCTR3
        SendCMDParam(LCD_SPI, 0x0D);
        SendCMDParam(LCD_SPI, 0x00);
        P15_OFF;
    }

    {
        // 0xC3 0x8D 0x6A PWCTR4
        SendCMD(LCD_SPI, 0xC3);                // PWCTR4
        SendCMDParam(LCD_SPI, 0x8D);
        SendCMDParam(LCD_SPI, 0x6A);
        P15_OFF;
    }

    {
        // C4 0x8D 0xEE PWCTR5
        SendCMD(LCD_SPI, 0xC4);                // PWCTR5
        SendCMDParam(LCD_SPI, 0x8D);
        SendCMDParam(LCD_SPI, 0xEE);
        P15_OFF;
    }

    {
        // 0xC5 0x1A VMCTR1
        SendCMD(LCD_SPI, 0xC5);                // VMCTR1
        SendCMDParam(LCD_SPI, 0x1A);
        P15_OFF;
    }

    {
        // 0x36 0x60 MADCTL
        SendCMD(LCD_SPI, 0x36);                // MADCTL
        SendCMDParam(LCD_SPI, 0x60);           // Set orientation
        P15_OFF;
    }

    {
        // 0xE0 04 22 07 0A 2E 30 25 2A 28 26 2E 3A 00 01 03 13 GMCTRP1
        SendCMD(LCD_SPI, 0xE0);                // GMCTRP1
        uint8_t params[] = {
            0x04, 0x22, 0x07, 0x0A, 0x2E, 0x30,
            0x25, 0x2A, 0x28, 0x26, 0x2E, 0x3A,
            0x00, 0x01, 0x03, 0x13
        };
        for (int i = 0; i < sizeof(params); i++) {
            SendCMDParam(LCD_SPI, params[i]);
        }
        P15_OFF;
    }

    {
        // 0xE1 04 16 06 0D 2D 26 23 27 27 25 2D 3B 00 01 04 13 GMCTRN1
        SendCMD(LCD_SPI, 0xE1);                // GMCTRN1
        uint8_t params[] = {
            0x04, 0x16, 0x06, 0x0D, 0x2D, 0x26,
            0x23, 0x27, 0x27, 0x25, 0x2D, 0x3B,
            0x00, 0x01, 0x04, 0x13
        };
        for (int i = 0; i < sizeof(params); i++) {
            SendCMDParam(LCD_SPI, params[i]);
        }
        P15_OFF;
    }

    {
        // 0x3A 0x05 COLMOD
        SendCMD(LCD_SPI, 0x3A);                // COLMOD
        SendCMDParam(LCD_SPI, 0x05);           // 16-bit color mode
        P15_OFF;
    }

    {
        // 0x29 DISPON
        SendCMD(LCD_SPI, 0x29);                // DISPON
        P15_OFF;
    }

    {
        // NOP
        SendCMD(LCD_SPI, 0x00);                // NOP
        P15_OFF;
        SendCMD(LCD_SPI, 0x00);                // NOP
        P15_OFF;
        SendCMD(LCD_SPI, 0x00);                // NOP
        P15_OFF;
    }

    {
        // 0x2A 0x00 0x00 0x00 0x9F CASET
        SendCMD(LCD_SPI, 0x2A);                // CASET
        SendCMDParam(LCD_SPI, 0x00);           // Start column
        SendCMDParam(LCD_SPI, 0x00);           //
        SendCMDParam(LCD_SPI, 0x00);           // End column
        SendCMDParam(LCD_SPI, 0x9F);           //
        P15_OFF;
    }

    {
        // 0x2B 0x00 0x00 0x00 0x7F RASET
        SendCMD(LCD_SPI, 0x2B);                // RASET
        SendCMDParam(LCD_SPI, 0x00);           // Start row
        SendCMDParam(LCD_SPI, 0x00);           //
        SendCMDParam(LCD_SPI, 0x00);           // End row
        SendCMDParam(LCD_SPI, 0x7F);           //
        P15_OFF;
    }

    {
        // 0x2C RAMWR
        SendCMD(LCD_SPI, 0x2C);                // RAMWR
        P15_ON;
        uint8_t rgb[3];
        // Fill the screen with white color // 16 bit mode
        for (int i = 0; i < 160 * 128; i++) {
            HEADER_PIXEL(header_data, rgb);
            uint16_t color = RGB888_TO_RGB565(rgb[0], rgb[1], rgb[2]);
            SendCMDParam(LCD_SPI,color >> 8); // Send high byte
            SendCMDParam(LCD_SPI, color & 0xFF); // Send low byte
        }
        P15_OFF;
    }

    {
        // NOP
        SendCMD(LCD_SPI, 0x00);                // NOP
        P15_OFF;
        SendCMD(LCD_SPI, 0x00);                // NOP
        P15_OFF;
        SendCMD(LCD_SPI, 0x00);                // NOP
        P15_OFF;
    }


    Delay_ms(1000);

    while (1) {
        //GPIO_ResetBits(GPIOA, GPIO_Pin_5);
        printf("RESET \r\n");
        Delay_ms(250);
        //GPIO_SetBits(GPIOA, GPIO_Pin_5);
        printf("SET\r\n");
        Delay_ms(250);
    }
}