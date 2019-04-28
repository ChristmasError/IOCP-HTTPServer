#pragma once

#include<HttpResponse.h>
#include<WinSock.h>		//��װ�õ�socket���,��Ŀ���������dllλ������Ŀ¼��lib�ļ�����

#include<Windows.h>
#include<MSWSock.h>
#include<vector>
#include<list>
#include<thread>
#include<iostream>

// ����������״̬
#define RUNNING true
#define STOP    false

// DATABUFĬ�ϴ�С
#define BUF_SIZE 102400
// ���ݸ�Worker�̵߳��˳��ź�
#define WORK_THREADS_EXIT NULL
// ͬʱͶ�ݵ�Accept����
#define MAX_POST_ACCEPT (2000)
// IOContextPool�еĳ�ʼ����
#define INIT_IOCONTEXT_NUM (100)				
// Ĭ�϶˿�
#define DEFAULT_PORT 8888
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

//====================================================================================
//
//				��IO���ݽṹ�嶨��(����ÿһ���ص������Ĳ���)
//
//====================================================================================
typedef struct _PER_IO_CONTEXT
{
	WSAOVERLAPPED	m_Overlapped;					// OVERLAPPED�ṹ���ýṹ�����һ��event�¼�����,������ڽṹ����λ����Ϊ�׵�ַ
	SOCKET			m_AcceptSocket;					// ��IO������Ӧ��socket
	WSABUF			m_wsaBuf;						// WSABUF�ṹ��������Ա��һ��ָ��ָ��buf����һ��buf�ĳ���len
	char			m_buffer[BUF_SIZE];			    // WSABUF�����ַ�������
	OPERATION_TYPE  m_OpType;						// ��־λ

	// ��ʼ��
	_PER_IO_CONTEXT()
	{
		ZeroMemory(&(m_Overlapped), sizeof(WSAOVERLAPPED));
		ZeroMemory(m_buffer, BUF_SIZE);
		m_AcceptSocket = INVALID_SOCKET;
		m_wsaBuf.buf = m_buffer;
		m_wsaBuf.len = BUF_SIZE;
		m_OpType = NULL_POSTED;
	}
	// �ͷ�
	~_PER_IO_CONTEXT()
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

} PER_IO_CONTEXT, *LPPER_IO_CONTEXT;
//========================================================
// IOContextPool
//========================================================
class IOContextPool
{
private:
	list<LPPER_IO_CONTEXT> contextList;
	CRITICAL_SECTION csLock;

public:
	IOContextPool()
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

	}

	~IOContextPool()
	{
		EnterCriticalSection(&csLock);
		for (list<LPPER_IO_CONTEXT>::iterator it = contextList.begin(); it != contextList.end(); it++)
		{
			delete (*it);
		}
		contextList.clear();
		LeaveCriticalSection(&csLock);

		DeleteCriticalSection(&csLock);
	}

	// ����һ��IOContxt
	LPPER_IO_CONTEXT AllocateIoContext()
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

	// ����һ��IOContxt
	void ReleaseIOContext(LPPER_IO_CONTEXT pContext)
	{
		pContext->Reset();
		EnterCriticalSection(&csLock);
		contextList.push_front(pContext);
		LeaveCriticalSection(&csLock);
	}
};
//====================================================================================
//
//				��������ݽṹ�嶨��(����ÿһ����ɶ˿ڣ�Ҳ����ÿһ��Socket�Ĳ���)
//
//====================================================================================
typedef struct _PER_SOCKET_CONTEXT
{	
private:
	vector<LPPER_IO_CONTEXT>	arrIoContext;		  // ͬһ��socket�ϵĶ��IO����
	static IOContextPool		ioContextPool;		  //���е�IOcontext����

public:
	WinSock					m_Sock;		          //ÿһ��socket����Ϣ

	// ��ʼ��
	_PER_SOCKET_CONTEXT()
	{
		m_Sock.socket = INVALID_SOCKET;
	}
	// �ͷ���Դ
	~_PER_SOCKET_CONTEXT()
	{
	}
	// ��ȡһ���µ�IO_DATA
	LPPER_IO_CONTEXT GetNewIOContext()
	{
		LPPER_IO_CONTEXT context = ioContextPool.AllocateIoContext();
		if (context != NULL)
		{
			//EnterCriticalSection(&csLock);
			arrIoContext.push_back(context);
			//LeaveCriticalSection(&csLock);
		}
		return context;
	}
	// ���������Ƴ�һ��ָ����IoContext
	void RemoveIOContext(LPPER_IO_CONTEXT pContext)
	{
		for (vector<LPPER_IO_CONTEXT>::iterator it = arrIoContext.begin(); it != arrIoContext.end(); it++)
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

} PER_SOCKET_CONTEXT,*LPPER_SOCKET_CONTEXT;

//====================================================================================
//
//								IOCPModel�ඨ��
//
//====================================================================================
class IOCPModel
{
public:
	// ����������Դ��ʼ��
	IOCPModel():m_ServerRunning(STOP),
				m_hIOCompletionPort(INVALID_HANDLE_VALUE),
				m_phWorkerThreadArray(NULL),
				m_ListenSockInfo(NULL),
				m_lpfnAcceptEx(NULL),
				m_lpfnGetAcceptExSockAddrs(NULL),
				m_nThreads(0)
	{
		if (_LoadSocketLib() == true)
			this->_ShowMessage("����Winsock��ɹ���\n");
		else
			// ����ʧ�� �׳��쳣
		// ��ʼ���˳��߳��¼�
		m_hWorkerShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	}
	~IOCPModel()
	{
		_Deinitialize();
	}
	// �������رշ�����
	bool StartServer();
	void StopServer();	

	// �¼�֪ͨ����(���������ش��庯��)
	//virtual void ConnectionEstablished(PER_HANDLE_DATA *handleInfo) = 0;
	//virtual void ConnectionClosed(PER_HANDLE_DATA *handleInfo) = 0;
	//virtual void ConnectionError(PER_HANDLE_DATA *handleInfo, int error) = 0;
	virtual void RecvCompleted(PER_SOCKET_CONTEXT *handleInfo, LPPER_IO_CONTEXT ioInfo) = 0;
	virtual void SendCompleted(PER_SOCKET_CONTEXT *handleInfo, LPPER_IO_CONTEXT ioInfo) = 0;

	bool PostSend(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT IoInfo)
	{
		return _PostSend( SocketInfo, IoInfo );
	}
private:
	// ����������
	bool _Start();

	// ��ʼ����������Դ
	// 1.��ʼ��Winsock����
	// 2.��ʼ��IOCP + ���������̳߳�
	bool _InitializeServerResource();

	// �ͷŷ�������Դ
	void _Deinitialize();

	// ����Winsock��:private
	bool _LoadSocketLib();

	// ж��Socket��
	bool _UnloadSocketLib()
	{
		WSACleanup();
	}

	// ��ʼ��IOCP:��ɶ˿��빤���߳��̳߳ش���
	bool _InitializeIOCP();

	// ���ݷ���������ģʽ�����������������׽���
	bool _InitializeListenSocket();

	// �̺߳�����ΪIOCP������������߳�
	static DWORD WINAPI _WorkerThread(LPVOID lpParam);

	// ��ӡ��Ϣ
	void _ShowMessage(const char*, ...) const;

	// ��ñ����д�����������
	int _GetNumberOfProcessors();

	// ����I/O����
	bool _DoAccept(LPPER_IO_CONTEXT pid);
	bool _DoRecv(LPPER_SOCKET_CONTEXT phd, LPPER_IO_CONTEXT pid);
	bool _DoSend(LPPER_SOCKET_CONTEXT phd, LPPER_IO_CONTEXT pid);

	// Ͷ��I/O����
	bool _PostAccept(LPPER_IO_CONTEXT pid);
	bool _PostRecv(LPPER_SOCKET_CONTEXT phd, LPPER_IO_CONTEXT pid);
	bool _PostSend(LPPER_SOCKET_CONTEXT phd, LPPER_IO_CONTEXT pid);

protected:															
	bool						  m_ServerRunning;				// ����������״̬�ж�

	SYSTEM_INFO					  m_SysInfo;					// ����ϵͳ��Ϣ

	HANDLE						  m_hIOCompletionPort;			// ��ɶ˿ھ��

	HANDLE					      m_hWorkerShutdownEvent;		// ֪ͨ�߳�ϵͳ�Ƴ��¼�

	HANDLE						  *m_phWorkerThreadArray;       // �����̵߳ľ��ָ��

	PER_SOCKET_CONTEXT            *m_ListenSockInfo;			// ������ListenContext

	LPFN_ACCEPTEX				  m_lpfnAcceptEx;				// AcceptEx����ָ��
	LPFN_GETACCEPTEXSOCKADDRS	  m_lpfnGetAcceptExSockAddrs;	// GetAcceptExSockAddrs()����ָ��

	int							  m_nThreads;				    // �����߳�����
};
