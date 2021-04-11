#pragma once
class ThreadPool;
class ThreadPara {
public:
	int id;//线程序号
	int hid;//线程句柄号
	bool run;//运行标志
	ThreadPool* pool;//线程池
public:
	ThreadPara() :id(0), hid(0), run(false), pool(nullptr) {}
	~ThreadPara() {}
};