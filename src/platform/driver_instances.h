// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#ifndef DRIVER_INSTANCES_H_
#define DRIVER_INSTANCES_H_

#include "rtos/drivers/gpio/api/rtos_gpio.h"
#include "rtos/drivers/i2c/api/rtos_i2c_master.h"
#include "rtos/drivers/i2s/api/rtos_i2s.h"
#include "rtos/drivers/intertile/api/rtos_intertile.h"
#include "rtos/drivers/qspi_flash/api/rtos_qspi_flash.h"

#define FLASH_TILE_NO     0
#define I2C_TILE_NO       0
#define AUDIO_HW_TILE_NO  1

extern rtos_intertile_t *intertile_ctx;
extern rtos_gpio_t *gpio_ctx_t0;
extern rtos_gpio_t *gpio_ctx_t1;
extern rtos_qspi_flash_t *qspi_flash_ctx;
extern rtos_i2c_master_t *i2c_master_ctx;
extern rtos_i2s_t *i2s_ctx;

#endif /* DRIVER_INSTANCES_H_ */
