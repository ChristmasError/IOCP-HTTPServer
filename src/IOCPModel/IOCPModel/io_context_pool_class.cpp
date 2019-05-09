#pragma once

#include"io_context_pool_class.h"
#include<iostream>
// IOContextPool�еĳ�ʼ����
#define INIT_IOCONTEXT_NUM (100)

IOContextPool::IOContextPool()
{
	InitializeCriticalSection(&csLock);
	contextList.clear();

	EnterCriticalSection(&csLock);
	for (size_t i = 0; i < INIT_IOCONTEXT_NUM; i++)
	{
		LPPER_IO_CONTEXT context = new PER_IO_CONTEXT;
		contextList.push_back(context);
	}
	LeaveCriticalSection(&csLock);
	std::cout << "IOContextPool ��ʼ�����\n";
}

IOContextPool::~IOContextPool()
{
	EnterCriticalSection(&csLock);
	for (std::list<LPPER_IO_CONTEXT>::iterator it = contextList.begin(); it != contextList.end(); it++)
	{
		delete (*it);
	}
	contextList.clear();
	LeaveCriticalSection(&csLock);

	DeleteCriticalSection(&csLock);
}

LPPER_IO_CONTEXT IOContextPool::AllocateIoContext()
{
	LPPER_IO_CONTEXT context = NULL;

	EnterCriticalSection(&csLock);
	if (contextList.size() > 0) //list��Ϊ�գ���list��ȡһ��
	{
		context = contextList.back();
		contextList.pop_back();
	}
	else	//listΪ�գ��½�һ��
	{
		context = new PER_IO_CONTEXT;
	}
	LeaveCriticalSection(&csLock);

	return context;
}

void IOContextPool::ReleaseIOContext(LPPER_IO_CONTEXT pContext)
{
	pContext->Reset();

	EnterCriticalSection(&csLock);
	contextList.push_front(pContext);
	LeaveCriticalSection(&csLock);
}