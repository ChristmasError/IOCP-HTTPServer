#include<WinSocket.h>		//��װ�õ�socket���,��Ŀ���������dllλ������Ŀ¼��lib�ļ�����
#include<WinSock2.h>
#include<thread>
#include<iostream>
#include<stdio.h>
#include"XHttpResponse.h"
using namespace std;

#pragma comment(lib, "Kernel32.lib")	// IOCP��Ҫ�õ��Ķ�̬���ӿ�

#define READ 3
#define WRITE 5
#define DATABUF_SIZE 1024
#define WORK_THREADS_EXIT_CODE NULL
HANDLE WorkThreadShutdownEvent = NULL;

typedef struct {
	WinSocket socket;//�ͻ���socket��Ϣ		 
}PER_HANDLE_DATA,*LPPER_HANDLE_DATA;

//�ص�IO�õĽṹ�壬��ʱ��¼IO����
typedef struct {
	WSAOVERLAPPED Overlapped;   //OVERLAPPED�ṹ���ýṹ�����һ��event�¼�����,������ڽṹ����λ����Ϊ�׵�ַ
	WSABUF DataBuf;				//WSABUF�ṹ��������Ա��һ��ָ��ָ��buf����һ��buf�ĳ���len
	char buffer[DATABUF_SIZE];			//��Ϣ����
	DWORD rmMode;		//��־λREAD OR WRITE
}PER_IO_DATA,*LPPER_IO_DATA;

typedef struct{
	HANDLE completionPort;
	HANDLE workThreadsQuitEvent;
}IOCP_DATA;

DWORD WINAPI ServerWorkThread(LPVOID completionPort);

int main(int argc, char* argv[])
{
	//HANDLE completionPort;
	IOCP_DATA IOCP;
	SYSTEM_INFO mySysInfo;	// ȷ���������ĺ�������	
	WinSocket serverSock;	// ������������ʽ�׽���
	WinSocket acceptSock; //listen���յ��׽���
	LPPER_HANDLE_DATA PerSocketData;

	DWORD RecvBytes, Flags = 0;

	IOCP.completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	IOCP.workThreadsQuitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (IOCP.completionPort == NULL)
		cout << "������ɶ˿�ʧ��!\n";

	// ����IO�߳�--�߳����洴���̳߳�
	// ���ڴ������ĺ������������߳�
	GetSystemInfo(&mySysInfo);
	for (DWORD i = 0; i < (mySysInfo.dwNumberOfProcessors * 2); ++i) {
		// �����������������̣߳�������ɶ˿ڴ��ݵ����߳�
		HANDLE WORKThread = CreateThread(NULL, 0, ServerWorkThread, (LPVOID)&IOCP, 0, NULL);
		if (NULL == WORKThread) {
			cerr << "�����߳̾��ʧ�ܣ�Error:" << GetLastError() << endl;
			system("pause");
			return -1;
		}
		CloseHandle(WORKThread);
	}
	//for (DWORD i = 0; i <(mySysInfo.dwNumberOfProcessors ); ++i) {
	//	HANDLE SENDThread = CreateThread(NULL, 0, ServerSendThread, 0, 0, NULL);
	//	if (NULL == SENDThread) {
	//		cerr << "�������;��ʧ�ܣ�Error:" << GetLastError() << endl;
	//		system("pause");
	//		return -1;
	//	}
	//	CloseHandle(SENDThread);
	//}

	serverSock.CreateSocket();
	unsigned port = 8888;
	if (argc > 1)
		port = atoi(argv[1]);
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
		PerSocketData->socket = acceptSock;

		CreateIoCompletionPort((HANDLE)(PerSocketData->socket.socket), IOCP.completionPort, (DWORD)PerSocketData, 0);

		// ��ʼ�ڽ����׽����ϴ���I/Oʹ���ص�I/O����,���½����׽�����Ͷ��һ�������첽,WSARecv��WSASend����
		// ��ЩI/O������ɺ󣬹������̻߳�ΪI/O�����ṩ����	
		// ��I/O��������(I/O�ص�)
		PER_IO_DATA* PerIoData = new PER_IO_DATA();
		ZeroMemory(&(PerIoData->Overlapped), sizeof(WSAOVERLAPPED));
		PerIoData->DataBuf.len = 1024;
		PerIoData->DataBuf.buf = PerIoData->buffer;
		PerIoData->rmMode = READ;	// read
		WSARecv(PerSocketData->socket.socket, &(PerIoData->DataBuf), 1, &RecvBytes, &Flags, &(PerIoData->Overlapped), NULL);
	}
	if (serverSock.socket != INVALID_SOCKET)
	{
		WorkThreadShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		SetEvent(WorkThreadShutdownEvent);
		for(int i=0;i<(mySysInfo.dwNumberOfProcessors * 2); ++i)
		{
			PostQueuedCompletionStatus(IOCP.completionPort, 0, (DWORD)WORK_THREADS_EXIT_CODE, NULL);
		}
		ResetEvent(WorkThreadShutdownEvent);
	}
	serverSock.Close();
	acceptSock.Close();
	WSACleanup();
	while (1);
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI ServerWorkThread(LPVOID IpParam)
{
	//HANDLE CompletionPort = (HANDLE)IpParam;
	IOCP_DATA* IOCP=(IOCP_DATA*)IpParam;
	DWORD BytesTrans;
	LPOVERLAPPED IpOverlapped;
	LPPER_HANDLE_DATA handleInfo=NULL;
	LPPER_IO_DATA ioInfo = NULL;
	//����WSARecv()
	DWORD RecvBytes;
	DWORD Flags = 0;
	
	XHttpResponse res;     //���ڴ���http����
	while (WAIT_OBJECT_0!=WaitForSingleObject(IOCP->workThreadsQuitEvent , 0))
	{
		//if (GetQueuedCompletionStatus(
		//			CompletionPort,					//�����IO��Ϣ����ɶ˿ھ��
		//			&BytesTrans,					//���ڱ���I/O�����д�ˮ���ݴ�С�ı�����ַ
		//			(PULONG_PTR)&(handleInfo),		//����CreateIoCompletionPort()�����������ı�����ֵַ
		//			(LPOVERLAPPED*)&ioInfo,			//�������WSASend(),WSARecv()ʱ���ݵ�OVERLAPPED�ṹ���ַ�ı�����ֵַ
		//			INFINITE)						//��ʱ��Ϣ
		//	== false)
		if (GetQueuedCompletionStatus(IOCP->completionPort,&BytesTrans,(PULONG_PTR)&(handleInfo),(LPOVERLAPPED*)&ioInfo,INFINITE)== false)
		{
			cerr << "GetQueuedCompletionStatus Error: " << GetLastError() << endl;
			handleInfo->socket.Close();
			delete handleInfo;
			delete ioInfo;
			continue;
		}
		//�յ��˳��ñ�־��ֱ���˳������߳�
		if (handleInfo == WORK_THREADS_EXIT_CODE) {
			break;
		}
			
		//�ͻ��˵���closesocket�����˳�
		if (BytesTrans == 0) {
			handleInfo->socket.Close();
			delete handleInfo;
			delete ioInfo;
			continue;
		}
		//�ͻ���ֱ���˳���64����,ָ������������������
		if ((GetLastError() == WAIT_TIMEOUT) || (GetLastError() == ERROR_NETNAME_DELETED))
		{
			handleInfo->socket.Close();
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
				int buflend = strlen(ioInfo->buffer);

				if (buflend <= 0) {
					break;
				}
				if (!res.SetRequest(ioInfo->buffer)) {
					cerr<<"SetRequest failed!"<<endl;
					error = true;
					break;
				}
				string head = res.GetHead();
				if (handleInfo->socket.Send(head.c_str(), head.size()) <= 0)
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
					if (handleInfo->socket.Send(buf, file_len) <= 0)
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
					cerr << "send file happen wrong ! client close !"<<endl;
					handleInfo->socket.Close();
					delete handleInfo;
					delete ioInfo;
					break;
				}
			}
			break;
			//case READ
		}//switch

		 // Ϊ��һ���ص����ý�����I/O��������
		ZeroMemory(&(ioInfo->Overlapped), sizeof(OVERLAPPED)); // ����ڴ�
		ioInfo->DataBuf.len = 1024;
		ioInfo->DataBuf.buf = ioInfo->buffer;
		ioInfo->rmMode = READ;
		WSARecv(handleInfo->socket.socket,		//client socket
				&(ioInfo->DataBuf),				//*WSABUF
				1,								//number of BUFFER	
				&RecvBytes,						//���ܵ��ֽ���
				&Flags,							//��־λ
				&(ioInfo->Overlapped),			//overlaopped�ṹ��ַ
				NULL);							//ûɶ��
	}//while
	cout << "�����߳��˳���" << endl;
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DWORD WINAPI ServerSendThread(LPVOID IpParam)
//{
//	XHttpResponse res;     //���ڴ���http����
//	while (1)
//	{
//		WaitForSingleObject(hMutex, INFINITE);
//		if (!Queue.empty())
//		{
//			SEND_INFO SENDINFO = Queue.front();
//			Queue.pop();
//			ReleaseMutex(hMutex);
//			//���´���GET����
//			char* buf = SENDINFO.ioInfo.buffer;
//			int buflend = strlen(SENDINFO.ioInfo.buffer);
//
//			if (buflend <= 0) {
//				continue;
//			}
//			if (!res.SetRequest(buf)) {
//				cout << "SetRequest failed!\n";
//				continue;
//			}
//			string head = res.GetHead();
//			if (SENDINFO.handleInfo.socket.Send(head.c_str(), head.size()) <= 0)
//			{
//				//������
//				break;
//			}//��Ӧ��ͷ
//			bool error = false;
//			for (;;)
//			{
//				//���ͻ���������ļ�����buf�в������ļ�����_len
//				int file_len = res.Read(buf, buflend);
//				if (file_len < 0)
//				{
//					error = true;
//					break;
//				}
//				else if (file_len == 0) {
//					break;
//				}
//
//				memset(&(SENDINFO.ioInfo.Overlapped), 0, sizeof(OVERLAPPED));
//				SENDINFO.ioInfo.DataBuf.len = file_len;
//				SENDINFO.ioInfo.DataBuf.buf = buf;
//				SENDINFO.ioInfo.rmMode = WRITE;//WRITE
//
//				if (SENDINFO.handleInfo.socket.Send(buf, file_len) <= 0)
//				{					
//					break;
//				}
//				//WSASend(SENDINFO.handleInfo.socket.socket, &(SENDINFO.ioInfo.DataBuf), 1, NULL, 0, &(SENDINFO.ioInfo.Overlapped), NULL);
//			}
//			if (error)
//			{
//				cout << "something wrong ! client close !\n";
//				SENDINFO.handleInfo.socket.Close();
//			}
//		}
//		else
//		{
//			ReleaseMutex(hMutex);
//			continue;
//		}
//
//	}//while
//	return 0;
//};
