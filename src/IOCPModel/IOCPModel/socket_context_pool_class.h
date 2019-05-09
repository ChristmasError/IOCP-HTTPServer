#pragma once

#include"memory_pool_class.h"
#include"memory_pool_class.cpp"
#include"per_socket_context_struct.h"
#include<iostream>

//========================================================
//                 
//		  SocketContextPool������SocketContext��
//
//========================================================

class SocketContextPool
{
private:
	MemoryPool<_PER_SOCKET_CONTEXT, 102400> SocketPool;
	unsigned int nConnectionSocket;

	CRITICAL_SECTION csLock;
public:
	SocketContextPool();
	~SocketContextPool();

	// ����һ���µ�SocketContext
	LPPER_SOCKET_CONTEXT AllocateSocketContext();

	// ����һ��SocketContext
	void ReleaseSocketContext(LPPER_SOCKET_CONTEXT pSocket);

	// ���ڴ�������״̬��SocketContext����
	unsigned int NumOfConnectingServer();
};
