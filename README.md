# tiny-nginx

## 项目介绍

仿nginx实现的服务器框架

## Ⅰ、线程池模块

### 1. 概述

#### 1.1 设计目标

基于 POSIX 线程（pthread）实现的轻量级线程池，主要功能包括：

- 动态线程生命周期管理
- 多线程任务调度
- 线程安全的任务队列管理
- 系统资源使用控制

#### 1.2 核心能力

| 功能模块 | 能力描述        | 关键技术实现              |
|------|-------------|---------------------|
| 线程管理 | 动态创建/销毁工作线程 | pthread_create/join |
| 任务调度 | 异步消息处理机制    | 条件变量通知机制            |
| 资源控制 | 线程规模限制与监控   | 原子计数器+时间戳记录         |
| 同步机制 | 并发访问控制      | 互斥锁+条件变量组合          |

### 2. 类架构设计

#### 2.1 CThreadPool 核心类

##### 2.1.1 类定义要点

- `CThreadPool`类是线程池的核心实现类。它包含了多个成员变量和成员函数，用于管理线程池的创建、运行、停止以及消息队列的操作等。

### 2.2 成员变量

#### 2.2.1 静态成员变量

- `pthread_mutex_t m_pthreadMutex`：用于线程同步的互斥锁，初始化为`PTHREAD_MUTEX_INITIALIZER`
  。它保证了在多线程环境下对共享资源（如消息队列）的互斥访问。

- `pthread_cond_t m_pthreadCond`：线程同步的条件变量，初始化为`PTHREAD_COND_INITIALIZER`。用于在线程之间进行条件通知和等待操作。
- `std::atomic<bool> m_shutDown`：表示线程池的关闭标志，初始化为`false`。原子类型确保了在多线程环境下对该变量的安全访问，不需要额外的锁操作。

#### 2.2.2 非静态成员变量

- `uint32_t m_threadCap`：线程池的容量，表示线程池中最多可以容纳的线程数量
- `std::atomic<uint32_t> m_runThreadCount`：记录当前正在运行的线程数量。原子类型保证了多线程环境下对该变量的正确更新。
- `time_t m_lastEmgTime`：记录上次线程不够用的时间，用于在特定场景下的时间判断。
- `std::vector<ThreadItem *> m_threadPool`：存储线程池中的线程项指针的向量，每个`ThreadItem`包含了线程相关的信息，如线程ID和线程状态等。
- `std::list<char *> m_msgRecvQueue`：接收数据消息队列，用于存储接收到的消息（以字符指针形式）。
- `uint32_t m_recvMsgQueueCount`：消息队列的大小，记录当前消息队列中消息的数量。

### 2.3 成员函数

#### 2.3.1 构造函数和析构函数

##### CThreadPool::CThreadPool()：

- 初始化`m_runThreadCount`为0，表示初始时没有运行的线程。
- 初始化`m_lastEmgTime`为0。
- 初始化`m_recvMsgQueueCount`为0。

##### CThreadPool::~CThreadPool()：

- 如果`m_shutDown`为`false`，调用`StopAll()`函数来停止所有线程并清理资源。
- 调用`ClearMsgRecvQueue()`函数来清除消息接收队列。

#### 2.3.2 线程池创建函数Create

##### bool CThreadPool::Create(int threadNum)：

- 功能：创建指定数量（`threadNum`）的线程并加入到线程池中。
- 流程：
    - 首先设置`m_threadCap`为传入的`threadNum`。
    - 循环`threadNum`次：
        - 创建一个`ThreadItem`对象，并将当前`CThreadPool`对象指针传递给它。
        - 调用`pthread_create`函数创建线程，线程执行函数为`ThreadFunc`，如果创建失败（`err!= 0`），返回`false`。
    - 然后遍历`m_threadPool`中的线程项：
        - 如果线程项的`isRunning`为`true`，表示线程已经正常启动并运行到`pthread_cond_wait()`，则继续检查下一个线程项。
        - 如果线程项的`isRunning`为`false`，则调用`usleep(100 * 1000)`等待100ms，确保每个线程都能正常启动并进入等待状态后，函数才返回
          `true`。

#### 2.3.3 线程池停止函数StopAll

##### void CThreadPool::StopAll()：

- 功能：停止并清理线程池中的所有线程，释放相关资源。
- 流程：
    - 如果`m_shutDown`已经为`true`，直接返回。
    - 调用`Pthread_mutex_lock`函数加锁。
    - 设置`m_shutDown`为`true`，由于是原子变量，不需要额外的锁保护。
    - 调用`Pthread_cond_broadcast`函数唤醒所有正在等待条件变量的线程。
    - 调用`Pthread_mutex_unlock`函数解锁。
    - 遍历`m_threadPool`中的线程项：
        - 调用`pthread_join`函数等待线程结束。
        - 删除`ThreadItem`对象。
    - 最后清空`m_threadPool`向量。

#### 2.3.4 消息队列操作函数

##### AddMsgRecvQueue函数：

- 函数签名：`void CThreadPool::AddMsgRecvQueue(char *buf)`。
- 功能：向消息接收队列中添加一条消息。
- 流程：
    - 调用`Pthread_mutex_lock`函数加锁。
    - 将传入的消息指针`buf`添加到`m_msgRecvQueue`列表中，并将`m_recvMsgQueueCount`加1。
    - 调用`Pthread_mutex_unlock`函数解锁。
    - 调用`Call`函数通知线程有新消息到达。

##### Call函数：

- 函数签名：`void CThreadPool::Call()`。
- 功能：唤醒一个等待条件变量的线程来处理消息队列中的消息。
- 流程：
    - 调用`Pthread_cond_signal`函数唤醒一个被`pthread_cond_wait`阻塞的线程。
    - 如果当前运行的线程数量`m_runThreadCount`等于线程池容量`m_threadCap`：
        - 获取当前时间`currTime`。
        - 如果`currTime - m_lastEmgTime > 10`，更新`m_lastEmgTime`为`currTime`，并且可以在这里添加一些处理无可用线程情况的代码（在
          `todo`处）。

#### 2.3.5 线程同步操作函数

##### Pthread_mutex_lock、Pthread_mutex_unlock、Pthread_cond_signal、Pthread_cond_wait和Pthread_cond_broadcast函数：

- 这些函数分别对互斥锁进行加锁、解锁操作，对条件变量进行发送信号、等待和广播操作。它们在内部调用了`pthread`
  库中的相关函数，并对函数调用失败的情况进行了简单的处理（在`todo`处可以进一步完善错误处理逻辑）。

#### 2.3.6 线程回调函数ThreadFunc

- 函数签名：`static void *CThreadPool::ThreadFunc(void *arg)`。
- 目前该函数返回`nullptr`，可能需要在后续完善该函数的实现，以实现线程在启动后的具体任务逻辑，例如从消息队列中获取消息并处理等。

#### 2.3.7 消息队列清理函数ClearMsgRecvQueue

- 函数签名：`void CThreadPool::ClearMsgRecvQueue()`。目前函数体为空，可能需要在后续实现对消息队列的清理逻辑，例如释放消息指针所指向的内存等。

## Ⅱ、配置文件

### 1. CConfig类设计

#### 1.1 单例模式

- 获取实例函数
    - `GetInstance`函数实现单例模式。
    - 通过定义静态局部变量`m_instance`，保证多线程环境下只创建一个`CConfig`类实例，并返回对该实例的引用。
- 拷贝和移动语义禁用
    - 将拷贝构造函数、拷贝赋值运算符、移动构造函数和移动赋值运算符都声明为`delete`，禁止对`CConfig`
      类对象的拷贝和移动操作，确保单例模式完整性，避免逻辑错误。

#### 2. 成员变量

- m_configItemMap

    - 类型为`std::unordered_map<std::string, std::string>`，用于存储配置项的键值对。
    - 键是配置项名称（`std::string`类型），值也是`std::string`类型，用于存储对应配置项的值。

- m_mutex

    - 类型为`std::mutex`，用于在多线程环境下保护对`m_configItemMap`的并发访问。

#### 3. 成员函数

- Load函数
    - **功能**：加载配置文件。
    - 操作：
        - 使用`std::lock_guard<std::mutex>`结合`m_mutex`确保多线程环境下配置文件加载操作的互斥性。
        - 调用`open`函数打开指定配置文件（通过传入的`pConfName`参数），若打开失败（`fd == -1`），需进行错误处理。
        - 成功打开文件后，要进行一系列操作来解析文件内容并将配置项存储到`m_configItemMap`中。
        - 最后调用`close`函数关闭文件，并返回`false`（返回值可能需根据实际加载成功与否调整）。
- GetString函数
    - **功能**：根据传入的配置项名称获取对应的字符串值。
    - 操作：
        - 同样使用`std::lock_guard<std::mutex>`保证并发安全。
        - 通过`m_configItemMap.find`函数查找传入的`pItemName`对应的迭代器。
        - 若找到（`iter!= m_configItemMap.end()`），则返回对应的`std::string`值的`c_str`版本（因为`m_configItemMap`存储
          `std::string`类型，外部可能需要`const char*`类型结果）；若未找到，则返回`nullptr`。
- GetIntDefault函数
    - **功能**：根据传入的配置项名称获取对应的整数值，若未找到则返回默认值0。
    - 操作：
        - 使用`std::lock_guard<std::mutex>`确保并发安全。
        - 通过`m_configItemMap.find`查找对应的迭代器。
        - 若找到，则使用`std::stoi`函数将找到的`std::string`值转换为`int32_t`类型并返回；若未找到，则返回默认值0。