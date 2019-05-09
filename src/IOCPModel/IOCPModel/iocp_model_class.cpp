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

/////////////////////////////////////////////////////////////////
// ����&����
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
		this->_ShowMessage("��ʼ��WinSock 2.2�ɹ�!\n");
	else
		// ����ʧ�� �׳��쳣
		// ��ʼ���˳��߳��¼�
		m_hWorkerShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

}
IOCPModel::~IOCPModel()
{
	_Deinitialize();
}

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

/////////////////////////////////////////////////////////////////
// ��ʼ����Դ,�������������
bool IOCPModel::StartServer()
{
	// ����������״̬���
	if (m_ServerRunning)
	{
		_ShowMessage("������������,�����ظ�����!\n");
		return false;
	}
	// ��ʼ��������������Դ
	_InitializeServerResource();

	// ��������ʼ����
	_Start();
}
// private:
bool IOCPModel::_Start()
{
	m_ServerRunning = RUNNING;
	printf("��������׼������:IP %s ���ڵȴ��ͻ��˵Ľ���......\n", GetLocalIP());
	//this->_ShowMessage("��������׼������:IP %s�����ڵȴ��ͻ��˵Ľ���......\n",GetLocalIP());

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
		this->_ShowMessage("��ʼ��IOCP���!\n");
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
		this->_ShowMessage("��ʼ��������ListenSocket���!\n");
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
		this->_ShowMessage("��ʼ��WinSock 2.2 ʧ��!\n");
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
		std::cout << "������ɶ˿�ʧ��!\n";

	// ����IO�߳�--�߳����洴���̳߳�
	// ���ڴ������ĺ������������߳�
	m_nThreads = _GetNumberOfProcessors() * 2;
	m_phWorkerThreadArray = new HANDLE[m_nThreads];
	for (DWORD i = 0; i <m_nThreads; ++i)
	{
		// �����������������̣߳�������ɶ˿ڴ��ݵ����߳�
		HANDLE hWorkThread = CreateThread(NULL, NULL, _WorkerThread, (void*)this, 0, NULL);
		if (NULL == hWorkThread) {
			std::cerr << "�����߳̾��ʧ��!Error:" << GetLastError() << std::endl;
			system("pause");
			return false;
		}
		m_phWorkerThreadArray[i] = hWorkThread;
		CloseHandle(hWorkThread);
	}
	this->_ShowMessage("��ɴ����������߳�������%d ��\n", m_nThreads);
	return true;
}


/////////////////////////////////////////////////////////////////
// ��ʼ��������Socket
bool IOCPModel::_InitializeListenSocket()
{
	// �������ڼ����� Listen Socket Context

	m_ListenSockInfo = new PER_SOCKET_CONTEXT;
	m_ListenSockInfo->m_Sock.CreateWSASocket();
	if (INVALID_SOCKET == m_ListenSockInfo->m_Sock.socket)
	{
		this->_ShowMessage("��������socketʧ��!\n");
		return false;
	}
	//listen socket����ɶ˿ڰ�
	if (NULL == CreateIoCompletionPort((HANDLE)m_ListenSockInfo->m_Sock.socket, m_hIOCompletionPort, (DWORD)m_ListenSockInfo, 0))
	{
		//RELEASE_SOCKET(m_ListenSockInfo->m_Sock.socket)
		this->_ShowMessage("����socket����ɶ˿ڰ�ʧ��!\n");
		return false;
	}
	else
	{
		this->_ShowMessage("����socket����ɶ˿ڰ����!\n");
	}
	// �󶨵�ַ&�˿�	
	m_ListenSockInfo->m_Sock.Bind(DEFAULT_PORT);

	// ��ʼ���м���
	if (SOCKET_ERROR == listen(m_ListenSockInfo->m_Sock.socket, SOMAXCONN)) {
		std::cerr << "����ʧ��. Error: " << GetLastError() << std::endl;
		return false;
	}
	else
	{
		printf("Listen() ���.\n");
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
		printf("WSAIoctl δ�ܻ�ȡAcceptEx����ָ��,�������: %d\n", WSAGetLastError());
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
		printf("WSAIoctl δ�ܻ�ȡGetAcceptExSockAddrs����ָ��,�������: %d\n", WSAGetLastError());
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
	printf("Ͷ�� %d ��AcceptEx�������", MAX_POST_ACCEPT);
	return true;
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
/////////////////////////////////////////////////////////////////
// �����̺߳���
DWORD WINAPI IOCPModel::_WorkerThread(LPVOID lpParam)
{
	IOCPModel* IOCP = (IOCPModel*)lpParam;
	;
	LPPER_SOCKET_CONTEXT handleInfo = NULL;
	LPPER_IO_CONTEXT ioInfo = NULL;
	DWORD RecvBytes = 0;
	DWORD Flags = 0;

	while (WAIT_OBJECT_0 != WaitForSingleObject(IOCP->m_hWorkerShutdownEvent, 0))
	{
		bool bRet = GetQueuedCompletionStatus(IOCP->m_hIOCompletionPort, &RecvBytes, (PULONG_PTR)&(handleInfo), (LPOVERLAPPED*)&ioInfo, INFINITE);
		//�յ��˳��̱߳�־��ֱ���˳������߳�

		if (EXIT_CODE == (DWORD)handleInfo)
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
			// �Ƿ�ʱ�������ʱ�ȴ�
			if (dwError == WAIT_TIMEOUT)
			{
				// �ͻ������ڻ
				if (/*IsAlive()*/1)
				{
					//ConnectionClose(handleInfo)
					//����socket
					continue;
				}
				else
				{
					continue;
				}
			}
			// ����64 �ͻ����쳣�˳�
			else if (dwError == ERROR_NETNAME_DELETED)
			{
				//����ص�����
				//iocp->ConnectionError				
				//����socket
			}
			else
			{
				//����ص�����
				//iocp->ConnectionError				
				//����socket
			}
		}
		else
		{
			// �ж��Ƿ��пͻ��˶Ͽ�
			if ((RecvBytes == 0) && (ioInfo->m_OpType == RECV_POSTED) ||
				(SEND_POSTED == ioInfo->m_OpType))
			{
				//ConnectionClose(handleInfo);
				//����socket
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
					IOCP->_DoRecv(handleInfo, ioInfo);
					break;
				case SEND_POSTED:
					IOCP->_DoSend(handleInfo, ioInfo);
					break;
				default:
					break;
				}
			}
		}

	}//while

	 // �ͷ��̲߳���
	RELEASE_HANDLE(lpParam);
	std::cout << "�����߳��˳�!" << std::endl;
	return 0;
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

	this->_ShowMessage("�������ͷ���Դ���!\n");
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
	std::cout << msg;
}

/////////////////////////////////////////////////////////////////
// Ͷ��I/O����
bool IOCPModel::_PostAccept(LPPER_IO_CONTEXT pAcceptIoInfo)
{
	ASSERT(ioinfo != m_ListenSockInfo.m_sock.socket);

	//׼������
	// ��accept������,Ϊ�����ӵĿͻ���׼����socket
	DWORD dwBytes = 0;
	pAcceptIoInfo->m_OpType = ACCEPT_POSTED;
	pAcceptIoInfo->m_AcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == pAcceptIoInfo->m_AcceptSocket)
	{
		printf("��������Accept��Socketʧ��!�������: %d", WSAGetLastError());
		return false;
	}

	// Ͷ��AccpetEX
	if (FALSE == m_lpfnAcceptEx(
		m_ListenSockInfo->m_Sock.socket,
		pAcceptIoInfo->m_AcceptSocket,
		pAcceptIoInfo->m_wsaBuf.buf,
		0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		&dwBytes,
		&pAcceptIoInfo->m_Overlapped))
	{
		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			printf("Ͷ�� AcceptEx ����ʧ�ܣ��������: %d", WSAGetLastError());
			return false;
		}
	}
	return true;
}

bool IOCPModel::_PostRecv(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT IoInfo)
{
	DWORD RecvBytes = 0, Flags = 0;
	IoInfo->Reset();
	IoInfo->m_OpType = RECV_POSTED;
	int nBytesRecv = WSARecv(SocketInfo->m_Sock.socket, &(IoInfo->m_wsaBuf), 1, &RecvBytes, &Flags, &(IoInfo->m_Overlapped), NULL);
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		std::cout << "�������ֵ���󣬲��Ҵ���Ĵ��벢����Pending�Ļ����Ǿ�˵������ص�����ʧ����\n";
		return false;
	}
	return true;
}

bool IOCPModel::_PostSend(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT IoInfo)
{
	IoInfo->m_OpType = SEND_POSTED;
	DWORD dwFlags = 0, dwBytes = 0;
	if (::WSASend(IoInfo->m_AcceptSocket, &IoInfo->m_wsaBuf, 1, &dwBytes, dwFlags, &IoInfo->m_Overlapped, NULL) != NO_ERROR)
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
bool IOCPModel::_DoAccept(LPPER_IO_CONTEXT IoInfo)
{
	SOCKADDR_IN *clientAddr = NULL;
	SOCKADDR_IN *localAddr = NULL;
	int clientAddrLen = sizeof(SOCKADDR_IN);
	int localAddrLen = sizeof(SOCKADDR_IN);

	// 1. ��ȡ��ַ��Ϣ ��GetAcceptExSockAddrs�����������Ի�ȡ��ַ��Ϣ��������˳��ȡ����һ�����ݣ�
	m_lpfnGetAcceptExSockAddrs(IoInfo->m_wsaBuf.buf, 0, localAddrLen, clientAddrLen, (LPSOCKADDR *)&localAddr, &localAddrLen, (LPSOCKADDR *)&clientAddr, &clientAddrLen);

	// 2. ÿһ���ͻ������룬��Ϊ�����ӽ���һ��SocketContext 
	LPPER_SOCKET_CONTEXT pNewSocketInfo = m_ServerSocketPool.AllocateSocketContext();
	pNewSocketInfo->m_Sock.socket = IoInfo->m_AcceptSocket;
	if (INVALID_SOCKET == pNewSocketInfo->m_Sock.socket)
	{
		m_ServerSocketPool.ReleaseSocketContext(pNewSocketInfo);
		std::cout << "���յ��Ŀͻ���Socket��Ч!\n";
		return false;
	}
	memcpy(&(pNewSocketInfo->m_Sock.addr), clientAddr, sizeof(SOCKADDR_IN));

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
		std::cerr << "Ͷ���׸�WSARecv()ʧ��!\n";
		//DoClose(sockContext);
		return false;
	}

	// 5. ��listenSocketContext��IOContext ���ú����Ͷ��AcceptEx
	IoInfo->Reset();
	if (false == this->_PostAccept(IoInfo))
	{
		m_ListenSockInfo->RemoveIOContext(IoInfo);
	}
	//// ������tcp_keepalive
	//tcp_keepalive alive_in;
	//tcp_keepalive alive_out;
	//alive_in.onoff = TRUE;
	//alive_in.keepalivetime = 1000 * 60;		// 60s  �೤ʱ�䣨 ms ��û�����ݾͿ�ʼ send ������
	//alive_in.keepaliveinterval = 1000 * 10; //10s  ÿ���೤ʱ�䣨 ms �� send һ��������
	//unsigned long ulBytesReturn = 0;
	//if (SOCKET_ERROR == WSAIoctl(pnewSockContext->m_Sock.socket, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in), &alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL))
	//{
	//	//TRACE(L"WSAIoctl failed: %d/n", WSAGetLastError());
	//}


	////OnConnectionEstablished(newSockContext);
	return true;
}

bool IOCPModel::_DoRecv(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT ioInfo)
{
	RecvCompleted(SocketInfo, ioInfo);
	// �ڴ�socketInfo��Ͷ���µ�RECV����
	LPPER_IO_CONTEXT pNewIoContext = SocketInfo->GetNewIOContext();
	pNewIoContext->m_OpType = RECV_POSTED;
	pNewIoContext->m_AcceptSocket = SocketInfo->m_Sock.socket;
	if (false == _PostRecv(SocketInfo, pNewIoContext))
	{
		this->_ShowMessage("Ͷ��WSARecv()ʧ��!\n");
		return false;
	}
	return true;
}

bool IOCPModel::_DoSend(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT IoInfo)
{
	this->SendCompleted(SocketInfo, IoInfo);
	return true;
}
