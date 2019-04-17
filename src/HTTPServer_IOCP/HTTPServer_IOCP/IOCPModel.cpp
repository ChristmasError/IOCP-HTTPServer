#include<IOCPModel.h>

/////////////////////////////////////////////////////////////////
// ����������
bool IOCPModel::Start()
{
	// ����������״̬���
	if (_ServerRunning == RUNNING) {
		_ShowMessage("������������,�����ظ����У�\n");
		return false;
	}
	else
		_ServerRunning = RUNNING;

	WinSocket acceptSock; //listen���յ��׽���
	LPPER_HANDLE_DATA PerSocketData;

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

		PerSocketData = new PER_HANDLE_DATA();
		PerSocketData->m_Sock = acceptSock;

		CreateIoCompletionPort((HANDLE)(PerSocketData->m_Sock.socket), m_IOCompletionPort, (DWORD)PerSocketData, 0);

		// ��ʼ�ڽ����׽����ϴ���I/Oʹ���ص�I/O����,���½����׽�����Ͷ��һ�������첽,WSARecv��WSASend����
		// ��ЩI/O������ɺ󣬹������̻߳�ΪI/O�����ṩ����	
		// ��I/O��������(I/O�ص�)
		PER_IO_DATA* PerIoData = new PER_IO_DATA();
		PerIoData->m_OpType = RECV_POSTED;	// read
		int nBytesRecv = WSARecv(PerSocketData->m_Sock.socket, &(PerIoData->m_wsaBuf), 1, &RecvBytes, &Flags, &(PerIoData->m_Overlapped), NULL);
		if ((SOCKET_ERROR == nBytesRecv) && (WSA_IO_PENDING != WSAGetLastError()))
		{
			cout << "�������ֵ���󣬲��Ҵ���Ĵ��벢����Pending�Ļ����Ǿ�˵������ص�����ʧ����\n";
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

	XHttpResponse res;     //���ڴ���http����
	while (WaitForSingleObject(IOCP->m_WorkerShutdownEvent,0) != WAIT_OBJECT_0)
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
				case SEND_POSTED:
					break;
					//case WRITE
				case RECV_POSTED:
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
				ioInfo->Reset();
				ioInfo->m_OpType = RECV_POSTED;	// read
				WSARecv(handleInfo->m_Sock.socket, &(ioInfo->m_wsaBuf), 1, &RecvBytes, &Flags, &(ioInfo->m_Overlapped), NULL);
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
bool IOCPModel::InitializeServerResource()
{
	// ������������
	_ServerRunning = STOP;

	// ��ʼ���˳��߳��¼�
	m_WorkerShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

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
	if (m_IOCompletionPort == NULL)
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
	m_ServerSocket.CreateSocket();
	m_ServerSocket.port = DEFAULT_PORT;
	m_ServerSocket.Bind(m_ServerSocket.port);

	//if (NULL == CreateIoCompletionPort((HANDLE)&m_ServerSocket.socket, m_IOCompletionPort, (DWORD)&m_ServerSocket, 0))
	//{
	//	//RELEASE_SOCKET(listenSockContext->connSocket);
	//	return false;
	//}

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

