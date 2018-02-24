## 功能介绍
本程序是为了展示，如何让一个tcp连接在两个独立的进程之间进行迁移。

### 应用场景
很多网络程序在进行热升级的时候，针对tcp的长连接，必须在老的进程里close掉后，然后再新的进程里重新连接才能使用。
因此，不可避免的造成用户的一次闪断，无法做的用户无感知。

## 设计思路

### 基本原理
    本程序的原理在apue里已经有很详细的描述。详见apue的第17.4小节。
    或者这篇文章
    http://poincare.matf.bg.ac.rs/~ivana/courses/ps/sistemi_knjige/pomocno/apue/APUE/0201433079/ch17lev1sec4.html

    原理可以简单概括为：

    用户空间的每个fd。这个fd是个整数，对应的是一个file描述符指针数组元素的下标。
    这个file描述符指针数组是每个进程下，用来记录打开的文件的列表。

    以tcp连接为例，每个socket对应一个文件描述符。

    借助内核的帮助， 我们让两个进程的fd（fd不要求一样），指向同一个file描述符。
    而这个文件描述符，就是对应我们要迁移的tcp连接的文件描述符。

    一句话总结：换个马甲。

### 与tcp repair的比较
    有的同学也许会使用tcp repair完成同样的工作。两者各有千秋。

    tcp repair是为了跨物理机迁移做的，因此在本机热升级的时候，也是适用的。
    tcp repair的思路是把tcp socket的内核层面所有的必要的socket信息，全部dump出来，
    然后把这些信息注入到另外一个新的tcp socket上。相对来说动作和延迟都会比较高。

    一句话总结：借尸还魂。

### 具体实现
    借助unix socket在两个进程之间建立一个channel。
    发送端调用send_fd将被发送的fd传送给对端，返回0表示成功，否则表示失败。
    对端在channlesocket等待可读信号，只有调用recv_fd获取对端传送过来的new fd。
    注意：new fd是想说明，接收端进程里获得的fd跟发送端进场的fd不只是指向同一个tcp连接，
    但是fd的值是各自独立的，没有必然联系，大多数情况下是不一样的。

### 接口
    接口非常简单，只有两个。

+ send_fd
```
int send_fd(int fd, int fd_to_send);
```
    在发送端调用，两个参数:
        fd: 两个进程间的channle描述符（即 unix socket fd）。
        fd_to_send: 即将被迁移的tcp 连接的fd
    返回值：
        0： 发送成功。
        其他：失败。

+ recv_fd
```
int recv_fd(int fd);
```
    fd: 两个进程间的channle描述符（即 unix socket fd）。
    返回值：
        >0: 迁移成功，返回值即为 tcp socket在本进程的fd。
        其他： 失败。

### 验证方法

使用三个进程，分别记为 tcp_server， unix_socket_server， unix_socket_client.
在进程里unix_socket_client里创建一个tcp client的socket，然后把这个socket迁移到unix_socket_server。
tcp server就是个监听127.0.0.1：9999服务端口，只是用来显示client端发送过来的文本消息，

两个进程通过tcp socket发送到tcp serve的消息是不同的。
tcp client1发送的是`Test message from tcp client to python server!!!`
tcp client2发送的是`Migrate to new Process`

#### 测试步骤
1. make： 编译二进制文件得到 unix_socket_server 和unix_socket_client两个二进制文件
2. python tcp_server.py：启动tcp server进程。
3. ./unix_socket_server：启动unix_socket_server进程，等待接受被迁移的tcp socket。
4. ./unix_socket_client：启动unix_socket_client进程，创建一个tcp socket，并连接到tcp_server后，
      将该进程发送迁移消息给unix_socket_server

预期结果：
1. unix_socket_client
```
[]# ./unix_socket_client
tcp socket (fd: 3) has benn sent to perr Success !
```
fd: 3  表示tcp socket的fd在unix_socket_client是3， 你的测试可能是其他值。只要是正整数就没问题。


2. unix_socket_server
```
[]# ./unix_socket_server
Socket has CHANNEL_NAME channel_to_migrate_socket   <== 启动后打印
Peer's IP address is: 127.0.0.1:9999                <== 迁移成功
test_tcp_sock: write 22 bytes OK.                   <== 每隔1s往tcp server发送22字节的文本消息
test_tcp_sock: write 22 bytes OK.
test_tcp_sock: write 22 bytes OK.
test_tcp_sock: write 22 bytes OK.
test_tcp_sock: write 22 bytes OK.
test_tcp_sock: write 22 bytes OK.
test_tcp_sock: write 22 bytes OK.
test_tcp_sock: write 22 bytes OK.
test_tcp_sock: write 22 bytes OK.
test_tcp_sock: write 22 bytes OK.
```

3. tcp_server
```
[]# python tcp_server.py
127.0.0.1 wrote:
Test message from tcp client to python server!!!   <== 来自unix_socket_client的测试消息
127.0.0.1 wrote:
Migrate to new Process  <== 迁移成功后，每隔1秒来自unix_socket_server进程的测试消息
127.0.0.1 wrote:
Migrate to new Process
127.0.0.1 wrote:
Migrate to new Process
127.0.0.1 wrote:
Migrate to new Process
127.0.0.1 wrote:
Migrate to new Process
127.0.0.1 wrote:
Migrate to new Process
127.0.0.1 wrote:
Migrate to new Process
127.0.0.1 wrote:
Migrate to new Process
127.0.0.1 wrote:
Migrate to new Process
127.0.0.1 wrote:
Migrate to new Process
```
