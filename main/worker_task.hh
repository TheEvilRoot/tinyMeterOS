#ifndef WORKER_TASK_HH
#define WORKER_TASK_HH

#include <freertos/FreeRTOS.h>
#include <functional>

template<typename T>
struct WorkerTask {
    std::function<void(T&)> work;
    T* param;

    explicit WorkerTask(std::function<void(T&)> work, T& param) : work{std::move(work)}, param{&param} {
        xTaskCreate([](void* arg) {
            auto& task = *static_cast<WorkerTask*>(arg);
            while (true) { task.work(*task.param); }
        }, "Worker", 4096, this, 5, nullptr);
    }
};

#endif //WORKER_TASK_HH
