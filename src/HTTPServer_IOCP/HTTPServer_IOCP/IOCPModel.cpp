#include <IOCPModel.h>
#include <mstcpip.h>

IOContextPool _PER_SOCKET_DATA::ioContextPool;		// static��Ա��ʼ��

/////////////////////////////////////////////////////////////////
// ����������
// public:
bool IOCPModel::StartServer(bool serveroption)
{
	// ����������״̬���
	if (m_ServerRunning) 
	{
		_ShowMessage("������������,�����ظ����У�\n");
		return false;
	}
	// ���ݴ���ѡ�񣬳�ʼ��������������Դ
	m_useAcceptEx = serveroption;
	_InitializeServerResource(m_useAcceptEx);

	// ��������ʼ����
	if (true == m_useAcceptEx)
	{
		_StartEX();
	}
	else// (false == m_useAcceptEx)
	{
		_Start();
	}
	return true;
}
// private:
bool IOCPModel::_StartEX()
{
	m_ServerRunning = RUNNING;
	this->_ShowMessage("����������׼�����������ڵȴ��ͻ��˵Ľ���......\n");

	while (m_ServerRunning)
	{
		Sleep(1000);
	}
	return true;
}
bool IOCPModel::_Start()
{
	m_ServerRunning = RUNNING;
	this->_ShowMessage("����������׼�����������ڵȴ��ͻ��˵Ľ���......\n");

	// ������accept()���ܿͻ������׽���
	WinSock acceptSock; 
	LPPER_SOCKET_DATA handleInfo;

	DWORD RecvBytes, Flags = 0;

	while (true)
	{
		acceptSock = m_ServerSock.Accept();
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
		handleInfo = new PER_SOCKET_DATA();
		handleInfo->m_Sock = acceptSock;
		CreateIoCompletionPort((HANDLE)(handleInfo->m_Sock.socket), m_hIOCompletionPort, (DWORD)handleInfo, 0);
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
	LPPER_SOCKET_DATA handleInfo = NULL;
	LPPER_IO_DATA ioInfo = NULL;
	DWORD RecvBytes;
	DWORD Flags = 0;

	while (WAIT_OBJECT_0 != WaitForSingleObject(IOCP->m_hWorkerShutdownEvent,0))
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
bool IOCPModel::_InitializeServerResource(bool useAcceptEX)
{
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
		
	// ��ʼ��������socket,�ж��Ƿ�ʹ��AcceptEX()
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
	// ʹ��AcceptEX()
	if (useAcceptEX == true)
	{
		if (false == _InitializeListenSocket())
		{
			this->_ShowMessage("��ʼ������������Socketʧ�ܣ�\n");
			this->_Deinitialize();
			return false;
		}
		else
		{
			this->_ShowMessage("��ʼ������������Socket��ϣ�\n");
		}
	}

	return true;
}

/////////////////////////////////////////////////////////////////
// ����SOCKET��
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
	m_hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	//m_WorkThreadShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (NULL == m_hIOCompletionPort)
		cout << "������ɶ˿�ʧ��!\n";

	// ����IO�߳�--�߳����洴���̳߳�
	// ���ڴ������ĺ������������߳�
	m_nThreads = _GetNumberOfProcessors() * 2;
	m_phWorkerThreadArray = new HANDLE[m_nThreads];
	for (DWORD i = 0; i <m_nThreads; ++i)
	{
		// �����������������̣߳�������ɶ˿ڴ��ݵ����߳�
		HANDLE hWorkThread = CreateThread(NULL, NULL, _WorkerThread, (void*)this, 0, NULL);
		if (NULL == hWorkThread) {
			cerr << "�����߳̾��ʧ�ܣ�Error:" << GetLastError() << endl;
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

	m_ListenSockInfo = new PER_SOCKET_DATA;
	m_ListenSockInfo->m_Sock.socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if(INVALID_SOCKET == m_ListenSockInfo->m_Sock.socket)
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
		cerr << "����ʧ��. Error: " << GetLastError() << endl;
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
		LPPER_IO_DATA  pAcceptIoContext = new PER_IO_DATA;
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
// �����������׽���
bool IOCPModel::_InitializeServerSocket()
{
	m_ServerSock.CreateSocket();
	m_ServerSock.Bind(DEFAULT_PORT);

	// ��ʼ����
	if (SOCKET_ERROR == listen(m_ServerSock.socket, SOMAXCONN)) {
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
	RELEASE_HANDLE(m_hWorkerShutdownEvent);

	// �ͷŹ������߳̾��ָ��
	for (int i = 0; i < m_nThreads; i++)
	{
		RELEASE_HANDLE(m_phWorkerThreadArray[i]);
	}
	delete[] m_phWorkerThreadArray;

	// �ر�IOCP���
	RELEASE_HANDLE(m_hIOCompletionPort);

	// �رշ�����socket
	// ������������ûд �ͷ� WinSocket    m_ServerSocket;

	this->_ShowMessage("�ͷ���Դ��ϣ�\n");
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
bool IOCPModel::_PostAccept(PER_IO_DATA *pAcceptIoContext)
{
	ASSERT(ioinfo != m_ListenSockInfo.m_sock.socket);
	
	//׼������
	// ��accept������,Ϊ�����ӵĿͻ���׼����socket
	DWORD dwBytes = 0;
	pAcceptIoContext->m_OpType = ACCEPT_POSTED;
	pAcceptIoContext->m_AcceptSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == pAcceptIoContext->m_AcceptSocket)
	{
		printf("��������Accept��Socketʧ�ܣ��������: %d", WSAGetLastError());
		return false;
	}

	// Ͷ��AccpetEX
	//cout << "Process AcceptEx function wait for client connect..." << endl;
	if (FALSE == m_lpfnAcceptEx(
		m_ListenSockInfo->m_Sock.socket,
		pAcceptIoContext->m_AcceptSocket,
		pAcceptIoContext->m_wsaBuf.buf,
		0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		&dwBytes,
		&pAcceptIoContext->m_Overlapped))
	{
		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			printf("Ͷ�� AcceptEx ����ʧ�ܣ��������: %d", WSAGetLastError());
			return false;
		}
	}
	return true;
}

bool IOCPModel::_PostRecv(PER_SOCKET_DATA* phd, PER_IO_DATA *pid)
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

bool IOCPModel::_PostSend(PER_SOCKET_DATA* phd, PER_IO_DATA *pid)
{
	return true;
}
/////////////////////////////////////////////////////////////////
// ����I/O����
bool IOCPModel::_DoAccept(PER_SOCKET_DATA* handleInfo, PER_IO_DATA *ioInfo)
{
	SOCKADDR_IN *clientAddr = NULL;
	SOCKADDR_IN *localAddr = NULL;
	int clientAddrLen = sizeof(SOCKADDR_IN);
	int localAddrLen = sizeof(SOCKADDR_IN);

	// 1. ��ȡ��ַ��Ϣ ��GetAcceptExSockAddrs�����������Ի�ȡ��ַ��Ϣ��������˳��ȡ����һ�����ݣ�
	m_lpfnGetAcceptExSockAddrs(ioInfo->m_wsaBuf.buf, 0, localAddrLen, clientAddrLen, (LPSOCKADDR *)&localAddr, &localAddrLen, (LPSOCKADDR *)&clientAddr, &clientAddrLen);
	//cout << "�ͻ��ˣ�" << inet_ntoa(clientAddr->sin_addr) << ":" << ntohs(clientAddr->sin_port)<< " ����\n";


	// 2. Ϊ�����ӽ���һ��SocketContext 
	PER_SOCKET_DATA *pNewSockContext = new PER_SOCKET_DATA;
	pNewSockContext->m_Sock.socket	 = ioInfo->m_AcceptSocket;
	memcpy(&(pNewSockContext->m_Sock.addr), clientAddr, sizeof(SOCKADDR_IN));

	// 3. ����socket����ɶ˿ڰ�
	if (NULL == CreateIoCompletionPort((HANDLE)pNewSockContext->m_Sock.socket, m_hIOCompletionPort, (DWORD)pNewSockContext, 0))
	{
		DWORD dwErr = WSAGetLastError();
		if (dwErr != ERROR_INVALID_PARAMETER)
		{
			//DoClose(newSockContext);
			return false;
		}
	}

	// 4. ����recv���������ioContext���������ӵ�socket��Ͷ��recv����
	PER_IO_DATA *pNewIoContext = pNewSockContext->GetNewIOContext();
	pNewIoContext->m_OpType = RECV_POSTED;
	pNewIoContext->m_AcceptSocket = pNewSockContext->m_Sock.socket;
	// Ͷ��recv����
	if (false == this->_PostRecv(pNewSockContext, pNewIoContext))
	{
		//DoClose(sockContext);
		return false;
	}

	// 5. ��listenSocketContext��IOContext ���ú����Ͷ��AcceptEx
	ioInfo->Reset();
	if (false == this->_PostAccept(ioInfo))
	{
		m_ListenSockInfo->RemoveIOContext(ioInfo);
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

bool IOCPModel::_DoRecv(PER_SOCKET_DATA* handleInfo, PER_IO_DATA *ioInfo)
{
	RecvCompleted(handleInfo, ioInfo);

	if (false == _PostRecv(handleInfo, ioInfo))
	{
		this->_ShowMessage("Ͷ��WSARecv()ʧ��!\n");
		return false;
	}
	return true;
}

bool IOCPModel::_DoSend(PER_SOCKET_DATA* phd, PER_IO_DATA *pid)
{
	return true;
}
