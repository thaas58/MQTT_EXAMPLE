/*
 * aht20.c
 *
 *  Created on: May 19, 2022
 *      Author: PickleRix - Alien Firmware Engineer
 *
 * Code to support the AHT20 temperature/humidity sensor.
 *
 * I personally use this code to monitor the climate control in
 * my spacecraft while traveling around Uranus, capturing Klingons.
 *
 */

#include "aht20.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "mqtt_example.h"

#define cmdMAX_MUTEX_WAIT pdMS_TO_TICKS( 300 )

static float temperature_store = 0;
static float humidity_store = 0;
static SemaphoreHandle_t storeMutex = NULL;

static bool set_aht20_values(float humidity, float temperature)
{
	bool status = false;

	if(xSemaphoreTake( storeMutex, cmdMAX_MUTEX_WAIT ) == pdPASS)
	{
		humidity_store = humidity;
		temperature_store = temperature;
		/* Must ensure to give the mutex back. */
		xSemaphoreGive( storeMutex );
		status = true;
	}
	return(status);
}

bool get_aht20_values(float* humidity, float* temperature)
{
	bool status = false;

	if( xSemaphoreTake( storeMutex, cmdMAX_MUTEX_WAIT ) == pdPASS )
	{
		*humidity = humidity_store;
		*temperature = temperature_store;
		/* Must ensure to give the mutex back. */
		xSemaphoreGive( storeMutex );
		status = true;
	}
	return(status);
}

bool aht20_i2c_init(void)
{
	//HAL_StatusTypeDef hal_status = HAL_ERROR;
	uint8_t command[3];
	uint8_t status = 0;
	/* Create the semaphore used to access stored humidity and temperature values. */
	storeMutex = xSemaphoreCreateMutex();
	configASSERT(storeMutex);

	/* Create the semaphore used to access stored humidity and temperature values. */
	//Delay for 40 milliseconds
	osDelay(40);
	while ((status = get_aht20_status()) & AHT20_STATUS_BUSY)
	{
		osDelay(10);
	}

	if(!(status & AHT20_STATUS_CALIBRATED))
	{
		osDelay(20);
		command[0] = AHT20_CMD_CALIBRATE;
		command[1] = 0x08;
		command[2] = 0x00;
		i2c_write_blocking(i2c_default, AHT20_ADDR, command, 1, false);
	}

	while ((status = get_aht20_status()) & AHT20_STATUS_BUSY)
	{
		osDelay(10);
	}

	if(status == AHT20_STATUS_ERROR)
	{
		return false;
	}

	if(!(status & AHT20_STATUS_CALIBRATED))
	{
		return false;
	}

	return true;
}

void send_aht20_data(uint8_t* command_buffer, size_t size)
{
	i2c_write_blocking(i2c_default, AHT20_ADDR, command_buffer, size, false);
}

void get_aht20_data(uint8_t* data_buffer, size_t size)
{
	i2c_read_blocking(i2c_default, AHT20_ADDR, data_buffer, size, false); 
}

uint8_t get_aht20_status(void)
{
	uint8_t status = 0;
	uint8_t buffer[1];

	buffer[0] = AHT20_STATUS_CMD;
	i2c_write_blocking(i2c_default, AHT20_ADDR, buffer, 1, false);
	get_aht20_data(buffer, 1);
	status = buffer[0];

	return(status);
}

bool read_aht20_values(float* humidity, float* temperature)
{
	float _humidity = 0;
	float _temperature = 0;
	uint8_t command[3];
	uint8_t data[6];

	command[0] = AHT20_CMD_TRIGGER;
	command[1] = 0x33;
	command[2] = 0x00;
	send_aht20_data(command, 3);
	// AHT20 datasheet says to wait at least 80 milliseconds for measurement.
	osDelay(80);
	while(get_aht20_status() & AHT20_STATUS_BUSY)
	{
		osDelay(10);
	}

	get_aht20_data(data, 6);

	// Extract and calculate humidity percentage
	uint32_t h_data = data[1];
	h_data <<= 8;
	h_data |= data[2];
	h_data <<= 4;
	h_data |= data[3] >> 4;
	_humidity = ((float)h_data * 100) / 0x100000;

	// Extract and calculate temperature
	uint32_t t_data = data[3] & 0x0F;
	t_data <<= 8;
	t_data |= data[4];
	t_data <<= 8;
	t_data |= data[5];
	_temperature = (((float)t_data * 200) / 0x100000) - 50;

	if(humidity != NULL)
	{
		*humidity = _humidity;
	}

	if(temperature != NULL)
	{
		*temperature = _temperature;
	}

	set_aht20_values(_humidity, _temperature);
	return true;
}
