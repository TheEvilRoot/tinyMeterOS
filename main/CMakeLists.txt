file(GLOB_RECURSE SOURCE_FILES *.cc)

idf_component_register(
        SRCS ${SOURCE_FILES}
        INCLUDE_DIRS "."
        REQUIRES
            driver
            esp_lcd
            lvgl
            esp_lvgl_port
)
