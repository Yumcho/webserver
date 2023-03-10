1. 基础概念
1.1 事件处理模式
(1) reactor模式中，主线程(I/O处理单元)只负责监听文件描述符上是否有事件发生，有的话立即通知工作线程(逻辑单元 )，读写数据、接受新连接及处理客户请求均在工作线程中完成。通常由同步I/O实现。
(2) proactor模式中，主线程和内核负责处理读写数据、接受新连接等I/O操作，工作线程仅负责业务逻辑，如处理客户请求。通常由异步I/O实现。

1.2 同步I/O模拟proactor模式

由于异步I/O并不成熟，实际中使用较少，这里将使用同步I/O模拟实现proactor模式。
同步I/O模型的工作流程如下（epoll_wait为例）：
(1) 主线程往epoll内核事件表注册socket上的读就绪事件。
(2) 主线程调用epoll_wait等待socket上有数据可读
(3) 当socket上有数据可读，epoll_wait通知主线程,主线程从socket循环读取数据，直到没有更多数据可读，然后将读取到的数据封装成一个请求对象并插入请求队列。
(4) 睡眠在请求队列上某个工作线程被唤醒，它获得请求对象并处理客户请求，然后往epoll内核事件表中注册该socket上的写就绪事件
(5) 主线程调用epoll_wait等待socket可写。
(6)当socket上有数据可写，epoll_wait通知主线程。主线程往socket上写入服务器处理客户请求的结果。

1.3 并发编程模式

并发编程方法的实现有多线程和多进程两种，但这里涉及的并发模式指I/O处理单元与逻辑单元的协同完成任务的方法。
(1) 半同步/半异步模式
(2) 领导者/追随者模式

1.4 半同步/半反应堆
半同步/半反应堆并发模式是半同步/半异步的变体，将半异步具体化为某种事件处理模式.

1.4.1 并发模式中的同步和异步
(1) 同步指的是程序完全按照代码序列的顺序执行
(2) 异步指的是程序的执行需要由系统事件驱动

1.4.2 半同步/半异步模式工作流程
(1) 同步线程用于处理客户逻辑
(2) 异步线程用于处理I/O事件
(3) 异步线程监听到客户请求后，就将其封装成请求对象并插入请求队列中
(4) 请求队列将通知某个工作在同步模式的工作线程来读取并处理该请求对象

1.4.3半同步/半反应堆工作流程（以Proactor模式为例）
(1) 主线程充当异步线程，负责监听所有socket上的事件
(2) 若有新请求到来，主线程接收之以得到新的连接socket，然后往epoll内核事件表中注册该socket上的读写事件
(3) 如果连接socket上有读写事件发生，主线程从socket上接收数据，并将数据封装成请求对象插入到请求队列中
(4) 所有工作线程睡眠在请求队列上，当有任务到来时，通过竞争（如互斥锁）获得任务的接管权





