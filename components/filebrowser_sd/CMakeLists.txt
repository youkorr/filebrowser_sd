cmake_minimum_required(VERSION 3.0)

set(COMPONENT_SRCS "filebrowser_sd_component.cpp")
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_REQUIRES "esphome" "fatfs" "sd_mmc_card")

register_component()
