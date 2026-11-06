# Projeto ESP-IDF usado SO para gerar o bootloader do Orbita.
# O app "main" daqui e vazio e nao e usado.
#
#   cd bootloader
#   idf.py set-target esp32s3     (so na primeira vez)
#   idf.py bootloader
#   -> build/bootloader/bootloader.bin
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(orbita_bootloader)
