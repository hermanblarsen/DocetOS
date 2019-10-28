#include "os.h"
#include <stdio.h>
#include "utils/serial.h"
#include "roundRobin.h"
#include "sleep.h"
#include "mutex.h"
#include "semaphore.h"
#include "queue.h"
#include "mempool.h"

/* Define which tests to run - uncomment to not run */
#define TEST_SLEEP          //3 tasks
#define TEST_MUTEX          //5 tasks
#define TEST_SEMAPHORE      //3 tasks
#define TEST_QUEUE          //3 tasks
#define TEST_MEMPOOL        //3 tasks

#if defined (TEST_SLEEP) || defined (TEST_MUTEX) || defined (TEST_SEMAPHORE) || \
         defined (TEST_QUEUE) || defined (TEST_MEMPOOL) 
# define TESTS_ACTIVE
#endif


/* Function Prototypes  */
void my_welcome(void);
/* Test Function Prototypes  */
void task_mutex_1(void const * const args);
void task_mutex_2(void const * const args);
void task_mutex_3(void const * const args);
void task_mutex_4(void const * const args);
void task_mutex_5(void const * const args);

void task_sleep_1(void const * const args);
void task_sleep_2(void const * const args);
void task_sleep_3(void const * const args);

void task_semaphore_1(void const * const args);
void task_semaphore_2(void const * const args);
void task_semaphore_3(void const * const args);

void task_queue_1(void const * const args);
void task_queue_2(void const * const args);
void task_queue_3(void const * const args);

void task_mempool_1(void const * const args);
void task_mempool_2(void const * const args);
void task_mempool_3(void const * const args);

void myOverflowTest(void);

/* Global Variables , including mutexes, semaphores, queues, etc.*/
/* Recursive Mutex for testing mutexes */
static OS_Mutex_t _mutex_printf;

/* Counting Semaphore for testing mutexes */
static OS_Semaphore_t _semaphore_test;
#define SEMAPHORE_TEST_SIZE 4


/* Task Communication Test */
#define TEST_QUEUE_SIZE 5
/* Make a queue that test tasks can communicate over */
static OS_Queue_t _queue_test;
/* Make a random test structure that we can test with. 
    This struct should either naturally be word aligned (4 bytes),
    or compiler padding should be allowed optimisation to achieve word aligment. */
/* When the memory for the queue of ANY type, this must be word aligned.  */
typedef struct {
    uint32_t field_4byte; // 4 bytes
    uint16_t field_2byte_1, field_2byte_2, field_2byte_3 ;
    uint8_t test_array[1];
} QueueTestStruct_t;
/* Create an AT LEAST word aligned, double word aligned also works, static storage
    location for the queue. */
__align(4)
static QueueTestStruct_t _queue_store[TEST_QUEUE_SIZE];


/* Memory Pools Test */
typedef struct {
    uint32_t id, num_arr[1];
} MemPoolTestStruct_t;
#define MEMORY_POOL_SIZE 4
#define MEMORY_POOL_QUEUE_SIZE 1
static OS_MemPool_t _memory_pool_test;
static MemPoolTestStruct_t _memory_pool_mem_block[MEMORY_POOL_SIZE];
static OS_Queue_t _mempool_queue;
__align(4)
static uint32_t * _mempool_queue_store[MEMORY_POOL_QUEUE_SIZE];

/*=============================================================================
**       
=============================================================================*/
/*****************************************************************************
**  
******************************************************************************/

/* MAIN FUNCTION */
int main(void) {
	/* Initialise the serial port so printf() works */
	serial_init();
    
    //myOverflowTest();
    
    /* Deliver a personal welcome to the developer of this OS. 
        TODO Should be removed prior to release*/
    my_welcome();
    
    /* Reserve memory for stacks and TCBs. Stacks must be 8-byte aligned. */
#ifdef TESTS_ACTIVE
    /* Only align if there are any TCB stacks to statically allocate - will cause error 
        otherwise, as function call to OS_init(...) cannot be 'aligned' */
    __align(8)
#endif
#ifdef TEST_SLEEP
    static uint32_t stack_sleep_1[64],\
                    stack_sleep_2[64],\
                    stack_sleep_3[64];
	static OS_TCB_t tcb_sleep_1,\
                    tcb_sleep_2,\
                    tcb_sleep_3;
#endif
#ifdef TEST_MUTEX
	static uint32_t stack_mutex_1[64],\
                    stack_mutex_2[64],\
                    stack_mutex_3[64],\
                    stack_mutex_4[64],\
                    stack_mutex_5[64];
    static OS_TCB_t tcb_mutex_1,\
                    tcb_mutex_2,\
                    tcb_mutex_3,\
                    tcb_mutex_4,\
                    tcb_mutex_5;
#endif
#ifdef TEST_SEMAPHORE
    static uint32_t stack_semaphore_1[64],\
                    stack_semaphore_2[64],\
                    stack_semaphore_3[64];
	static OS_TCB_t tcb_semaphore_1,\
                    tcb_semaphore_2,\
                    tcb_semaphore_3;
#endif 
#ifdef TEST_QUEUE
    static uint32_t stack_queue_1[64],\
                    stack_queue_2[64],\
                    stack_queue_3[64];
	static OS_TCB_t tcb_queue_1,\
                    tcb_queue_2,\
                    tcb_queue_3;
#endif
#ifdef TEST_MEMPOOL
    static uint32_t stack_mempool_1[64],\
                    stack_mempool_2[64],\
                    stack_mempool_3[64];
	static OS_TCB_t tcb_mempool_1,\
                    tcb_mempool_2,\
                    tcb_mempool_3;
#endif

	/* Initialise TCBs */
#ifdef TEST_SLEEP   
    OS_initialiseTCB(&tcb_sleep_1, stack_sleep_1+64, task_sleep_1, PRIORITY_MAX, NULL);
    OS_initialiseTCB(&tcb_sleep_2, stack_sleep_2+64, task_sleep_2, PRIORITY_MAX, NULL);
    OS_initialiseTCB(&tcb_sleep_3, stack_sleep_3+64, task_sleep_3, PRIORITY_MAX, NULL);
#endif
#ifdef TEST_MUTEX
	OS_initialiseTCB(&tcb_mutex_1, stack_mutex_1+64, task_mutex_1, PRIORITY_MAX, NULL);
	OS_initialiseTCB(&tcb_mutex_2, stack_mutex_2+64, task_mutex_2, PRIORITY_MAX, NULL);
    OS_initialiseTCB(&tcb_mutex_3, stack_mutex_3+64, task_mutex_3, PRIORITY_MAX, NULL);
    OS_initialiseTCB(&tcb_mutex_4, stack_mutex_4+64, task_mutex_4, PRIORITY_MAX, NULL);
    OS_initialiseTCB(&tcb_mutex_5, stack_mutex_5+64, task_mutex_5, PRIORITY_MAX, NULL);
#endif
#ifdef TEST_SEMAPHORE   
    OS_initialiseTCB(&tcb_semaphore_1, stack_semaphore_1+64, task_semaphore_1, PRIORITY_MAX, NULL);
    OS_initialiseTCB(&tcb_semaphore_2, stack_semaphore_2+64, task_semaphore_2, PRIORITY_MAX, NULL);
    OS_initialiseTCB(&tcb_semaphore_3, stack_semaphore_3+64, task_semaphore_3, PRIORITY_MAX, NULL);
#endif
#ifdef TEST_QUEUE   
    OS_initialiseTCB(&tcb_queue_1, stack_queue_1+64, task_queue_1, PRIORITY_MAX, NULL);
    OS_initialiseTCB(&tcb_queue_2, stack_queue_2+64, task_queue_2, PRIORITY_MAX, NULL);
    OS_initialiseTCB(&tcb_queue_3, stack_queue_3+64, task_queue_3, PRIORITY_MAX, NULL);
#endif
#ifdef TEST_MEMPOOL
    OS_initialiseTCB(&tcb_mempool_1, stack_mempool_1+64, task_mempool_1, PRIORITY_MAX, NULL);
    OS_initialiseTCB(&tcb_mempool_2, stack_mempool_2+64, task_mempool_2, PRIORITY_MAX, NULL);
    OS_initialiseTCB(&tcb_mempool_3, stack_mempool_3+64, task_mempool_3, PRIORITY_MAX, NULL);
#endif

	/* Initialise the scheduler */
	OS_init(&round_robin_scheduler);
    
    /* Initialise Mutexes for serial port print access (and testing) */
    OS_mutexInitialise(&_mutex_printf);
    
    /* Initialise Semaphore for semaphore testing */
    OS_semaphoreInitialise(&_semaphore_test, SEMAPHORE_TEST_SIZE, SEMAPHORE_TEST_SIZE);
    
    /* Initialise the queue for inter-task communication */
    OS_queueInitialise(&_queue_test, &_queue_store, TEST_QUEUE_SIZE, sizeof(QueueTestStruct_t));
    
    /* Initialise the memory pool */
    OS_memPoolInitialise(&_memory_pool_test, &_memory_pool_mem_block, MEMORY_POOL_SIZE, sizeof(MemPoolTestStruct_t));
    OS_queueInitialise(&_mempool_queue, &_mempool_queue_store, MEMORY_POOL_QUEUE_SIZE, sizeof(_mempool_queue_store[0]));
    

    /* Add tasks to the scheduler */
#ifdef TEST_SLEEP 
    OS_addTask(&tcb_sleep_1);
    OS_addTask(&tcb_sleep_2);
    OS_addTask(&tcb_sleep_3);
#endif   
#ifdef TEST_MUTEX
	OS_addTask(&tcb_mutex_1);
	OS_addTask(&tcb_mutex_2);
    OS_addTask(&tcb_mutex_3);
    OS_addTask(&tcb_mutex_4);
    OS_addTask(&tcb_mutex_5);
#endif
#ifdef TEST_SEMAPHORE 
    OS_addTask(&tcb_semaphore_1);
    OS_addTask(&tcb_semaphore_2);
    OS_addTask(&tcb_semaphore_3);
#endif
#ifdef TEST_QUEUE 
    OS_addTask(&tcb_queue_1);
    OS_addTask(&tcb_queue_2);
    OS_addTask(&tcb_queue_3);
#endif
#ifdef TEST_MEMPOOL
    OS_addTask(&tcb_mempool_1);
    OS_addTask(&tcb_mempool_2);
    OS_addTask(&tcb_mempool_3);
#endif   
    
    /* Finally start the OS */
	OS_start();
}

/*=============================================================================
    Test Tasks used to test parts of the OS functionality whilst developing 
=============================================================================*/
/*****************************************************************************
    Test Tasks for Semaphores and Wait mechanism. 
    Requires from OS specific headers:
        #include "queue.h"
        #include "mutex.h" (for printf)
******************************************************************************/
void task_mempool_1(void const * const args) {
    static MemPoolTestStruct_t * block[10] = {0};
    uint32_t max_loop_count = 1, increment_count = 1;
    while (1) {
        OS_mutexAcquire(&_mutex_printf);
        printf("MEMPOOL\tTask 1  : Alloc/Dealloc %d times: \n\r", max_loop_count);
        OS_mutexRelease(&_mutex_printf);
        
        for (uint32_t i = 0; i < max_loop_count; i++) {
            block[i] = OS_memPoolAllocate(&_memory_pool_test);
            
            OS_mutexAcquire(&_mutex_printf);
            printf("MEMPOOL\tTask 1  : Allocated block address %p\n\r", block[i]);
            OS_mutexRelease(&_mutex_printf);
        }
        for (uint32_t i = max_loop_count; i > 0 ; i--) {
            OS_memPoolDeallocate(&_memory_pool_test, block[i-1]);
            
            OS_mutexAcquire(&_mutex_printf);
            printf("MEMPOOL\tTask 1  : Deallocated block address %p\n\r", block[i-1]);
            OS_mutexRelease(&_mutex_printf);
        }
        if (max_loop_count == 1 || max_loop_count == MEMORY_POOL_SIZE) {
            increment_count = !increment_count;
        }
        if (max_loop_count < MEMORY_POOL_SIZE && increment_count) {
            max_loop_count++;
        } else if (max_loop_count > 1 && !increment_count) {
            max_loop_count--;
        }
        OS_sleep(400);
	}    
}
void task_mempool_2(void const * const args) {
    /* Allocate from the queue and send the pointer in the queue */
    MemPoolTestStruct_t * block;
    while (1) {
        for (uint32_t i = 0; i < 50; i++) {
            block = OS_memPoolAllocate(&_memory_pool_test);
            block->id = i*10;
            block->num_arr[0] = i*100;
            OS_mutexAcquire(&_mutex_printf);            
            printf("MEMPOOL\tTask  2 : Allocating, populating and sending block %p\n\r", block);
            OS_mutexRelease(&_mutex_printf);
            OS_queueEnqueue(&_mempool_queue, &block);
            OS_sleep(200);
        }
	}
}

void task_mempool_3(void const * const args) {    
    /* Receive pointers to structs from a queue and deallocate them */
    MemPoolTestStruct_t * block;
    while (1) {
        OS_queueDequeue(&_mempool_queue, &block);
        OS_mutexAcquire(&_mutex_printf);            
        printf("MEMPOOL\tTask   3: Block Received: ID %d , num %d. Deallocated %p\n\r", block->id, block->num_arr[0], block);
        OS_mutexRelease(&_mutex_printf);
        OS_memPoolDeallocate(&_memory_pool_test, block);
	}
}


/*****************************************************************************
    Test Tasks for queues for intern-task communication. 
    The tasls more specifically test a use case that should not very often be
     used, namely copying a larger structure of data over the queue rather
     than a pointer to it. This operation does however test the queue operation
     fully, and verifies the internal copying mechanisms. Using the queue like 
     this should only be done with small data-types, and if bigger types are 
     used the user should use pointers to the data to send instead.
    Requires from OS specific headers:
        #include "queue.h"
        #include "mutex.h" (for printf)
******************************************************************************/
void task_queue_1(void const * const args) {
    QueueTestStruct_t message;

    /* Send populated message to the queue forever */
    while (1) {        
        for (uint_fast8_t i = 0; i < 30; i++) {
            message.field_4byte = 100*i;
            message.field_2byte_1 = 10*i;
            message.field_2byte_2 = 1*i;
            OS_queueEnqueue(&_queue_test, &message);
            OS_sleep(100);
        }
	}
}
#define QUEUE_TEST_SEND_NOT_RECEIVE 1
void task_queue_2(void const * const args) {
    /* Make a message that we can write and send to the queue */
    QueueTestStruct_t message;
    QueueTestStruct_t received_message;

    /* Send populated message to the queue forever */
    while (1) {        
        if (QUEUE_TEST_SEND_NOT_RECEIVE) {
            for (uint_fast8_t i = 0; i < 20; i++) {
                message.field_4byte = 1000*i;
                message.field_2byte_1 = 100*i;
                message.field_2byte_2 = 10*i;
                OS_queueEnqueue(&_queue_test, &message);
                OS_sleep(200);
            }
        } else {
            OS_queueDequeue(&_queue_test, &received_message);
            OS_mutexAcquire(&_mutex_printf);
            printf("QUEUE\tTask  2 : Fields Recevied: 4B:%d \t2B_1: %d \t2B_2: %d\r\n", \
                received_message.field_4byte, received_message.field_2byte_1, received_message.field_2byte_2);
            OS_mutexRelease(&_mutex_printf);
        }
	}
}

void task_queue_3(void const * const args) {    
    /* A local message that we can populate with queue data */
    QueueTestStruct_t received_message;
    /* Retrieve messages from the queue forever, and print some content from received message */
    while (1) {
        OS_queueDequeue(&_queue_test, &received_message);
        OS_mutexAcquire(&_mutex_printf);
        printf("QUEUE\tTask   3: Fields Recevied: 4B:%d \t2B_1: %d \t2B_2: %d\r\n", \
            received_message.field_4byte, received_message.field_2byte_1, received_message.field_2byte_2);
        OS_mutexRelease(&_mutex_printf);
	} 
}

/*****************************************************************************
    Test Tasks for Semaphores and Wait mechanism. 
    Requires from OS specific headers:
        #include "semaphore.h"
        #include "mutex.h" (for printf)        
******************************************************************************/
void task_semaphore_1(void const * const args) {
	uint_fast8_t tokens;
    while (1) {
        OS_semaphoreTake(&_semaphore_test);
        tokens = _semaphore_test.tokens;
        
        OS_mutexAcquire(&_mutex_printf);
        printf("SEMPHOR\tTask 1  \tTakes, Tokens Left: %x, Tick %x\r\n", tokens, OS_elapsedTicks());
        OS_mutexRelease(&_mutex_printf);
        
        OS_yield();
	}
}
void task_semaphore_2(void const * const args) {
    uint_fast8_t tokens;
    while (1) {
        uint_fast8_t take_semaphore = 1;
        if (take_semaphore) {
            OS_semaphoreTake(&_semaphore_test);
        } else {
            OS_semaphoreGive(&_semaphore_test);
        }
        tokens = _semaphore_test.tokens;
        
        OS_mutexAcquire(&_mutex_printf);
        printf("SEMPHOR\tTask  2 \t%s, Tokens Left: %x, Tick %x\r\n", take_semaphore ? "Takes" : "Gives", tokens, OS_elapsedTicks());
        OS_mutexRelease(&_mutex_printf);
        OS_yield();
	}
}

void task_semaphore_3(void const * const args) {
    uint_fast8_t tokens;
    while (1) {
        OS_semaphoreGive(&_semaphore_test);
        tokens = _semaphore_test.tokens;
        
        OS_mutexAcquire(&_mutex_printf);
        printf("SEMPHOR\tTask   3\tGives, Tokens Left: %x, Tick %x\r\n", tokens, OS_elapsedTicks());
        OS_mutexRelease(&_mutex_printf);
        
        OS_yield();
	}
}

/*****************************************************************************
    Test Tasks for Mutexes and Wait mechanism. 
    Requires from OS specific headers:
        #include "mutex.h"
        #include "sleep.h"
******************************************************************************/
void task_mutex_1(void const * const args) {
	while (1) {
        OS_mutexAcquire(&_mutex_printf);
        printf("MUTEX\tTask 1     \tTick %x\r\n", OS_elapsedTicks());
        OS_mutexRelease(&_mutex_printf);
        OS_yield();
        //OS_sleep(4);
	}
}
void task_mutex_2(void const * const args) {
    while (1) {
        OS_mutexAcquire(&_mutex_printf);
        printf("MUTEX\tTask  2    \tTick %x\r\n", OS_elapsedTicks());
        OS_mutexRelease(&_mutex_printf);
        OS_yield();
        //OS_sleep(8);
	}
}

void task_mutex_3(void const * const args) {
    while (1) {
        OS_mutexAcquire(&_mutex_printf);
        printf("MUTEX\tTask   3   \tTick %x\r\n", OS_elapsedTicks());
        OS_mutexRelease(&_mutex_printf);
        OS_yield();
        //OS_sleep(8);
	}
}

void task_mutex_4(void const * const args) {
    while (1) {
        OS_mutexAcquire(&_mutex_printf);
        //puts("Task    4 ");
        printf("MUTEX\tTask    4 \tTick %x\r\n", OS_elapsedTicks());
        OS_mutexRelease(&_mutex_printf);
        OS_yield();
        //OS_sleep(32);
	}
}

void task_mutex_5(void const * const args) {
    while (1) {
        OS_mutexAcquire(&_mutex_printf);
        //puts("Task     5");
        printf("MUTEX\tTask     5\tTick %x\r\n", OS_elapsedTicks());
        OS_mutexRelease(&_mutex_printf);
        OS_yield();
        //OS_sleep(64);
	}
}

/*****************************************************************************
    Test Tasks for Sleep Mechanism. 
    Requires from OS specific headers: 
        #include "sleep.h"
        #include "mutex.h" (for printf)
******************************************************************************/
void task_sleep_1(void const * const args) {
    while(1) {
        OS_mutexAcquire(&_mutex_printf);
        printf("SLEEP\tTask 1  \tTick %x\r\n", OS_elapsedTicks());
        OS_mutexRelease(&_mutex_printf);
        OS_sleep(100);
    }
}

void task_sleep_2(void const * const args) {
    while(1) {
        OS_mutexAcquire(&_mutex_printf);
        printf("SLEEP\tTask  2 \tTick %x\r\n", OS_elapsedTicks());
        OS_mutexRelease(&_mutex_printf);
        OS_sleep(200);
    }
}

void task_sleep_3(void const * const args) {
    while(1) {
        OS_mutexAcquire(&_mutex_printf);
        printf("SLEEP\tTask   3\tTick %x\r\n", OS_elapsedTicks());
        OS_mutexRelease(&_mutex_printf);
        OS_sleep(300);
    }
}


/*****************************************************************************
    Personal Welcoma Task to my self through development - makes sure I know 
     which mode is being used, as well as if no tests are activated, etc. 
******************************************************************************/
void my_welcome(void) {
    puts("\n\n\rDocetOS ASSIGNMENT Let's'a go!!!!\r");
    	
    /* Notify user at compile time if either the less intrusive DEBUG_SOFT 
    or more intrusive DEBUG_HARD is defined */ 
#ifdef DEBUG_SOFT
# warning "DEBUG (SOFT) mode is activated using option \"-DDEBUG_SOFT\" in cmd options"
    printf("Debug mode 'SOFT' is activated\n\r");
#endif
#ifdef DEBUG_HARD
# warning "DEBUG (HARD) mode is activated using option \"-DDEBUG_HARD\" in cmd options"
    printf("Debug mode 'HARD' is activated\n\r");
#endif

#ifndef TESTS_ACTIVE
# warning "No Tests have been activated - only IDLE will be running"
    printf("No tests have been activated - idling...\n\r");
#endif
    
}

#define sleep_timeIsAfterRefTime(time1,time2,ref) ( ( (uint8_t)( (uint8_t)(time1)-(uint8_t)(ref) ) > (uint8_t)( (uint8_t)(time2)-(uint8_t)(ref) )) )

void myOverflowTest(void) {
      uint8_t a, b;

    printf("\n\n\r");
    
    a=0x7F;;//after
    b=0x80; //128
    printf("\n\rA: %d \tB: %d --A after, True!\n\r", a, b);
    (sleep_timeIsAfterRefTime(a,b, 126)) ? printf("True!\n\r") :  printf("False!\n\r");
     printf("a-b = %d \t b-a = %d,\t int a-b = %d \t int b-a = %d  \n\r", (uint8_t)(a-b), (uint8_t)(b-a), (uint8_t)((int8_t)a-(int8_t)b), (uint8_t)((int8_t)b-(int8_t)a) );

    a=0x00;
    b=0x7F; //127 //after
    printf("\n\rA: %d \tB: %d --B after, False!\n\r", a, b);
    (sleep_timeIsAfterRefTime(a,b,0)) ? printf("True!\n\r") :  printf("False!\n\r");
     printf("a-b = %d \t b-a = %d,\t int a-b = %d \t int b-a = %d  \n\r", (uint8_t)(a-b),(uint8_t)(b-a), (uint8_t)((int8_t)a-(int8_t)b), (uint8_t)((int8_t)b-(int8_t)a) );

    a=1; //after
    b=0;
    printf("\n\rA: %d -1 = \tB: %d --A after, True!\n\r", a, b);
    (sleep_timeIsAfterRefTime(a,b, 0)) ? printf("True!\n\r") :  printf("False!\n\r");
     printf("a-b = %d \t b-a = %d,\t int a-b = %d \t int b-a = %d  \n\r", (uint8_t)(a-b), (uint8_t)(b-a), (uint8_t)((int8_t)a-(int8_t)b), (uint8_t)((int8_t)b-(int8_t)a) );

    a=2; //after
    b=1;
    printf("\n\rA: %d -1 = \tB: %d --A after, True!\n\r", a, b);
    (sleep_timeIsAfterRefTime(a,b, 0)) ? printf("True!\n\r") :  printf("False!\n\r");
     printf("a-b = %d \t b-a = %d,\t int a-b = %d \t int b-a = %d  \n\r", (uint8_t)(a-b), (uint8_t)(b-a), (uint8_t)((int8_t)a-(int8_t)b), (uint8_t)((int8_t)b-(int8_t)a) );

    a=128; //after
    b=127;
    printf("\n\rA: %d -1 = \tB: %d  -- A after, True!\n\r", a, b);
    (sleep_timeIsAfterRefTime(a,b,20)) ? printf("True!\n\r") :  printf("False!\n\r");
    printf("a-b = %d \t b-a = %d,\t int a-b = %d \t int b-a = %d  \n\r", (uint8_t)(a-b), (uint8_t)(b-a), (uint8_t)((int8_t)a-(int8_t)b), (uint8_t)((int8_t)b-(int8_t)a) );


    a=129; //after
    b=126;
    printf("\n\rA: %d -1 = \tB: %d  -- A after, True!\n\r", a, b);
    (sleep_timeIsAfterRefTime(a,b,20)) ? printf("True!\n\r") :  printf("False!\n\r");
    printf("a-b = %d \t b-a = %d,\t int a-b = %d \t int b-a = %d  \n\r", (uint8_t)(a-b), (uint8_t)(b-a), (uint8_t)((int8_t)a-(int8_t)b), (uint8_t)((int8_t)b-(int8_t)a) );


    a=255; //after
    b=254;
    printf("\n\rA: %d -1 = \tB: %d --A after, True!\n\r", a, b);
    (sleep_timeIsAfterRefTime(a,b, 130)) ? printf("True!\n\r") :  printf("False!\n\r");
     printf("a-b = %d \t b-a = %d,\t int a-b = %d \t int b-a = %d  \n\r", (uint8_t)(a-b), (uint8_t)(b-a), (uint8_t)((int8_t)a-(int8_t)b), (uint8_t)((int8_t)b-(int8_t)a) );

    a=0;
    b=127; //after
    printf("\n\rA: %d +FF = \tB: %d --B after, False!\n\r", a, b);
    (sleep_timeIsAfterRefTime(a,b,0)) ? printf("True!\n\r") :  printf("False!\n\r");
     printf("a-b = %d \t b-a = %d,\t int a-b = %d \t int b-a = %d  \n\r", (uint8_t)(a-b), (uint8_t)(b-a), (uint8_t)((int8_t)a-(int8_t)b), (uint8_t)((int8_t)b-(int8_t)a) );

    a=255;//after
    b=128;
    printf("\n\rA: %d +FF = \tB: %d--A after, True!\n\r", a, b);
    (sleep_timeIsAfterRefTime(a,b,128)) ? printf("True!\n\r") :  printf("False!\n\r");
     printf("a-b = %d \t b-a = %d,\t int a-b = %d \t int b-a = %d  \n\r", (uint8_t)(a-b), (uint8_t)(b-a), (uint8_t)((int8_t)a-(int8_t)b), (uint8_t)((int8_t)b-(int8_t)a) );

    
    
    while (1);
}
