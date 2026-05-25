# Find all source files in a single current directory
# Save the name to DIR_Config_SRCS
aux_source_directory(. DIR_Config_SRCS)

# Generate the link library
add_library(Config ${DIR_Config_SRCS})
target_link_libraries(Config PUBLIC pico_stdlib hardware_spi hardware_adc hardware_i2c hardware_dma FatFs_SPI)

target_include_directories(Config PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../lib/FatFs_SPI/include
    ${CMAKE_CURRENT_LIST_DIR}/../../lib/FatFs_SPI/sd_driver
    ${CMAKE_CURRENT_LIST_DIR}/../../lib/FatFs_SPI/ff14a/source
)