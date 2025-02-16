cmake_minimum_required(VERSION 3.13)

# Nome do projeto
project(pico-ssd1306)

# Adicionar os arquivos da biblioteca
add_library(pico-ssd1306
    ssd1306.c  # Arquivo fonte principal
)

# Incluir os diretÃ³rios de cabeÃ§alhos
target_include_directories(pico-ssd1306 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR} 
    )

# DependÃªncias da biblioteca
target_link_libraries(pico-ssd1306
    pico_stdlib    # Biblioteca padrÃ£o do Pico SDK
    hardware_i2c   # Biblioteca para suporte ao I2C
)