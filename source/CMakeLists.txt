add_executable(${PROJECT_NAME}
    main.cpp
    oled/oled.cpp
    QR-Code-generator/qrcodegen.c
    # Add other cpp files here, including their directory, e.g.
    # webserver/webserver.cpp
    )

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/no-OS-FatFS-SD-SPI-Rpi-Pico/FatFs_SPI build-fatfs-spi-pico)

# Adding a directory here means anything within this project can include it
# Might be the wrong way to do it but I couldn't figure out the right way
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/oled
    ${CMAKE_CURRENT_LIST_DIR}/oled/font
    ${CMAKE_CURRENT_LIST_DIR}/QR-Code-generator
    # Add the folders that are needed for compile, e.g.
    # ${CMAKE_CURRENT_LIST_DIR}/webserver
    )

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_http
    FatFs_SPI
    # You'll need to link other libraries for other pico functions
    )

# Enable USB and UART for serial output
pico_enable_stdio_usb(${PROJECT_NAME} 1)  # For USB output
pico_enable_stdio_uart(${PROJECT_NAME} 0) # For picoprobe debug output

# By default make only produces a .elf, this function allows uf2 and others to be generated
pico_add_extra_outputs(${PROJECT_NAME})
