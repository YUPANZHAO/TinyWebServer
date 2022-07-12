#include "thread_pool.h"
#include <unistd.h>
#include <iostream>
#include <cmath>
#include <functional>
#include <future>

ThreadPool::ThreadPool(int low, int up, int ch) 
: min_num(low), max_num(up), change_num(ch),
busy_num(0), live_num(low), exit_num(0),
shutdown(false) {
    // 初始化全局锁
    pthread_mutex_init(&mutex, NULL);
    // 初始化条件变量
    pthread_cond_init(&cond, NULL);
    pthread_cond_init(&done, NULL);
    // 创建工作者
    worker_pid = new pthread_t [max_num];
    // 初始化工作者线程
    for(int i=0; i < min_num; ++i) {
        pthread_create(worker_pid+i, NULL, worker, this);
        // std::cout << "线程" << worker_pid[i] << " 被创建\n";
    }
    // 初始化管理者线程
    pthread_create(&manager_pid, NULL, manager, this);
}

ThreadPool::~ThreadPool() {
    shutdown = true;
    pthread_join(manager_pid, NULL);
    for(int i=0; i < live_num; ++i)
        pthread_cond_signal(&cond);
    pthread_cond_destroy(&cond);
    pthread_cond_destroy(&done);
    pthread_mutex_destroy(&mutex);
    if(worker_pid) delete [] worker_pid;
}

void* ThreadPool::worker(void* arg) {
    // 获取线程池数据
    ThreadPool* pool = (ThreadPool*)arg;
    // 不断工作(从任务队列取任务)
    while(true) {
        pthread_mutex_lock(&pool->mutex);
        // 等待任务
        while(!pool->shutdown && pool->taskQ.empty()) {
            pthread_mutex_unlock(&pool->mutex);
            if(pool->shutdown == 0 && pool->taskQ.size() == 0) {
                pthread_cond_signal(&pool->done);
            }
            pthread_cond_wait(&pool->cond, &pool->mutex);
            if(pool->exit_num > 0) {
                // 自杀
                pool->exit_num--;
                pool->live_num--;
                pthread_mutex_unlock(&pool->mutex);
                pool->threadExit();
                return NULL;
            }
        }
        // 线程池关闭
        if(pool->shutdown) {
            pthread_mutex_unlock(&pool->mutex);
            pool->threadExit();
            return NULL;
        }
        // 获取任务
        pool->busy_num++;
        auto task = pool->taskQ.get();
        pthread_mutex_unlock(&pool->mutex);
        // 执行任务
        task->func();
        // 任务完成，忙线程数量减少
        pthread_mutex_lock(&pool->mutex);
        pool->busy_num--;
        pthread_mutex_unlock(&pool->mutex);
    }
    return NULL;
}

void* ThreadPool::manager(void* arg) {
    // 获取线程池数据
    ThreadPool* pool = (ThreadPool*)arg;
    // 不断工作(直到线程池关闭)
    while(!pool->shutdown) {
        sleep(3);
        pthread_mutex_lock(&pool->mutex);
        int task_num = pool->taskQ.size();
        // 添加线程
        if(task_num > (pool->live_num << 1) && pool->live_num < pool->max_num) {
            int add_num = std::min(pool->max_num - pool->live_num, pool->change_num);
            for(int i=0; i < pool->max_num && add_num; ++i) {
                if(pool->worker_pid[i] == 0) {
                    pthread_create(pool->worker_pid+i, NULL, worker, pool);
                    // std::cout << "线程" << pool->worker_pid[i] << " 被创建\n";
                    add_num--;
                    pool->live_num++;
                }
            }
            pthread_mutex_unlock(&pool->mutex);
            continue;
        }
        // 销毁线程
        if(pool->live_num > (pool->busy_num << 1) && pool->live_num > pool->min_num) {
            int del_num = std::min(pool->live_num - pool->min_num, pool->change_num);
            pool->exit_num = del_num;
            pthread_mutex_unlock(&pool->mutex);
            for(int i=0; i < del_num; ++i)
                pthread_cond_signal(&pool->cond);
            continue;
        }
        pthread_mutex_unlock(&pool->mutex);
    }
    return NULL;
}

void ThreadPool::threadExit() {
    pthread_t pid = pthread_self();
    for(int i=0; i < max_num; ++i)
        if(worker_pid[i] == pid) {
            worker_pid[i] = 0;
            // std::cout << "线程 " << pid << " 被销毁\n";
            break;
        }
    pthread_exit(NULL);
}

void ThreadPool::waitForAllDone() {
    pthread_mutex_lock(&mutex);
    while(busy_num != 0 || taskQ.size() != 0) {
        pthread_mutex_unlock(&mutex);
        pthread_cond_wait(&done, &mutex);
    }
    pthread_mutex_unlock(&mutex);
}