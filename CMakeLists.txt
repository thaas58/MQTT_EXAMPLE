cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(mqtt_example C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.2.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.2.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
      )  

if (PICO_CYW43_SUPPORTED) # set by BOARD=pico-w
    if (NOT TARGET pico_cyw43_arch)
        message("Skipping build as support is not available")
    else()
        set(WIFI_SSID "${WIFI_SSID}" CACHE INTERNAL "WiFi SSID")
        set(WIFI_PASSWORD "${WIFI_PASSWORD}" CACHE INTERNAL "WiFi password")
        set(USER_NAME "${USER_NAME}" CACHE INTERNAL "MQTT user name")
        set(USER_PW "${USER_PW}" CACHE INTERNAL "MQTT user password")

        if ("${WIFI_SSID}" STREQUAL "")
            message("Skipping build as WIFI_SSID is not defined")
        elseif ("${WIFI_PASSWORD}" STREQUAL "")
            message("Skipping build as WIFI_PASSWORD is not defined")
        else()
            add_executable(mqtt_example
	        mqtt_example.c
	        mqtt_example.h
            aht20.c
            aht20.h
	        lwipopts.h
            )

            target_compile_definitions(mqtt_example PRIVATE
            WIFI_SSID=\"${WIFI_SSID}\"
            WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
            USER_NAME=\"${USER_NAME}\"
            USER_PW=\"${USER_PW}\"
            TEST_MQTT_SERVER_IP=\"${TEST_MQTT_SERVER_IP}\"
            PICO_DEFAULT_I2C=0
            PICO_DEFAULT_I2C_SDA_PIN=20
            PICO_DEFAULT_I2C_SCL_PIN=21
            )

            target_include_directories(mqtt_example PRIVATE
                ${CMAKE_CURRENT_LIST_DIR}
                ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
            )

            target_link_libraries(mqtt_example
                hardware_i2c
                pico_cyw43_arch_lwip_threadsafe_background 
                pico_stdlib
                pico_lwip_iperf
                pico_lwip_mqtt
            )

            pico_add_extra_outputs(mqtt_example)
        endif()
    endif()
endif()
