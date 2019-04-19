#include<IOCPModel.h>
#include <mstcpip.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS 

IOContextPool _PER_HANDLE_DATA::ioContextPool;		// ��ʼ��

/////////////////////////////////////////////////////////////////
// ����������
bool IOCPModel::StartEX()
{
	useAcceptEx = true;
	InitializeIOCPResource(useAcceptEx);

	return true;
}
bool IOCPModel::Start()
{
	useAcceptEx = false;
	InitializeIOCPResource(useAcceptEx);

	// ����������״̬���
	if (_ServerRunning == RUNNING) {
		_ShowMessage("������������,�����ظ����У�\n");
		return false;
	}
	else 
	{
		_ServerRunning = RUNNING;
		this->_ShowMessage("����������׼�����������ڵȴ��ͻ��˵Ľ���......\n");
	}

	WinSocket acceptSock; //listen���յ��׽���
	LPPER_HANDLE_DATA handleInfo;

	DWORD RecvBytes, Flags = 0;

	while (true)
	{
		// �������ӣ���������ɶˣ����������AcceptEx()
		acceptSock = m_ServerSocket.Accept();
		acceptSock.SetBlock(false);
		if (INVALID_SOCKET == acceptSock.socket)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
				continue;
			else
			{
				cerr << "Accept Socket Error: " << GetLastError() << endl;
				system("pause");
				return -1;
			}
		}

		handleInfo = new PER_HANDLE_DATA();
		handleInfo->m_Sock = acceptSock;

		CreateIoCompletionPort((HANDLE)(handleInfo->m_Sock.socket), m_IOCompletionPort, (DWORD)handleInfo, 0);

		// ��ʼ�ڽ����׽����ϴ���I/Oʹ���ص�I/O����,���½����׽�����Ͷ��һ�������첽,WSARecv��WSASend����
		// ��ЩI/O������ɺ󣬹������̻߳�ΪI/O�����ṩ����	
		// ��I/O��������(I/O�ص�)
		PER_IO_DATA* ioInfo = new PER_IO_DATA();
		if (!_PostRecv(handleInfo, ioInfo))
		{
			return false;
		}
	}

	return true;
}

/////////////////////////////////////////////////////////////////
// �����̺߳���
DWORD WINAPI IOCPModel::_WorkerThread(LPVOID lpParam)
{
	IOCPModel* IOCP = (IOCPModel*)lpParam;

	LPPER_HANDLE_DATA handleInfo = NULL;
	LPPER_IO_DATA ioInfo = NULL;
	DWORD RecvBytes;
	DWORD Flags = 0;

	while (WAIT_OBJECT_0 != WaitForSingleObject(IOCP->m_WorkerShutdownEvent,0))
	{
		bool bRet = GetQueuedCompletionStatus(IOCP->m_IOCompletionPort, &RecvBytes, (PULONG_PTR)&(handleInfo), (LPOVERLAPPED*)&ioInfo, INFINITE);
		//�յ��˳��̱߳�־��ֱ���˳������߳�
		if (EXIT_CODE == (DWORD)handleInfo)
		{
			break;
		}
		// NULL_POSTED ����δ��ʼ��
		if (ioInfo->m_OpType == NULL_POSTED) {
			continue;
		}

		if(bRet== false)
		{
			DWORD dwError = GetLastError();
			// �Ƿ�ʱ�������ʱ�ȴ�
			if (dwError == WAIT_TIMEOUT)
			{
				// �ͻ������ڻ
				if(/*IsAlive()*/1)
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
		// GetQueuedCompletionStatus()�޴��󷵻�
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
					IOCP->_DoAccept(handleInfo, ioInfo);
					break;
				case RECV_POSTED:
					IOCP->_DoRecv(handleInfo, ioInfo);
					break;
				case SEND_POSTED:
					//IOCP->_DoSend(handleInfo, ioInfo);
					break;
				default:
					break;
				}				
			}
		}
		
	}//while

	// �ͷ��̲߳���
	RELEASE_HANDLE(lpParam);
	cout << "�����߳��˳���" << endl;
	return 0;
}

/////////////////////////////////////////////////////////////////
// ��ʼ����������Դ
bool IOCPModel::InitializeIOCPResource(bool useAcceptEX)
{
	// ������������
	_ServerRunning = STOP;

	// ��ʼ���˳��߳��¼�
	m_WorkerShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// ��ʼ��IOCP
	if (false == _InitializeIOCP())
	{
		this->_ShowMessage("��ʼ��IOCPʧ�ܣ�\n");
		return false;
	}
	else
	{
		this->_ShowMessage("��ʼ��IOCP��ϣ�\n");
	}
	// ��ʼ��������socket,�ж��Ƿ�ʹ��AcceptEX().
	if (useAcceptEX==false)
	{
		if (false == _InitializeServerSocket())
		{
			this->_ShowMessage("��ʼ��������Socketʧ�ܣ�\n");
			this->_Deinitialize();
			return false;
		}
		else
		{
			this->_ShowMessage("��ʼ��������Socket��ϣ�\n");
		}
	}
	else
	{
		if (false == _InitializeListenSocket())
		{
			this->_ShowMessage("��ʼ��������Socketʧ�ܣ�\n");
			this->_Deinitialize();
			return false;
		}
		else
		{
			this->_ShowMessage("��ʼ��������Socket��ϣ�\n");
		}
	}


	return true;
}

/////////////////////////////////////////////////////////////////
// ����SOCKET��
// public
bool IOCPModel::LoadSocketLib()
{
	if (_LoadSocketLib() == true)
		return true;
	else
		return false;
}

// ����SOCKET��
// private
bool IOCPModel::_LoadSocketLib()
{
	int nResult;
	nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	// ����(һ�㶼�����ܳ���)
	if (NO_ERROR != nResult)
	{
		this->_ShowMessage("��ʼ��WinSock 2.2 ʧ�ܣ�\n");
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////
// ��ʼ��IOCP:��ɶ˿��빤���߳��̳߳ش���
bool IOCPModel::_InitializeIOCP()
{
	// ��ʼ����ɶ˿�
	m_IOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	//m_WorkThreadShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (NULL == m_IOCompletionPort)
		cout << "������ɶ˿�ʧ��!\n";

	// ����IO�߳�--�߳����洴���̳߳�
	// ���ڴ������ĺ������������߳�
	GetSystemInfo(&m_SysInfo);
	for (DWORD i = 0; i < (m_SysInfo.dwNumberOfProcessors * 2); ++i) {
		// �����������������̣߳�������ɶ˿ڴ��ݵ����߳�
		HANDLE WORKThread = CreateThread(NULL, 0, _WorkerThread, (void*)this, 0, NULL);
		if (NULL == WORKThread) {
			cerr << "�����߳̾��ʧ�ܣ�Error:" << GetLastError() << endl;
			system("pause");
			return -1;
		}
		CloseHandle(WORKThread);
	}
	this->_ShowMessage("��ɴ����������߳�������%d ��\n", m_nThreads);
	return true;	
}


/////////////////////////////////////////////////////////////////
// ��ʼ��������Socket
bool IOCPModel::_InitializeListenSocket()
{
	// AcceptEx �� GetAcceptExSockaddrs ��GUID�����ڵ�������ָ��
	GUID guidAcceptEx = WSAID_ACCEPTEX;
	GUID guidGetAcceptSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;
	// �������ڼ����� Listen Socket Context
	m_ListenSockInfo = new PER_HANDLE_DATA;
	//m_ListenSockInfo->m_Sock.socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if(INVALID_SOCKET == m_ListenSockInfo->m_Sock.socket)
	{ 
		this->_ShowMessage("��������socket contextʧ��!\n");
		return false;
	}
	//listen socket����ɶ˿ڰ�
	if (NULL == CreateIoCompletionPort((HANDLE)m_ListenSockInfo->m_Sock.socket, m_IOCompletionPort, (DWORD)m_ListenSockInfo, 0))
	{
		//RELEASE_SOCKET(m_ListenSockInfo->m_Sock.socket)
		return false;
	}
	// �󶨵�ַ&�˿�	
	m_ListenSockInfo->m_Sock.port = DEFAULT_PORT;
	m_ListenSockInfo->m_Sock.Bind(m_ListenSockInfo->m_Sock.port);
	//////////////////�󶨶˿�//////////////////////////////////////////////////////////////////////////////////
	//if (m_ListenSockInfo->m_Sock.socket <= 0)
	//	m_ListenSockInfo->m_Sock.CreateSocket();
	//sockaddr_in saddr;
	//saddr.sin_family = AF_INET;
	//saddr.sin_port = htons(DEFAULT_PORT);
	//saddr.sin_addr.s_addr = htonl(0);
	//if (::bind(m_ListenSockInfo->m_Sock.socket, (sockaddr*)&saddr, sizeof(saddr)) != 0)
	//{
	//	printf("�� %d �˿�ʧ��!\n", DEFAULT_PORT);
	//	return false;
	//}
	//printf("�� %d �˿ڳɹ�!\n", DEFAULT_PORT);
	////m_ListenSockInfo->m_Sock.Bind(DEFAULT_PORT);
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	// ��ʼ����
	if (SOCKET_ERROR == listen(m_ListenSockInfo->m_Sock.socket, 5)) {
		cerr << "����ʧ��. Error: " << GetLastError() << endl;
		return false;
	}

	// ��ȡ��չ����ָ��
	DWORD dwBytes = 0;
	if (SOCKET_ERROR == WSAIoctl(
		m_ListenSockInfo->m_Sock.socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidAcceptEx,
		sizeof(guidAcceptEx),
		&fnAcceptEx,
		sizeof(fnAcceptEx),
		&dwBytes,
		NULL,
		NULL))
	{
		_Deinitialize();
		return false;
	}

	if (SOCKET_ERROR == WSAIoctl(
		m_ListenSockInfo->m_Sock.socket, 
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guidGetAcceptSockAddrs,
		sizeof(guidGetAcceptSockAddrs),
		&fnGetAcceptExSockAddrs,
		sizeof(fnGetAcceptExSockAddrs),
		&dwBytes,
		NULL,
		NULL))
	{
		_Deinitialize();
		return false;
	}

	for (size_t i = 0; i < MAX_POST_ACCEPT; i++)
	{
		PER_IO_DATA *ioContext = m_ListenSockInfo->GetNewIOContext();
		if (false == _PostAccept(m_ListenSockInfo, ioContext))
		{
			m_ListenSockInfo->RemoveIOContext(ioContext);
			return false;
		}
	}

	return true;
}
bool IOCPModel::_InitializeServerSocket()
{
	m_ServerSocket.CreateSocket();
	m_ServerSocket.port = DEFAULT_PORT;
	m_ServerSocket.Bind(m_ServerSocket.port);

	// ��ʼ����
	if (SOCKET_ERROR == listen(m_ServerSocket.socket, 5)) {
		cerr << "����ʧ��. Error: " << GetLastError() << endl;
		return false;
	}

	return true;
}


/////////////////////////////////////////////////////////////////
// �ͷ�������Դ
void IOCPModel::_Deinitialize()
{
	// �ر�ϵͳ�˳��¼����
	RELEASE_HANDLE(m_WorkerShutdownEvent);

	// �ͷŹ������߳̾��ָ��
	for (int i = 0; i < m_nThreads; i++)
	{
		RELEASE_HANDLE(m_WorkerThreadsHandleArray[i]);
	}
	delete[] m_WorkerThreadsHandleArray;

	// �ر�IOCP���
	RELEASE_HANDLE(m_IOCompletionPort);

	// �رշ�����socket
	// ������������ûд �ͷ� WinSocket    m_ServerSocket;

	this->_ShowMessage("�ͷ���Դ��ϣ�\n");
}

/////////////////////////////////////////////////////////////////
// ��ӡ��Ϣ
void IOCPModel::_ShowMessage(const char* msg, ...) const
{
	std::cout << msg;
}

/////////////////////////////////////////////////////////////////
// ����I/O����
bool IOCPModel::_DoAccept(PER_HANDLE_DATA* handleInfo, PER_IO_DATA *ioInfo)
{
	//InterlockedIncrement(&connectCnt);
	//InterlockedDecrement(&acceptPostCnt);
	SOCKADDR_IN *clientAddr = NULL;
	SOCKADDR_IN *localAddr = NULL;
	int clientAddrLen = sizeof(SOCKADDR_IN);
	int localAddrLen = sizeof(SOCKADDR_IN);

	// 1. ��ȡ��ַ��Ϣ ��GetAcceptExSockAddrs�����������Ի�ȡ��ַ��Ϣ��������˳��ȡ����һ�����ݣ�
	fnGetAcceptExSockAddrs(ioInfo->m_wsaBuf.buf, 0, localAddrLen, clientAddrLen, (LPSOCKADDR *)&localAddr, &localAddrLen, (LPSOCKADDR *)&clientAddr, &clientAddrLen);

	// 2. Ϊ�����ӽ���һ��SocketContext 
	PER_HANDLE_DATA *newSockContext = new PER_HANDLE_DATA;
	newSockContext->m_Sock.socket = ioInfo->m_AcceptSocket;
	//memcpy_s(&(newSockContext->clientAddr), sizeof(SOCKADDR_IN), clientAddr, sizeof(SOCKADDR_IN));

	// 3. ��listenSocketContext��IOContext ���ú����Ͷ��AcceptEx
	ioInfo->Reset();
	if (false == _PostAccept(m_ListenSockInfo, ioInfo))
	{
		m_ListenSockInfo->RemoveIOContext(ioInfo);
	}

	// 4. ����socket����ɶ˿ڰ�
	if (NULL == CreateIoCompletionPort((HANDLE)newSockContext->m_Sock.socket, m_IOCompletionPort, (DWORD)newSockContext, 0))
	{
		DWORD dwErr = WSAGetLastError();
		if (dwErr != ERROR_INVALID_PARAMETER)
		{
			//DoClose(newSockContext);
			return false;
		}
	}

	// ������tcp_keepalive
	tcp_keepalive alive_in;
	tcp_keepalive alive_out;
	alive_in.onoff = TRUE;
	alive_in.keepalivetime = 1000 * 60;		// 60s  �೤ʱ�䣨 ms ��û�����ݾͿ�ʼ send ������
	alive_in.keepaliveinterval = 1000 * 10; //10s  ÿ���೤ʱ�䣨 ms �� send һ��������
	unsigned long ulBytesReturn = 0;
	if (SOCKET_ERROR == WSAIoctl(newSockContext->m_Sock.socket, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in), &alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL))
	{
		//TRACE(L"WSAIoctl failed: %d/n", WSAGetLastError());
	}


	//OnConnectionEstablished(newSockContext);

	// 5. ����recv���������ioContext���������ӵ�socket��Ͷ��recv����
	PER_IO_DATA *newIoContext = newSockContext->GetNewIOContext();
	newIoContext->m_OpType = RECV_POSTED;
	newIoContext->m_AcceptSocket = newSockContext->m_Sock.socket;
	// Ͷ��recv����
	if (false == _PostRecv(newSockContext, newIoContext))
	{
		//DoClose(sockContext);
		return false;
	}

	return true;
}

bool IOCPModel::_DoRecv(PER_HANDLE_DATA* handleInfo, PER_IO_DATA *ioInfo)
{
	RecvCompleted(handleInfo, ioInfo);

	if (false == _PostRecv(handleInfo, ioInfo))
	{
		this->_ShowMessage("Ͷ��WSARecv()ʧ��!\n");
			return false;
	}
	return true;
}

bool IOCPModel::_DoSend(PER_HANDLE_DATA* phd, PER_IO_DATA *pid)
{
	return true;
}


/////////////////////////////////////////////////////////////////
// Ͷ��I/O����
bool IOCPModel::_PostAccept(PER_HANDLE_DATA* handleInfo, PER_IO_DATA *ioInfo)
{
	DWORD dwBytes = 0;
	ioInfo->m_OpType = ACCEPT_POSTED;
	//ioInfo->ioSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == ioInfo->m_AcceptSocket)
	{
		return false;
	}

	// �����ջ�����Ϊ0,��AcceptExֱ�ӷ���,��ֹ�ܾ����񹥻�
	if (false == fnAcceptEx(m_ListenSockInfo->m_Sock.socket, ioInfo->m_AcceptSocket, ioInfo->m_wsaBuf.buf, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &dwBytes, &ioInfo->m_Overlapped))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			return false;
		}
	}

	//InterlockedIncrement(&acceptPostCnt);
	return true;
}

bool IOCPModel::_PostRecv(PER_HANDLE_DATA* phd, PER_IO_DATA *pid)
{
	DWORD RecvBytes = 0, Flags = 0;

	pid->Reset();
	pid->m_OpType = RECV_POSTED;	// read
	int nBytesRecv = WSARecv(phd->m_Sock.socket, &(pid->m_wsaBuf), 1, &RecvBytes, &Flags, &(pid->m_Overlapped), NULL);
	if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
	{
		cout << "�������ֵ���󣬲��Ҵ���Ĵ��벢����Pending�Ļ����Ǿ�˵������ص�����ʧ����\n";
		return false;
	}
	return true;
}

bool IOCPModel::_PostSend(PER_HANDLE_DATA* phd, PER_IO_DATA *pid)
{
	return true;
}