#include "lst_timer.h"
#include "../http/http_conn.h"

lst_container::lst_container()
{
    head = NULL;
    tail = NULL;
}
lst_container::~lst_container()
{
    lst_timer *tmp = head;
    while (tmp)
    {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void lst_container::add_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if (!head)
    {
        head = tail = (lst_timer *)timer;
        return;
    }
    lst_timer * orig = (lst_timer *)timer;
    if (orig->expire < head->expire)
    {
        orig->next = head;
        head->prev = orig;
        head = orig;
        return;
    }
    add_timer((lst_timer *)timer, head);
}
void lst_container::adjust_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    lst_timer *orig = (lst_timer *)timer;
    lst_timer *tmp = orig->next;
    if (!tmp || (orig->expire < tmp->expire))
    {
        return;
    }
    if (orig == head)
    {
        head = head->next;
        head->prev = NULL;
        orig->next = NULL;
        add_timer(orig, head);
    }
    else
    {
        orig->prev->next = orig->next;
        orig->next->prev = orig->prev;
        add_timer(orig, orig->next);
    }
}
void lst_container::del_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    lst_timer *orig = (lst_timer *)timer;
    if ((orig == head) && (orig == tail))
    {
        delete orig;
        head = NULL;
        tail = NULL;
        return;
    }
    if (orig == head)
    {
        head = head->next;
        head->prev = NULL;
        delete orig;
        return;
    }
    if (orig == tail)
    {
        tail = tail->prev;
        tail->next = NULL;
        delete orig;
        return;
    }
    orig->prev->next = orig->next;
    orig->next->prev = orig->prev;
    delete orig;
}
void lst_container::tick()
{
    if (!head)
    {
        return;
    }
    
    time_t cur = time(NULL);
    lst_timer *tmp = head;
    while (tmp)
    {
        if (cur < tmp->expire)
        {
            break;
        }
        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if (head)
        {
            head->prev = NULL;
        }
        delete tmp;
        tmp = head;
    }
}

void lst_container::add_timer(lst_timer *timer, lst_timer *lst_head)
{
    lst_timer *prev = lst_head;
    lst_timer *tmp = prev->next;
    while (tmp)
    {
        if (timer->expire < tmp->expire)
        {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    if (!tmp)
    {
        prev->next = timer;
        timer->prev = prev;
        timer->next = NULL;
        tail = timer;
    }
}

void heap_container::add_timer(util_timer *timer)
{
    if(!timer)return;
    if(cur_size >= capacity){  // 如果当前堆数组容量不够，则将其扩大1倍
        resize();
    }
    // 新插入了一个元素，当前堆大小加1，hole是新建空穴的位置
    int hole = cur_size++;
    int parent = 0;
    heap_timer *orig = (heap_timer *)timer;
    // 对从空穴到根结点的路径上的所有结点执行上虑操作
    for( ; hole>0; hole=parent){
        parent = (hole-1)/2;
        if(array[parent]->expire <= orig->expire)break;
        array[hole] = array[parent];
    }
    array[hole] = orig;    
}
void heap_container::adjust_timer(util_timer *timer)
{
    heap_timer *tmp = new heap_timer();
    tmp->cb_func = timer->cb_func;
    tmp->expire = timer->expire;
    tmp->user_data = timer->user_data;
    del_timer(tmp);
    add_timer(tmp);
}
void heap_container::del_timer(util_timer *timer)
{
    if(!timer)return;
    // 仅仅将目标定时器的回调函数设置为空，即所谓的延迟销毁。
    // 这将节省真正删除该定时器造成的开销，但这样做容易使堆数组膨胀
    timer->cb_func = NULL;
}
void heap_container::tick()
{
    heap_timer* tmp = array[0];
    time_t cur = time(NULL);  // 循环处理堆中到期的定时器
    while(!empty()){
        if(!tmp)break;
        // 如果堆顶定时器没到期，则退出循环
        if(tmp->expire > cur)break;
        // 否则就执行堆顶定时器中的任务
        if(array[0]->cb_func){
            array[0]->cb_func(array[0]->user_data);
        }
        // 将堆顶元素删除，同时生成新的堆顶定时器array[0]
        pop_timer();
        tmp = array[0];
    }
}
heap_timer* heap_container::top() const
{
    if(empty())return NULL;
        return array[0];
}
void heap_container::pop_timer()
{
    if(empty())return;
    if(array[0]){
        delete array[0];
        // 将原来的堆顶元素替换成堆数组的最后一个元素
        array[0] = array[--cur_size];
        percolate_down(0);
    }
}
void heap_container::percolate_down(int hole)
{
    heap_timer* temp = array[hole];
    int child = 0;
    for( ; ((hole*2+1) <= cur_size-1); hole=child){
        child = hole*2+1;  // hole的左孩子
        if((child < (cur_size-1)) && (array[child+1]->expire < array[child]->expire)){
            ++child;  // 如果hole有右孩子，且右孩子比左孩子小，则child指向右孩子
        }
        if(array[child]->expire < temp->expire){
            array[hole] = array[child];
        }
        else{
            break;
        }
    }
    array[hole] = temp;
}
void heap_container::resize()
{
    heap_timer** temp = new heap_timer*[2*capacity];
    for(int i=0; i<2*capacity; ++i){
        temp[i] = NULL;
    }
    if(!temp){
        throw std::exception();
    }
    capacity = 2*capacity;
    for(int i=0; i<cur_size; ++i){
        temp[i] = array[i];
    }
    delete[] array;
    array = temp;
}


void Utils::init(int timeslot, int container)
{
    m_TIMESLOT = timeslot;
    switch (container)
    {
    case 0:
    {
        m_timer_container = new lst_container;
        break;
    }
    case 1:
    {
        m_timer_container = new heap_container(500);
        break;
    }
    default:
    {
        break;
    }
    }
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_timer_container->tick();
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}
