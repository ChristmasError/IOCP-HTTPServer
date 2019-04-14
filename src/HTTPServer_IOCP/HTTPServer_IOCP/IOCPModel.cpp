#include<IOCPModel.h>

bool IOCPModel::InitializeServer() 
{
	// ��ʼ���˳��߳��¼�
	m_WorkThreadShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// ��ʼ��IOCP
	if (_InitializeIOCP == false)
	{
		this->_ShowMessage("��ʼ��IOCPʧ�ܣ�\n");
		return false;
	}
	else
	{
		this->_ShowMessage("��ʼ��IOCP��ϣ�\n");
	}
	// ��ʼ��������socket
	if (_InitializeListenSocket == false)
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
		HANDLE WORKTHREAD = CreateThread(NULL, 0, _WorkerThread, (LPVOID)&m_IOCompletionPort, 0, NULL);
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
	m_ServerSocket.port=atoi(DEFAULT_IP);
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
// ��ӡ��Ϣ
void IOCPModel::_ShowMessage(const char* msg, ...) const
{
	std::cout << msg;
}