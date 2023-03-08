* 使用 **线程池 + 非阻塞socket + epoll(ET和LT均实现) + 事件处理(Reactor和模拟Proactor均实现)** 的并发模型
* 使用**状态机**解析HTTP请求报文，支持解析**GET和POST**请求
* 访问服务器数据库实现web端用户**注册、登录**功能，可以请求服务器**图片和视频文件**
* 实现**同步/异步日志系统**，记录服务器运行状态
* 经Webbench压力测试可以实现**上万的并发连接**数据交换

个性化运行
------

```C++
./server [-p port] [-l LOGWrite] [-m TRIGMode] [-o OPT_LINGER] [-s sql_num] [-t thread_num] [-c close_log] [-a actor_model] [-d container_type]
```

温馨提示:以上参数不是非必须，不用全部使用，根据个人情况搭配选用即可.

* -p，自定义端口号
	* 默认9006
* -l，选择日志写入方式，默认同步写入
	* 0，同步写入
	* 1，异步写入
* -m，listenfd和connfd的模式组合，默认使用LT + LT
	* 0，表示使用LT + LT
	* 1，表示使用LT + ET
    * 2，表示使用ET + LT
    * 3，表示使用ET + ET
* -o，优雅关闭连接，默认不使用
	* 0，不使用
	* 1，使用
* -s，数据库连接数量
	* 默认为8
* -t，线程数量
	* 默认为8
* -c，关闭日志，默认打开
	* 0，打开日志
	* 1，关闭日志
* -a，选择反应堆模型，默认Proactor
	* 0，Proactor模型
	* 1，Reactor模型
* -d，选择定时器容器类型，默认链表
	* 0，链表
	* 1，堆

测试示例命令与含义

```C++
./server -p 9007 -l 1 -m 0 -o 1 -s 10 -t 10 -c 1 -a 1 -d 0
```

- [x] 端口9007
- [x] 异步写入日志
- [x] 使用LT + LT组合
- [x] 使用优雅关闭连接
- [x] 数据库连接池内有10条连接
- [x] 线程池内有10条线程
- [x] 关闭日志
- [x] Reactor反应堆模型
- [x] 链表定时器容器

main()函数运行过程:
1. server.log_write()
    1. 如果开启日志，则使用单例模式创建日志对象并初始化日志对象，即Log::get_instance()->init（）
    2. m_log_write=1使用异步写：Log::init()中阻塞队列长度的参数为正，创建阻塞队列和异步写线程。异步写线程不断从阻塞队列中pop出string，如果队列为空，则线程会在pop()中被条件变量阻塞
    3. 生成带有时间的文件名用于记录日志
2. server.sql_pool()
    1. 使用单例模式，创建一个数据库连接池的静态对象
    2. 初始化数据库连接池，初始化内容包括数据库的地址url:port，用户名/密码，数据库名，连接的数量，日志开关
    3. 使用http_conn::initmysql_result将数据库中的<用户名,密码>对存入哈希表users中，用于检查账户
3. server.thread_pool()
    1. 创建线程池，所有线程运行worker()中的run()
    2. 线程不断从请求队列中取出类型为http_conn的request，request就是客户请求
    3. 如果是reactor模型，工作线程完成IO操作和业务逻辑操作
        1. m_state=0表示request为读。
            1. 读取数据。调用request->read_once()读取客户数据，直到无数据可读或对方关闭连接。如果是ET模式需要一次性读完客户socket上的所有数据。
            2. 处理数据。调用request->process()处理读缓冲区内的数据。
                1. 分析请求报文。调用process_read()读取请求行、请求头、请求体，并保存读取结果。
                    1. 如果请求报文未接受完，process_read()返回NO_REQUEST，继续注册客户socket可读事件，返回。
                    2. 如果读取到一个完整的请求报文，则process_read()调用do_request()分析请求报文，do_request()根据url中的标志位，判断客户请求的内容。
                2. 生成响应报文。调用process_write()，根据process_read()结果生成500、404、403、200响应报文。200响应报文包含响应体。
                3. 注册客户socket的可写事件
            3. 如果read_once()读取数据失败，则设timer_flag=1
            4. 完成读操作，无论是否读成功，都设imporv=1
        2. m_state=1表示request为写。
            1. 将保存在m_iv中的响应报文，发送至客户socket
            2. 如果中间出错，或者发送完毕且m_linger=false，则返回false，设timer_flag=1；如果未发送被打断，或者发送完毕且m_linger=true，则返回true
            3. 完成写操作，无论是否成功写，都设imporv=1
    4. 如果是proactor模型，主线程和内核完成IO操作，工作线程直接调用process()

4. server.trig_mode(）
	设置主线程和工作线程的ET模式

5. server.eventListen()
    1. 监听socket的标准创建流程
        1. 创建socket
        2. 命名socket
        3. 监听socket
    2. 统一事件源
        1. 创建内核事件表
        2. 注册监听socket上的可读事件
        3. 创建管道，捕获SIGALRM和SIGTERM信号后写入管道的写端socket，注册管道读端socket上的可读事件
    3. 设置一个闹钟，将于TIMESLOT秒后发送SIGALRM信号

6. sever.eventLoop()
    1. 如果未收到SIGTERM信号一直循环
    2. 调用epoll_wait等待注册的事件就绪
    3. 处理就需事件
        1. 如果是新到的客户连接，调用dealclientdata()，接收连接，得到客户连接的socket地址和分配给客户连接的socket connfd。使用数组users，数组编号为connfd的元素存储该客户连接对应的http_conn对象；使用数组users_timer，数组编号为connfd的元素存储用户连接和为该连接创建定时器。
        2. 如果发生错误或客户端关闭连接，则使用deal_timer()调用定时器中的cb_func()。从事件表中删除connfd，关闭connfd，从定时器容器中删除定时器
        3. 如果是信号到来，则调用dealwithsignal()。从管道读端m_pipefd[0]读取信号
            1. 如果是SIGALRM，设超时标志timeout=true
            2. 如果是SIGTERM，设主线程循环停止标志stop_server = true
        4. 如果客户连接上有数据可读，则调用dealwithread()
            1. 如果是reactor模式
                1. 延迟对应定时器
                2. 将读事件放入请求队列，并设m_state=0标记为读
                3. 进入循环，等待工作线程调用read_once()完成读操作后检测到improv=1，如果timer_flag=1表明读失败，则调用deal_timer()，设improv=0
            2. 如果是proactor模式
                1. 调用read_once()完成读操作
                    1. 若读成功，将读事件放入请求队列，延迟对应定时器
                    2. 若读失败，则调用deal_timer()
        5. 如果客户连接上有数据可写，则调用dealwithwrite()
            1. 如果是reactor模式
                1. 延迟对应定时器
                2. 将写事件放入请求队列，并设m_state=1标记为写
                3. 进入循环，等待工作线程调用write()完成写操作后检测到improv=1，如果timer_flag=1表明写失败，则调用deal_timer()，设improv=0
            2. 如果是proactor模式
                1. 调用write()完成写操作
                    1. 若写成功，延迟对应定时器
                    2. 若写失败，则调用deal_timer()

感谢项目 https://github.com/qinguoyi/TinyWebServer.git 的参考