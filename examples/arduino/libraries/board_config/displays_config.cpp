#include "displays_config.h"

int set_display_backlight(DEV_I2C_Port port, uint8_t brightness)
{
    DEV_I2C_Set_Slave_Addr(&port.dev, display_cfg.i2c_address);
    if (brightness > 0xFF)
    {
        return -1;
    }

    DEV_I2C_Write_Byte(port.dev, 0x96, brightness);
    return 0;
}

bool display_init(DEV_I2C_Port port)
{
    DEV_I2C_Set_Slave_Addr(&port.dev, display_cfg.i2c_address);

    for (size_t i = 0; i < display_cfg.i2c_init_seq_size; i++)
    {
        DEV_I2C_Write_Byte(port.dev,
                           display_cfg.i2c_init_seq[i][0],
                           display_cfg.i2c_init_seq[i][1]);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    set_display_backlight(port, 255);

    return true;
}
