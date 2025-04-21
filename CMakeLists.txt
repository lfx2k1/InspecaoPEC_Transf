cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico CACHE STRING "Board type")
include(pico_sdk_import.cmake)

#Define o nome do projeto como projeto_residencia
project(projeto_residencia C CXX ASM)
pico_sdk_init()

add_executable(${PROJECT_NAME}
        projeto_residencia.c # Código principal em C 
        lib/ssd1306.c # Biblioteca para o display OLED
        )

pico_set_program_name(${PROJECT_NAME} "projeto_residencia.c")
pico_set_program_version(${PROJECT_NAME} "0.1")

# Link com as bibliotecas necessárias
target_link_libraries(${PROJECT_NAME} 
        pico_stdlib 
        hardware_i2c 
        hardware_adc
        hardware_gpio
        hardware_pio
        hardware_clocks 
        pico_bootsel_via_double_reset 
        pico_bootrom
        )

# Gerar cabeçalhos PIO
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)        

# Adicione o diretório atual aos caminhos de inclusão
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

pico_add_extra_outputs(${PROJECT_NAME})

