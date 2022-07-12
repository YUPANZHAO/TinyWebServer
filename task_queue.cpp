#include "task_queue.h"

TaskQueue::TaskQueue() {
    pthread_mutex_init(&mutex, NULL);
}

TaskQueue::~TaskQueue() {
    pthread_mutex_destroy(&mutex);
}

typename TaskQueue::Task_Type TaskQueue::get() {
    Task_Type task;
    pthread_mutex_lock(&mutex);
    if(!taskQ.empty()) {
        task = std::move(taskQ.front());
        taskQ.pop();
    }
    pthread_mutex_unlock(&mutex);
    return task;
}

void TaskQueue::add(TaskQueue::Task_Type task) {
    pthread_mutex_lock(&mutex);
    taskQ.push(task);
    pthread_mutex_unlock(&mutex);
}