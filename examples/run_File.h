#ifndef _RUN_FILE_H_
#define _RUN_FILE_H_

#include "DEV_Config.h"

#define fileNumber 100
#define fileLen 100

char sdTest(void);
void sdInitTest(void);

void run_mount(void);
void run_unmount(void);

void file_cat(void);

void sdScanDir(void);

char isFileExist(const char *path);
void setFilePath(void);
char readBootTimeConfig(int *year, int *month, int *day, int *hour, int *minute);
char readRefreshConfig(int *refreshHours, int *count);
void readEventsFile(char *eventBuffer, int bufferSize);
void updatePathIndex(void);
void file_sort();

#endif
