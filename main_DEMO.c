#include <stdio.h>
#include "os.h"
#include "utils/serial.h"
#include "roundRobin.h"
#include "sleep.h"
#include "mutex.h"
#include "semaphore.h"
#include "queue.h"
#include "mempool.h"

/**
 *  This file contains the demonstration code that shows the created OS' features
 *   in a semi-probable use-case.
 *  No hardware peripherals are used in this demo (apart from serial),
 *   and sensors are 'imaginary' for the demo.
 */

/*=============================================================================
**      Definitions
=============================================================================*/
#define NUMBER_OF_SENSORS 2
#define SENSOR_1_FREQUENCY 50
#define SENSOR_1_NUMBER_OF_AVERAGES 100

#define SENSOR_QUEUE_SIZE 4
#define SENSOR_PACKET_MEMORY_POOL_SIZE (2 * NUMBER_OF_SENSORS)
#define SENSOR_PACKET_DATA_LENGTH 3

/*=============================================================================
**      Structure Definitions and Enumerations
=============================================================================*/
/* A sensor packet of 16 bytes that can be utilised by any sensor.
    The structure is big enough for multiple sensors to utilise
    it in various ways. */
typedef struct {
    /* The sensor ID */
    uint32_t id;
    /* Field for various data from sensor */
    uint32_t data[SENSOR_PACKET_DATA_LENGTH];
} SensorPacket_t;

/*  Enum to determine sensor type */
enum DEMO_SENSOR_ID_e {
	TEMPERATURE=0x00,
    ACCELEROMETER,
    LIGHT
};


/*=============================================================================
**      Task Function Prototypes
=============================================================================*/
void task_sensor_1(void const * const args);
void task_sensor_2(void const * const args);
void task_sensor_3(void const * const args);
void task_compile_transmit_sens_2_3(void const * const args);
void task_compile_print_sens_1(void const * const args);
void task_low_pri_print(void const * const args);


/*=============================================================================
**      Global Variables , including mutexes, semaphores, queues, mempools, etc.
=============================================================================*/
/* Queues that the sensors can rapidly transmit readings over to other tasks */
static OS_Queue_t   queue_sensor_1,
                    queue_sensor_2_3;

/* A memory pool for sensor packet blocks to be allocated and deallocated when needed */
static OS_MemPool_t mempool_sensor_packet;


static OS_Mutex_t serial_mutex;


/*=============================================================================
**      Main Function
=============================================================================*/
int main(void) {
	/* Initialise the serial port for printf() */
	serial_init();
    puts("\n\n\rDocetOS Demo\r");


    /* Reserve memory for stacks and TCBs. Stacks must be 8-byte aligned. */
    __align(8)
    static uint32_t stack_sensor_1[64],
                    stack_sensor_2[64],
                    stack_sensor_3[64],
                    stack_low_pri[64],
                    stack_compile_transmit_1[64],
                    stack_compile_transmit_2_3[64];

	static OS_TCB_t tcb_sensor_1,
                    tcb_sensor_2,
                    tcb_sensor_3,
                    tcb_low_pri,
                    tcb_compile_transmit_1,
                    tcb_compile_transmit_2_3;

	/* Initialise TCBs */
	OS_initialiseTCB(&tcb_sensor_1, stack_sensor_1 + 64, task_sensor_1, PRIORITY_MAX, NULL);
	OS_initialiseTCB(&tcb_sensor_2, stack_sensor_2 + 64, task_sensor_2, PRIORITY_MAX-1, NULL);
    OS_initialiseTCB(&tcb_sensor_3, stack_sensor_3 + 64, task_sensor_3, PRIORITY_MAX-1, NULL);
    OS_initialiseTCB(&tcb_compile_transmit_1, stack_compile_transmit_1 + 64, task_compile_print_sens_1, PRIORITY_MAX-2, NULL);
    OS_initialiseTCB(&tcb_compile_transmit_2_3, stack_compile_transmit_2_3 + 64, task_compile_transmit_sens_2_3, PRIORITY_MAX-2, NULL);
    OS_initialiseTCB(&tcb_low_pri, stack_low_pri + 64, task_low_pri_print, PRIORITY_MAX-3, NULL);


	/* Initialise the scheduler */
	OS_init(&round_robin_scheduler);

    /* Initialise Mutex for serial port print access */
    OS_mutexInitialise(&serial_mutex);

    /* Word aligned static memory for the queues - word alignment reqired by the OS  */
    __align(4)
    static SensorPacket_t   queue_store_sensor_1[SENSOR_QUEUE_SIZE];
    static uint32_t queue_store_sensor_2_3[SENSOR_QUEUE_SIZE];

    /* Initialise queues for sharing sensor data */
    OS_queueInitialise(&queue_sensor_1, &queue_store_sensor_1, SENSOR_QUEUE_SIZE, sizeof(SensorPacket_t));
    OS_queueInitialise(&queue_sensor_2_3, &queue_store_sensor_2_3, SENSOR_QUEUE_SIZE, sizeof(uint32_t));

    /* Memory for sensor and compile packet memory pools */
    static SensorPacket_t mempool_sensor_packets_mem[SENSOR_PACKET_MEMORY_POOL_SIZE];
    /* Initialise memory pools */
    OS_memPoolInitialise(&mempool_sensor_packet, &mempool_sensor_packets_mem , SENSOR_PACKET_MEMORY_POOL_SIZE, sizeof(SensorPacket_t));

    /* Add tasks to the scheduler */
	OS_addTask(&tcb_sensor_1);
	OS_addTask(&tcb_sensor_2);
    OS_addTask(&tcb_sensor_3);
    OS_addTask(&tcb_low_pri);
    OS_addTask(&tcb_compile_transmit_1);
    OS_addTask(&tcb_compile_transmit_2_3);

    /* Finally start the OS */
	OS_start();
}

/*=============================================================================
**      Functions
=============================================================================*/
/**
 * [task_sensor_1 Sends "sensor" data over queue_sensor_1 to
 *   task_compile_print_sens_1 every 1/SENSOR_1_FREQUENCY ms.
 *  Utilises a memory pool for allocation of packets.]
 * @param args [NA]
 */
void task_sensor_1(void const * const args) {
    /*  A pointer to a packet, memory to be allocated from a memory pool */
    SensorPacket_t * packet;
    uint32_t sensor_data_counter = 0;
    while(1) {
        /* Allocate memory for the senor packet */
        packet = OS_memPoolAllocate(&mempool_sensor_packet);
        /* Fill the packet with data from peripheral */
        packet->id = ACCELEROMETER;
        for(uint_fast8_t i = 1; i <=SENSOR_PACKET_DATA_LENGTH; i++) {
            /* "Read" Sensor peripheral */
            packet->data[i-1] = i * sensor_data_counter++;
        }
        /* Send the pointer to the allocated block via the queue -
            it will be deallocated by the receiving task */
        OS_queueEnqueue(&queue_sensor_1, &packet);
        OS_sleep(1000 / SENSOR_1_FREQUENCY);
    }
}


/**
 * [task_compile_print_sens_1 Dequeues, compiles, averages and prints the
 *  received data from task_sensor_1 over queue_sensor_1. After use it
 *  deallocates the sensor packets so they can be reused by the sensor.]
 * @param args [NA]
 */
void task_compile_print_sens_1(void const * const args) {
    /*  A pointer to a packet to be received from queue and deallocated
        to a memory pool after data processing, as well as the new compiled packet
        to create and send further */
    SensorPacket_t * packet_r;
    uint32_t num_averages, sensor_id;
    while(1) {
        uint32_t data_average[SENSOR_PACKET_DATA_LENGTH] = {0};
        /* Retrieve a packet from the queue */
        num_averages = 0;
        /* For a set number of averages, deqeue sensor data and add to avarages,
            then deallocate the memory block so it can be reused by the sensor */
        while (num_averages <= SENSOR_1_NUMBER_OF_AVERAGES) {
            OS_queueDequeue(&queue_sensor_1, &packet_r);
            sensor_id = packet_r->id;
            for (uint_fast8_t i = 1; i <=SENSOR_PACKET_DATA_LENGTH; i++) {
                /* "Store" Sensor data for processing */
                data_average[i-1] += packet_r->data[i-1];
            }
            /* Deallocate the packet once its finished. */
            OS_memPoolDeallocate(&mempool_sensor_packet, packet_r);
            num_averages++;
        }
        for (uint_fast8_t i = 1; i <=SENSOR_PACKET_DATA_LENGTH; i++) {
            /* Find the average */
            data_average[i-1] /= num_averages;
        }
        OS_mutexAcquire(&serial_mutex);
        printf("Sensor %d Transmitted: \tTime: %d, \tD1: %d, \tD2: %d, \tD3: %d\n\r", sensor_id,
                OS_elapsedTicks(), data_average[0], data_average[1], data_average[2]);
        OS_mutexRelease(&serial_mutex);
    }
}


/**
 * [task_sensor_2 Sends very simple and unchanging uint32_t data over a second
 *   queue, queue_sensor_2_3, to task_compile_transmit_sens_2_3.
 *  No allocation or deallocation required. ]
 * @param args [NA]
 */
void task_sensor_2(void const * const args) {
    /*  A pointer to a packet, memory to be allocated from a memory pool */
    uint32_t packet;
    while(1) {
        /* Fill the packet with data from peripheral */
        packet = TEMPERATURE;

        /* Send the packet  via the queue */
        OS_queueEnqueue(&queue_sensor_2_3, &packet);
        OS_sleep(4000);
    }
}

/**
 * [task_sensor_3 Sends very simple and unchanging uint32_t data over a second
 *   queue, queue_sensor_2_3, to task_compile_transmit_sens_2_3.
 *  No allocation or deallocation required. ]
 * @param args [NA]
 */
void task_sensor_3(void const * const args) {
    /*  A pointer to a packet, memory to be allocated from a memory pool */
    uint32_t packet;
    while(1) {
        /* Fill the packet with data from peripheral */
        packet = LIGHT;

        /* Send the packet  via the queue */
        OS_queueEnqueue(&queue_sensor_2_3, &packet);
        OS_sleep(8000);
    }
}


/**
 * [task_compile_transmit_sens_2_3 Prints data received from sensor 1 and 2
 *  when available. ]
 * @param args [NA]
 */
void task_compile_transmit_sens_2_3(void const * const args) {
    /*  A pointer to a packet to be received from queue and deallocated
        to a memory pool after transmitting the data*/
        uint32_t packet_r;
    while (1) {
        OS_queueDequeue(&queue_sensor_2_3, &packet_r);
        OS_mutexAcquire(&serial_mutex);
        printf("Sensor: %d Transmitted\r\n", packet_r);
        OS_mutexRelease(&serial_mutex);
    }
}

/**
 * [task_low_pri_print Lowest priority task - prints every 16s -
 *  the system is not really busy enough to block it. ]
 * @param args [NA]
 */
void task_low_pri_print(void const * const args) {
    while (1) {
        OS_mutexAcquire(&serial_mutex);
        puts("Minimum Priority Task\r");
        OS_mutexRelease(&serial_mutex);
        OS_sleep(16000);
    }
}
