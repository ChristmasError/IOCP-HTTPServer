#pragma once

#include "iocp_model_class.h"
#include <mstcpip.h>

// ���ݸ�Worker�̵߳��˳��ź�
#define WORK_THREADS_EXIT NULL
// ͬʱͶ�ݵ�Accept����
#define MAX_POST_ACCEPT (500)				
// Ĭ�϶˿�
#define DEFAULT_PORT 8888
// �˳���־
#define EXIT_CODE (-1)

IOContextPool _PER_SOCKET_CONTEXT::ioContextPool;
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
	m_ListenSockInfo(NULL),
	m_lpfnAcceptEx(NULL),
	m_lpfnGetAcceptExSockAddrs(NULL),
	m_nThreads(0)
{
	if (_LoadSocketLib() == true)
		this->_ShowMessage("��ʼ��WinSock 2.2�ɹ�...\n");
	else
		// ����ʧ�� �׳��쳣
		// ��ʼ���˳��߳��¼�
		m_hWorkerShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}
// ����
IOCPModel::~IOCPModel()
{
	_Deinitialize();
}
/////////////////////////////////////////////////////////////////
// ��ʼ����Դ,�������������
bool IOCPModel::StartServer()
{
	// ����������״̬���
	if (m_ServerRunning)
	{
		this->_ShowMessage("������������,�����ظ�����!\n");
		return false;
	}
	// ��ʼ��������������Դ
	_InitializeServerResource();

	// ��������ʼ����
	_Start();
}

bool IOCPModel::_Start()
{
	m_ServerRunning = RUNNING;
	this->_ShowMessage("��������׼������:IP %s ���ڵȴ��ͻ��˵Ľ���......\n", GetLocalIP());

	while (m_ServerRunning)
	{
		Sleep(1000);
	}
	return true;
}
/////////////////////////////////////////////////////////////////
// ��ʼ����������Դ
bool IOCPModel::_InitializeServerResource()
{
	// ��ʼ��IOCP
	if (false == _InitializeIOCP())
	{
		this->_ShowMessage("��ʼ��IOCPʧ��!\n");
		return false;
	}
	else
	{
		this->_ShowMessage("��ʼ��IOCP���...\n");
	}

	// ��ʼ��������ListenSocket
	if (false == _InitializeListenSocket())
	{
		this->_ShowMessage("��ʼ��������ListenSocketʧ��!\n");
		this->_Deinitialize();
		return false;
	}
	else
	{
		this->_ShowMessage("��ʼ��������ListenSocket���...\n");
	}
	return true;
}

/////////////////////////////////////////////////////////////////
// ����SOCKET��
bool IOCPModel::_LoadSocketLib()
{
	WSADATA	wsaData;	// Winsock�����ʼ��
	int nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	// ����(һ�㶼�����ܳ���)
	if (NO_ERROR != nResult)
	{
		this->_ShowMessage("_LoadSocketLib()����ʼ��WinSock 2.2 ʧ��!\n");
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
		this->_ShowMessage("_InitializeIOCP()��������ɶ˿�ʧ��!\n");

	// ����IO�߳�--�߳����洴���̳߳�
	// ���ڴ������ĺ������������߳�
	m_nThreads = _GetNumberOfProcessors() * 2;
	m_phWorkerThreadArray = new HANDLE[m_nThreads];
	for (DWORD i = 0; i <m_nThreads; ++i)
	{
		// �����������������̣߳�������ɶ˿ڴ��ݵ����߳�
		HANDLE hWorkThread = CreateThread(NULL, NULL, _WorkerThread, (void*)this, 0, NULL);
		if (NULL == hWorkThread) {
			this->_ShowMessage("_InitializeIOCP()�������߳̾��ʧ��! Error:%d\n", GetLastError());
			system("pause");
			return false;
		}
		m_phWorkerThreadArray[i] = hWorkThread;
		CloseHandle(hWorkThread);
	}
	this->_ShowMessage("_InitializeIOCP()����ɴ����������߳�������%d ��...\n", m_nThreads);
	return true;
}

/////////////////////////////////////////////////////////////////
// ��ʼ��listenSocket
bool IOCPModel::_InitializeListenSocket()
{
	// �������ڼ����� Listen Socket Context

	m_ListenSockInfo = new PER_SOCKET_CONTEXT;
	m_ListenSockInfo->m_Sock.CreateWSASocket();
	if (INVALID_SOCKET == m_ListenSockInfo->m_Sock.socket)
	{
		this->_ShowMessage("_InitializeListenSocket()����������socketʧ��!\n");
		return false;
	}
	//listen socket����ɶ˿ڰ�
	if (NULL == CreateIoCompletionPort((HANDLE)m_ListenSockInfo->m_Sock.socket, m_hIOCompletionPort, (DWORD)m_ListenSockInfo, 0))
	{
		//RELEASE_SOCKET(m_ListenSockInfo->m_Sock.socket)
		this->_ShowMessage("_InitializeListenSocket()������socket����ɶ˿ڰ�ʧ��!\n");
		return false;
	}
	else
	{
		this->_ShowMessage("_InitializeListenSocket()������socket����ɶ˿ڰ����...\n");
	}
	// �󶨵�ַ&�˿�	
	m_ListenSockInfo->m_Sock.Bind(DEFAULT_PORT);

	// ��ʼ���м���
	if (SOCKET_ERROR == listen(m_ListenSockInfo->m_Sock.socket, SOMAXCONN)) {
		this->_ShowMessage("_InitializeListenSocket()������ʧ��. Error:%d\n", GetLastError());
		return false;
	}
	else
	{
		this->_ShowMessage("_InitializeListenSocket()��������ʼ...\n");
	}

	// AcceptEx �� GetAcceptExSockaddrs ��GUID�����ڵ�������ָ��
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	GUID guidGetAcceptEXSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
	DWORD dwBytes = 0;
	if (SOCKET_ERROR == WSAIoctl(
		m_ListenSockInfo->m_Sock.socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx,
		sizeof(guidAcceptEx),
		&m_lpfnAcceptEx,
		sizeof(m_lpfnAcceptEx),
		&dwBytes,
		NULL,
		NULL))
	{
		this->_ShowMessage("WSAIoctl()δ�ܻ�ȡAcceptEx����ָ��!�������: %d\n", WSAGetLastError());
		// �ͷ���Դ_Deinitialize
		return false;
	}

	// ͬ����ȡGetAcceptExSockAddrs����ָ��
	if (SOCKET_ERROR == WSAIoctl(
		m_ListenSockInfo->m_Sock.socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidGetAcceptEXSockAddrs,
		sizeof(guidGetAcceptEXSockAddrs),
		&m_lpfnGetAcceptExSockAddrs,
		sizeof(m_lpfnGetAcceptExSockAddrs),
		&dwBytes,
		NULL,
		NULL))
	{
		this->_ShowMessage("WSAIoctl δ�ܻ�ȡGetAcceptExSockAddrs����ָ��,�������: %d\n", WSAGetLastError());
		// �ͷ���Դ_Deinitialize
		return false;
	}

	for (int i = 0; i < MAX_POST_ACCEPT; i++)
	{
		// Ͷ��accept
		LPPER_IO_CONTEXT  pAcceptIoContext = new PER_IO_CONTEXT;
		if (false == this->_PostAccept(pAcceptIoContext))
		{
			//m_pListenContext->RemoveContext(pAcceptIoContext);
			return false;
		}
	}
	this->_ShowMessage("Ͷ�� %d ��AcceptEx�������...\n", MAX_POST_ACCEPT);
	return true;
}

/////////////////////////////////////////////////////////////////
// �ͷ�������Դ
void IOCPModel::_Deinitialize()
{
	// �ر�ϵͳ�˳��¼����
	RELEASE_HANDLE(m_hWorkerShutdownEvent);

	// �ͷŹ������߳̾��ָ��
	for (int i = 0; i < m_nThreads; i++)
	{
		RELEASE_HANDLE(m_phWorkerThreadArray[i]);
	}
	delete[] m_phWorkerThreadArray;

	// �ر�IOCP���
	RELEASE_HANDLE(m_hIOCompletionPort);

	this->_ShowMessage("�������ͷ���Դ���...\n");
}

/////////////////////////////////////////////////////////////////
// �رշ�����
void IOCPModel::StopServer()
{
	SetEvent(m_hWorkerShutdownEvent);
	for (int i = 0; i < m_nThreads; i++)
	{
		PostQueuedCompletionStatus(m_hIOCompletionPort, 0, (DWORD)EXIT_CODE, NULL);
	}
	WaitForMultipleObjects(m_nThreads, m_phWorkerThreadArray, TRUE, INFINITE);
	// �ͷ���Դ
	_Deinitialize();
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
			DWORD dwError = GetLastError();
			// ��ʱ��⣬�����ʱ�ȴ�
			if (WAIT_TIMEOUT == dwError)
			{
				// �ͻ������ڻ
				if (IOCP->_IsSocketAlive(socketInfo))
				{
					IOCP->ConnectionClosed(socketInfo);
					//ConnectionClose(handleInfo)
					continue;
				}
				else
				{
					continue;
				}
			}
			// ����64 �ͻ����쳣�˳�
			else if (ERROR_NETNAME_DELETED == dwError)
			{
				IOCP->ConnectionError(socketInfo, dwError);
				//ConnectionClose(handleInfo)
			}
			else
			{
				IOCP->ConnectionError(socketInfo, dwError);
				//ConnectionClose(handleInfo)
			}
		}
		else
		{
			// �������ж��Ƿ��пͻ��˶Ͽ�
			if ((RecvBytes == 0) && (ioInfo->m_OpType == RECV_POSTED))
			{
				IOCP->ConnectionClosed(socketInfo);
				//ConnectionClose(handleInfo);
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
					break;
				}
			}
		}

	}//while

	 // �ͷ��̲߳���
	RELEASE_HANDLE(lpParam);
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
	return m_ListenSockInfo->m_Sock.GetLocalIP();
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

/////////////////////////////////////////////////////////////////
// �Ͽ���ͻ�������
bool IOCPModel::_DoClose(LPPER_SOCKET_CONTEXT socektContext)
{
	CSAutoLock cslock(m_csLock);
	delete socektContext;
	return true;
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
	//std::cout << va_arg(msg, const char*) << std::endl;
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
	ASSERT(ioInfo != m_ListenSockInfo.m_sock.socket);

	//׼������
	// ��accept������,Ϊ�����ӵĿͻ���׼����socket
	DWORD dwBytes = 0;
	pAcceptioInfo->m_OpType = ACCEPT_POSTED;
	pAcceptioInfo->m_AcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == pAcceptioInfo->m_AcceptSocket)
	{
		this->_ShowMessage("��������Accept��Socketʧ��!�������: %d", WSAGetLastError());
		return false;
	}

	// Ͷ��AccpetEX
	if (FALSE == m_lpfnAcceptEx(
		m_ListenSockInfo->m_Sock.socket,
		pAcceptioInfo->m_AcceptSocket,
		pAcceptioInfo->m_wsaBuf.buf,
		0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		&dwBytes,
		&pAcceptioInfo->m_Overlapped))
	{
		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			this->_ShowMessage("Ͷ��AcceptEx()����ʧ��!�������: %d", WSAGetLastError());
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
		this->_ShowMessage( "����ֵ���󣬴�������Pending���ص�����ʧЧ!\n");
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
			//DoClose(SocketInfo);
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

	// 1. ��ȡ��ַ��Ϣ ��GetAcceptExSockAddrs�����������Ի�ȡ��ַ��Ϣ��������˳��ȡ����һ�����ݣ�
	m_lpfnGetAcceptExSockAddrs(ioInfo->m_wsaBuf.buf, 0, localAddrLen, clientAddrLen, (LPSOCKADDR *)&localAddr, &localAddrLen, (LPSOCKADDR *)&clientAddr, &clientAddrLen);

	// 2. ÿһ���ͻ������룬��Ϊ�����ӽ���һ��SocketContext 
	LPPER_SOCKET_CONTEXT pNewSocketInfo = m_ServerSocketPool.AllocateSocketContext();
	pNewSocketInfo->m_Sock.socket = ioInfo->m_AcceptSocket;
	if (INVALID_SOCKET == pNewSocketInfo->m_Sock.socket)
	{
		m_ServerSocketPool.ReleaseSocketContext(pNewSocketInfo);
		this->_ShowMessage("��Ч�Ŀͻ���socket!\n");
		return false;
	}
	memcpy(&(pNewSocketInfo->m_Sock.addr), clientAddr, sizeof(SOCKADDR_IN));
	pNewSocketInfo->m_Sock.ip = inet_ntoa(clientAddr->sin_addr);
	// 3. ����SocketContext����ɶ˿ڰ�
	if (NULL == CreateIoCompletionPort((HANDLE)pNewSocketInfo->m_Sock.socket, m_hIOCompletionPort, (DWORD)pNewSocketInfo, 0))
	{
		DWORD dwErr = WSAGetLastError();
		if (dwErr != ERROR_INVALID_PARAMETER)
		{
			//DoClose(newSockContext);
			return false;
		}
	}

	// 4. Ϊ����ͻ��˽���һ��NewIoContext,��Ͷ��һ��RECV
	LPPER_IO_CONTEXT pNewIoContext = pNewSocketInfo->GetNewIOContext();
	pNewIoContext->m_OpType = RECV_POSTED;
	pNewIoContext->m_AcceptSocket = pNewSocketInfo->m_Sock.socket;
	if (false == this->_PostRecv(pNewSocketInfo, pNewIoContext))
	{
		this->_ShowMessage( "Ͷ��WSARecv()ʧ��!\n");
		//DoClose(sockContext);
		return false;
	}

	// 5. ��listenSocketContext��IOContext ���ú����Ͷ��AcceptEx
	ioInfo->Reset();
	if (false == this->_PostAccept(ioInfo))
	{
		m_ListenSockInfo->RemoveIOContext(ioInfo);
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
		//TRACE(L"WSAIoctl failed: %d/n", WSAGetLastError());
	}

	this->ConnectionEstablished(pNewSocketInfo);
	return true;
}

bool IOCPModel::_DoRecv(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT ioInfo)
{
	this->RecvCompleted(SocketInfo, ioInfo);
	// �ڴ�socketInfo��Ͷ���µ�RECV����
	LPPER_IO_CONTEXT pNewIoContext = SocketInfo->GetNewIOContext();
	pNewIoContext->m_OpType = RECV_POSTED;
	pNewIoContext->m_AcceptSocket = SocketInfo->m_Sock.socket;
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

