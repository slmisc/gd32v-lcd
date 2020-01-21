#include "lcd.h"
#include "gd32vf103.h"
#include "systick.h"

// ------------------------------------------------------------------------

#define spi_wait_idle()     do { while (SPI_STAT(SPI0) & SPI_STAT_TRANS); } while(0)
#define spi_wait_tbe()      do { while (!(SPI_STAT(SPI0) & SPI_STAT_TBE)); } while(0)
#define spi_wait_rbne()     do { while (!(SPI_STAT(SPI0) & SPI_STAT_RBNE)); } while(0)
#define dma_wait_recv()     do { while (DMA_CHCNT(DMA0, DMA_CH1); } while(0)
#define lcd_mode_cmd()      do { gpio_bit_reset(GPIOB, GPIO_PIN_0); } while(0)
#define lcd_mode_data()     do { gpio_bit_set  (GPIOB, GPIO_PIN_0); } while(0)
#define lcd_cs_enable()     do { gpio_bit_reset(GPIOB, GPIO_PIN_2); } while(0)
#define lcd_cs_disable()    do { gpio_bit_set  (GPIOB, GPIO_PIN_2); } while(0)

// ------------------------------------------------------------------------

typedef enum {
    WAIT_NONE       = 0,
    WAIT_READ_U24   = 1,
    WAIT_WRITE_U24  = 2,
} WaitStatus;

static WaitStatus g_waitStatus = WAIT_NONE;
static uint32_t   g_fbAddress  = 0;
static int        g_fbEnabled  = 0;

// ------------------------------------------------------------------------
// Internal functions.
// ------------------------------------------------------------------------

void spi_set_8bit()
{
    if (SPI_CTL0(SPI0) & (uint32_t)(SPI_CTL0_FF16))
    {
        SPI_CTL0(SPI0) &= ~(uint32_t)(SPI_CTL0_SPIEN);
        SPI_CTL0(SPI0) &= ~(uint32_t)(SPI_CTL0_FF16);
        SPI_CTL0(SPI0) |= (uint32_t)(SPI_CTL0_SPIEN);
    }
}

void spi_set_16bit()
{
    if (!(SPI_CTL0(SPI0) & (uint32_t)(SPI_CTL0_FF16)))
    {
        SPI_CTL0(SPI0) &= ~(uint32_t)(SPI_CTL0_SPIEN);
        SPI_CTL0(SPI0) |= (uint32_t)(SPI_CTL0_FF16);
        SPI_CTL0(SPI0) |= (uint32_t)(SPI_CTL0_SPIEN);
    }
}

void dma_send_u8(const void* src, uint32_t count)
{
    spi_wait_idle();
    lcd_mode_data();
    spi_set_8bit();
    dma_channel_disable(DMA0, DMA_CH2);
    dma_memory_width_config(DMA0, DMA_CH2, DMA_MEMORY_WIDTH_8BIT);
    dma_periph_width_config(DMA0, DMA_CH2, DMA_PERIPHERAL_WIDTH_8BIT);
    dma_memory_address_config(DMA0, DMA_CH2, (uint32_t)src);
    dma_memory_increase_enable(DMA0, DMA_CH2);
    dma_transfer_number_config(DMA0, DMA_CH2, count);
    dma_channel_enable(DMA0, DMA_CH2);
}

void dma_send_u16(const void* src, uint32_t count)
{
    spi_wait_idle();
    lcd_mode_data();
    spi_set_16bit();
    dma_channel_disable(DMA0, DMA_CH2);
    dma_memory_width_config(DMA0, DMA_CH2, DMA_MEMORY_WIDTH_16BIT);
    dma_periph_width_config(DMA0, DMA_CH2, DMA_PERIPHERAL_WIDTH_16BIT);
    dma_memory_address_config(DMA0, DMA_CH2, (uint32_t)src);
    dma_memory_increase_enable(DMA0, DMA_CH2);
    dma_transfer_number_config(DMA0, DMA_CH2, count);
    dma_channel_enable(DMA0, DMA_CH2);
}

uint32_t g_dma_const_value = 0;

void dma_send_const_u8(uint8_t data, uint32_t count)
{
    spi_wait_idle();
    g_dma_const_value = data;
    lcd_mode_data();
    spi_set_8bit();
    dma_channel_disable(DMA0, DMA_CH2);
    dma_memory_width_config(DMA0, DMA_CH2, DMA_MEMORY_WIDTH_8BIT);
    dma_periph_width_config(DMA0, DMA_CH2, DMA_PERIPHERAL_WIDTH_8BIT);
    dma_memory_address_config(DMA0, DMA_CH2, (uint32_t)(&g_dma_const_value));
    dma_memory_increase_disable(DMA0, DMA_CH2);
    dma_transfer_number_config(DMA0, DMA_CH2, count);
    dma_channel_enable(DMA0, DMA_CH2);
}

void dma_send_const_u16(uint16_t data, uint32_t count)
{
    spi_wait_idle();
    g_dma_const_value = data;
    lcd_mode_data();
    spi_set_16bit();
    dma_channel_disable(DMA0, DMA_CH2);
    dma_memory_width_config(DMA0, DMA_CH2, DMA_MEMORY_WIDTH_16BIT);
    dma_periph_width_config(DMA0, DMA_CH2, DMA_PERIPHERAL_WIDTH_16BIT);
    dma_memory_address_config(DMA0, DMA_CH2, (uint32_t)(&g_dma_const_value));
    dma_memory_increase_disable(DMA0, DMA_CH2);
    dma_transfer_number_config(DMA0, DMA_CH2, count);
    dma_channel_enable(DMA0, DMA_CH2);
}

void lcd_reg(uint8_t x)
{
    spi_wait_idle();
    spi_set_8bit();
    lcd_mode_cmd();
    spi_i2s_data_transmit(SPI0, x);
}

void lcd_u8(uint8_t x)
{
    spi_wait_idle();
    spi_set_8bit();
    lcd_mode_data();
    spi_i2s_data_transmit(SPI0, x);
}

void lcd_u8c(uint8_t x)
{
    spi_wait_tbe();
    spi_i2s_data_transmit(SPI0, x);
}

void lcd_u16(uint16_t x)
{
    spi_wait_idle();
    spi_set_16bit();
    lcd_mode_data();
    spi_i2s_data_transmit(SPI0, x);
}

void lcd_u16c(uint16_t x)
{
    spi_wait_tbe();
    spi_i2s_data_transmit(SPI0, x);
}

void lcd_set_addr(int x, int y, int w, int h)
{
    lcd_reg(0x2a);
    lcd_u16(x+1);
    lcd_u16c(x+w);
    lcd_reg(0x2b);
    lcd_u16(y+26);
    lcd_u16c(y+h+25);
    lcd_reg(0x2c);
}

// ------------------------------------------------------------------------
// Public functions.
// ------------------------------------------------------------------------

void lcd_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_SPI0);

    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5 | GPIO_PIN_7);
    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);
    gpio_bit_reset(GPIOB, GPIO_PIN_0 | GPIO_PIN_1); // DC=0, RST=0
    lcd_cs_disable();

    delay_1ms(1);
    gpio_bit_set(GPIOB, GPIO_PIN_1); // RST=1
    delay_1ms(5);

    // Deinit SPI and DMA.
    spi_i2s_deinit(SPI0);
    dma_deinit(DMA0, DMA_CH1);
    dma_deinit(DMA0, DMA_CH2);

    // Configure DMA, do not enable.
    DMA_CHCTL(DMA0, DMA_CH1) = (uint32_t)(DMA_PRIORITY_ULTRA_HIGH | DMA_CHXCTL_MNAGA);  // Receive.
    DMA_CHCTL(DMA0, DMA_CH2) = (uint32_t)(DMA_PRIORITY_ULTRA_HIGH | DMA_CHXCTL_DIR);    // Transmit.
    DMA_CHPADDR(DMA0, DMA_CH1) = (uint32_t)&SPI_DATA(SPI0);
    DMA_CHPADDR(DMA0, DMA_CH2) = (uint32_t)&SPI_DATA(SPI0);

    // Configure and enable SPI.
    SPI_CTL0(SPI0) = (uint32_t)(SPI_MASTER | SPI_TRANSMODE_FULLDUPLEX | SPI_FRAMESIZE_8BIT | SPI_NSS_SOFT | SPI_ENDIAN_MSB | SPI_CK_PL_LOW_PH_1EDGE | SPI_PSC_8);
    SPI_CTL1(SPI0) = (uint32_t)(SPI_CTL1_DMATEN);
    spi_enable(SPI0);

    // Enable lcd controller.
    lcd_cs_enable();

    // Initialization settings. Based on lcd.c in gd32v_lcd example.
    static const uint8_t init_sequence[] =
    {
        0x21, 0xff,
        0xb1, 0x05, 0x3a, 0x3a, 0xff,
        0xb2, 0x05, 0x3a, 0x3a, 0xff,
        0xb3, 0x05, 0x3a, 0x3a, 0x05, 0x3a, 0x3a, 0xff,
        0xb4, 0x03, 0xff,
        0xc0, 0x62, 0x02, 0x04, 0xff,
        0xc1, 0xc0, 0xff,
        0xc2, 0x0d, 0x00, 0xff,
        0xc3, 0x8d, 0x6a, 0xff,
        0xc4, 0x8d, 0xee, 0xff,
        0xc5, 0x0e, 0xff,
        0xe0, 0x10, 0x0e, 0x02, 0x03, 0x0e, 0x07, 0x02, 0x07, 0x0a, 0x12, 0x27, 0x37, 0x00, 0x0d, 0x0e, 0x10, 0xff,
        0xe1, 0x10, 0x0e, 0x03, 0x03, 0x0f, 0x06, 0x02, 0x08, 0x0a, 0x13, 0x26, 0x36, 0x00, 0x0d, 0x0e, 0x10, 0xff,
        0x3a, 0x55, 0xff,
        0x36, 0x78, 0xff,
        0x29, 0xff,
        0x11, 0xff,
        0xff
    };

    // Initialize the display.
    for (const uint8_t* p = init_sequence; *p != 0xff; p++)
    {
        lcd_reg(*p++);
        if (*p == 0xff)
            continue;
        spi_wait_idle();
        lcd_mode_data();
        while(*p != 0xff)
            lcd_u8c(*p++);
    }

    // Clear display.
    lcd_clear(0);

    // Init internal state.
    g_waitStatus = WAIT_NONE;
    g_fbAddress  = 0;
    g_fbEnabled  = 0;
}

void lcd_clear(uint16_t color)
{
    if (g_fbEnabled)
        return;

    lcd_wait();
    lcd_set_addr(0, 0, LCD_WIDTH, LCD_HEIGHT);
    dma_send_const_u16(color, LCD_WIDTH * LCD_HEIGHT);
}

void lcd_setpixel(int x, int y, unsigned short int color)
{
    if (g_fbEnabled)
        return;

    lcd_wait();
    lcd_set_addr(x, y, 1, 1);
    lcd_u8(color >> 8);
    lcd_u8c(color);
}

void lcd_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    if (g_fbEnabled)
        return;

    lcd_wait();
    lcd_set_addr(x, y, w, h);
    dma_send_const_u16(color, w*h);
}

void lcd_rect(int x, int y, int w, int h, uint16_t color)
{
    if (g_fbEnabled)
        return;

    lcd_wait();
    lcd_fill_rect(x, y, x+w, y+1, color);
    lcd_fill_rect(x, y+w-1, x+w, y+w, color);
    lcd_fill_rect(x, y+1, x+1, y+w-1, color);
    lcd_fill_rect(x+w-1, y+1, x+w, y+w-1, color);
}

void lcd_write_u16(int x, int y, int w, int h, const void* buffer)
{
    if (g_fbEnabled)
        return;

    lcd_wait();
    lcd_set_addr(x, y, w, h);
    dma_send_u16(buffer, w*h);
}

void lcd_write_u24(int x, int y, int w, int h, const void* buffer)
{
    if (g_fbEnabled)
        return;

    lcd_wait();
    lcd_reg(0x3a);  // COLMOD
    lcd_u8(0x66);   // RGB666 (transferred as 3 x 8b)
    lcd_set_addr(x, y, w, h);
    dma_send_u8(buffer, w*h*3);
    g_waitStatus = WAIT_WRITE_U24;
}

void lcd_read_u24(int x, int y, int w, int h, void* buffer)
{
    if (g_fbEnabled)
        return;

    lcd_wait();

    // Send receive commands.
    lcd_set_addr(x, y, w, h);
    lcd_reg(0x3a);  // COLMOD
    lcd_u8(0x66);   // RGB666 (transferred as 3 x 8b)
    lcd_reg(0x2e);  // RAMRD
    lcd_u8(0x00);   // Flush dummy first byte sent by display.

    // Configure SPI and DMA for receiving.
    spi_wait_idle();
    spi_disable(SPI0);
    lcd_mode_data();
    SPI_DATA(SPI0); // Clear RBNE.
    SPI_CTL0(SPI0) = (uint32_t)(SPI_MASTER | SPI_TRANSMODE_BDRECEIVE | SPI_FRAMESIZE_8BIT | SPI_NSS_SOFT | SPI_ENDIAN_MSB | SPI_CK_PL_HIGH_PH_2EDGE | SPI_PSC_8);
    SPI_CTL1(SPI0) = (uint32_t)(SPI_CTL1_DMAREN);
    dma_memory_address_config(DMA0, DMA_CH1, (uint32_t)buffer);
    dma_transfer_number_config(DMA0, DMA_CH1, w*h*3);
    dma_channel_enable(DMA0, DMA_CH1);
    spi_enable(SPI0); // Go.
    g_waitStatus = WAIT_READ_U24;
}

void lcd_wait(void)
{
    if (g_fbEnabled)
        return;

    if (g_waitStatus == WAIT_NONE)
        return;

    if (g_waitStatus == WAIT_READ_U24)
    {
        // Poll until reception is complete.
        while(dma_transfer_number_get(DMA0, DMA_CH1));

        // Reception is complete, reconfigure SPI for sending and toggle LCD CS to stop transmission.
        dma_channel_disable(DMA0, DMA_CH1);
        spi_disable(SPI0);
        lcd_cs_disable();
        SPI_CTL0(SPI0) = (uint32_t)(SPI_MASTER | SPI_TRANSMODE_FULLDUPLEX | SPI_FRAMESIZE_8BIT | SPI_NSS_SOFT | SPI_ENDIAN_MSB | SPI_CK_PL_LOW_PH_1EDGE | SPI_PSC_8);
        SPI_CTL1(SPI0) = (uint32_t)(SPI_CTL1_DMATEN);
        lcd_cs_enable();
        spi_enable(SPI0);

        // Return to normal color mode.
        lcd_reg(0x3a);  // COLMOD
        lcd_u8(0x55);   // RGB565 (transferred as 16b)

        // Clear wait status and return.
        g_waitStatus = WAIT_NONE;
        return;
    }

    if (g_waitStatus == WAIT_WRITE_U24)
    {
        // Wait until send is complete, then restore normal color mode.
        spi_wait_idle();
        lcd_reg(0x3a);  // COLMOD
        lcd_u8(0x55);   // RGB565 (transferred as 16b)

        // Clear wait status and return.
        g_waitStatus = WAIT_NONE;
        return;
    }
}

// ------------------------------------------------------------------------
// Framebuffer functions.
// ------------------------------------------------------------------------

void DMA0_Channel2_IRQHandler(void)
{
    // Clear the interrupt flag to avoid retriggering.
    dma_interrupt_flag_clear(DMA0, DMA_CH2, DMA_INT_FLAG_G);

    if (g_fbEnabled)
    {
        // Restart transmission.
        lcd_set_addr(0, 0, LCD_WIDTH, LCD_HEIGHT); // Technically not needed?
        dma_send_u16((const void*)g_fbAddress, LCD_FRAMEBUFFER_PIXELS);
    }
    else
    {
        // Disable the interrupt. Ends the wait for complete.
        dma_interrupt_disable(DMA0, DMA_CH2, DMA_INT_FTF);
    }
}

void lcd_fb_setaddr(const void* buffer)
{
    if (g_fbEnabled)
        return;

    g_fbAddress = (uint32_t)buffer;
}

void lcd_fb_enable(void)
{
    if (g_fbEnabled || !g_fbAddress)
        return;

    // Wait and set enabled flag.
    lcd_wait();
    spi_wait_idle();
    g_fbEnabled = 1;

    // Enable interrupt controller.
    eclic_global_interrupt_enable();
    eclic_enable_interrupt(DMA0_Channel2_IRQn);

    // Set up transfer complete interrupt.
    dma_channel_disable(DMA0, DMA_CH2);
    dma_flag_clear(DMA0, DMA_CH2, DMA_FLAG_G);
    dma_interrupt_enable(DMA0, DMA_CH2, DMA_INT_FTF);

    // Start the first transfer.
    lcd_set_addr(0, 0, LCD_WIDTH, LCD_HEIGHT);
    dma_send_u16((const void*)(g_fbAddress), LCD_FRAMEBUFFER_PIXELS);
}

void lcd_fb_disable(void)
{
    if (!g_fbEnabled)
        return;

    // Disable and wait until handler disables the interrupt.
    g_fbEnabled = 0;
    while(DMA_CHCTL(DMA0, DMA_CH2) & DMA_CHXCTL_FTFIE);
}

// ------------------------------------------------------------------------
