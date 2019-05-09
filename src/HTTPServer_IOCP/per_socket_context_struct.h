#pragma once

#include<per_io_context_struct.h>
#include<winsock_class.h>
#include<io_context_pool_class.h>
#include<vector>

//====================================================================================
//
//				��������ݽṹ�嶨��(����ÿһ����ɶ˿ڣ�Ҳ����ÿһ��Socket�Ĳ���)
//
//====================================================================================

typedef struct _PER_SOCKET_CONTEXT
{
private:
	static IOContextPool			 ioContextPool;		  // ���е�IOcontext��
private:
	std::vector<LPPER_IO_CONTEXT>	 arrIoContext;		  // ͬһ��socket�ϵĶ��IO����
	CRITICAL_SECTION				 csLock;
public:
	WinSock		m_Sock;		//ÿһ��socket����Ϣ

	// ��ʼ��
	_PER_SOCKET_CONTEXT()
	{
		m_Sock.socket = INVALID_SOCKET;
	}
	// �ͷ���Դ
	~_PER_SOCKET_CONTEXT()
	{
		for (auto it : arrIoContext)
		{
			ioContextPool.ReleaseIOContext(it);
		}
		EnterCriticalSection(&csLock);
		arrIoContext.clear();
		LeaveCriticalSection(&csLock);

		DeleteCriticalSection(&csLock);
	}
	// ��ȡһ���µ�IO_DATA
	LPPER_IO_CONTEXT GetNewIOContext()
	{
		LPPER_IO_CONTEXT context = ioContextPool.AllocateIoContext();
		if (context != NULL)
		{
			EnterCriticalSection(&csLock);
			arrIoContext.push_back(context);
			LeaveCriticalSection(&csLock);
		}
		return context;
	}
	// ���������Ƴ�һ��ָ����IoContext
	void RemoveIOContext(LPPER_IO_CONTEXT pContext)
	{
		for (std::vector<LPPER_IO_CONTEXT>::iterator it = arrIoContext.begin(); it != arrIoContext.end(); it++)
		{
			if (pContext == *it)
			{
				ioContextPool.ReleaseIOContext(*it);

				EnterCriticalSection(&csLock);
				arrIoContext.erase(it);
				LeaveCriticalSection(&csLock);

				break;
			}
		}
	}

} PER_SOCKET_CONTEXT, *LPPER_SOCKET_CONTEXT;


