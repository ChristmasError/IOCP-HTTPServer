#pragma once

#include "cs_auto_lock_class.h"
#include "per_io_context_struct.h"
#include "io_context_pool_class.h"
#include "winsock_class.h"
#include <vector>

//====================================================================================
//
//				��������ݽṹ�嶨��(����ÿһ����ɶ˿ڣ�Ҳ����ÿһ��Socket�Ĳ���)
//
//====================================================================================

typedef struct _PER_SOCKET_CONTEXT
{
private:
	static IOContextPool			 ioContextPool;		  // ���е�IOcontext��

	std::vector<LPPER_IO_CONTEXT>	 arrIoContext;		  // ͬһ��socket�ϵĶ��IO����
	CSLock	  m_csLock;

public: 
	WinSock	  m_Sock;		//ÿһ��socket����Ϣ

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
		CSAutoLock cs(m_csLock);
		arrIoContext.clear();
	}
	// ��ȡһ���µ�IO_DATA
	LPPER_IO_CONTEXT GetNewIOContext()
	{
		LPPER_IO_CONTEXT context = ioContextPool.AllocateIoContext();
		if (context != NULL)
		{
			CSAutoLock cs(m_csLock);
			arrIoContext.push_back(context);
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

				CSAutoLock cs(m_csLock);
				arrIoContext.erase(it);

				break;
			}
		}
	}

} PER_SOCKET_CONTEXT, *LPPER_SOCKET_CONTEXT;


