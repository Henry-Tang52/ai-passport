idf_component_register(
    SRCS "main.c"
         "time_track_model.c"
         "time_track_store.c"
         "time_track_ui.c"
    INCLUDE_DIRS "."
    REQUIRES bsp nvs_flash esp_timer
)

target_sources(${COMPONENT_LIB} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/time_track_font_16.c"
)
