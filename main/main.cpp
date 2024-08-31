/**
 * @file main.cpp
 * @brief Foo-Bar is a variation of the classic Fizz-Buzz test given in coding interviews; the basic
 * functionality is easy to achieve, but a robust solution with good modularity and accurate timing
 * will score better than a solution without these features.
 */


/******************************************************************************/
/*                              I N C L U D E S                               */
/******************************************************************************/


#include <iostream>
#include <cstring>
#include <cstdlib>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

/******************************************************************************/
/*                     F U N C T I O N  P R O T O T Y P E S                   */
/******************************************************************************/

void bar_task(void *pvParameter);
void foo_task(void *pvParameter);
void idleTask(void *pvParameter);
bool is_prime(uint32_t n);

extern "C" {
    void app_main(void);
}


/******************************************************************************/
/*                               D E F I N E S                                */
/******************************************************************************/

#define EVENT_FLAG_1    (1 << 0)
#define BLINK_GPIO CONFIG_BLINK_GPIO
#define BUF_SIZE (1024 * 2)
#define MAX_NUM 16777216
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define QUEUE_LENGTH 8
#define ITEM_SIZE sizeof(uint32_t)

/******************************************************************************/
/*                              T Y P E D E F S                               */
/******************************************************************************/

QueueHandle_t xQueue;
TaskHandle_t idleTaskHandle, fooTaskHandle, barTaskHandle;
static QueueHandle_t uart_queue;
EventGroupHandle_t eventGroup;
SemaphoreHandle_t xSemaphore = NULL;


/******************************************************************************/
/*                         G L O B A L  V A R I A B L E S                     */
/******************************************************************************/

uint32_t rxbuf[256];
char message[64];

/******************************************************************************/
/*                           F U N C T I O N S                                */
/******************************************************************************/


// =============================================================================
// is_prime
// =============================================================================

/**
 * The is_prime function checks whether a given unsigned integer n is a prime number
 *
 * @param uint32_t number
 * @return True or False based on if the input was a prime number or not
 */


bool is_prime(uint32_t n) {
    if (n <= 1) return false;
    for (uint32_t i = 2; i * i <= n; ++i) {
        if (n % i == 0) return false;
    }
    return true;
}


// =============================================================================
// idleTask
// =============================================================================

/**
 * @brief [idleTask] idleTask serves as the central coordinator in this application, 
 * managing the flow of numbers from the UART to the countdown logic, 
 * ensuring numbers are processed in the order they're received, 
 * and maintaining system responsiveness through effective task synchronization and CPU load balancing.
 *
 * @note Within its main loop, idleTask checks for new numbers in the queue (xQueue). 
 * If a new number is detected, it sets an event flag to start processing, retrieves the number,
 * and resets the semaphore with the new count. It then waits for 1 second before repeating the process, 
 * ensuring tasks for "Foo" or "Bar" output are triggered accordingly. 
 * This loop allows the system to sequentially process each received number, restarting with zero or a new number as needed, 
 * while balancing the workload across the ESP32's cores for efficient operation.
 */

void idleTask(void *pvParameter) {
    xTaskCreatePinnedToCore(&foo_task, "foo_task", 2048, NULL, 5, &fooTaskHandle, 0);
    xTaskCreatePinnedToCore(&bar_task, "bar_task", 2048, NULL, 5, &barTaskHandle, 1);
    
    uint32_t uReceivedValue;
    xSemaphore = xSemaphoreCreateCounting(MAX_NUM, 0);
    eventGroup = xEventGroupCreate();
    uart_write_bytes(UART_NUM_2, "Enter the number : \n", 22);

    
    while (true) {
        if (!(xEventGroupGetBits(eventGroup) & EVENT_FLAG_1)) {
            if (uxQueueSpacesAvailable(xQueue) < 8) {
                xEventGroupSetBits(eventGroup, EVENT_FLAG_1);
                xQueueReceive(xQueue, &uReceivedValue, 0);
                if (xSemaphore != NULL) {
                    vSemaphoreDelete(xSemaphore);
                }
                xSemaphore = xSemaphoreCreateCounting(MAX_NUM, uReceivedValue);
            }
            
            std::cout << "IDLE = " << uxQueueSpacesAvailable(xQueue) << std::endl;
            
            vTaskDelay(1000 / portTICK_PERIOD_MS);

        } else {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}


// =============================================================================
// foo_task
// =============================================================================

/**
 * @brief [foo_task] The foo_task function is designed to run indefinitely on one of the cores of an ESP32, 
 * as part of a multitasking environment managed by FreeRTOS. 
 * Its primary purpose is to handle the "Foo" part of the "Foo-Bar" problem, 
 * specifically dealing with even numbers in a countdown sequence.
 * 
 * @note The foo_task function continuously checks if the current count c, 
 * obtained from a semaphore, is even. If so, it waits for 1 second, 
 * then prints "Foo" alongside the count. If the count is also prime, 
 * it appends "Prime" to the output. After displaying, if c is not zero, 
 * it decrements the semaphore count, allowing the task to proceed with the next number. 
 * If c is zero and the event flag is set, indicating the start of a new sequence, 
 * it clears the event flag and waits again. This cycle repeats indefinitely, handling even-numbered counts in the sequence.
 */



void foo_task(void *pvParameter) {
    uint32_t c = 0; 
    int len;
    while(1) {
        c = uxSemaphoreGetCount(xSemaphore);
        if(xEventGroupGetBits(eventGroup) & EVENT_FLAG_1) {
            if (c % 2 == 0) {
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                std::cout << "Foo " << c;
                if(is_prime(c)) {
                    std::cout << " Prime ";
                }
                std::cout << std::endl;
                if (is_prime(c)) {
                    len = snprintf(message, sizeof(message), "Foo %lu Prime\n", c);
                } else {
                    len = snprintf(message, sizeof(message), "Foo %lu\n", c);
                }
                uart_write_bytes(UART_NUM_2, message, len);
                
                if(c != 0) {
                    xSemaphoreTake(xSemaphore, portMAX_DELAY);
                } else {
                    if(xEventGroupGetBits(eventGroup) & EVENT_FLAG_1) {
                        xEventGroupClearBits(eventGroup, EVENT_FLAG_1);
                    }
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                }
            }
        } else {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

// =============================================================================
// bar_task
// =============================================================================


/**
 * @brief [bar_task] The bar_task function is designed to run indefinitely on one of the cores of an ESP32, 
 * as part of a multitasking environment managed by FreeRTOS. 
 * Its primary purpose is to handle the "Bar" part of the "Foo-Bar" problem, 
 * specifically dealing with odd numbers in a countdown sequence.
 * 
 * @note The foo_task function continuously checks if the current count c, 
 * obtained from a semaphore, is odd. If so, it waits for 1 second, 
 * then prints "Foo" alongside the count. If the count is also prime, 
 * it appends "Prime" to the output. This cycle repeats indefinitely, handling odd-numbered counts in the sequence.
 */

void bar_task(void *pvParameter) {
    uint32_t c = 0;
    int len;
    while(1) {        
        c = uxSemaphoreGetCount(xSemaphore);
        if(xEventGroupGetBits(eventGroup) & EVENT_FLAG_1) {
            if (c % 2 == 1) {
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                std::cout << "Bar " << c;
                xSemaphoreTake(xSemaphore, portMAX_DELAY);
                if(is_prime(c)) {
                    std::cout << " Prime ";
                }
                std::cout << std::endl;

                if (is_prime(c)) {
                    len = snprintf(message, sizeof(message), "Bar %lu Prime\n", c);
                } else {
                    len = snprintf(message, sizeof(message), "Bar %lu\n", c);
                }
                uart_write_bytes(UART_NUM_2, message, len);
            }        
        } else {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

// =============================================================================
// uart_event_task
// =============================================================================

/**
 * @brief [uart_event_task] The bar_task function is designed to run indefinitely on one of the cores of an ESP32, 
 * as part of a multitasking environment managed by FreeRTOS. 
 * Its primary purpose is to handle the "Bar" part of the "Foo-Bar" problem, 
 * specifically dealing with odd numbers in a countdown sequence.
 * 
 * 
 * @note The uart_event_task function continuously monitors UART events on an ESP32, processing incoming data as numbers:
 * Initialization: Sets up a buffer data to hold incoming UART data and variables for event handling and number storage.
 * Infinite Loop: Perpetually waits for UART events. When data arrives, it clears the buffer and reads the bytes.
 * Data Conversion: Converts the received data to an integer number. If the queue is not full and number is non-zero, 
 * it queues the number for further processing and outputs a confirmation message.
 * Special Case: If 0 is received, the ESP32 restarts, acting as a remote reset mechanism.
 * This task serves as the communication interface, handling data reception and facilitating system control via UART.
 */

void uart_event_task(void *pvParameters) {
    uart_event_t event;
    uint32_t number;
    int len;
    auto* data = new uint32_t[BUF_SIZE / sizeof(uint32_t)]; // Adjusted for C++

    for (;;) {
        if (xQueueReceive(uart_queue, &event, portMAX_DELAY)) {
            memset(data, 0, BUF_SIZE);
            switch (event.type) {
                case UART_DATA:
                    uart_read_bytes(UART_NUM_2, data, event.size, portMAX_DELAY);
                    number = std::atoi(reinterpret_cast<char*>(data));
                    if (number != 0) {
                        if (uxQueueSpacesAvailable(xQueue) == 0) {
                            // Handle queue full scenario
                            uart_write_bytes(UART_NUM_2, "Currently full\n", 16);
                            
                        } else {
                            if (xQueueSend(xQueue, &number, 0) == pdPASS) {
                                std::cout << "Received " << number << std::endl;

                                len = snprintf(message, sizeof(message), "Received %lu\n", number);
                                uart_write_bytes(UART_NUM_2, message, len);
                            }
                        }
                    } else {
                        esp_restart();
                    }
                    break;
                default:
                    break;
            }
        }
    }

    delete[] data;
    vTaskDelete(NULL);
}



// =============================================================================
// app_main
// =============================================================================

/**
 * @brief [app_main] The app_main function serves as the entry point for applications running on the ESP32 
 * when using the ESP-IDF framework. It's responsible for initializing the application, 
 * setting up hardware configurations, and starting the FreeRTOS tasks that comprise the application's functionality. 
 * 
 * 
 */


void app_main(void)
{
    
    xQueue = xQueueCreate(QUEUE_LENGTH, ITEM_SIZE);

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_2, BUF_SIZE * 2, BUF_SIZE * 2, 10, &uart_queue, 0);

    xTaskCreatePinnedToCore(&idleTask, "idle", 2048, NULL, 24, &idleTaskHandle, 0);
    xTaskCreate(&uart_event_task, "uart_event_task", 2048, NULL, 12, NULL); 
}