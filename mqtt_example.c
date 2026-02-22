/*
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Dirk Ziegelmeier <dziegel@gmx.de>
 * Modified by: Terry T. Haas and PickleRix, alien firmware engineer
 *
 */
#include "pico/cyw43_arch.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

#include "hardware/i2c.h"
#include "aht20.h"


#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/apps/lwiperf.h"

#ifndef USE_LED
#define USE_LED 1
#endif
#include "lwip/apps/mqtt.h"
#include "mqtt_example.h"
 
//u16_t mqtt_port = 9883;
u16_t mqtt_port = 1883;
#if LWIP_TCP

/** Define this to a compile-time IP address initialization
 * to connect anything else than IPv4 loopback
 */
#ifndef LWIP_MQTT_EXAMPLE_IPADDR_INIT
#if LWIP_IPV4
/*192.168.1.166 0xc0a800a6 LWIP_MQTT_EXAMPLE_IPADDR_INIT */
#define LWIP_MQTT_EXAMPLE_IPADDR_INIT = IPADDR4_INIT(PP_HTONL(0xc0a801a6))
#else
#define LWIP_MQTT_EXAMPLE_IPADDR_INIT
#endif
#endif

#define TEST_TASK_PRIORITY  (tskIDLE_PRIORITY + 1UL)

static ip_addr_t mqtt_ip LWIP_MQTT_EXAMPLE_IPADDR_INIT;
static mqtt_client_t* mqtt_client;

static const struct mqtt_connect_client_info_t mqtt_client_info =
{
  "pico_w_test1",
  USER_NAME, /* user */
  USER_PW, /* password */
  100,  /* keep alive */
  "topic/will", /* will_topic */
  "pico_w_test1 disconnected!", /* will_msg */
  0,    /* will_qos */
  0     /* will_retain */
#if LWIP_ALTCP && LWIP_ALTCP_TLS
  , NULL
#endif
};

/**
* @brief   Wait for Timeout (Time Delay)
* @param   millisec      time delay value
*/
void osDelay (uint32_t millisec)
{
  TickType_t ticks = millisec / portTICK_PERIOD_MS;

  vTaskDelay(ticks ? ticks : 1);          /* Minimum delay = 1 tick */
}

static void
mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
  LWIP_UNUSED_ARG(data);

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" data cb: len %d, flags %d\n", client_info->client_id,
          (int)len, (int)flags));
}

static void
mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" publish cb: topic %s, len %d\n", client_info->client_id,
          topic, (int)tot_len));
}

static void
mqtt_request_cb(void *arg, err_t err)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" request cb: err %d\n", client_info->client_id, (int)err));
}

static void
mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
  const struct mqtt_connect_client_info_t* client_info = (const struct mqtt_connect_client_info_t*)arg;
  LWIP_UNUSED_ARG(client);

  LWIP_PLATFORM_DIAG(("MQTT client \"%s\" connection cb: status %d\n", client_info->client_id, (int)status));

  if (status == MQTT_CONNECT_ACCEPTED) {
    LWIP_PLATFORM_DIAG(("MQTT client \"%s\"  MQTT Connect Accepted!\n", client_info->client_id));
    mqtt_sub_unsub(client,
                   "topic_qos1", 1,
                   mqtt_request_cb, LWIP_CONST_CAST(void *, client_info),
                   1);
    mqtt_sub_unsub(client,
                   "topic_qos0", 0,
                   mqtt_request_cb, LWIP_CONST_CAST(void *, client_info),
                   1);

    mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb,
                            mqtt_incoming_data_cb, LWIP_CONST_CAST(void *, client_info));
    mqtt_subscribe(client,
                   "topic/TEST_PUB", 0,
                   mqtt_request_cb, LWIP_CONST_CAST(void *, client_info));
  }
}
#endif /* LWIP_TCP */

void
mqtt_example_init(void)
{
#if LWIP_TCP
  mqtt_client = mqtt_client_new();
  printf("mqtt_client 0x%x &mqtt_client 0x%x \n", mqtt_client,&mqtt_client);	
  
  mqtt_set_inpub_callback(mqtt_client,
          mqtt_incoming_publish_cb,
          mqtt_incoming_data_cb,
          LWIP_CONST_CAST(void*, &mqtt_client_info));
  printf("mqtt_set_inpub_callback 0x%x\n",mqtt_set_inpub_callback);
  
  mqtt_client_connect(mqtt_client,
          &mqtt_ip, mqtt_port,
          mqtt_connection_cb, LWIP_CONST_CAST(void*, &mqtt_client_info),
          &mqtt_client_info);
  printf("mqtt_client_connect 0x%x\n",mqtt_client_connect);
          
#endif /* LWIP_TCP */
}

void main_task(__unused void *params)
{
#if (!defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN))
    #warning i2c a board with I2C pins
    puts("Default I2C pins were not defined");
    return;
#else
    // I2C is "open drain", pull ups to keep signal high when no data is being sent
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    if(aht20_i2c_init())
    {
        read_aht20_values(NULL, NULL);
        //printf("First read - humidity: %5.2f%%, temperature: %5.2f\n", humidity, temperature);
    }
    else
    {
        printf("I2C failed to initialize\n");
        return;
    }

    if (cyw43_arch_init()) {
        printf("failed to initialize\n");
        return;
    }

    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("failed to connect.\n");
        exit(1);
    } else
    {
        printf("Connected.\n");
        printf("mqtt_port = %d &mqtt_port 0x%x\n", mqtt_port, &mqtt_port);
        printf("mqtt_ip = 0x%x &mqtt_ip = 0x%x\n", mqtt_ip, &mqtt_ip);
        printf("IPADDR_LOOPBACK = 0x%x \n", IPADDR_LOOPBACK);
        mqtt_example_init();
    }
#endif

    static uint32_t counter = 1;
    static int led_on = true;
    float humidity;
    float temperature;
    while(true)
    {
        led_on = !led_on;
        cyw43_gpio_set(&cyw43_state, 0, led_on);
        // Check we can read back the led value
        bool actual_led_val = !led_on;
        cyw43_gpio_get(&cyw43_state, 0, &actual_led_val);
        assert(led_on == actual_led_val);
        if((counter % 5) == 0)
        {
            //Read and store humidity and temperature values
            read_aht20_values(NULL, NULL);
        }

        if((counter % 10) == 0)
        {
            char buffer[80];
            sprintf(buffer, "%u", counter);
            u8_t qos    = 2;
            u8_t retain = 0;
            mqtt_publish(mqtt_client, "topic/pico_w_test/counter", buffer, strlen(buffer), qos, retain, mqtt_request_cb, (void *)&mqtt_client_info);
            // Get stored AHT20 values
            get_aht20_values(&humidity, &temperature);
            sprintf(buffer, "\nhumidity: %5.2f%%, temperature: %5.2f \u00B0C\n", humidity, temperature);
            printf("%s", buffer);
            mqtt_publish(mqtt_client, "topic/pico_w_test/humidity_temperature", buffer, strlen(buffer), qos, retain, mqtt_request_cb, (void *)&mqtt_client_info);
        }
        // not much to do as LED is in another task, and we're using RAW (callback) lwIP API
        osDelay(1000);
        counter++;
    }
    cyw43_arch_deinit();
}

void vLaunch( void) {
    TaskHandle_t task;
    xTaskCreate(main_task, "MainTaskThread", configMINIMAL_STACK_SIZE, NULL, TEST_TASK_PRIORITY, &task);

#if NO_SYS && configUSE_CORE_AFFINITY && configNUMBER_OF_CORES > 1
    // we must bind the main task to one core (well at least while the init is called)
    // (note we only do this in NO_SYS mode, because cyw43_arch_freertos
    // takes care of it otherwise)
    vTaskCoreAffinitySet(task, 1);
#endif

    /* Start the tasks and timer running. */
    vTaskStartScheduler();
}

int main() {
    const char *rtos_name;
    stdio_init_all();

#if ( configNUMBER_OF_CORES > 1 )
    rtos_name = "FreeRTOS SMP";
#else
    rtos_name = "FreeRTOS";
#endif

#if ( configNUMBER_OF_CORES == 2 )
    printf("Starting %s on both cores:\n", rtos_name);
    vLaunch();
#elif ( RUN_FREERTOS_ON_CORE == 1 )
    printf("Starting %s on core 1:\n", rtos_name);
    multicore_launch_core1(vLaunch);
    while (true);
#else
    printf("Starting %s on core 0:\n", rtos_name);
    vLaunch();
#endif
    return 0;
}
