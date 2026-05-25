#include "run_File.h"
#include "f_util.h"
#include "pico/stdlib.h"
#include "hw_config.h"
#include "waveshare_PCF85063.h"

#include <stdio.h>
#include <stdlib.h> // malloc() free()
#include <string.h>

const char *fileList = "fileList.txt";          // Picture names store files
const char *fileListNew = "fileListNew.txt";    // Sort good picture name temporarily store file
char pathName[fileLen];                         // The name of the picture to display
int scanFileNum = 0;                            // The number of images scanned

static sd_card_t *sd_get_by_name(const char *const name) {
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name)) return sd_get_by_num(i);
    // DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}

static FATFS *sd_get_fs_by_name(const char *name) {
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name)) return &sd_get_by_num(i)->fatfs;
    // DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}

/*
    function:
        Mount an sd card
    parameter:
        none
*/
void run_mount() {
    sd_card_t *pSD = sd_get_by_num(0);
    if (!pSD) {
        printf("Unknown logical drive number: \"0\"\n");
        return;
    }
    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (FR_OK != fr) {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    pSD->mounted = true;
}

/*
    function:
        Uninstalling an sd card
    parameter:
        none
*/
void run_unmount() {
    sd_card_t *pSD = sd_get_by_num(0);
    if (!pSD) {
        printf("Unknown logical drive number: \"0\"\n");
        return;
    }
    FRESULT fr = f_unmount(pSD->pcName);
    if (FR_OK != fr) {
        printf("f_unmount error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    pSD->mounted = false;
}

/* 
    function: 
        Query file content
    parameter: 
        path: File path
*/
static void run_cat(const char *path) {
    // char *arg1 = strtok(NULL, " ");
    if (!path) 
    {
        printf("Missing argument\n");
        return;
    }
    FIL fil;
    FRESULT fr = f_open(&fil, path, FA_READ);
    if (FR_OK != fr) 
    {
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    char buf[256];
    int i=0;
    while (f_gets(buf, sizeof buf, &fil)) 
    {
        printf("%5d,%s", ++i,buf);
    }

    printf("The number of file names read is %d",i);
    scanFileNum = i;

    fr = f_close(&fil);
    if (FR_OK != fr) 
        printf("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
}

/* 
    function: 
        Query files in the corresponding directory
    parameter: 
        dir: Directory path
*/
void ls(const char *dir) {
    char cwdbuf[FF_LFN_BUF] = {0};
    FRESULT fr; /* Return value */
    char const *p_dir;
    if (dir[0]) {
        p_dir = dir;
    } else {
        fr = f_getcwd(cwdbuf, sizeof cwdbuf);
        if (FR_OK != fr) {
            printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
            return;
        }
        p_dir = cwdbuf;
    }
    printf("Directory Listing: %s\n", p_dir);
    DIR dj;      /* Directory object */
    FILINFO fno; /* File information */
    memset(&dj, 0, sizeof dj);
    memset(&fno, 0, sizeof fno);
    fr = f_findfirst(&dj, &fno, p_dir, "*");
    if (FR_OK != fr) {
        printf("f_findfirst error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    while (fr == FR_OK && fno.fname[0]) { /* Repeat while an item is found */
        /* Create a string that includes the file name, the file size and the
         attributes string. */
        const char *pcWritableFile = "writable file",
                   *pcReadOnlyFile = "read only file",
                   *pcDirectory = "directory";
        const char *pcAttrib;
        /* Point pcAttrib to a string that describes the file. */
        if (fno.fattrib & AM_DIR) {
            pcAttrib = pcDirectory;
        } else if (fno.fattrib & AM_RDO) {
            pcAttrib = pcReadOnlyFile;
        } else {
            pcAttrib = pcWritableFile;
        }
        /* Create a string that includes the file name, the file size and the
         attributes string. */
        printf("%s [%s] [size=%llu]\n", fno.fname, pcAttrib, fno.fsize);

        fr = f_findnext(&dj, &fno); /* Search for next item */
    }
    f_closedir(&dj);
}

/* 
    function: 
        Query the images in the directory and save their names to the appropriate file
    parameter: 
        dir: Directory path
        path: File path
*/
void ls2file(const char *dir, const char *path) {
    char cwdbuf[FF_LFN_BUF] = {0};
    FRESULT fr; /* Return value */
    char const *p_dir;
    if (dir[0]) {
        p_dir = dir;
    } else {
        fr = f_getcwd(cwdbuf, sizeof cwdbuf);
        if (FR_OK != fr) {
            printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
            return;
        }
        p_dir = cwdbuf;
    }
    printf("Directory Listing: %s\n", p_dir);
    DIR dj;      /* Directory object */
    FILINFO fno; /* File information */
    memset(&dj, 0, sizeof dj);
    memset(&fno, 0, sizeof fno);
    fr = f_findfirst(&dj, &fno, p_dir, "*");
    if (FR_OK != fr) {
        printf("f_findfirst error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }

    int filNum=0;
    FIL fil;
    printf("[DEBUG] ls2file: creating %s\r\n", path);
    fr =  f_open(&fil, path, FA_CREATE_ALWAYS | FA_WRITE);
    if(FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d) \n", path, FRESULT_str(fr), fr);
    // f_printf(&fil, "{");
    printf("[DEBUG] ls2file: scanning directory...\r\n");
    while (fr == FR_OK && fno.fname[0]) { /* Repeat while an item is found */
        /* Create a string that includes the file name, the file size and the
         attributes string. */
        const char *pcWritableFile = "writable file",
                   *pcReadOnlyFile = "read only file",
                   *pcDirectory = "directory";
        const char *pcAttrib;
        /* Point pcAttrib to a string that describes the file. */
        if (fno.fattrib & AM_DIR) {
            pcAttrib = pcDirectory;
        } else if (fno.fattrib & AM_RDO) {
            pcAttrib = pcReadOnlyFile;
        } else {
            pcAttrib = pcWritableFile;
        }
        /* Create a string that includes the file name, the file size and the
         attributes string. */
        if(fno.fname) {
            // f_printf(&fil, "%d %s\r\n", filNum, fno.fname);
            f_printf(&fil, "0:/pic/%s\r\n", fno.fname);
            filNum++;
        }
        fr = f_findnext(&dj, &fno); /* Search for next item */
    }
    printf("The number of file names written is: %d\r\n", filNum);
    scanFileNum = filNum;
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }
    f_closedir(&dj);
}

/* 
    function: 
        TF card and file system initialization and testing
    parameter: 
        none
*/
void sdInitTest(void)
{
    puts("Hello, world!");

    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html
    sd_card_t *pSD = sd_get_by_num(0);
    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    FIL fil;
    const char* const filename = "filename.txt";
    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    if (f_printf(&fil, "Hello, world!\n") < 0) {
        printf("f_printf failed\n");
    }
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }
    f_unmount(pSD->pcName);

    puts("Goodbye, world!");
}

/* 
    function: 
        TF card mounting test
    parameter: 
        none
*/
char sdTest(void)
{
    printf("[DEBUG] sdTest: checking SD card...\r\n");
    sd_card_t *pSD = sd_get_by_num(0);
    if(pSD == NULL) {
        printf("[DEBUG] sdTest: sd_get_by_num(0) returned NULL\r\n");
        return 1;
    }
    printf("[DEBUG] sdTest: pcName=%s\r\n", pSD->pcName);
    FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
    printf("[DEBUG] sdTest: f_mount result=%d\r\n", fr);
    if(FR_OK != fr) {
        printf("[DEBUG] sdTest: mount failed, FRESULT=%d\r\n", fr);
        return 1;
    }
    else {
        printf("[DEBUG] sdTest: mount success, unmounting...\r\n");
        f_unmount(pSD->pcName);
        return 0;
    }
}


/* 
    function: 
        Gets the contents of the list file
    parameter: 
        none
*/
void file_cat(void)
{
    run_mount();

    run_cat(fileList);

    run_unmount();
}

/* 
    function: 
        Scan the image directory and save the results directly, regardless of whether the list file exists
    parameter: 
        none
*/
void sdScanDir(void)
{
    printf("[DEBUG] sdScanDir: mounting SD card...\r\n");
    run_mount();

    printf("[DEBUG] sdScanDir: scanning 0:/pic...\r\n");
    ls2file("0:/pic", fileList);
    printf("ls %s\r\n", fileList);
    run_cat(fileList);

    run_unmount();
}

/* 
    function: 
        Read the name of the image to be refreshed and store it in an array for use by this program
    parameter: 
        none
*/
void fil2array(int index)
{
    run_mount();

    FRESULT fr; /* Return value */
    FIL fil;

    printf("[DEBUG] fil2array: opening %s\r\n", fileList);
    fr =  f_open(&fil, fileList, FA_READ);
    if(FR_OK != fr && FR_EXIST != fr) {
        printf("[DEBUG] fil2array ERROR: f_open failed, fr=%d\r\n", fr);
        printf("fil2array open error\r\n");
        run_unmount();
        return;
    }

    printf("[DEBUG] fil2array: reading line %d\r\n", index);
    // printf("ls array path\r\n");
    for(int i=0; i<index; i++) {
        if(f_gets(pathName, 999, &fil) == NULL) {
            printf("[DEBUG] fil2array: f_gets returned NULL at i=%d\r\n", i);
            break;
        }
        printf("[DEBUG] fil2array: line %d = %s", i+1, pathName);
    }

    f_close(&fil);
    run_unmount();
}

/* 
    function: 
        Set the image index number, will be written to the index file
    parameter: 
        index: Picture index number
*/
static void setPathIndex(int index)
{
    FRESULT fr; /* Return value */
    FIL fil;
    UINT br;

    run_mount();

    fr =  f_open(&fil, "index.txt", FA_OPEN_ALWAYS | FA_WRITE);
    if(FR_OK != fr && FR_EXIST != fr) {
        printf("setPathIndex open error\r\n");
        run_unmount();
        return;
    }
    f_printf(&fil, "%d\r\n", index);
    printf("set index is %d\r\n", index);

    f_close(&fil);
    run_unmount();
}

/* 
    function: 
        Gets the current index number from the index file
    parameter: 
        none
    return: 
        Picture index number
*/
static int getPathIndex(void)
{
    int index = 0;
    char indexs[10];
    FRESULT fr; /* Return value */
    FIL fil;

    run_mount();

    fr =  f_open(&fil, "index.txt", FA_READ);
    if(FR_OK != fr && FR_EXIST != fr) {
        printf("getPathIndex open error\r\n");
        run_unmount();
        return 0;
    }
    f_gets(indexs, 10, &fil);
    sscanf(indexs, "%d", &index);   // char to int
    if(index > scanFileNum) 
    {
        index = 1;
        printf("get index over scanFileNum\r\n");    
    }
    if(index < 1)
    {
        index = 1;
        printf("get index over one\r\n");  
    }
    printf("get index is %d\r\n", index);
    
    f_close(&fil);
    run_unmount();
    
    return index;
}

/* 
    function: 
        Set the image path according to the current image index number
    parameter: 
        none
*/
void setFilePath(void)
{
    int index = 1;

    printf("[DEBUG] setFilePath called, hasCard=1\r\n");

    if(isFileExist("index.txt")) {
        printf("[DEBUG] index.txt exists, reading index...\r\n");
        index = getPathIndex();
        printf("[DEBUG] index from file: %d\r\n", index);
    }
    else {
        printf("[DEBUG] index.txt NOT exist, creating with index=1\r\n");
        setPathIndex(1);
    }

    printf("[DEBUG] Calling fil2array with index=%d\r\n", index);
    fil2array(index);
    printf("[DEBUG] fil2array done, pathName=%s\r\n", pathName);
    printf("setFilePath is %s\r\n", pathName);
}

/* 
    function: 
        Update the image index. Update the image index after the image refresh is successful
    parameter: 
        none
*/
void updatePathIndex(void)
{
    int index = 1;

    index = getPathIndex();
    index++;
    if(index > scanFileNum)
        index = 1;
    setPathIndex(index);
    printf("updatePathIndex index is %d\r\n", index);
}

/*
    function:
        Read boot_time.txt from SD card and return time data
    parameter:
        year, month, day, hour, minute: pointers to store parsed values
    return:
        1 if file exists and was parsed successfully, 0 otherwise
*/
char readBootTimeConfig(int *year, int *month, int *day, int *hour, int *minute)
{
    FRESULT fr;
    FIL fil;
    char line[64];
    char *p;

    run_mount();

    fr = f_open(&fil, "boot_time.txt", FA_READ);
    if(FR_NO_FILE == fr) {
        printf("[DEBUG] boot_time.txt not found\r\n");
        run_unmount();
        return 0;
    }
    if(FR_OK != fr) {
        printf("[DEBUG] boot_time.txt open error: %d\r\n", fr);
        run_unmount();
        return 0;
    }

    if(f_gets(line, sizeof(line), &fil) == NULL) {
        printf("[DEBUG] boot_time.txt read error\r\n");
        f_close(&fil);
        run_unmount();
        return 0;
    }

    f_close(&fil);

    // Delete boot_time.txt after reading to avoid re-setting on next boot
    fr = f_unlink("boot_time.txt");
    if(FR_OK != fr) {
        printf("[DEBUG] boot_time.txt delete warning, fr=%d\r\n", fr);
    } else {
        printf("[DEBUG] boot_time.txt deleted after reading\r\n");
    }

    run_unmount();

    printf("[DEBUG] boot_time content: %s", line);

    // Parse format: "2026/05/13 14:35"
    // Expected format: yyyy/MM/dd HH:mm
    p = line;

    // Parse year (4 digits)
    if(strlen(p) < 16) {
        printf("[DEBUG] boot_time format error: line too short\r\n");
        return 0;
    }

    // Year: first 4 digits before '/'
    char yearStr[5] = {0}, monthStr[3] = {0}, dayStr[3] = {0}, hourStr[3] = {0}, minStr[3] = {0};

    strncpy(yearStr, p, 4); *year = atoi(yearStr) - 2000;  // RTC only supports 2-digit year (0-99)
    p += 4; if(*p == '/') p++;

    strncpy(monthStr, p, 2); *month = atoi(monthStr);
    p += 2; if(*p == '/') p++;

    strncpy(dayStr, p, 2); *day = atoi(dayStr);
    p += 2; if(*p == ' ') p++;

    strncpy(hourStr, p, 2); *hour = atoi(hourStr);
    p += 2; if(*p == ':') p++;

    strncpy(minStr, p, 2); *minute = atoi(minStr);

    printf("[DEBUG] Parsed time: %04d/%02d/%02d %02d:%02d\r\n", *year, *month, *day, *hour, *minute);

    return 1;
}

/*
    function:
        Read refresh_config.txt to get refresh hours
    parameter:
        refreshHours: array to store refresh hours (output)
        count: number of refresh hours parsed (output)
    return:
        1 if file exists and was parsed successfully, 0 otherwise
*/
char readRefreshConfig(int *refreshHours, int *count)
{
    FRESULT fr;
    FIL fil;
    char line[64];
    char *p;
    int i;

    run_mount();

    fr = f_open(&fil, "refresh_config.txt", FA_READ);
    if(FR_NO_FILE == fr) {
        printf("[DEBUG] refresh_config.txt not found, using default\r\n");
        run_unmount();
        refreshHours[0] = 7;
        *count = 1;
        return 0;
    }
    if(FR_OK != fr) {
        printf("[DEBUG] refresh_config.txt open error: %d\r\n", fr);
        run_unmount();
        refreshHours[0] = 7;
        *count = 1;
        return 0;
    }

    if(f_gets(line, sizeof(line), &fil) == NULL) {
        printf("[DEBUG] refresh_config.txt read error\r\n");
        f_close(&fil);
        run_unmount();
        refreshHours[0] = 7;
        *count = 1;
        return 0;
    }

    f_close(&fil);
    run_unmount();

    printf("[DEBUG] refresh_config content: %s", line);

    // Parse format: "7,12,17,19" (comma-separated hours)
    *count = 0;
    p = line;

    while(*p != '\0' && *count < 10) {
        // Skip non-digit characters
        while(*p != '\0' && (*p < '0' || *p > '9')) {
            p++;
        }

        if(*p == '\0') break;

        refreshHours[*count] = atoi(p);
        (*count)++;

        // Skip digits
        while(*p >= '0' && *p <= '9') {
            p++;
        }

        // Skip comma/space
        while(*p == ',' || *p == ' ' || *p == '\r' || *p == '\n') {
            p++;
        }
    }

    if(*count == 0) {
        printf("[DEBUG] refresh_config format error, using default\r\n");
        refreshHours[0] = 7;
        *count = 1;
        return 0;
    }

    printf("[DEBUG] Parsed %d refresh hours: ", *count);
    for(i = 0; i < *count; i++) {
        printf("%d ", refreshHours[i]);
    }
    printf("\r\n");

    return 1;
}

/*
    function:
        Checks if the file exists
    parameter:
        none
    return:
        0: inexistence
        1: exist
*/
char isFileExist(const char *path)
{
    FRESULT fr; /* Return value */
    FIL fil;

    run_mount();

    fr =  f_open(&fil, path, FA_READ);
    if(FR_OK != fr && FR_EXIST != fr) {
        printf("%s is not exist\r\n", path);
        run_unmount();
        return 0;
    }
    
    f_close(&fil);
    run_unmount();

    return 1;
}


// Compare function for qsort
int compare_strings(const char *a, const char *b) {
    return strcmp(a, b);
}


/* 
    function: 
        Custom quick sort for sorting a 2D array of strings
    parameter: 
        arr: The array to sort
        left: Sort starting point
        right: End of the order, notice minus one
*/
void custom_qsort(char arr[fileNumber][fileLen], int left, int right) {
    if (left >= right) {
        return;
    }

    int pivot_index = left;
    char pivot[100];
    strcpy(pivot, arr[pivot_index]);

    int i = left;
    int j = right;

    while (i <= j) {
        while (compare_strings(arr[i], pivot) < 0) {
            i++;
        }
        while (compare_strings(arr[j], pivot) > 0) {
            j--;
        }
        if (i <= j) {
            char temp[100];
            strcpy(temp, arr[i]);
            strcpy(arr[i], arr[j]);
            strcpy(arr[j], temp);
            i++;
            j--;
        }
    }

    custom_qsort(arr, left, j);
    custom_qsort(arr, i, right);
}


/* 
    function: 
        Array copy and sort
*/
void file_copy(char temp[fileNumber][fileLen], char templist[fileNumber/2][fileLen], char templistnew[fileNumber/2][fileLen], char count)
{
    memcpy(temp, templist, fileNumber/2*fileLen);
    memcpy(temp[fileNumber/2], templistnew, count*fileLen);
    custom_qsort(temp, 0, fileLen/2 + count -1);
    memcpy(templist, temp, fileNumber/2*fileLen);
    memcpy(templistnew, temp[fileNumber/2], count*fileLen);
}

/* 
    function: 
        Array copy
*/
void file_copy1(char temp[fileNumber][fileLen], char templist[fileNumber/2][fileLen])
{
    memcpy(templist, temp, fileNumber/2*fileLen);
}

void file_copy2(char temp[fileNumber][fileLen], char templist[fileNumber/2][fileLen])
{
    memcpy(templist, temp[fileNumber/2], fileNumber/2*fileLen);
}


/* 
    function: 
        Read the contents of the file, write them into an array, and sort
    parameter: 
        temp: Array to be written
        count: The amount to write
        fil: File pointer
    return: 
        Returns the number of arrays written
*/
char file_gets(char temp[][fileLen], char count, FIL* fil)
{
    for(char i=0; i<count; i++)
    {
        strcpy(temp[i], "");
    }

    char i=0;
    for(i=0; i<count; i++)
    {
        if(f_gets(temp[i], 999, fil) == NULL) 
        {
            custom_qsort(temp, 0, i-1);
            return i;
            break;
        }
    }
    custom_qsort(temp, 0, count-1);
    return i;
}

/* 
    function: 
        Read the contents saved in a temporary file
    parameter: 
        temp: Array to be written
        path: Temporary file name
    return: 
        Returns the number of arrays written
*/
char file_temporary_gets(char temp[][fileLen], const char *path)
{
    for(char i=0; i<50; i++)
    {
        strcpy(temp[i], "");
    }

    FRESULT fr; /* Return value */
    FIL fil;
    char i=0;

    fr =  f_open(&fil, path, FA_READ);
    if(FR_OK != fr && FR_EXIST != fr) {
        printf("Error opening temporary file\r\n");
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        run_unmount();
        return -1;
    }

    for(i=0; i<fileNumber/2; i++)
    {
        if(f_gets(temp[i], 999, &fil) == NULL) 
        {
            f_close(&fil);
            return i;
        }
    }
    f_close(&fil);
    return i;
}


/* 
    function: 
        Write count string of data to temporary file
    parameter: 
        temp: What to write
        count: Number of writes
        path: Temporary file name
*/
void file_temporary_puts(char temp[][fileLen], char count, const char *path)
{
    FRESULT fr; /* Return value */
    FIL fil;
    fr =  f_open(&fil, path, FA_CREATE_ALWAYS | FA_WRITE);
    if(FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d) \n", path, FRESULT_str(fr), fr);

    for(char i=0; i<count; i++)
        f_puts(temp[i], &fil);

    f_close(&fil);
}

/* 
    function: 
        Write count string of data to file
    parameter: 
        temp: What to write
        count: Number of writes
        fil: File pointer
*/
void file_puts(char temp[][fileLen], char count, FIL* fil)
{
    for(char i=0; i<count; i++)
        f_puts(temp[i], fil);
}



/* 
    function: 
        The name of the file that created the temporary file
    parameter: 
        temp: File name storage array
        count: Number of pictures
    return: 
        Generate the number of temporary file names
*/
int Temporary_file(char temp[][10], int count)
{
    int k = 0;
    int i = 0;
    char str1[10] = "ls";
    char str2[10];
    k = (count % 50) ? (count / 50) : (count / 50 - 1);
    for (i = 0 ; i < k; i++)
    {
        memcpy(temp[i], str1, sizeof(str1));
        sprintf(str2, "%d", i);
        strcat(temp[i], str2);
    }
    printf("Total number of temporary file names generated: %d\r\n",k);
    return i;
}

/* 
    function: 
        Delete the temporary file name and rename the sorted file
    parameter: 
        temp: File name storage array
        count: Number of temporary files
*/
void file_rm_ren(char temp[][10], int count)
{
    FRESULT fr; /* Return value */

    printf("remove temporary file\r\n");
    for(int i=0; i<count; i++)
    {
        fr = f_unlink(temp[i]);
        if (FR_OK != fr) 
        {
            printf("f_unlink error: %s (%d)\n", FRESULT_str(fr), fr);
        }
    }

    printf("remove fileList\r\n");
    fr = f_unlink(fileList);
    if (FR_OK != fr) {
        printf("f_unlink error: %s (%d)\n", FRESULT_str(fr), fr);
    }

    printf("rename fileListNew to fileList\r\n");
    fr = f_rename(fileListNew, fileList);
    if (FR_OK != fr) {
        printf("f_rename error: %s (%d)\n", FRESULT_str(fr), fr);
    }
}

/*
    function:
        Read events.txt from SD card and find today's events
    parameter:
        eventBuffer: buffer to store matched event text (should be at least 128 bytes)
        bufferSize: size of eventBuffer
    return:
        1 if events found and written to buffer, 0 otherwise
*/
void readEventsFile(char *eventBuffer, int bufferSize)
{
    FRESULT fr;
    FIL fil;
    char line[128];
    char currentDate[6];
    int foundCount = 0;

    run_mount();

    // Try to open file - use full path like other file operations
    fr = f_open(&fil, "0:/events.txt", FA_READ);
    if(FR_NO_FILE == fr) {
        printf("[DEBUG] events.txt does not exist\r\n");
        run_unmount();
        return;
    }
    if(FR_OK != fr) {
        printf("[DEBUG] events.txt open error: %d (%s)\r\n", fr, FRESULT_str(fr));
        run_unmount();
        return;
    }

    printf("[DEBUG] events.txt opened successfully\r\n");

    // Get current RTC date
    Time_data rtcTime = PCF85063_GetTime();
    sprintf(currentDate, "%02d-%02d", rtcTime.months, rtcTime.days);
    printf("[DEBUG] Looking for events on: %s\r\n", currentDate);

    eventBuffer[0] = '\0';

    while(f_gets(line, sizeof(line), &fil) != NULL) {
        // Skip empty lines
        if(line[0] == '\0' || line[0] == '\r' || line[0] == '\n') {
            continue;
        }

        // Remove trailing newline characters
        int len = strlen(line);
        while(len > 0 && (line[len-1] == '\r' || line[len-1] == '\n')) {
            line[len-1] = '\0';
            len--;
        }

        // Check if line starts with current date (MM-DD format)
        if(strncmp(line, currentDate, 5) == 0) {
            // Found matching date, extract event text after comma
            char *comma = strchr(line, ',');
            if(comma != NULL) {
                comma++; // Skip past the comma
                // Remove leading space if present
                while(*comma == ' ') comma++;

                if(foundCount > 0) {
                    // Append with space separator
                    int currentLen = strlen(eventBuffer);
                    if(currentLen < bufferSize - 1) {
                        strncat(eventBuffer, " ", bufferSize - currentLen - 1);
                    }
                }

                int remaining = bufferSize - strlen(eventBuffer) - 1;
                if(remaining > 0) {
                    strncat(eventBuffer, comma, remaining);
                }
                foundCount++;
                printf("[DEBUG] Event found: %s\r\n", comma);
            }
        }
    }

    f_close(&fil);
    run_unmount();

    if(foundCount > 0) {
        printf("[DEBUG] Events for today: %s\r\n", eventBuffer);
    } else {
        printf("[DEBUG] No events for today\r\n");
    }
}


/*
    Sort the image names in the file
*/
void file_sort()
{
    char temp[fileNumber][fileLen];
    char templist1[fileNumber/2][fileLen];
    char templist2[fileNumber/2][fileLen];
    char Temporary_file_name[1000][10];
    int file_count, file_count1;

    file_count = Temporary_file(Temporary_file_name, scanFileNum);  
      
    run_mount();

    FRESULT fr; /* Return value */
    FIL fil, fil1;

    fr =  f_open(&fil, fileList, FA_READ);
    if(FR_OK != fr && FR_EXIST != fr) {
        printf("file open error2\r\n");
        run_unmount();
        return;
    }

    int scanFileNum1=0;
    int scanFileNum2=0;
    int js=0;
    for(char i=0; i<fileNumber; i++)
    {
        if(f_gets(temp[i], 999, &fil) == NULL) 
        {
            scanFileNum1 = i;
            break;
        }
        if(i == 99)
            scanFileNum1 = 100;
    }

    if(file_count < 2)
    {
        custom_qsort(temp, 0, scanFileNum1-1);
        fr = f_close(&fil);
        if (FR_OK != fr) {
            printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        }

        fr =  f_open(&fil, fileList, FA_CREATE_ALWAYS | FA_WRITE);
        if(FR_OK != fr && FR_EXIST != fr)
            panic("f_open1(%s) error: %s (%d) \n", fileList, FRESULT_str(fr), fr);

        file_puts(temp, scanFileNum1, &fil);

        fr = f_close(&fil);
        if (FR_OK != fr) {
            printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        }
    }
    else
    {
        file_count1 = 0; 
        custom_qsort(temp, 0, fileLen-1);
        file_copy1(temp, templist1);
        file_copy2(temp, templist2);

        file_temporary_puts(templist2, fileNumber/2, Temporary_file_name[file_count1++]);

        do
        {
            scanFileNum1 = file_gets(templist2, fileNumber/2, &fil);
            if(scanFileNum1 <= 0) break;
            if(!(strcmp(templist1[fileNumber/2-1], templist2[0]) < 0))
            {
                file_copy(temp, templist1, templist2, scanFileNum1);
            }
            file_temporary_puts(templist2, scanFileNum1, Temporary_file_name[file_count1++]);
            DEV_Delay_ms(1);
            // printf("js = %d   scanFileNum1 = %d \r\n", js++, scanFileNum1);
        }while(scanFileNum1 == fileNumber/2 && !f_eof(&fil));

        fr = f_close(&fil);
        if (FR_OK != fr) {
            printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        }

        fr =  f_open(&fil, fileListNew, FA_CREATE_ALWAYS | FA_WRITE);
        if(FR_OK != fr && FR_EXIST != fr)
            panic("f_open1(%s) error: %s (%d) \n", fileListNew, FRESULT_str(fr), fr);

        file_puts(templist1, fileNumber/2, &fil);

        js = 0;
        for (int i = 0; i < file_count; i++)
        {
            scanFileNum2 = file_temporary_gets(templist1, Temporary_file_name[i]);
            for (int j = i+1; j < file_count; j++)
            {
                scanFileNum1 = file_temporary_gets(templist2, Temporary_file_name[j]);
                if(!(compare_strings(templist1[fileNumber/2-1], templist2[0]) < 0))
                {
                    file_copy(temp, templist1, templist2, scanFileNum1);
                    file_temporary_puts(templist2, scanFileNum1, Temporary_file_name[j]);
                }
                // DEV_Delay_ms(1);
            }
            file_puts(templist1, scanFileNum2, &fil);
            DEV_Delay_ms(1);
            // printf("js1 = %d \r\n", js++);
        }
        fr = f_close(&fil);
        if (FR_OK != fr) {
            printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        }

        file_rm_ren(Temporary_file_name, file_count);
    }
    run_unmount();
}



