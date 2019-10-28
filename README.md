# DocetOS

A small and incomplete RTOS developed through teaching and assignments at the University of York.

## Functionality Developed through Assignment:
+ Preemptive Scheduler with N fixed-priority roundrobin buckets for task management, with 
  - Sleep: Sleep for N ms
  - Wait: Sleep and wait for some system resource (mutex/semaphore) to become available. OS notifies first task in resource que when available
+ Inter-task Communication: Tasks can have shared queues to send information from one task to the next without global variables.
+ Memory Pools: The safer embedded version of malloc() and free() used in embedded systems for improved system control and reduced static memory demand
+ Demonstration code: main_DEMO.c is a demonstration of the OS capabilities.

## Improvements:
+ Priority inheritance: Lower priority tasks holding resources needed for higher priority tasks are temporarily boosted to the highest waiting tasks' priority.
+ FPU support
+ Ability to notify tasks via ISR (triggered by hardware)
+ Reduce the scheduler overhead by utilising a hardware timer and ISR for waking sleeping tasks instead of checking for next wakeup every context switch.


## Assignment Brief:
Task: To modify DocetOS to increase its functionality.

Features must include:
+ An efficient fixed-priority scheduler
+ A queue-based task communication system

Must also include at least TWO of the following:
+ A memory pool implementation, properly protected by mutexes
+ Support for code that uses the floating-point unit (FPU)
+ A more efficient sleep/wait mechanism

Optionally, to make your design more useful, you may implement more than two of the
features from the list above, or change or extend any other features of the operating
system as you see fit.

The implementation details of the features are entirely up to the developer, but should ensure
avoidance of race conditions, efficient use of CPU time and RAM, and to a sensible, readable code structure. 
