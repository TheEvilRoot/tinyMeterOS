#ifndef QUEUE_HH
#define QUEUE_HH

#include <freertos/FreeRTOS.h>

template<typename T>
struct Queue {
    QueueHandle_t queue;
    explicit Queue(size_t size) : queue{xQueueCreate(size, sizeof(T))} { }

    Queue(const Queue& copy) = default;
    Queue(Queue&& move) = delete;

    void PushFromIsr(const T& item) {
        if (!queue) { return; }
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(queue, &item, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken == pdTRUE) {
            portYIELD_FROM_ISR();
        }
    }

    bool Receive(T& item) {
        if (!queue) { return false; }
        if (xQueueReceive(queue, &item, portMAX_DELAY) == pdTRUE) {
            return true;
        }
        return false;
    }
};


#endif //QUEUE_HH
