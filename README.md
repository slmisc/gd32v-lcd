# gd32v-lcd
LCD library with DMA support for Sipeed Longan Nano (GD32VF103). Also supports setting up automated framebuffer refresh in background.

# Overview
The official example repository for GD32V has a rudimentary [LCD library](https://github.com/sipeed/Longan_GD32VF_examples/blob/master/gd32v_lcd/src/lcd/lcd.c) but the support for DMA transfers appears to be unfinished and I could not get it working. Therefore, I decided to roll my own. This project contains a DMA-enabled LCD library with all bulk operations such as buffer reads and writes running asynchronously in background for maximum performance. The display controller interface is similar to ST7735 [(Datasheet PDF)](https://www.displayfuture.com/Display/datasheet/controller/ST7735.pdf) in case you're looking for low-level documentation.

This library also supports setting up continuous automatic framebuffer upload in background so that you can effectively use the display as a memory mapped device. Unfortunately, there is no vertical sync pin connected on Sipeed Longan Nano, so avoiding tearing is not possible without hardware modifications (assuming that the LCD controller is similar to ST7735 in this regard).

There are two simple graphics effects in the main file, one demonstrating on-demand framebuffer upload, and one demonstrating automatic background refresh.

<table style="text-align:center;border:none;">
<tbody>
<tr style="border:none;background-color:initial">
<td style="border:none"><img width="320" height="160" src="https://user-images.githubusercontent.com/60066031/72683258-73501400-3ade-11ea-9001-f4994a55cb39.gif"></td>
<td style="border:none"><img width="320" height="160" src="https://user-images.githubusercontent.com/60066031/72683259-73501400-3ade-11ea-946c-7aa0a31ada32.gif"></td>
</tr>
<tr style="border:none;background-color:initial">
<td style="border:none">Amiga ball</td>
<td style="border:none">Starfield</td>
</tr>
</tbody>
</table>

Thanks to [Kevin Sangeelee](https://github.com/Kevin-Sangeelee) for the comprehensive [blog post](https://www.susa.net/wordpress/2019/10/longan-nano-gd32vf103) on GD32VF103 that was very helpful for me in understanding how to set up the interrupts on this hardware.
