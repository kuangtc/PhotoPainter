# PhotoPainter 專案技術文件

## 概述

PhotoPainter 是一個使用電子紙顯示器 (e-Paper)、RTC 時鐘晶片 (PCF85063) 和 SD 卡實現自動輪播圖片的專案。

---

## 1. 日期與時間顯示

### 1.1 RTC 晶片 (PCF85063)

**型號**: Waveshare PCF85063
**I2C 位址**: `0x51`

#### 主要暫存器

| 暫存器 | 功能 |
|--------|------|
| `SECONDS_REG` (0x04) | 秒 |
| `MINUTES_REG` (0x05) | 分 |
| `HOURS_REG` (0x06) | 時 |
| `DAYS_REG` (0x07) | 日 |
| `WEEKDAYS_REG` (0x08) | 星期 |
| `MONTHS_REG` (0x09) | 月 |
| `YEARS_REG` (0x0A) | 年 (兩位數) |

#### 時間資料結構

```c
typedef struct {
    UWORD years;    // 00-99 (2000年起)
    UWORD months;   // 01-12
    UWORD days;     // 01-31
    UWORD hours;    // 00-23
    UWORD minutes;  // 00-59
    UWORD seconds;  // 00-59
} Time_data;
```

#### 取得目前時間

```c
Time_data PCF85063_GetTime();
// 回傳 Time_data 結構，包含目前年月日與時分秒
```

#### 設定時間

**設定日期** (年月日):
```c
void PCF85063_SetTime_YMD(int Years, int Months, int Days);
```

**設定時間** (時分秒):
```c
void PCF85063_SetTime_HMS(int hour, int minute, int second);
```

---

## 2. 事件顯示功能

### 2.1 事件檔案格式

事件儲存於 SD 卡 `0:/events.txt`，格式為：

```
MM-DD,事件內容
```

範例：
```
05-20,生日快樂
06-15,端午節
12-25,聖誕節
```

### 2.2 讀取今日事件

```c
void readEventsFile(char *eventBuffer, int bufferSize);
```

**參數**:
- `eventBuffer`: 儲存事件文字的緩衝區 (建議至少 128 位元組)
- `bufferSize`: 緩衝區大小

**行為**:
- 讀取 RTC 目前日期 (月-日)
- 掃描 `events.txt` 找出符合日期的事件
- 將多個事件以空白分隔串接至 `eventBuffer`
- 輸出格式: `"事件1 事件2 事件3"`

**範例輸出**:
```
[DEBUG] Events for today: 生日快樂 旅遊
```

---

## 3. 重設系統時間

### 3.1 使用 boot_time.txt 檔案

將 SD 卡根目錄新增 `boot_time.txt` 檔案，系統開機時會自動讀取並設定 RTC 時間。

#### 檔案格式

```
yyyy/MM/dd HH:mm
```

範例：
```
2026/05/20 14:35
```

#### 行為說明

1. 系統啟動時檢查 `boot_time.txt` 是否存在
2. 若存在，解析檔案內容並設定 RTC
3. **設定完成後自動刪除檔案**，避免下次開機重複設定
4. 若檔案不存在或格式錯誤，RTC 維持目前時間

#### 程式碼位置

[run_File.c:487-559](lib/FatFs_SPI/src/run_File.c#L487-L559) - `readBootTimeConfig()`

---

## 4. 刷新時間設定

### 4.1 使用 refresh_config.txt 檔案

刷新時間由 SD 卡 `refresh_config.txt` 設定，定義每天自動刷新圖片的時間點。

#### 檔案格式

以逗號分隔的小時列表 (24小時制)：

```
7,12,17,19
```

此範例表示每天的 07:00、12:00、17:00、19:00 刷新圖片。

#### 預設值

若檔案不存在或格式錯誤，預設為每天 **07:00** 刷新。

#### 行為說明

1. 開機時讀取 `refresh_config.txt`
2. 根據目前時間計算下一個刷新時間點
3. 設定 RTC 鬧鐘觸發刷新
4. 觸發後自動計算下一個時間點

#### 程式碼位置

[run_File.c:570-650](lib/FatFs_SPI/src/run_File.c#L570-L650) - `readRefreshConfig()`

---

## 5. RTC 鬧鐘功能

### 5.1 啟用鬧鐘

```c
void PCF85063_alarm_Time_Enabled(Time_data time);
```

### 5.2 停用鬧鐘

```c
void PCF85063_alarm_Time_Disable();
```

### 5.3 取得鬧鐘標誌

```c
int PCF85063_get_alarm_flag();
// 回傳 1 表示鬧鐘已觸發，0 表示未觸發
```

### 5.4 清除鬧鐘標誌

```c
void PCF85063_clear_alarm_flag();
```

### 5.5 執行鬧鐘

```c
void rtcRunAlarm(Time_data time, Time_data alarmTime);
```

---

## 6. SD 卡檔案總覽

| 檔案 | 用途 |
|------|------|
| `boot_time.txt` | 設定 RTC 開機時間 (一次性) |
| `refresh_config.txt` | 設定自動刷新時間點 |
| `events.txt` | 儲存日期事件 |
| `fileList.txt` | 圖片檔案列表 |
| `index.txt` | 目前播放圖片索引 |
| `0:/pic/` | 圖片目錄 (BMP 格式) |

---

## 7. 系統運作流程

### 7.1 開機流程

```
1. 模組初始化 (DEV_Module_Init)
2. SD 卡測試與掛載
3. 讀取 boot_time.txt → 設定 RTC (如存在則刪除檔案)
4. 讀取 refresh_config.txt → 設定第一個刷新時間
5. 讀取 events.txt → 快取今日事件
6. 根據目前電壓與充電狀態進入不同模式
```

### 7.2 主循環

- **未充電模式**: 顯示圖片後進入深度睡眠
- **充電模式**: 持續監聽 RTC 鬧鐘與按鍵中斷，有事件時刷新圖片

---

## 8. 電壓與電源管理

| 電壓範圍 | 行為 |
|----------|------|
| < 3.1V | 低電量警告，LED 閃爍，關機 |
| 3.1V - 3.3V | 正常運作 |
| > 3.3V (USB供電) | 進入充電模式 |

---

## 9. 相關檔案列表

### 核心檔案

- [main.c](main.c) - 主程式
- [lib/RTC/waveshare_PCF85063.c](lib/RTC/waveshare_PCF85063.c) - RTC 驅動程式
- [lib/RTC/waveshare_PCF85063.h](lib/RTC/waveshare_PCF85063.h) - RTC 驅動程式表頭
- [lib/FatFs_SPI/src/run_File.c](lib/FatFs_SPI/src/run_File.c) - 檔案系統操作
- [lib/FatFs_SPI/src/run_File.h](lib/FatFs_SPI/src/run_File.h) - 檔案系統 API
- [lib/e-Paper/EPD_7in3f.c](lib/e-Paper/EPD_7in3f.c) - 顯示器驅動程式