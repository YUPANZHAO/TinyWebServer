#ifndef _TASK_QUEUE_H_
#define _TASK_QUEUE_H_

#include <pthread.h>
#include <queue>
#include <functional>
#include <memory>

// 任务结构体
struct Task {
    std::function<void()> func;  //任务体
    Task() {}
    Task(const std::function<void()>& func) {
        this->func = func;
    } 
    Task(const std::function<void()>&& func) {
        this->func = std::move(func);
    }
};

// 任务队列类
class TaskQueue {

public:
    typedef std::shared_ptr<Task> Task_Type;
    TaskQueue();
    ~TaskQueue();
    inline int size() { return taskQ.size(); };
    inline bool empty() { return taskQ.empty(); };
    Task_Type get();
    void add(Task_Type task);

private:
    std::queue<Task_Type> taskQ; //任务队列
    pthread_mutex_t mutex;  //队列锁

};

#endif
