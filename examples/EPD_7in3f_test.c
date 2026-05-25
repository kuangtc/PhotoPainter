/*****************************************************************************
* | File      	:   EPD_7in3f_test.c
* | Author      :   Waveshare team
* | Function    :   7.3inch e-Paper (F) Demo
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2023-03-13
* | Info        :
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "EPD_Test.h"
#include "ImageData.h"
#include "run_File.h"
#include "EPD_7in3f.h"
#include "GUI_Paint.h"
#include "GUI_BMPfile.h"
#include "waveshare_PCF85063.h"
#include "fonts.h"

#include <stdlib.h> // malloc() free()
#include <string.h>

int EPD_7in3f_display_BMP(const char *path, float vol)
{
    printf("e-Paper Init and Clear...\r\n");
    EPD_7IN3F_Init();

    //Create a new image cache
    UBYTE *BlackImage;
    UDOUBLE Imagesize = ((EPD_7IN3F_WIDTH % 2 == 0)? (EPD_7IN3F_WIDTH / 2 ): (EPD_7IN3F_WIDTH / 2 + 1)) * EPD_7IN3F_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    printf("Paint_NewImage\r\n");
    Paint_NewImage(BlackImage, EPD_7IN3F_WIDTH, EPD_7IN3F_HEIGHT, 0, EPD_7IN3F_WHITE);
    Paint_SetScale(7);

    run_mount();

    printf("[DEBUG] EPD_7in3f_display_BMP: path=%s\r\n", path);
    printf("Display BMP\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(EPD_7IN3F_WHITE);

    printf("[DEBUG] EPD_7in3f_display_BMP: calling GUI_ReadBmp_RGB_6Color\r\n");
    GUI_ReadBmp_RGB_6Color(path, 0, 0);

    // === DATE/TIME DISPLAY (mark to disable) ===
    // Reset rotation to 0 (no rotation) for correct text orientation
    Paint_SetRotate(180);

    // Draw date at top-left corner (yyyy/MM/dd format, Font24)
    Time_data rtcTime = PCF85063_GetTime();

    
    char dateStr[24];
    sprintf(dateStr, "%04d/%02d/%02d", rtcTime.years + 2000, rtcTime.months, rtcTime.days);
    
    //讀取refresh_config.txt，格式為小時，如：7,12,17,19，表示每天的7點、12點、17點、19點刷新圖片
    //如果文件不存在或格式錯誤，則默認為每天的7點刷新圖片
    int refreshHours[10];
    int refreshCount = 0;
    int i;

    readRefreshConfig(refreshHours, &refreshCount);

    // Read events for today from SD card
    char eventBuffer[128] = {0};
    readEventsFile(eventBuffer, sizeof(eventBuffer));

    // Combine date and events for display
    char dateWithEvent[160];
    if(strlen(eventBuffer) > 0) {
        sprintf(dateWithEvent, "%s %s", dateStr, eventBuffer);
    } else {
        strcpy(dateWithEvent, dateStr);
    }
    Paint_DrawString_EN(10, 10, dateWithEvent, &Font24, EPD_7IN3F_BLACK, FONT_BACKGROUND);

    // Draw refresh datetime at bottom-right corner (yyyyMMddHHmm format, Font8)
    char refreshStr[4];
    sprintf(refreshStr, "%02d%02d",rtcTime.hours, rtcTime.minutes);
    // Calculate position for bottom-right
    UWORD refreshStrWidth = 16 * 4; // Font16 width is 16, 4 chars = 64 pixels
    UWORD refreshX = EPD_7IN3F_WIDTH - refreshStrWidth - 10; // 800 - 64 - 10 = 726
    UWORD refreshY = EPD_7IN3F_HEIGHT - 16 - 10; // 480 - 16 - 10 = 454
    Paint_DrawString_EN(refreshX, refreshY, refreshStr, &Font16, EPD_7IN3F_BLACK, EPD_7IN3F_WHITE);
    // === END DATE/TIME DISPLAY ===

    char strvol[21] = {0};
    sprintf(strvol, "%f V", vol);
    if(vol < 3.3) {
        Paint_DrawString_EN(10, 50, "Low voltage, please charge in time.", &Font16, EPD_7IN3F_BLACK, EPD_7IN3F_WHITE);
        Paint_DrawString_EN(10, 66, strvol, &Font16, EPD_7IN3F_BLACK, EPD_7IN3F_WHITE);
    }

    printf("EPD_Display\r\n");
    EPD_7IN3F_Display(BlackImage);

    run_unmount();
    printf("Update Path Index...\r\n");
    updatePathIndex();

    printf("Goto Sleep...\r\n\r\n");
    EPD_7IN3F_Sleep();
    free(BlackImage);
    BlackImage = NULL;

    return 0;
}

int EPD_7in3f_display(float vol)
{
    printf("e-Paper Init and Clear...\r\n");
    EPD_7IN3F_Init();

    //Create a new image cache
    UBYTE *BlackImage;
    UDOUBLE Imagesize = ((EPD_7IN3F_WIDTH % 2 == 0)? (EPD_7IN3F_WIDTH / 2 ): (EPD_7IN3F_WIDTH / 2 + 1)) * EPD_7IN3F_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    printf("Paint_NewImage\r\n");
    Paint_NewImage(BlackImage, EPD_7IN3F_WIDTH, EPD_7IN3F_HEIGHT, 0, EPD_7IN3F_WHITE);
    Paint_SetScale(7);

    printf("Display BMP\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(EPD_7IN3F_WHITE);

    Paint_DrawBitMap(Image7color);

    Paint_SetRotate(270);

    // Get current RTC time
    Time_data rtcTime = PCF85063_GetTime();

    // Draw date at top-left corner (yyyy/MM/dd format, Font24)
    char dateStr[24];
    sprintf(dateStr, "%04d/%02d/%02d", rtcTime.years + 2000, rtcTime.months, rtcTime.days);

    // Read events for today from SD card
    char eventBuffer[128] = {0};
    readEventsFile(eventBuffer, sizeof(eventBuffer));

    // Combine date and events for display
    char dateWithEvent[160];
    if(strlen(eventBuffer) > 0) {
        sprintf(dateWithEvent, "%s %s", dateStr, eventBuffer);
    } else {
        strcpy(dateWithEvent, dateStr);
    }
    Paint_DrawString_EN(10, 10, dateWithEvent, &Font24, EPD_7IN3F_BLACK, FONT_BACKGROUND);

    // Draw refresh datetime at bottom-right corner (yyyyMMddHHmm format, Font8)
    char refreshStr[20];
    sprintf(refreshStr, "%04d%02d%02d%02d%02d",
            rtcTime.years + 2000, rtcTime.months, rtcTime.days,
            rtcTime.hours, rtcTime.minutes);
    UWORD refreshStrWidth = 12 * 8;
    UWORD refreshX = EPD_7IN3F_WIDTH - refreshStrWidth - 10;
    UWORD refreshY = EPD_7IN3F_HEIGHT - 12 - 10;
    Paint_DrawString_EN(refreshX, refreshY, refreshStr, &Font8, EPD_7IN3F_BLACK, FONT_BACKGROUND);

    char strvol[21] = {0};
    sprintf(strvol, "%f V", vol);
    if(vol < 3.3) {
        Paint_DrawString_EN(10, 50, "Low voltage, please charge in time.", &Font16, EPD_7IN3F_BLACK, EPD_7IN3F_WHITE);
        Paint_DrawString_EN(10, 66, strvol, &Font16, EPD_7IN3F_BLACK, EPD_7IN3F_WHITE);
    }

    printf("EPD_Display\r\n");
    EPD_7IN3F_Display(BlackImage);

    printf("Goto Sleep...\r\n\r\n");
    EPD_7IN3F_Sleep();
    free(BlackImage);
    BlackImage = NULL;

    return 0;
}

int EPD_7in3f_test(void)
{
    printf("e-Paper Init and Clear...\r\n");
    EPD_7IN3F_Init();

    //Create a new image cache
    UBYTE *BlackImage;
    UDOUBLE Imagesize = ((EPD_7IN3F_WIDTH % 2 == 0)? (EPD_7IN3F_WIDTH / 2 ): (EPD_7IN3F_WIDTH / 2 + 1)) * EPD_7IN3F_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    printf("Paint_NewImage\r\n");
    Paint_NewImage(BlackImage, EPD_7IN3F_WIDTH, EPD_7IN3F_HEIGHT, 0, EPD_7IN3F_WHITE);
    Paint_SetScale(7);

#if 1   // Drawing on the image
    //1.Select Image
    printf("SelectImage:BlackImage\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(EPD_7IN3F_WHITE);

    int hNumber, hWidth, vNumber, vWidth;
    hNumber = 20;
	hWidth = EPD_7IN3F_HEIGHT/hNumber; // 800/20
    vNumber = 10;
	vWidth = EPD_7IN3F_WIDTH/vNumber; // 480/10

    // 2.Drawing on the image
    printf("Drawing:BlackImage\r\n");
	for(int i=0; i<vNumber; i++) {
		Paint_DrawRectangle(1, 1+i*vWidth, 800, vWidth*(i+1), EPD_7IN3F_GREEN + (i % 5), DOT_PIXEL_1X1, DRAW_FILL_FULL);
	}
	for(int i=0, j=0; i<hNumber; i++) {
		if(i%2) {
			j++;
			Paint_DrawRectangle(1+i*hWidth, 1, hWidth*(1+i), 480, j%2 ? EPD_7IN3F_BLACK : EPD_7IN3F_WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
		}
	}

    printf("EPD_Display\r\n");
    EPD_7IN3F_Display(BlackImage);
#endif

    printf("Goto Sleep...\r\n");
    EPD_7IN3F_Sleep();
    free(BlackImage);
    BlackImage = NULL;

    return 0;
}