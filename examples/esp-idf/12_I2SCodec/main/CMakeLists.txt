idf_component_register(SRCS "i2s_es8311_example.c"
                    PRIV_REQUIRES esp_driver_i2s esp_driver_gpio esp_driver_i2c esp32_p4_platform esp_codec_dev
                    EMBED_FILES "canon.pcm")
