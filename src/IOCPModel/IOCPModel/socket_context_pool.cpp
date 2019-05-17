#pragma once

#include "socket_context_pool_class.h"

SocketContextPool::SocketContextPool()
{
	nConnectionSocket = 0;
	std::cout << "SocketContertPool ��ʼ�����!\n";
}

SocketContextPool::~SocketContextPool()
{

}

LPPER_SOCKET_CONTEXT SocketContextPool::AllocateSocketContext()
{
	CSAutoLock cs(m_csLock);
	LPPER_SOCKET_CONTEXT psocket = SocketPool.newElement();
	nConnectionSocket++;
	
	return psocket;
}

void SocketContextPool::ReleaseSocketContext(LPPER_SOCKET_CONTEXT pSocket)
{
	//CSAutoLock cs(m_csLock);
	SocketPool.deleteElement(pSocket);
	nConnectionSocket--;
}
unsigned int SocketContextPool::NumOfConnectingServer()
{
	CSAutoLock cs(m_csLock);
	return nConnectionSocket;
}