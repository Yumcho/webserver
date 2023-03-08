#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <time.h>
#include "../log/log.h"

class util_timer;

struct client_data
{
    sockaddr_in address;
    int sockfd;
    util_timer *timer;
};
// 定时器基类
class util_timer
{
public:
    time_t expire;
    void (* cb_func)(client_data *);
    client_data *user_data; 
};
// 链表定时器类
class lst_timer : public util_timer
{
public:
    lst_timer() : prev(NULL), next(NULL) {}

public:
    lst_timer *prev;
    lst_timer *next;
};
// 堆定时器类
class heap_timer : public util_timer
{
public:
    // heap_timer(int delay){
    //     expire = time(NULL) + delay;
    // }
    // heap_timer();
};

// 定时器容器基类
class timer_container
{
public:
    // timer_container();
    // ~timer_container();

    virtual void add_timer(util_timer *timer){};
    virtual void adjust_timer(util_timer *timer){};
    virtual void del_timer(util_timer *timer){};
    virtual void tick(){};
};
// 链表定时器容器
class lst_container : public timer_container
{
public:
    lst_container();
    ~lst_container();

public:
    void add_timer(util_timer *timer);
    void adjust_timer(util_timer *timer);
    void del_timer(util_timer *timer);
    void tick();

private:
    void add_timer(lst_timer *timer, lst_timer *lst_head);
    lst_timer *head;
    lst_timer *tail;
};
// 堆定时器容器
class heap_container : public timer_container
{
public:
    // 构造函数之一，初始化一个大小为cap的空集
    // throw(str::exception)放在函数头和函数体之间，表示函数能够抛出的异常类型
    heap_container(int cap): capacity(cap), cur_size(0){
        array = new heap_timer*[capacity];  // 创建堆数组
        if(!array){
            throw std::exception();
        }
        for(int i=0; i<capacity; ++i){
            array[i] = NULL;
        }
    }
    // 构造函数之二，用已有数组来初始化堆
    heap_container(heap_timer** init_array, int size, int capacity): cur_size(size), capacity(capacity){
        if(capacity < size){
            throw std::exception();
        }
        array = new heap_timer*[capacity];  // 创建堆数组
        if(!array){
            throw std::exception();
        }
        for(int i=0; i<capacity; ++i){
            array[i] = NULL;
        }
        if(size != 0){
            // 初始化堆数组
            for(int i=0; i<size; ++i){
                array[i] = init_array[i];
            }
            for(int i=(cur_size-1)/2; i>0; --i){
                // 对数组中的第 (cur_size-1)/2~0 个元素执行下虑操作
                percolate_down(i);
            }
        }
    }
    ~heap_container(){
        for(int i=0; i<cur_size; ++i){
            delete array[i];
        }
        delete[] array;
    }

public:
    void add_timer(util_timer *timer);
    void adjust_timer(util_timer *timer);
    void del_timer(util_timer *timer);
    void tick();
    heap_timer* top() const;
    void pop_timer();
    bool empty() const{return cur_size == 0;}

private:
    void percolate_down(int hole);  // 最小堆的下虑操作，它确保堆数组中以第hole个结点作为根的子树拥有最小堆性质
    void resize();  // 将堆数组容量扩大1倍

private:
    heap_timer** array;
    int capacity;
    int cur_size;
};

class Utils
{
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot, int container);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    timer_container *m_timer_container;
    static int u_epollfd;
    int m_TIMESLOT;
};

void cb_func(client_data *user_data);

#endif
