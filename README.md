zrsocket

一个非常优秀的Reactor模式的高性能网络框架

特性

    1.eventloop
        支持 epoll,select等        
    2.tcp 和 udp    
        tcpclient/tcpserver/udp
    3.http
        http client/http server
        经测试可达40万qps
    4.seda
        seda事件处理模式
        经测试每秒可达20000万以上事件处理
    5.log
        支持同步和异步的高性能日志
        经测试每秒写入可达500万以上条日志消息(每条300个字节)
    6.其他
        无锁队列lockfree_queue
            spsc_queue/mpmc_queue/mpsc_queue/spmc_queue
        测量计数器(MeasureCounter)
        跨平台api        
        ......
    
支持平台

    1.linux
    2.window
