#pragma once
#include<WinSocket.h>		//��װ�õ�socket���,��Ŀ���������dllλ������Ŀ¼��lib�ļ�����
#include<WinSock2.h>
#include<Windows.h>
#include<MSWSock.h>

#include<vector>
#include<string>
#include<list>

#include<iostream>
#include"XHttpResponse.h"

#pragma comment(lib, "Kernel32.lib")	// IOCP��Ҫ�õ��Ķ�̬���ӿ�

// ����������״̬
#define RUNNING true
#define STOP    false
#define EX      true

// DATABUFĬ�ϴ�С
#define BUF_SIZE 1024
// ���ݸ�Worker�̵߳��˳��ź�
#define WORK_THREADS_EXIT NULL
// ͬʱͶ�ݵ�Accept����
#define MAX_POST_ACCEPT (10)
// IOContextPool�еĳ�ʼ����
#define INIT_IOCONTEXT_NUM (100)				
// Ĭ�϶˿�
#define DEFAULT_PORT 8888
// Ĭ��IP
#define DEFAULT_IP "10.11.147.70"
// �˳���־
#define EXIT_CODE (-1)

// �ͷž��
inline void RELEASE_HANDLE(HANDLE handle)
{
	if ( handle != NULL && handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(handle);
		handle = NULL;
	}
}
// �ͷ�ָ��
inline void RELEASE_POINT(void* point)
{
	if (point != NULL)
	{
		delete point;
		point = NULL;
	}
}

//========================================================
// ����ɶ˿���Ͷ�ݵ�I/O����������
//========================================================
typedef enum _OPERATION_TYPE
{
	ACCEPT_POSTED,		// ��־Ͷ�ݵ��ǽ��ղ���
	RECV_POSTED,		// ��־Ͷ�ݵ��ǽ��ղ���
	SEND_POSTED,		// ��־Ͷ�ݵ��Ƿ��Ͳ���
	NULL_POSTED			// ��ʼ����
} OPERATION_TYPE;

//========================================================
// ��IO���ݽṹ�嶨��(����ÿһ���ص������Ĳ���)
//========================================================
typedef struct _PER_IO_DATA
{
	WSAOVERLAPPED	m_Overlapped;					// OVERLAPPED�ṹ���ýṹ�����һ��event�¼�����,������ڽṹ����λ����Ϊ�׵�ַ
	SOCKET			m_AcceptSocket;						// ��IO������Ӧ��socket
	WSABUF			m_wsaBuf;						// WSABUF�ṹ��������Ա��һ��ָ��ָ��buf����һ��buf�ĳ���len
	char			m_buffer[BUF_SIZE];			    // WSABUF�����ַ�������
	OPERATION_TYPE           m_OpType;
	//OPERATION_TYPE  m_OpType;						// ��־λ

	// ��ʼ��
	_PER_IO_DATA()
	{
		ZeroMemory(&(m_Overlapped), sizeof(WSAOVERLAPPED));
		ZeroMemory(m_buffer, BUF_SIZE);
		m_wsaBuf.buf = m_buffer;
		m_wsaBuf.len = BUF_SIZE;
		m_OpType = NULL_POSTED;
	}
	// �ͷ�
	~_PER_IO_DATA()
	{

	}
	// ����
	void Reset()
	{
		ZeroMemory(&m_Overlapped, sizeof(WSAOVERLAPPED));
		if (m_wsaBuf.buf != NULL)
			ZeroMemory(m_wsaBuf.buf, BUF_SIZE);
		else
			m_wsaBuf.buf= (char *)::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUF_SIZE);
		m_OpType = NULL_POSTED;
		ZeroMemory(m_buffer, BUF_SIZE);
	}
} PER_IO_DATA, *LPPER_IO_DATA;
//========================================================
// IOContextPool
//========================================================
class IOContextPool
{
private:
	list<PER_IO_DATA *> contextList;
	CRITICAL_SECTION csLock;

public:
	IOContextPool()
	{
		InitializeCriticalSection(&csLock);
		contextList.clear();

		EnterCriticalSection(&csLock);
		for (size_t i = 0; i < INIT_IOCONTEXT_NUM; i++)
		{
			PER_IO_DATA *context = new PER_IO_DATA;
			contextList.push_back(context);
		}
		LeaveCriticalSection(&csLock);

	}

	~IOContextPool()
	{
		EnterCriticalSection(&csLock);
		for (list<PER_IO_DATA *>::iterator it = contextList.begin(); it != contextList.end(); it++)
		{
			delete (*it);
		}
		contextList.clear();
		LeaveCriticalSection(&csLock);

		DeleteCriticalSection(&csLock);
	}

	// ����һ��IOContxt
	PER_IO_DATA *AllocateIoContext()
	{
		PER_IO_DATA *context = NULL;

		EnterCriticalSection(&csLock);
		if (contextList.size() > 0) //list��Ϊ�գ���list��ȡһ��
		{
			context = contextList.back();
			contextList.pop_back();
		}
		else	//listΪ�գ��½�һ��
		{
			context = new PER_IO_DATA;
		}
		LeaveCriticalSection(&csLock);

		return context;
	}

	// ����һ��IOContxt
	void ReleaseIOContext(PER_IO_DATA *pContext)
	{
		pContext->Reset();
		EnterCriticalSection(&csLock);
		contextList.push_front(pContext);
		LeaveCriticalSection(&csLock);
	}
};
//========================================================
// ��IO���ݽṹ�ⶨ��(ÿһ���ͻ���socket������
//========================================================
typedef struct _PER_HANDLE_DATA
{	
private:
	vector<PER_IO_DATA*>	arrIoContext;		  // ͬһ��socket�ϵĶ��IO����
	static IOContextPool    ioContextPool;		  //���е�IOcontext����
	//CRITICAL_SECTION		csLock;
	//vector<PER_IO_DATA*>    m_vecIoContex;  //ÿ��socket���յ�������IO��������
public:
	WinSocket				m_Sock;		      //ÿһ��socket

	// ��ʼ��
	_PER_HANDLE_DATA()
	{
		m_Sock.CreateSocket();
	}
	// �ͷ���Դ
	~_PER_HANDLE_DATA()
	{
	}

	// ��ȡһ���µ�IO_DATA
	PER_IO_DATA *GetNewIOContext()
	{
		PER_IO_DATA *context = ioContextPool.AllocateIoContext();
		if (context != NULL)
		{
			//EnterCriticalSection(&csLock);
			arrIoContext.push_back(context);
			//LeaveCriticalSection(&csLock);
		}
		return context;
	}
	// ���������Ƴ�һ��ָ����IoContext
	void RemoveIOContext(PER_IO_DATA* pContext)
	{
		for (vector<PER_IO_DATA*>::iterator it = arrIoContext.begin(); it != arrIoContext.end(); it++)
		{
			if (pContext == *it)
			{
				ioContextPool.ReleaseIOContext(*it);

				//EnterCriticalSection(&csLock);
				arrIoContext.erase(it);
				//LeaveCriticalSection(&csLock);

				break;
			}
		}
	}
} PER_HANDLE_DATA,*LPPER_HANDLE_DATA;

//========================================================
//					IOCPModel�ඨ��
//========================================================
class IOCPModel
{
public:
	// ����������Դ��ʼ��
	IOCPModel()
	{
		m_ListenSockInfo = NULL;
		useAcceptEx = false;
		m_WorkerShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}
	~IOCPModel()
	{
		_Deinitialize();
	}
public:
	// ����������
	bool Start();
	bool StartEX();
	// �رշ�����
	//void Stop();

	// ��ʼ����������Դ
	// 1.��ʼ��Winsock����
	// 2.��ʼ��IOCP + ���������̳߳�
	// 3.�����������׽���
	// 4.accept()
	bool InitializeIOCPResource(bool ex);

	// ����Winsock��:public
	bool LoadSocketLib();

	// ж��Socket��
	bool UnloadSocketLib() 
	{ 
		WSACleanup(); 
	}

	// �¼�֪ͨ����(���������ش��庯��)
	//virtual void ConnectionEstablished(PER_HANDLE_DATA *handleInfo) = 0;
	//virtual void ConnectionClosed(PER_HANDLE_DATA *handleInfo) = 0;
	//virtual void ConnectionError(PER_HANDLE_DATA *handleInfo, int error) = 0;
	virtual void RecvCompleted(PER_HANDLE_DATA *handleInfo, PER_IO_DATA *ioInfo) = 0;
	//virtual void SendCompleted(PER_HANDLE_DATA *handleInfo, PER_IO_DATA *ioInfo) = 0;

private:
	// ����Winsock��:private
	bool _LoadSocketLib();

	// ��ʼ��IOCP:��ɶ˿��빤���߳��̳߳ش���
	bool _InitializeIOCP();

	// �����������׽���
	bool _InitializeServerSocket();
	bool _InitializeListenSocket();

	// �̺߳�����ΪIOCP������������߳�
	static DWORD WINAPI _WorkerThread(LPVOID lpParam);

	// ��ӡ��Ϣ
	void _ShowMessage(const char*, ...) const;

	// �ͷ�������Դ
	void _Deinitialize();

	// ����I/O����
	bool _DoAccept(PER_HANDLE_DATA* phd, PER_IO_DATA *pid);
	bool _DoRecv(PER_HANDLE_DATA* phd, PER_IO_DATA *pid);
	bool _DoSend(PER_HANDLE_DATA* phd, PER_IO_DATA *pid);

	// Ͷ��I/O����
	bool _PostAccept(PER_HANDLE_DATA* phd, PER_IO_DATA *pid);
	bool _PostRecv(PER_HANDLE_DATA* phd, PER_IO_DATA *pid);
	bool _PostSend(PER_HANDLE_DATA* phd, PER_IO_DATA *pid);

protected:
	bool						  useAcceptEx;					// ѡ��ʹ��AcceptEX()����

	bool						  _ServerRunning;				// �����������ж�

	WSADATA						  wsaData;						// Winsock�����ʼ��

	SYSTEM_INFO					  m_SysInfo;					// ����ϵͳ��Ϣ

	HANDLE					      m_WorkerShutdownEvent;		// ֪ͨ�߳�ϵͳ�Ƴ��¼�

	HANDLE						  m_IOCompletionPort;			// ��ɶ˿ھ��

	// AcceptEX()

	PER_HANDLE_DATA               *m_ListenSockInfo;			// ���ڼ���Context
	LPFN_ACCEPTEX				  fnAcceptEx;					// AcceptEx����ָ��
	LPFN_GETACCEPTEXSOCKADDRS	  fnGetAcceptExSockAddrs;		// GetAcceptExSockAddrs()����ָ��
	//LONG						  connectCnt;					// ��ǰ����������
	//LONG						  acceptPostCnt;				// ��ǰͶ�ݵĵ�Accept����

	// Accept()
	WinSocket					  m_ServerSocket;				// ��װ��socket���

	HANDLE*						  m_WorkerThreadsHandleArray;	// �������߳̾������

	int							  m_nThreads;				    // �����߳�����


};
