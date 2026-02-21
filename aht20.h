/*
 * aht20.h
 *
 *  Created on: May 19, 2022
 *      Author: PickleRix - Alien Firmware Engineer
 */

#ifndef INC_AHT20_H_
#define INC_AHT20_H_

#ifdef __cplusplus
extern "C" {
#endif

//#include "main.h"
//#include "cmsis_os.h"
#include "stdbool.h"
#include "stdint.h"

#ifndef PI_PICO_SUPPORT
#define PI_PICO_SUPPORT
#endif

#define AHT20_TIMEOUT		        100

#define AHT20_ADDR  			    0x38 // Use 7-bit address

// AHT20 Commands
#define AHT20_STATUS_CMD 		    0x71 // Status command
#define AHT20_CMD_SOFTRESET 	    0xBA // Soft reset command
#define AHT20_CMD_CALIBRATE 	    0xBE // Calibration command
#define AHT20_CMD_TRIGGER 		    0xAC // Trigger reading command

// Status bits
#define AHT20_STATUS_BUSY 		    0x80 // Busy Status bit
#define AHT20_STATUS_CALIBRATED     0x08 // Calibrated Status bit
#define AHT20_STATUS_ERROR		    ~(AHT20_STATUS_BUSY|AHT20_STATUS_CALIBRATED)

bool Get_Values(float* humidity, float* temperature);
bool aht20_i2c_init(void);
bool get_aht20_values(float* humidity, float* temperature);
uint8_t get_aht20_status(void);
#ifdef __cplusplus
}
#endif

#endif /* INC_AHT20_H_ */
