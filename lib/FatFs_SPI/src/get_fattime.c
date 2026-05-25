/* get_fattime.c
 * Stub implementation for FatFs file timestamps
 * Returns a fixed time (2024-01-01 00:00:00) when no RTC is available
 */
#include "ff.h"

DWORD get_fattime(void) {
    // Fixed timestamp: 2024-01-01 00:00:00
    // Year: 2024-1980 = 44 -> 0x2C
    // Month: 1, Day: 1, Hour: 0, Minute: 0, Second: 0
    return (44 << 25) | (1 << 21) | (1 << 16);
}