#pragma once

#include<winsock_class.h>		//��װ�õ�socket���,��Ŀ���������dllλ������Ŀ¼��lib�ļ�����
#include<http_response_class.h>
#include<per_io_context_struct.cpp>
#include<per_socket_context_struct.cpp>
#include<socket_context_pool_class.h>

#include<MSWSock.h>


// ����������״̬
#define RUNNING true
#define STOP    false

// �ͷ�ָ��
inline void RELEASE_POINT(void* point)
{
	if (point != NULL)
	{
		delete point;
		point = NULL;
	}
}
// �ͷž��
inline void RELEASE_HANDLE(HANDLE handle)
{
	if (handle != NULL && handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(handle);
		handle = NULL;
	}
}

//====================================================================================
//
//								IOCPModel�ඨ��
//
//====================================================================================
class IOCPModel
{
public:
	// ����������Դ��ʼ��
	IOCPModel();
	~IOCPModel();
	
	// �������رշ�����
	bool StartServer();
	void StopServer();	

	// ��ӡ����(������)ip��ַ
	const char* GetLocalIP();

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

	static unsigned int NumOfConnectingServer()
	{
		return m_ServerSocketPool.NumOfConnectingServer();
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

	// ��ȡ����(������)IP
	const char* _GetLocalIP();

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

	LPPER_SOCKET_CONTEXT          m_ListenSockInfo;				// ������ListenContext

	LPFN_ACCEPTEX				  m_lpfnAcceptEx;				// AcceptEx����ָ��
	LPFN_GETACCEPTEXSOCKADDRS	  m_lpfnGetAcceptExSockAddrs;	// GetAcceptExSockAddrs()����ָ��

	int							  m_nThreads;				    // �����߳�����

	static SocketContextPool      m_ServerSocketPool;			// ����ͻ��˵��ڴ��
};

