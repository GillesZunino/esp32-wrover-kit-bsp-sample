# -----------------------------------------------------------------------------------
# Copyright 2026, Gilles Zunino
# -----------------------------------------------------------------------------------

set(srcs
    "pretty_effect.c"
    "decode_image.c"
    "main.c"
)

idf_component_register(SRCS ${srcs}
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_spi esp_driver_gpio esp_wrover_kit_bsp esp_timer
    EMBED_FILES image.jpg
)
