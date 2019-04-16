#include<IOCPModel.h>

/////////////////////////////////////////////////////////////////
// ����������
bool IOCPModel::Start()
{
	SYSTEM_INFO mySysInfo;	// ȷ���������ĺ�������	
	WinSocket serverSock;	// ������������ʽ�׽���
	WinSocket acceptSock; //listen���յ��׽���
	LPPER_HANDLE_DATA PerSocketData;

	DWORD RecvBytes, Flags = 0;

	m_IOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	//m_WorkThreadShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_IOCompletionPort == NULL)
		cout << "������ɶ˿�ʧ��!\n";

	// ����IO�߳�--�߳����洴���̳߳�
	// ���ڴ������ĺ������������߳�
	GetSystemInfo(&mySysInfo);
	for (DWORD i = 0; i < (mySysInfo.dwNumberOfProcessors * 2); ++i) {
		// �����������������̣߳�������ɶ˿ڴ��ݵ����߳�
		HANDLE WORKThread = CreateThread(NULL, 0, _WorkerThread, (void*)this, 0, NULL);
		if (NULL == WORKThread) {
			cerr << "�����߳̾��ʧ�ܣ�Error:" << GetLastError() << endl;
			system("pause");
			return -1;
		}
		CloseHandle(WORKThread);
	}

	serverSock.CreateSocket();
	unsigned port = 8888;
	serverSock.Bind(port);

	int listenResult = listen(serverSock.socket, 5);
	if (SOCKET_ERROR == listenResult) {
		cerr << "����ʧ��. Error: " << GetLastError() << endl;
		system("pause");
		return -1;
	}
	else
		cout << "����������׼�����������ڵȴ��ͻ��˵Ľ���...\n";

	while (true)
	{
		//break;//���Թرչ����߳�
		// �������ӣ���������ɶˣ����������AcceptEx()
		acceptSock = serverSock.Accept();
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

		PerSocketData = new PER_HANDLE_DATA();
		PerSocketData->m_Sock = acceptSock;

		CreateIoCompletionPort((HANDLE)(PerSocketData->m_Sock.socket), m_IOCompletionPort, (DWORD)PerSocketData, 0);

		// ��ʼ�ڽ����׽����ϴ���I/Oʹ���ص�I/O����,���½����׽�����Ͷ��һ�������첽,WSARecv��WSASend����
		// ��ЩI/O������ɺ󣬹������̻߳�ΪI/O�����ṩ����	
		// ��I/O��������(I/O�ص�)
		PER_IO_DATA* PerIoData = new PER_IO_DATA();
		ZeroMemory(&(PerIoData->m_Overlapped), sizeof(WSAOVERLAPPED));
		PerIoData->m_wsaBuf.len = DATABUF_SIZE;
		PerIoData->m_wsaBuf.buf = PerIoData->m_buffer;
		PerIoData->rmMode = READ;	// read
		WSARecv(PerSocketData->m_Sock.socket, &(PerIoData->m_wsaBuf), 1, &RecvBytes, &Flags, &(PerIoData->m_Overlapped), NULL);
		//int nBytesRecv = WSARecv(PerSocketData->m_Sock.socket, &(PerIoData->m_wsaBuf), 1, &RecvBytes, &Flags, &(PerIoData->m_Overlapped), NULL);
		//if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
		//{
		//	cout << "�������ֵ���󣬲��Ҵ���Ĵ��벢����Pending�Ļ����Ǿ�˵������ص�����ʧ����\n";
		//	return false;
		//}
	}

	return true;
}

/////////////////////////////////////////////////////////////////
// ��ʼ��������
bool IOCPModel::InitializeServer() 
{
	// ��ʼ���˳��߳��¼�
	m_WorkThreadShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// ��ʼ��IOCP
	if (_InitializeIOCP() == false)
	{
		this->_ShowMessage("��ʼ��IOCPʧ�ܣ�\n");
		return false;
	}
	else
	{
		this->_ShowMessage("��ʼ��IOCP��ϣ�\n");
	}
	// ��ʼ��������socket
	if (_InitializeListenSocket() == false)
	{
		this->_ShowMessage("��ʼ��������Socketʧ�ܣ�\n");
		this->_Deinitialize();
		return false;
	}
	else
	{
		this->_ShowMessage("��ʼ��������Socket��ϣ�\n");
	}

	this->_ShowMessage("����������׼�����������ڵȴ��ͻ��˵Ľ���......\n");
	return true;
}

/////////////////////////////////////////////////////////////////
// ����SOCKET��
bool IOCPModel::LoadSocketLib()
{
	WSADATA wsaData;
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
	if (m_IOCompletionPort == NULL)
	{
		this->_ShowMessage("��ɶ˿ڴ���ʧ��! Error��%d \n", WSAGetLastError());
		return false;
	}

	// ����ϵͳ��Ϣȷ���������������߳�����
	GetSystemInfo(&m_SysInfo);
	m_nThreads = m_SysInfo.dwNumberOfProcessors * 2;
	m_WorkerThreadsHandleArray = new HANDLE[m_nThreads];
	for (DWORD i = 0; i < m_nThreads; ++i)
	{
		HANDLE WORKTHREAD = CreateThread(NULL, 0, _WorkerThread, (LPVOID)this, 0, NULL);
		m_WorkerThreadsHandleArray[i] = WORKTHREAD;
		if (NULL == WORKTHREAD)
		{
			this->_ShowMessage("�����߳̾��ʧ�ܣ�Error��%d \n", GetLastError());
			system("pause");
			return -1;
		}
		CloseHandle(WORKTHREAD);
	}
	this->_ShowMessage("��ɴ����������߳�������%d ��\n", m_nThreads);

	return true;	
}


/////////////////////////////////////////////////////////////////
// ��ʼ��������Socket
bool IOCPModel::_InitializeListenSocket()
{
	m_ServerSocket.CreateSocket();
	m_ServerSocket.port=DEFAULT_PORT;
	m_ServerSocket.Bind(m_ServerSocket.port);
	
	if (SOCKET_ERROR == listen(m_ServerSocket.socket, 5/*SOMAXCONN*/))
	{
		this->_ShowMessage("����ʧ�ܣ�Error��%d \n", GetLastError());
		return false;
	}
	
	return true;
}


/////////////////////////////////////////////////////////////////
// �ͷ�������Դ
void IOCPModel::_Deinitialize()
{
	// �ر�ϵͳ�˳��¼����
	RELEASE_HANDLE(m_WorkThreadShutdownEvent);

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
// �����̺߳���
 DWORD WINAPI IOCPModel::_WorkerThread(LPVOID lpParam)
{
	 IOCPModel* IOCP = (IOCPModel*)lpParam;
	 LPPER_HANDLE_DATA handleInfo = NULL;
	 LPPER_IO_DATA ioInfo = NULL;
	 //����WSARecv()
	 DWORD RecvBytes;
	 DWORD Flags = 0;

	 XHttpResponse res;     //���ڴ���http����
	 while (true)
	 {

		 bool bRet = GetQueuedCompletionStatus(IOCP->m_IOCompletionPort, &RecvBytes, (PULONG_PTR)&(handleInfo),(LPOVERLAPPED*)&ioInfo, INFINITE);
		 if (bRet==false)
		 {
			 cerr << "GetQueuedCompletionStatus Error: " << GetLastError() << endl;
			 handleInfo->m_Sock.Close();    //����
			 delete handleInfo;
			 delete ioInfo;
			 continue;
		 }
		 //�յ��˳��ñ�־��ֱ���˳������߳�
		 //if (ioInfo->m_OpType==NULL_POSTED) {
			// break;
		 //}

		 //�ͻ��˵���closesocket�����˳�
		 if (RecvBytes == 0) {
			 handleInfo->m_Sock.Close();
			 delete handleInfo;
			 delete ioInfo;
			 continue;
		 }
		 //�ͻ���ֱ���˳���64����,ָ������������������
		 if ((GetLastError() == WAIT_TIMEOUT) || (GetLastError() == ERROR_NETNAME_DELETED))
		 {
			 handleInfo->m_Sock.Close();
			 delete handleInfo;
			 delete ioInfo;
			 continue;
		 }
		 switch (ioInfo->rmMode)
		 {
		 case WRITE:
			 break;
			 //case WRITE
		 case READ:
			 bool error = false;
			 bool sendfinish = false;
			 for (;;)
			 {
				 //���´���GET����
				 int buflend = strlen(ioInfo->m_buffer);

				 if (buflend <= 0) {
					 break;
				 }
				 if (!res.SetRequest(ioInfo->m_buffer)) {
					 cerr << "SetRequest failed!" << endl;
					 error = true;
					 break;
				 }
				 string head = res.GetHead();
				 if (handleInfo->m_Sock.Send(head.c_str(), head.size()) <= 0)
				 {
					 break;
				 }//��Ӧ��ͷ
				 for (;;)
				 {
					 char buf[10240];
					 //���ͻ���������ļ�����buf�в������ļ�����_len
					 int file_len = res.Read(buf, 10240);
					 if (file_len == 0)
					 {
						 sendfinish = true;
						 break;
					 }
					 if (file_len < 0)
					 {
						 error = true;
						 break;
					 }
					 if (handleInfo->m_Sock.Send(buf, file_len) <= 0)
					 {
						 break;
					 }
				 }//for(;;)
				 if (sendfinish)
				 {
					 break;
				 }
				 if (error)
				 {
					 cerr << "send file happen wrong ! client close !" << endl;
					 handleInfo->m_Sock.Close();
					 delete handleInfo;
					 delete ioInfo;
					 break;
				 }
			 }
			 break;
			 //case READ
		 }//switch
	 }//while
	 cout << "�����߳��˳���" << endl;
	 return 0;
}


/////////////////////////////////////////////////////////////////
// ��ӡ��Ϣ
void IOCPModel::_ShowMessage(const char* msg, ...) const
{
	std::cout << msg;
}

