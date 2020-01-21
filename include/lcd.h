#ifndef __LCD_H__
#define __LCD_H__

// ------------------------------------------------------------------------

#define LCD_WIDTH               160
#define LCD_HEIGHT              80
#define LCD_FRAMEBUFFER_PIXELS  (LCD_WIDTH * LCD_HEIGHT)
#define LCD_FRAMEBUFFER_BYTES   (LCD_WIDTH * LCD_HEIGHT * 2)

// ------------------------------------------------------------------------
// Basic functions. All functions are asynchronous, i.e., they return while
// bulk of the operation is running using DMA. They all synchronize between
// themselves, but lcd_wait() should be called explicitly before accessing
// the buffer used by read/write operations. Otherwise there will be race
// conditions between DMA and CPU accessing the data.
// ------------------------------------------------------------------------

void lcd_init       (void);
void lcd_clear      (unsigned short int color);                                 // All colors are RGB565.
void lcd_setpixel   (int x, int y, unsigned short int color);
void lcd_fill_rect  (int x, int y, int w, int h, unsigned short int color);
void lcd_rect       (int x, int y, int w, int h, unsigned short int color);
void lcd_write_u16  (int x, int y, int w, int h, const void* buffer);           // Buffer size = w*h*2 bytes.
void lcd_write_u24  (int x, int y, int w, int h, const void* buffer);           // Buffer size = w*h*3 bytes.
void lcd_read_u24   (int x, int y, int w, int h, void* buffer);                 // Buffer size = w*h*3 bytes.
void lcd_wait       (void);                                                     // Wait until previous operation is complete.

// ------------------------------------------------------------------------
// Framebuffer functions.
// ------------------------------------------------------------------------

void lcd_fb_setaddr (const void* buffer);   // Buffer size = LCD_FRAMEBUFFER_BYTES. Does not enable auto-refresh by itself.
void lcd_fb_enable  (void);                 // Automatic framebuffer refresh. When enabled, other functions cannot be used.
void lcd_fb_disable (void);                 // Disable auto-refresh. Waits until last refresh is complete before returning.

// ------------------------------------------------------------------------

#endif // __LCD_H__
