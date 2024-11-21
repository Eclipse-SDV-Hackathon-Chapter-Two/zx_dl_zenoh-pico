

#ifndef ZENOH_PICO_SYSTEM_PLATFORM_THREADX_H
#define ZENOH_PICO_SYSTEM_PLATFORM_THREADX_H

#include <tx_api.h>



#if Z_FEATURE_MULTI_THREAD == 1
typedef struct {
    const char *name;
    UBaseType_t priority;
    size_t stack_depth;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    bool static_allocation;
    StackType_t *stack_buffer;
    StaticTask_t *task_buffer;
#endif /* SUPPORT_STATIC_ALLOCATION */
} z_task_attr_t;

typedef struct {
    TaskHandle_t handle;
    EventGroupHandle_t join_event;
} _z_task_t;

typedef SemaphoreHandle_t _z_mutex_t;
typedef void *_z_condvar_t;
#endif  // Z_MULTI_THREAD == 1

typedef TickType_t z_clock_t;
typedef TickType_t z_time_t;

#endif /* ZENOH_PICO_SYSTEM_PLATFORM_THREADX_H */
