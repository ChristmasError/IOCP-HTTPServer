#pragma once

#include "IOCPServerModel.h"

// ���ݸ�Worker�̵߳��˳��ź�
#define WORK_THREADS_EXIT NULL
// ͬʱͶ�ݵ�Accept����
#define MAX_POST_ACCEPT (200)			
// Ĭ�϶˿�
#define DEFAULT_PORT 8888
// �˳���־
#define EXIT_CODE (-1)


SocketContextPool IOCPModel::m_ServerSocketPool;

//=========================================================================
//
//							��ʼ�����ͷ�
//
//=========================================================================

// ����
IOCPModel::IOCPModel() :
	m_ServerRunning(STOP),
	m_hIOCompletionPort(INVALID_HANDLE_VALUE),
	m_phWorkerThreadArray(NULL),
	m_lpListenSockInfo(NULL),
	m_lpfnAcceptEx(NULL),
	m_lpfnGetAcceptExSockAddrs(NULL),
	m_nThreads(0)
{
	// ����WinSocket2.2��
	if (_LoadSocketLib() == true)
		this->_ShowMessage("��ʼ��WinSock 2.2�ɹ�...\n");
	else
		this->_ShowMessage("[ERROR] ��ʼ��WinSock 2.2ʧ��!\n");

	InitializeCriticalSection(&m_csLock);
}
// ����
IOCPModel::~IOCPModel()
{
	_DeinitializeResource();
}
/////////////////////////////////////////////////////////////////
// ����SOCKET��
bool IOCPModel::_LoadSocketLib()
{
	WSADATA	wsaData;	// Winsock�����ʼ��
	int nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	// ����
	if (NO_ERROR != nResult)
		return false;
	else
		return true;
}

/////////////////////////////////////////////////////////////////
// ��ʼ����Դ,�������������
bool IOCPModel::StartServer()
{
	// ����������״̬���
	if (m_ServerRunning)
	{
		this->_ShowMessage("[WARRING] ������������,�����ظ�����!\n");
		return false;
	}
	// ��ʼ��������������Դ
	if (false == _InitializeResource())//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	{
		return false;
	}
	else
	{
		this->_ShowMessage("��������Դ��ʼ�����...\n");
	}

	// ��������ʼ����
	m_ServerRunning = RUNNING;
	this->_ShowMessage("��������׼������:IP %s ,�Ⱥ�����...\n", GetLocalIP());

	while (1)
	{
		{
			m_lpListenSockInfo->ShowIoContextPoolInfo();
			Sleep(2000);
		}
		
	}
	return true;
}
/////////////////////////////////////////////////////////////////
// ��ʼ����������Դ
bool IOCPModel::_InitializeResource()
{	
	// ��ʼ���˳��߳��¼�
	m_hWorkerShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	// ��ʼ��IOCP�빤���߳�
	if (false == _InitializeIOCP())
	{
		this->_ShowMessage("[ERROR] ��ʼ��IOCP�빤���߳�ʧ��!\n");
		return false;
	}
	// ��ʼ��������ListenSocket
	if (false == _InitializeListenSocket())
	{
		this->_ShowMessage("[ERROR] ��ʼ��ListenSocketʧ��!\n");
		this->_CloseWorkerThreads();
		this->_DeinitializeResource();
		return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////
// ��ʼ��IOCP:��ɶ˿��빤���߳��̳߳ش���
bool IOCPModel::_InitializeIOCP()
{
	// ��ʼ����ɶ˿�
	m_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (NULL == m_hIOCompletionPort)
	{
		this->_ShowMessage("[ERROR] ������ɶ˿�ʧ��!\n");
		return false;
	}
	// ���ݴ������ĺ����������������߳�
	m_nThreads = _GetNumberOfProcessors() * 2;
	m_phWorkerThreadArray = new HANDLE[m_nThreads];
	for (DWORD i = 0; i <m_nThreads; ++i)
	{
		// �����������������̣߳�������ɶ˿ڴ��ݵ����߳�
		m_phWorkerThreadArray[i] = ::CreateThread(NULL, NULL, _WorkerThread, (void*)this, 0, NULL);
	}
	this->_ShowMessage("�����������߳�����:%d ��...\n", m_nThreads);

	return true;
}

/////////////////////////////////////////////////////////////////
// ��ʼ��listenSocket
bool IOCPModel::_InitializeListenSocket()
{
	// �������ڼ����� Listen Socket Context
	m_lpListenSockInfo = new PER_SOCKET_CONTEXT;
	m_lpListenSockInfo->m_Sock.CreateWSASocket();
	if (INVALID_SOCKET == m_lpListenSockInfo->m_Sock.socket)
	{
		this->_ShowMessage("[ERROR] ��ʼ��ListenSocketʧ��!\n");
		return false;
	}
	//listen socket����ɶ˿ڰ�
	if (NULL == CreateIoCompletionPort((HANDLE)m_lpListenSockInfo->m_Sock.socket, m_hIOCompletionPort, (DWORD)m_lpListenSockInfo, 0))
	{
		this->_ShowMessage("[ERROR] ListenSocket����ɶ˿ڰ�ʧ��!\n");
		closesocket(m_lpListenSockInfo->m_Sock.socket);
		return false;
	}
	// ����ɶ˿�
	m_lpListenSockInfo->m_Sock.Bind(DEFAULT_PORT);
	// ��ʼ���м���
	if (SOCKET_ERROR == listen(m_lpListenSockInfo->m_Sock.socket, SOMAXCONN)) {
		this->_ShowMessage("[ERROR] ����ʧ��! Error:%d\n",WSAGetLastError());
		return false;
	}

	// AcceptEx �� GetAcceptExSockaddrs ��GUID�����ڵ�������ָ��
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	GUID guidGetAcceptEXSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
	DWORD dwBytes = 0;
	if (SOCKET_ERROR == WSAIoctl(
		m_lpListenSockInfo->m_Sock.socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx,
		sizeof(guidAcceptEx),
		&m_lpfnAcceptEx,
		sizeof(m_lpfnAcceptEx),
		&dwBytes,
		NULL,
		NULL))
	{
		this->_ShowMessage("[ERROR] WSAIoctl()δ�ܻ�ȡAcceptEx����ָ��!Error: %d\n", WSAGetLastError());
		return false;
	}

	// ͬ����ȡGetAcceptExSockAddrs����ָ��
	if (SOCKET_ERROR == WSAIoctl(
		m_lpListenSockInfo->m_Sock.socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidGetAcceptEXSockAddrs,
		sizeof(guidGetAcceptEXSockAddrs),
		&m_lpfnGetAcceptExSockAddrs,
		sizeof(m_lpfnGetAcceptExSockAddrs),
		&dwBytes,
		NULL,
		NULL))
	{
		this->_ShowMessage("[ERROR] WSAIoctl()δ�ܻ�ȡGetAcceptExSockAddrs����ָ��,Error: %d\n", WSAGetLastError());
		return false;
	}

	for (int i = 0; i < MAX_POST_ACCEPT; i++)
	{
		// Ͷ��accept
		LPPER_IO_CONTEXT  ioContext = m_lpListenSockInfo->GetNewIOContext();
		//LPPER_IO_CONTEXT  ioContext = new PER_IO_CONTEXT;
		if (false == this->_PostAccept(ioContext))
		{
			m_lpListenSockInfo->RemoveIOContext(ioContext);
			//delete ioContext;
			this->_ShowMessage("[ERROR] Ͷ�ݵ� %d ��AcceptEx()����ʧ��!\n",i);
			return false;
		}
	}
	this->_ShowMessage("Ͷ�� %d ��AcceptEx()����...\n", MAX_POST_ACCEPT);
	return true;
}

/////////////////////////////////////////////////////////////////
// �رշ�����
void IOCPModel::StopServer()
{
	// �رչ����߳�
	_CloseWorkerThreads();
	// �ͷ���Դ
	_DeinitializeResource();
}

void IOCPModel::_CloseWorkerThreads()
{
	SetEvent(m_hWorkerShutdownEvent);
	for (int i = 0; i < m_nThreads; i++)
	{
		PostQueuedCompletionStatus(m_hIOCompletionPort, 0, (DWORD)EXIT_CODE, NULL);
	}
	WaitForMultipleObjects(m_nThreads, m_phWorkerThreadArray, TRUE, INFINITE);
}

/////////////////////////////////////////////////////////////////
// �ͷ�������Դ
void IOCPModel::_DeinitializeResource()
{
	RELEASE_HANDLE(m_hWorkerShutdownEvent);

	if (m_phWorkerThreadArray != NULL)
	{
		for (int i = 0; i < m_nThreads; i++)
		{
			RELEASE_HANDLE(m_phWorkerThreadArray[i]);
		}
	}
	RELEASE_HANDLE(m_hIOCompletionPort);

	RELEASE(m_lpListenSockInfo);

	InitializeCriticalSection(&m_csLock);

	this->_ShowMessage("��������Դ�ͷ����,�������ر�.\n");
}


//=========================================================================
//
//							�����̺߳���
//
//=========================================================================
DWORD WINAPI IOCPModel::_WorkerThread(LPVOID lpParam)
{
	IOCPModel* IOCP = (IOCPModel*)lpParam;
	;
	LPPER_SOCKET_CONTEXT socketInfo = NULL;
	LPPER_IO_CONTEXT ioInfo = NULL;
	DWORD RecvBytes = 0;
	DWORD Flags = 0;

	while (WAIT_OBJECT_0 != WaitForSingleObject(IOCP->m_hWorkerShutdownEvent, 0))
	{
		bool bRet = GetQueuedCompletionStatus(IOCP->m_hIOCompletionPort, &RecvBytes, (PULONG_PTR)&(socketInfo), (LPOVERLAPPED*)&ioInfo, INFINITE);
		// �յ�EXIT_CODE�˳��̱߳�־���˳������߳�
		if (EXIT_CODE == (DWORD)socketInfo)
		{
			break;
		}
		// NULL_POSTED ����δ��ʼ��
		if (ioInfo->m_OpType == NULL_POSTED)
		{
			continue;
		}

		if (bRet == false)
		{
			DWORD dwError = WSAGetLastError();
			// ��ʾ��ʾ��Ϣ
			if (false == IOCP->_HandleError(socketInfo, dwError))
			{
				break;
			}
			else
				continue;
		}
		else
		{
			// ��ȡ����Ĳ���
			// �������ж��Ƿ��пͻ��˶Ͽ�
			if ((RecvBytes == 0) && (ioInfo->m_OpType == RECV_POSTED || ioInfo->m_OpType == NULL_POSTED ))
			{
				IOCP->ConnectionClosed(socketInfo);
				IOCP->_DoClose(socketInfo);
				continue;
			}
			else
			{
				switch (ioInfo->m_OpType)
				{
				case ACCEPT_POSTED:
					IOCP->_DoAccept(ioInfo);
					break;
				case RECV_POSTED:
					IOCP->_DoRecv(socketInfo, ioInfo);
					break;
				case SEND_POSTED:
					IOCP->_DoSend(socketInfo, ioInfo);
					break;
				default:
					IOCP->_ShowMessage("[ERROR] ioInfo->m_OpType �����쳣!\n");
					break;
				}//switch
			}//if
		}//if

	}//while

	 // �ͷ��̲߳���
	IOCP->_ShowMessage("�����߳��˳�!\n");
	return 0;
}

//=========================================================================
//
//							������������
//
//=========================================================================

/////////////////////////////////////////////////////////////////
// ��ȡ����(������)IP
const char* IOCPModel::GetLocalIP()
{
	return _GetLocalIP();
}
const char* IOCPModel::_GetLocalIP()
{
	return m_lpListenSockInfo->m_Sock.GetLocalIP();
}

///////////////////////////////////////////////////////////////////
// socket�Ƿ���
bool IOCPModel::_IsSocketAlive(LPPER_SOCKET_CONTEXT socketInfo)
{
	int nByteSend = socketInfo->m_Sock.Send("", 0);
	if (nByteSend == -1)
		return false;
	else
		return true;
}

///////////////////////////////////////////////////////////////////
// ������ɶ˿��ϵĴ���
bool IOCPModel::_HandleError(LPPER_SOCKET_CONTEXT socketInfo, const DWORD&dwErr)
{
	if (socketInfo == NULL)
		return false;
	if (WAIT_TIMEOUT == dwErr)
	{
		// ���ͻ����Ƿ�����
		if ( !_IsSocketAlive(socketInfo))
		{
			this->_ShowMessage("�ͻ��˶Ͽ�����!\n");
			this->_DoClose(socketInfo);
			return true;
		}
		else
		{
			this->_ShowMessage("���糬ʱ!������...\n");
			return true;
		}
	}
	// �ͻ����쳣�˳�
	else if (ERROR_NETNAME_DELETED == dwErr)
	{
		this->_ShowMessage("��⵽�ͻ����쳣�˳�");
		this->_DoClose(socketInfo);
		return true;
	}
	else
	{
		this->_ShowMessage("��ɶ˿ڲ������ִ����߳��˳���Error:%d", dwErr);
		return false;
	}
}


/////////////////////////////////////////////////////////////////
// �Ͽ���ͻ�������,���ڴ淵�ظ�socket��
void IOCPModel::_DoClose(LPPER_SOCKET_CONTEXT socektContext)
{
	EnterCriticalSection(&m_csLock);
	m_ServerSocketPool.ReleaseSocketContext(socektContext);
	LeaveCriticalSection(&m_csLock);
}

///////////////////////////////////////////////////////////////////
// ��ñ����д�����������
int IOCPModel::_GetNumberOfProcessors()
{
	GetSystemInfo(&m_SysInfo);
	return m_SysInfo.dwNumberOfProcessors;
}

/////////////////////////////////////////////////////////////////
// ��ӡ��Ϣ
void IOCPModel::_ShowMessage(const char* msg, ...) const
{
	va_list valist;
	va_start(valist,msg);
	vprintf(msg, valist);
	va_end(valist);

}

//=========================================================================
//
//							Ͷ����ɶ˿�����
//
//=========================================================================

/////////////////////////////////////////////////////////////////
// Ͷ��I/O����
bool IOCPModel::_PostAccept(LPPER_IO_CONTEXT pAcceptioInfo)
{
	ASSERT( INVALID_SOCKET != m_lpListenSockInfo.m_sock.socket);

	//׼������
	DWORD dwBytes = 0;
	pAcceptioInfo->m_OpType = ACCEPT_POSTED;
	pAcceptioInfo->m_AcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == pAcceptioInfo->m_AcceptSocket)
	{
		this->_ShowMessage("[ERROR] ��������Accept��Socketʧ��!Error:%d", WSAGetLastError());
		return false;
	}

	// Ͷ��AccpetEX
	if (FALSE == m_lpfnAcceptEx(m_lpListenSockInfo->m_Sock.socket,pAcceptioInfo->m_AcceptSocket,
								pAcceptioInfo->m_wsaBuf.buf,0,sizeof(SOCKADDR_IN) + 16, 
								sizeof(SOCKADDR_IN) + 16,&dwBytes,&pAcceptioInfo->m_Overlapped))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			this->_ShowMessage("[ERROR] Ͷ��AcceptEx()����ʧ��!Error:%d", WSAGetLastError());
			return false;
		}
	}
	return true;
}

bool IOCPModel::_PostRecv(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT ioInfo)
{
	ioInfo->Reset();
	ioInfo->m_OpType = RECV_POSTED;
	DWORD RecvBytes = 0, Flags = 0;
	int nBytesRecv = WSARecv(SocketInfo->m_Sock.socket, &(ioInfo->m_wsaBuf), 1, &RecvBytes, &Flags, &(ioInfo->m_Overlapped), NULL);
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		return false;
	}
	return true;
}

bool IOCPModel::_PostSend(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT ioInfo)
{
	ioInfo->m_OpType = SEND_POSTED;
	DWORD dwFlags = 0, dwBytes = 0;
	if (::WSASend(ioInfo->m_AcceptSocket, &ioInfo->m_wsaBuf, 1, &dwBytes, dwFlags, &ioInfo->m_Overlapped, NULL) != NO_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			_DoClose(SocketInfo);
			return false;
		}
	}
	return true;
}
/////////////////////////////////////////////////////////////////
// ����I/O����
bool IOCPModel::_DoAccept(LPPER_IO_CONTEXT ioInfo)
{
	SOCKADDR_IN *clientAddr = NULL;
	SOCKADDR_IN *localAddr = NULL;
	int clientAddrLen = sizeof(SOCKADDR_IN);
	int localAddrLen = sizeof(SOCKADDR_IN);

	// ��ȡ��ַ��Ϣ ��GetAcceptExSockAddrs�����������Ի�ȡ��ַ��Ϣ��������˳��ȡ����һ�����ݣ�
	m_lpfnGetAcceptExSockAddrs(ioInfo->m_wsaBuf.buf, 0, localAddrLen, clientAddrLen, (LPSOCKADDR *)&localAddr, &localAddrLen, (LPSOCKADDR *)&clientAddr, &clientAddrLen);

	// ÿһ���ͻ������룬��Ϊ�����ӽ���һ��SocketContext 
	LPPER_SOCKET_CONTEXT pNewSocketInfo = m_ServerSocketPool.AllocateSocketContext();
	pNewSocketInfo->m_Sock.socket = ioInfo->m_AcceptSocket;
	if (INVALID_SOCKET == pNewSocketInfo->m_Sock.socket)
	{
		_DoClose(pNewSocketInfo);
		this->_ShowMessage("[ERROR] ��Ч�Ŀͻ���socket!\n");
		return false;
	}
	memcpy(&(pNewSocketInfo->m_Sock.addr), clientAddr, sizeof(SOCKADDR_IN));
	pNewSocketInfo->m_Sock.ip = inet_ntoa(clientAddr->sin_addr);

	// ����SocketContext����ɶ˿ڰ�
	if (NULL == CreateIoCompletionPort((HANDLE)pNewSocketInfo->m_Sock.socket, m_hIOCompletionPort, (DWORD)pNewSocketInfo, 0))
	{
		if (WSAGetLastError() == ERROR_INVALID_PARAMETER)
		{
			this->_ShowMessage("[ERROR] �ͻ���Socket����ɶ˿ڰ�ʧ��!\n");
			_DoClose(pNewSocketInfo);
			return false;
		}
	}

	// ����tcp_keepalive
	tcp_keepalive alive_in;
	tcp_keepalive alive_out;
	alive_in.onoff = TRUE;
	alive_in.keepalivetime = 1000 * 60;		// 60s  �೤ʱ�䣨 ms ��û�����ݾͿ�ʼ send ������
	alive_in.keepaliveinterval = 1000 * 10; //10s  ÿ���೤ʱ�䣨 ms �� send һ��������
	unsigned long ulBytesReturn = 0;
	if (SOCKET_ERROR == WSAIoctl(pNewSocketInfo->m_Sock.socket, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in), &alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL))
	{
		this->_ShowMessage("WSAIoctl() Error!\n");
	}
	// �������Ӻ���ûص�����
	this->ConnectionEstablished(pNewSocketInfo);

	// Ϊ��SocketContext����һ���µ�IoContext,��Ͷ��һ��RECV����
	LPPER_IO_CONTEXT pNewIoContext = pNewSocketInfo->GetNewIOContext();
	pNewIoContext->m_AcceptSocket = pNewSocketInfo->m_Sock.socket;
	m_lpListenSockInfo->ShowIoContextPoolInfo();
	if (false == this->_PostRecv(pNewSocketInfo, pNewIoContext))
	{
		_DoClose(pNewSocketInfo);
		this->_ShowMessage("[ERROR] Ͷ���׸�Recv()ʧ��!\n");
		return false;
	}

	// ��listenSocketContext��IOContext ���ú����Ͷ��AcceptEx
	ioInfo->Reset();
	if (false == this->_PostAccept(ioInfo))
	{
		m_lpListenSockInfo->RemoveIOContext(ioInfo);
	}

	return true;
}

bool IOCPModel::_DoRecv(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT ioInfo)
{
	this->RecvCompleted(SocketInfo, ioInfo);
	// �ڴ�socketInfo��Ͷ���µ�RECV����

	LPPER_IO_CONTEXT pNewIoContext = SocketInfo->GetNewIOContext();
	pNewIoContext->m_AcceptSocket = SocketInfo->m_Sock.socket;
	m_lpListenSockInfo->ShowIoContextPoolInfo();
	if (false == _PostRecv(SocketInfo, pNewIoContext))
	{
		this->_ShowMessage("Ͷ����WSARecv()ʧ��!\n");
		return false;
	}
	return true;
}

bool IOCPModel::_DoSend(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT ioInfo)
{
	this->SendCompleted(SocketInfo, ioInfo);
	return true;
}

