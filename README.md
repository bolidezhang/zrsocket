# zrsocket
an asynchronous event-driven network framework (c++11)

It simplifies the development of OO socket network applications and services that utilize socket communication, 
socket event demultiplexing, concurrency. 
zrsockert is targeted for developers of high-performance socket network communication services 
and applications on Window, Linux platforms.

http test result:
          4 threads and 100 connections
          Thread Stats   Avg      Stdev     Max   +/- Stdev
            Latency   321.46us    0.98ms  17.40ms   97.08%
            Req/Sec    96.87k     1.55k  100.39k    84.50%
          Latency Distribution
             50%  193.00us
             75%  236.00us
             90%  265.00us
             99%    5.72ms
          3854836 requests in 10.01s, 279.40MB read
        Requests/sec: 385248.64
