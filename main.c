#include "EPD_Test.h"   // Examples
#include "run_File.h"

#include "led.h"
#include "waveshare_PCF85063.h" // RTC
#include "DEV_Config.h"

#include <time.h>

extern const char *fileList;
extern char pathName[];

#define enChargingRtc 0

/*
Mode 0: Automatically get pic folder names and sort them
Mode 1: Automatically get pic folder names but not sorted
Mode 2: pic folder name is not automatically obtained, users need to create fileList.txt file and write the picture name in TF card by themselves
*/
#define Mode 0


float measureVBAT(void)
{
    float Voltage=0.0;
    const float conversion_factor = 3.3f / (1 << 12);
    uint16_t result = adc_read();
    Voltage = result * conversion_factor * 3;
    printf("Raw value: 0x%03x, voltage: %f V, Mode: %f\n", result, Voltage, Mode);
    return Voltage;
}

void chargeState_callback() 
{
    if(DEV_Digital_Read(VBUS)) {
        if(!DEV_Digital_Read(CHARGE_STATE)) {  // is charging
            ledCharging();
        }
        else {  // charge complete
            ledCharged();
        }
    }
}

void run_display(Time_data Time, Time_data alarmTime, char hasCard)
{
    printf("[DEBUG] run_display started, hasCard=%d\r\n", hasCard);
    DEV_Delay_ms(2000);  // Delay to let console output flush

    if(hasCard) {
        setFilePath();
        EPD_7in3f_display_BMP(pathName, measureVBAT());   // display bmp
    }
    else {
        printf("[DEBUG] No SD card, using default image\r\n");
        EPD_7in3f_display(measureVBAT());
    }

    PCF85063_clear_alarm_flag();    // clear RTC alarm flag
    rtcRunAlarm(Time,alarmTime);  // RTC run alarm
}

int main(void)
{
    char isCard = 0;

    printf("Init...\r\n");
    if(DEV_Module_Init() != 0) {  // DEV init
        return -1;
    }

    watchdog_enable(8*1000, 1);    // 8s
    DEV_Delay_ms(1000);

    if(!sdTest())
    {
        isCard = 1;
        if(Mode == 0)
        {
            sdScanDir();
            file_sort();
        }
        if(Mode == 1)
        {
            sdScanDir();
        }
        if(Mode == 2)
        {
            file_cat();
        }
        
    }
    else 
    {
        isCard = 0;
    }
    Time_data Time = PCF85063_GetTime();  
    int bootYear, bootMonth, bootDay, bootHour, bootMinute;
    if(readBootTimeConfig(&bootYear, &bootMonth, &bootDay, &bootHour, &bootMinute)==1){
        PCF85063_SetTime_YMD(bootYear, bootMonth, bootDay);
        PCF85063_SetTime_HMS(bootHour, bootMinute, 0);
        Time = PCF85063_GetTime();
    }
    Time_data alarmTime = Time;
    //讀取refresh_config.txt，格式為小時，如：7,12,17,19，表示每天的7點、12點、17點、19點刷新圖片
    //如果文件不存在或格式錯誤，則默認為每天的7點刷新圖片
    int refreshHours[10];
    int refreshCount = 0;
    int i;

    readRefreshConfig(refreshHours, &refreshCount);

    // Find the next refresh time from current hour
    // alarmTime should be set to the next refresh hour from current time
    alarmTime.hours = refreshHours[0];
    alarmTime.minutes = 0;
    alarmTime.seconds = 0;

    for(i = 0; i < refreshCount; i++) {
        if(refreshHours[i] > Time.hours) {
            alarmTime.hours = refreshHours[i];
            break;
        }
        // If all refresh hours are <= current hour, use the first one (tomorrow)
        // Set Alarm for the next day
        if(i == refreshCount - 1) {
            alarmTime.hours = refreshHours[0];
            // Handle month/year rollover if needed (simplified, not accounting for different month lengths)
            // 依月份進行判斷，1,3,5,7,8,10,12 為31天
            if(alarmTime.months ==2){
                if(alarmTime.years%4==0){  // leap year
                    if(alarmTime.days == 29) {
                        alarmTime.days = 1;
                        alarmTime.months += 1;
                    }else{
                        alarmTime.days += 1;
                    }
                }else{
                    if(alarmTime.days == 28) {
                        alarmTime.days = 1;
                        alarmTime.months += 1;
                    }else{
                        alarmTime.days += 1;
                    }
                }
            }
            else if(alarmTime.months == 1 || alarmTime.months == 3 || 
                alarmTime.months == 5 || alarmTime.months == 7 || alarmTime.months == 8 ||  
                alarmTime.months == 10 || alarmTime.months == 12) {                
                if(alarmTime.days == 31) {
                    alarmTime.days = 1;
                    if(alarmTime.months == 12){
                        alarmTime.years +=1;
                        alarmTime.months = 1;
                    }else{
                        alarmTime.months += 1;
                    }
                }else{
                    alarmTime.days += 1;
                }
            }
            else{
                if(alarmTime.days == 30) {
                    alarmTime.days = 1;
                    alarmTime.months += 1;
                }else{
                    alarmTime.days += 1;
                }
            }
        }   
    }

    rtcRunAlarm(Time,alarmTime);  // RTC run alarm
    printf("alarmTime set to %02d:00:00\r\n", alarmTime.hours);
    gpio_set_irq_enabled_with_callback(CHARGE_STATE, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, chargeState_callback);

    if(measureVBAT() < 3.1) {   // battery power is low
        printf("low power ...\r\n");
        PCF85063_alarm_Time_Disable();
        ledLowPower();  // LED flash for Low power
        powerOff(); // BAT off
        return 0;
    }
    else {
        printf("work ...\r\n");
        ledPowerOn();
    }

    if(!DEV_Digital_Read(VBUS)) {    // no charge state
        run_display(Time, alarmTime, isCard);
    }
    else {  // charge state
        chargeState_callback();
        while(DEV_Digital_Read(VBUS)) {
            measureVBAT();
            
            #if enChargingRtc
            if(!DEV_Digital_Read(RTC_INT)) {    // RTC interrupt trigger
                printf("rtc interrupt\r\n");
                run_display(Time, alarmTime, isCard);
            }
            #endif

            if(!DEV_Digital_Read(BAT_STATE)) {  // KEY pressed
                printf("key interrupt\r\n");
                run_display(Time, alarmTime, isCard);
            }
            DEV_Delay_ms(200);
        }
    }
    
    printf("power off ...\r\n");
    powerOff(); // BAT off

    return 0;
}
