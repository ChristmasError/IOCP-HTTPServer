#pragma once

#include "serverdatastructure_global.h"
#include "PerIOContext.h"
#include "IOContextPool.h"
#include "../WinSocket/WinSocket.h"

//====================================================================================
//
//				��������ݽṹ�嶨��(����ÿһ����ɶ˿ڣ�Ҳ����ÿһ��Socket�Ĳ���)
//
//====================================================================================

typedef class SERVERDATASTRUCTURE_API _PER_SOCKET_CONTEXT
{
public:
	WinSocket	 m_Sock;		//ÿһ��socket����Ϣ
private:
	static IOContextPool			 ioContextPool;		  // ���е�IOcontext��
	CRITICAL_SECTION                 m_csLock;			  // �����¼�
	std::vector<LPPER_IO_CONTEXT>	 arrIoContext;		  // ͬһ��socket�ϵĶ��IO����

public:
	_PER_SOCKET_CONTEXT()
	{
		InitializeCriticalSection(&m_csLock);
		m_Sock.socket = INVALID_SOCKET;	
	}
	// �ͷ���Դ
	~_PER_SOCKET_CONTEXT()
	{
		for (std::vector<LPPER_IO_CONTEXT>::iterator it = arrIoContext.begin(); it != arrIoContext.end(); it++)
		{
			ioContextPool.ReleaseIOContext(*it);
		}
		EnterCriticalSection(&m_csLock);
		arrIoContext.clear();
		LeaveCriticalSection(&m_csLock);

		DeleteCriticalSection(&m_csLock);
	}
	// ��ȡһ���µ�IO_DATA
	LPPER_IO_CONTEXT GetNewIOContext()
	{
		LPPER_IO_CONTEXT context = ioContextPool.AllocateIoContext();
		if (context != NULL)
		{
			EnterCriticalSection(&m_csLock);
			arrIoContext.push_back(context);
			LeaveCriticalSection(&m_csLock);
		}
		return context;
	}
	// ���������Ƴ�һ��ָ����IoContext
	void RemoveIOContext(LPPER_IO_CONTEXT ioContext)
	{
		for (std::vector<LPPER_IO_CONTEXT>::iterator it = arrIoContext.begin(); it != arrIoContext.end(); it++)
		{
			if (ioContext == *it)
			{
				ioContextPool.ReleaseIOContext(*it);

				EnterCriticalSection(&m_csLock);
				arrIoContext.erase(it);
				LeaveCriticalSection(&m_csLock);
				break;
			}
		}
	}
	static void ShowIoContextPoolInfo()
	{
		ioContextPool.ShowIOContextPoolInfo();
	}
} PER_SOCKET_CONTEXT, *LPPER_SOCKET_CONTEXT;