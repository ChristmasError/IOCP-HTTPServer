#pragma once

#include "../MemoryPool/MemoryPool.h"
#include "PerSocketContext.h"

//========================================================
//                 
//		  SocketContextPool������SocketContext��
//
//========================================================

class SERVERDATASTRUCTURE_API SocketContextPool
{
private:
	MemoryPool<_PER_SOCKET_CONTEXT, 102400> SocketPool;
	unsigned int nConnectionSocket;

	CSLock m_csLock;
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
