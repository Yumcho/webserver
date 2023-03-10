1. 基础概念
1.1 同步日志/异步日志
同步日志:日志写入函数与工作线程串行执行，由于涉及到I/O操作，当单条日志比较大的时候，同步模式会阻塞整个处理流程，服务器所能处理的并发能力将有所下降，尤其是在峰值的时候，写日志可能成为系统的瓶颈。
异步日志:将所写的日志内容先存入阻塞队列，写线程从阻塞队列中取出内容，写入日志。

1.2 单例模式(懒汉/饿汉模式)
单例模式:单例模式作为最常用的设计模式之一，保证一个类仅有一个实例，并提供一个访问它的全局访问点，该实例被所有程序模块共享。
实现思路：私有化它的构造函数，以防止外界创建单例类的对象；使用类的私有静态指针变量指向类的唯一实例，并用一个公有的静态方法获取该实例。
单例模式有两种实现方法，分别是懒汉和饿汉模式。顾名思义，懒汉模式，即非常懒，不用的时候不去初始化，所以在第一次被使用时才进行初始化；饿汉模式，即迫不及待，在程序运行时立即初始化。

不实用饿汉模式：编译器原因，非静态变量的初始化顺序未知，如果在创建静态实例之前还未初始化这些变量，将导致错误

2. 整体概述
本项目中，使用单例模式创建日志系统，对服务器运行状态、错误信息和访问数据进行记录，该系统可以实现按天分类，超行分类功能，可以根据实际情况分别使用同步和异步写入两种方式。
其中异步写入方式，将生产者-消费者模型封装为阻塞队列，创建一个写线程，工作线程将要写的内容push进队列，写线程从队列中取出内容，写入日志文件。
日志系统大致可以分成两部分，其一是单例模式与阻塞队列的定义，其二是日志类的定义与使用。


3. 基础API
3.1 fputs
#include <stdio.h>
int fputs(const char *str, FILE *stream);
fputs功能是向指定的文件写入一个字符串（不自动写入字符串结束标记符‘\0’）。成功写入一个字符串后，文件的位置指针会自动后移，函数返回值为非负整数；否则返回EOF(符号常量，其值为-1)。
str，一个数组，包含了要写入的以空字符终止的字符序列。
stream，指向FILE对象的指针，该FILE对象标识了要被写入字符串的流。

3.2 可变参数宏__VA_ARGS__
__VA_ARGS__是一个可变参数的宏，定义时宏定义中参数列表的最后一个参数为省略号，在实际使用时会发现有时会加##，有时又不加。

#define my_print3(format, ...) printf(format, ##__VA_ARGS__)
_VA_ARGS__宏前面加上##的作用在于，当可变参数的个数为0时，这里printf参数列表中的的##会把前面多余的","去掉，否则会编译出错，建议使用后面这种，使得程序更加健壮。

3.3 fflush
#include <stdio.h>
int fflush(FILE *stream);
fflush()会强迫将缓冲区内的数据写回参数 stream 指定的文件中，如果参数 stream 为NULL，fflush()会将所有打开的文件数据更新。
在使用多个输出函数连续进行多次输出到控制台时，有可能下一个数据再上一个数据还没输出完毕，还在输出缓冲区中时，下一个printf就把另一个数据加入输出缓冲区，结果冲掉了原来的数据，出现输出错误。
在prinf()后加上fflush(stdout); 强制马上输出到控制台，可以避免出现上述错误。


printf甚至fputs等等函数，其实他们只是把字符串压入缓冲区了就返回了，至于缓冲区对应的去显示或者写文件等耗时操作都是其他地方执行的，并非同步执行完才往下执行，所以这个时候的fflush就有了意义

