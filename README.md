http server

基于同步IO模拟的Proactor模式  

事件处理流程：
主线程负责事件分发与IO操作，主线程先注册socket之上的读就绪事件而后调用epoll_wait等待socket数据可读。

读操作完成后主线程将该链接对象插入线程池中的请求队列，某个工作线程被唤醒，工作线程进行解析、处理HTTP Request，生成HTTP Response包等工作，向epoll内核事件表注册可写事件。当socket可写时，epoll_wait通知主线程发送Response至客户端。

每个链接对应一个HTTPConnection对象，每个套接字描述符只由一个线程处理。
