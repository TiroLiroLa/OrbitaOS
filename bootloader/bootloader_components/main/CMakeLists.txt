idf_component_register(SRCS "bootloader_start.c"
                       REQUIRES bootloader bootloader_support)

idf_build_get_property(target IDF_TARGET)
# Usa os scripts de linker do bootloader original (igual ao exemplo
# examples/custom_bootloader/bootloader_override do ESP-IDF)
set(scripts "${IDF_PATH}/components/bootloader/subproject/main/ld/${target}/bootloader.ld"
            "${IDF_PATH}/components/bootloader/subproject/main/ld/${target}/bootloader.rom.ld")
target_linker_script(${COMPONENT_LIB} INTERFACE "${scripts}")
