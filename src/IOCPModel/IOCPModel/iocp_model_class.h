#pragma once

#include "winsock_class.h"		
#include "per_io_context_struct.h"
#include "per_socket_context_struct.h"
#include "socket_context_pool_class.h"

#include<MSWSock.h>


// ����������״̬
#define RUNNING true
#define STOP    false

// �ͷž��
#define RELEASE_HANDLE(x) {if(x!=NULL && x!=INVALID_HANDLE_VALUE){ CloseHandle(x);x=INVALID_HANDLE_VALUE;}}
// �ͷ�ָ��
#define RELEASE(x)	{if(x != NULL) {delete x; x = NULL;}}


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

	// Ͷ��Send����
	bool PostSend(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT IoInfo)
	{
		return _PostSend(SocketInfo, IoInfo);
	}
	
	// �ر�����
	void DoClose(LPPER_SOCKET_CONTEXT socketInfo)
	{
		_DoClose(socketInfo);
	}

	// ��ǰ��������Ծ��������
	static unsigned int GetConnectCnt()
	{
		return m_ServerSocketPool.NumOfConnectingServer();
	}

	// �¼�֪ͨ����(���������ش��庯��)
	virtual void ConnectionEstablished(LPPER_SOCKET_CONTEXT socketInfo) = 0;
	virtual void ConnectionClosed(LPPER_SOCKET_CONTEXT socketInfo) = 0;
	virtual void ConnectionError(LPPER_SOCKET_CONTEXT socketInfo, DWORD errorNum) = 0;
	virtual void RecvCompleted(LPPER_SOCKET_CONTEXT socketInfo, LPPER_IO_CONTEXT ioInfo) = 0;
	virtual void SendCompleted(LPPER_SOCKET_CONTEXT socketInfo, LPPER_IO_CONTEXT ioInfo) = 0;

private:
	// ��ʼ����������Դ
	// 1.��ʼ��Winsock����
	// 2.��ʼ��IOCP + ���������̳߳�
	bool _InitializeResource();

	// 1.����Winsock��:private
	bool _LoadSocketLib();

	// 2.��ʼ��IOCP:��ɶ˿��빤���߳��̳߳�
	bool _InitializeIOCP();

	// �ͷŷ�������Դ
	void _DeinitializeResource();

	// �ر����й����߳�
	void _CloseWorkerThreads();

	// ж��Socket��
	bool _UnloadSocketLib()
	{
		WSACleanup();
	}

	// ���������������׽���
	bool _InitializeListenSocket();

	// socket�Ƿ���
	bool _IsSocketAlive(LPPER_SOCKET_CONTEXT socketInfo);

	// ������ɶ˿��ϵĴ���
	bool _HandleError(LPPER_SOCKET_CONTEXT socketInfo, const DWORD&dwErr);

	// �Ͽ���ͻ�������
	void _DoClose(LPPER_SOCKET_CONTEXT socketInfo);

	// ��ӡ��Ϣ
	void _ShowMessage(const char*, ...) const;

	// ��ñ����д�����������
	int _GetNumberOfProcessors();

	// ��ȡ����(������)IP
	const char* _GetLocalIP();

	// �̺߳�����ΪIOCP������������߳�
	static DWORD WINAPI _WorkerThread(LPVOID lpParam);

	// ����I/O����
	bool _DoAccept(LPPER_IO_CONTEXT socketInfo);
	bool _DoRecv(LPPER_SOCKET_CONTEXT socketInfo, LPPER_IO_CONTEXT ioInfo);
	bool _DoSend(LPPER_SOCKET_CONTEXT socketInfo, LPPER_IO_CONTEXT ioInfo);

	// Ͷ��I/O����
	bool _PostAccept(LPPER_IO_CONTEXT socketInfo);
	bool _PostRecv(LPPER_SOCKET_CONTEXT socketInfo, LPPER_IO_CONTEXT ioInfo);
	bool _PostSend(LPPER_SOCKET_CONTEXT socketInfo, LPPER_IO_CONTEXT ioInfo);

protected:
	bool						  m_ServerRunning;				// ����������״̬�ж�

	SYSTEM_INFO					  m_SysInfo;					// ����ϵͳ��Ϣ

	HANDLE						  m_hIOCompletionPort;			// ��ɶ˿ھ��

	HANDLE					      m_hWorkerShutdownEvent;		// ֪ͨ�߳�ϵͳ�Ƴ��¼�

	HANDLE						  *m_phWorkerThreadArray;       // �����߳�����ľ��ָ��

	LPPER_SOCKET_CONTEXT          m_lpListenSockInfo;			// ������ListenContext

	LPFN_ACCEPTEX				  m_lpfnAcceptEx;				// AcceptEx����ָ��

	LPFN_GETACCEPTEXSOCKADDRS	  m_lpfnGetAcceptExSockAddrs;	// GetAcceptExSockAddrs()����ָ��

	unsigned int				  m_nThreads;				    // �����߳�����

	static SocketContextPool      m_ServerSocketPool;			// ����ͻ��˵��ڴ��

	//CSLock					      m_csLock;				
	
	CRITICAL_SECTION              m_csLock;						// �ٽ�����
};

