#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include "task_queue.h"
#include <future>

class ThreadPool {

public:
    ThreadPool(int low = 1, int up = 10, int ch = 3);
    ~ThreadPool();
    void waitForAllDone();
    
    template <class F>
    auto exec(F&& func) -> std::future<decltype(func())>
    {
        using Return_Type = decltype(func());
        auto task = std::make_shared<std::packaged_task<Return_Type()>>(std::forward<F>(func));
        auto func_ptr = std::make_shared<Task>();
        func_ptr->func = [task](){
            (*task)();
        };
        pthread_mutex_lock(&mutex);
        taskQ.add(func_ptr);
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond);
        return task->get_future();
    }
    
    template <class F, class... Args>
    auto exec(F&& func, Args&&... args) -> std::future<decltype(func(args...))> 
    {
        using Return_Type = decltype(func(args...));
        auto task = std::make_shared<std::packaged_task<Return_Type()>>(std::bind(std::forward<F>(func), std::forward<Args>(args)...));
        auto func_ptr = std::make_shared<Task>();
        func_ptr->func = [task](){
            (*task)();
        };
        pthread_mutex_lock(&mutex);
        taskQ.add(func_ptr);
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond);
        return task->get_future();
    }

private:
    static void* worker(void* arg);
    static void* manager(void* arg);
    void threadExit();

private:
    // 任务队列
    TaskQueue taskQ;
    // 线程ID
    pthread_t manager_pid;
    pthread_t* worker_pid;
    // 基本信息
    int min_num;
    int max_num;
    int change_num;
    int busy_num;
    int live_num;
    int exit_num;
    bool shutdown;
    // 互斥变量
    pthread_mutex_t mutex;
    // 条件变量
    pthread_cond_t cond;
    pthread_cond_t done;
};

#endif