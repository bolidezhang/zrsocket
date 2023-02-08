zrsocket

一个非常优秀的Reactor模式的高性能网络框架

特性：
    1.event loop 
        支持 epoll,select等
    2.tcp 和 udp
    3.http
        http client/http server
        经测试可达40万qps
    4.seda
        seda事件处理模式
        经测试每秒高达20000万以上事件处理
    5.log
        支持同步和异步的高性能日志
        经测试每秒高达500万以上条(每条300个字节)

支持平台:
    1.windows    
    2.linux   
