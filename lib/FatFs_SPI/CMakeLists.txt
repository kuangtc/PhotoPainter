add_library(FatFs_SPI)
target_sources(FatFs_SPI PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/ff14a/source/ffsystem.c
    ${CMAKE_CURRENT_LIST_DIR}/ff14a/source/ffunicode.c
    ${CMAKE_CURRENT_LIST_DIR}/ff14a/source/ff.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/sd_spi.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/demo_logging.c
#    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/hw_config.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/spi.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/sd_card.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/crc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/glue.c
    ${CMAKE_CURRENT_LIST_DIR}/src/f_util.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ff_stdio.c
    ${CMAKE_CURRENT_LIST_DIR}/src/my_debug.c
    ${CMAKE_CURRENT_LIST_DIR}/src/get_fattime.c
)
target_include_directories(FatFs_SPI PRIVATE
    ff14a/source
    sd_driver
    include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_rtc/include
    ${PICO_SDK_PATH}/src/common/pico_util/include
    ${PICO_SDK_PATH}/src/rp2_common/pico_stdio/include
    ${PICO_SDK_PATH}/src/common/pico_stdlib_headers/include
)
target_link_libraries(FatFs_SPI PRIVATE
        hardware_spi
        hardware_dma
        pico_stdlib
)
