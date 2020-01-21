#include "gd32vf103.h"
#include "lcd.h"
#include "led.h"

// ------------------------------------------------------------------------
// Framebuffer and other globals.
// ------------------------------------------------------------------------

uint16_t g_fb[LCD_FRAMEBUFFER_PIXELS]; // LCD Color: RGB565 (MSB rrrrrggggggbbbbb LSB)

uint16_t bgcolxy(int x, int y)
{
    x -= LCD_WIDTH >> 1;
    y -= LCD_HEIGHT >> 1;
    y <<= 1;
    int r = x*x + y*y;
    return r >> 11;
}

uint16_t bgcol(int p)
{
    int y = p / LCD_WIDTH;
    int x = p - y * LCD_WIDTH;
    return bgcolxy(x, y);
}

#include "amigaball.inl"
#include "starfield.inl"

int main(void)
{
    led_init();
    lcd_init();

    // Clear the framebuffer.
    for (int i=0; i < LCD_FRAMEBUFFER_PIXELS; i++)
        g_fb[i] = bgcol(i);

#if 0
    // Test pattern. Gray except for 1 pixel red border. Useful for detecting
    // timing/logic bugs that may cause the lcd internal pixel address register
    // getting out of sync with SPI upload.
    {
        for (int y=0,i=0; y < 80; y++)
        for (int x=0; x < 160; x++,i++)
        {
            if (x==0 || y==0 || x==159 || y==79)
                g_fb[i] = 0xc000;
            else
                g_fb[i] = 0x4208;
        }
        // Update in an infinite loop.
        for (;;)
            lcd_write_u16(0, 0, LCD_WIDTH, LCD_HEIGHT, g_fb);
    }

#elif 1
    // Example with framebuffer upload after each frame completion (Amiga ball).

    int px = 0;
    int py = 0;
    int dx = 1 << 8;
    int dy = 1 << 8;
    int ph = 0;
    for(;;)
    {
        // Render into framebuffer.
        uint16_t* pfb = g_fb;
        for (int y=0; y < LCD_HEIGHT; y++)
        for (int x=0; x < LCD_WIDTH; x++)
        {
            int fx = (x << 8) - px;
            int fy = (y << 8) - py;
            int c = amigaBall(fx, fy, ph);
            if (c >= 0)
                *pfb++ = c;
            else
                *pfb++ = bgcolxy(x, y);
        }

        // Trigger framebuffer upload. We rely on the upload being faster than
        // our framebuffer writes in the loop above so that we can render the
        // next frame while this one is being uploaded asynchronously in the
        // background.
        lcd_write_u16(0, 0, LCD_WIDTH, LCD_HEIGHT, g_fb);

        // Update ball position.
        px += dx; py += dy;
        if (dx > 0) ph += (1 << 8); else ph -= (1 << 8);         // Rotation.
        if (py > (16<<8))    { py = (16<<9) - py;    dy = -dy; } // Floor.
        if (px > (96<<8))    { px = (96<<9) - px;    dx = -dx; } // Right wall.
        if (px < (-64 << 8)) { px = (-64 << 9) - px; dx = -dx; } // Left wall.
        dy += 1 << 4; // Apply gravity.
    }

#else
    // Continuous framebuffer upload example (starfield).

    // Enable continuous framebuffer update.
    lcd_fb_setaddr(g_fb);
    lcd_fb_enable();

    // Initial stars.
    for (int i=0; i < NUM_STARS; i++)
    {
        g_stars[i].x = rnd_u32();
        g_stars[i].y = rnd_u32();
        g_stars[i].z = rnd_u32();
        g_stars[i].p = -1;
    }

    // Render star field. Note that we only need to update the framebuffer
    // in memory, and the continuous upload in background makes the changes
    // visible on display.
    for(;;)
    {
        for (int i=0; i < NUM_STARS; i++)
        {
            Star* s = &g_stars[i];
            if (s->p >= 0)
                g_fb[s->p] = bgcol(s->p);
            update_star(s, 10);
            if (s->p >= 0)
            {
                int g = (0x7fff - s->z) >> 9;
                if (g > 31) g = 31;
                int r = g>>1;
                g_fb[s->p] += (r << 11) | (g << 5) | r;
            }
        }
    }
#endif
}
