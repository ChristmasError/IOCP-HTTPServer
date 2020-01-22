#pragma once

#include "IOContextPool.h"

// IOContextPool�еĳ�ʼ����
#define INIT_IOCONTEXTPOOL_NUM (500)

IOContextPool::IOContextPool()
{
	contextList.clear();
	m_nFreeIoContext = INIT_IOCONTEXTPOOL_NUM;
	m_nActiveIoContext = 0;
	CSAutoLock lock(m_csLock);
	for (size_t i = 0; i < INIT_IOCONTEXTPOOL_NUM; i++)
	{
		LPPER_IO_CONTEXT context = new PER_IO_CONTEXT;
		contextList.push_back(context);
	}

	std::cout << "IOContextPool��ʼ�����...\n";
}

IOContextPool::~IOContextPool()
{
	CSAutoLock lock(m_csLock);

	for (std::list<LPPER_IO_CONTEXT>::iterator it = contextList.begin(); it != contextList.end(); it++)
	{
		delete (*it);
	}
	contextList.clear();

}

LPPER_IO_CONTEXT IOContextPool::AllocateIoContext()
{
	LPPER_IO_CONTEXT context = NULL;

	CSAutoLock lock(m_csLock);
	if (contextList.size() > 0) //list��Ϊ�գ���list��ȡһ��
	{
		context = contextList.back();
		contextList.pop_back();
	}
	else	//listΪ�գ��½�һ��
	{
		context = new PER_IO_CONTEXT;
	}
	m_nActiveIoContext++;
	m_nFreeIoContext--;
	return context;
}

void IOContextPool::ReleaseIOContext(LPPER_IO_CONTEXT pIOContext)
{	
	pIOContext->Reset();
	CSAutoLock lock(m_csLock);
	this->contextList.push_front(pIOContext);
	m_nActiveIoContext--;
	m_nFreeIoContext++;
}

void IOContextPool::ShowIOContextPoolInfo()
{
	printf("IoContextPool Info: number of ActiveIoContext:%d,number of FreeIoContext:%d\n",m_nActiveIoContext,m_nFreeIoContext);
}