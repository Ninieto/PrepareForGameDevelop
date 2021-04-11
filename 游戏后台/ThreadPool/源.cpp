
#pragma once
#include <Windows.h>
#include <list>
#include <vector>
using std::list;
using std::vector;
/* 前置声明 */
class Task;
class ThreadPara;//传递给线程的参数
class ThreadPool
{
public:
	ThreadPool();
	ThreadPool(int maxNum);
	~ThreadPool();

	//线程池初始化
	void init();

	//线程池销毁
	void release();
	//增加任务
	void addTask(Task* task);

	//线程主函数
	static void ThreadFunc(void* arg);
private:
	typedef list<Task*> TaskList;
	TaskList m_taskList;//未处理的任务池
	HANDLE m_mutexTaskList;//任务池锁
	HANDLE m_semNewTask;//增加新任务信号
	HANDLE m_semDoneTask;//完成任务信号
	typedef vector<ThreadPara> ThreadList;
	ThreadList m_threadList;//线程集合
	HANDLE m_mutexThreadPools;//线程池锁
	int m_threadNum;//线程数目
	HANDLE m_semDestoryThreadPool;//线程池释放信号
	bool isInit;//是否初始化
};


#include "ThreadPara.h"
#include <errno.h>
#include <process.h>
ThreadPool::ThreadPool() :m_threadNum(10), isInit(false)
{
}
ThreadPool::ThreadPool(int maxNum) {
	m_threadNum = maxNum;
}
ThreadPool::~ThreadPool()
{
}
void ThreadPool::init() {
	if (isInit) {
		return;
	}
	//创建信号量和锁对象
	m_mutexTaskList = CreateMutex(NULL, FALSE, L"m_mutexTaskList");//如果是true，则当前线程已经获取该锁对象，一定要注意
	m_mutexThreadPools = CreateMutex(NULL, FALSE, L"m_mutexThreadPools");
	m_semNewTask = CreateSemaphore(NULL, 0, 100, L"m_semNewTask");
	m_semDoneTask = CreateSemaphore(NULL, 0, 100, L"m_semDoneTask");
	m_semDestoryThreadPool = CreateSemaphore(NULL, 0, 100, L"m_semDestoryThreadPool");

	if (m_mutexTaskList == 0||m_mutexThreadPools == 0
		 ||m_semNewTask == 0
		 ||m_semDoneTask == 0
		 ||m_semDestoryThreadPool == 0) {
		printf("Fatal Error:创建信号量和锁失败!\n");
		return;
	}
	//创建线程
	m_threadList.reserve(m_threadNum);
	for (int i = 0; i < m_threadNum; i++)
	{
		ThreadPara thradPara;
		thradPara.id = i + 1;
		thradPara.pool = this;
		thradPara.run = true;
		m_threadList.push_back(thradPara);
		ThreadPara* para = &m_threadList[i];
		m_threadList[i].hid = _beginthread(ThreadFunc, 0, (void*)para);
	}
}
void ThreadPool::addTask(Task* task)
{
	WaitForSingleObject(m_mutexTaskList, INFINITE);
	m_taskList.push_back(task);
	printf("add task%d\n", task->id);
	ReleaseMutex(m_mutexTaskList);
	//释放信号量
	ReleaseSemaphore(m_semNewTask, 1, NULL);
}

void ThreadPool::ThreadFunc(void* arg) {
	//参数信息
	ThreadPara* threadPara = (ThreadPara*)arg;
	ThreadPool* pool = threadPara->pool;
	printf("线程%d[%d]启动,pool=%x...\n", threadPara->id, threadPara->hid, pool);
	//需要等待的信号
	HANDLE hWaitHandle[2];
	hWaitHandle[0] = pool->m_semDestoryThreadPool;
	hWaitHandle[1] = pool->m_semNewTask;
	while (threadPara->run)
	{
		//等待信号
		DWORD val = WaitForMultipleObjects(2, hWaitHandle, false, INFINITE);

		if (val == WAIT_OBJECT_0) {//线程池销毁
			printf("线程%d退出...\n", threadPara->id);
			break;
		}
		//处理新任务
		WaitForSingleObject(pool->m_mutexTaskList, INFINITE);
		Task* task = *pool->m_taskList.begin();
		pool->m_taskList.pop_front();
		ReleaseMutex(pool->m_mutexTaskList);
		task->run();
		delete task;
		//释放信号量
		//ReleaseSemaphore(pool->m_semDoneTask, 1, NULL);
	}
}
void ThreadPool::release() {
	ReleaseSemaphore(m_semDestoryThreadPool, m_threadNum, NULL);
	m_threadList.clear();
	for (TaskList::iterator iter = m_taskList.begin(); iter != m_taskList.end(); iter++) {
		delete* iter;
	}
	m_taskList.clear();
	CloseHandle(m_mutexTaskList);
	CloseHandle(m_mutexThreadPools);
	CloseHandle(m_semDestoryThreadPool);
	CloseHandle(m_semDoneTask);
	CloseHandle(m_semNewTask);
}



#pragma once
class Task
{
public:
	Task(int id);
	~Task();
	virtual void run() = 0;
public:
	int id;
};

Task::Task(int id)
{
	this->id = id;
}

Task::~Task()
{
}







class MyTask : public Task
{
public:
	MyTask(int id) :Task(id) {}
	~MyTask() {}
	virtual void run() { printf("Task %d run...\n", id); Sleep(100); }
};

void main() {
	ThreadPool threadPool(10);
	threadPool.init();
	for (int i = 0; i < 20; i++) {
		Task* task = new MyTask(i + 1);
		threadPool.addTask(task);
	}
	Sleep(1000);
	threadPool.release();
	printf("release threadpool\n");
	getchar();
}

