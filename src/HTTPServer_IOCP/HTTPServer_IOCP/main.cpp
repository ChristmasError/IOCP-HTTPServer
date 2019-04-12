#include"WinSocket.h"
#include"XHttpResponse.h"
#include<thread>
#include<iostream>
#include <WinSock2.h>
#include <vector>
#include<queue>
#include<cstdlib>

using namespace std;

#pragma comment(lib, "Ws2_32.lib")		// Socket������õĶ�̬���ӿ�
#pragma comment(lib, "Kernel32.lib")	// IOCP��Ҫ�õ��Ķ�̬���ӿ�

#define READ 3
#define WRITE 5
#define test 111
typedef struct {
	WinSocket socket;//�ͻ���socket
					 //SOCKADDR_STORAGE ClientAddr;//�ͻ��˵�ַ
}PER_HANDLE_DATA,*LPPER_HANDLE_DATA;

//�ص�IO�õĽṹ�壬��ʱ��¼IO����
typedef struct {
	WSAOVERLAPPED Overlapped;   //OVERLAPPED�ṹ���ýṹ�����һ��event�¼�����,������ڽṹ����λ����Ϊ�׵�ַ
								//���¿��Զ�����չ
	WSABUF DataBuf;				//WSABUF�ṹ��������Ա��һ��ָ��ָ��buf����һ��buf�ĳ���len
	char buffer[10240];			//��Ϣ����
	DWORD rmMode;		//��־λREAD OR WRITE
}PER_IO_DATA,*LPPER_IO_DATA;

//vector < PER_IO_SOCKET* > clientGroup;		// ��¼�ͻ��˵�������

HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);

DWORD WINAPI ServerWorkThread(LPVOID completionPort);
//DWORD WINAPI ServerSendThread(LPVOID IpParam);

int main(int argc, char* argv[])
{
	HANDLE completionPort;
	SYSTEM_INFO mySysInfo;	// ȷ���������ĺ�������	
	WinSocket serverSock;	// ������������ʽ�׽���
	WinSocket acceptSock; //listen���յ��׽���
	LPPER_HANDLE_DATA PerSocketData;

	DWORD RecvBytes, Flags = 0;

	completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (completionPort == NULL)
		cout << "������ɶ˿�ʧ��!\n";

	// ����IO�߳�--�߳����洴���̳߳�
	// ���ڴ������ĺ������������߳�
	GetSystemInfo(&mySysInfo);
	for (DWORD i = 0; i < (mySysInfo.dwNumberOfProcessors * 2); ++i) {
		// �����������������̣߳�������ɶ˿ڴ��ݵ����߳�
		HANDLE WORKThread = CreateThread(NULL, 0, ServerWorkThread, completionPort, 0, NULL);
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

		CreateIoCompletionPort((HANDLE)(PerSocketData->socket.socket), completionPort, (DWORD)PerSocketData, 0);

		// ��ʼ�ڽ����׽����ϴ���I/Oʹ���ص�I/O����,���½����׽�����Ͷ��һ�������첽,WSARecv��WSASend����
		// ��ЩI/O������ɺ󣬹������̻߳�ΪI/O�����ṩ����	
		// ��I/O��������(I/O�ص�)
		PER_IO_DATA* PerIoData = new PER_IO_DATA();
		ZeroMemory(&(PerIoData->Overlapped), sizeof(WSAOVERLAPPED));
		PerIoData->DataBuf.len = 1024;
		PerIoData->DataBuf.buf = PerIoData->buffer;
		PerIoData->rmMode = READ;	// read
		WSARecv(PerSocketData->socket.socket, &(PerIoData->DataBuf), 1, &RecvBytes, &Flags, &(PerIoData->Overlapped), NULL);
		PerSocketData->socket.Close();
	}
	serverSock.Close();
	acceptSock.Close();
	WSACleanup();
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI ServerWorkThread(LPVOID IpParam)
{
	HANDLE CompletionPort = (HANDLE)IpParam;
	DWORD BytesTrans;
	LPOVERLAPPED IpOverlapped;
	LPPER_HANDLE_DATA handleInfo=NULL;
	LPPER_IO_DATA ioInfo = NULL;
	//����WSARecv()
	DWORD RecvBytes;
	DWORD Flags = 0;
	
	XHttpResponse res;     //���ڴ���http����
	while (true)
	{
		if (GetQueuedCompletionStatus(
					CompletionPort,					//�����IO��Ϣ����ɶ˿ھ��
					&BytesTrans,					//���ڱ���I/O�����д�ˮ���ݴ�С�ı�����ַ
					(PULONG_PTR)&(handleInfo),		//����CreateIoCompletionPort()�����������ı�����ֵַ
					(LPOVERLAPPED*)&ioInfo,			//�������WSASend(),WSARecv()ʱ���ݵ�OVERLAPPED�ṹ���ַ�ı�����ֵַ
					INFINITE)						//��ʱ��Ϣ
			== false)
		{
			cerr << "GetQueuedCompletionStatus Error: " << GetLastError() << endl;
			handleInfo->socket.Close();
			delete handleInfo;
			delete ioInfo;
			continue;
		}
		if (0 == BytesTrans) {
			handleInfo->socket.Close();
			//GlobalFree(handleInfo);//������
			//GlobalFree(ioInfo);
			delete handleInfo;
			delete ioInfo;
			continue;
		}
		WaitForSingleObject(hMutex, INFINITE);
		if (ioInfo->rmMode == WRITE)
			continue;
		if (ioInfo->rmMode == READ)
		{ 	
			bool error = false;
			bool sendfinish = false;
			for(;;)
			{
				//cout << "ioInfo->DataBuf.buf:\n" << ioInfo->buffer << endl;
				//���´���GET����
				int buflend = strlen(ioInfo->buffer);

				if (buflend <= 0) {
					break;
				}
				if (!res.SetRequest(ioInfo->buffer)) {
					cout << "SetRequest failed!\n";
					error = true;
					break;
				}
				//cout << "after set request:\n" << ioInfo->buffer << endl;
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
					if (file_len < 0)
					{
						error = true;
						break;
					}
					else if (file_len == 0) {
						sendfinish = true;
						break;
					}
					if (handleInfo->socket.Send(buf, file_len) <= 0)
					{
						break;
					}
					//memset(&(ioInfo->Overlapped), 0, sizeof(OVERLAPPED));
					//ioInfo->DataBuf.len = file_len;
					//ioInfo->DataBuf.buf = buf;
					//ioInfo->rmMode = WRITE;//WRITE
					//WSASend(SENDINFO.handleInfo.socket.socket, &(SENDINFO.ioInfo.DataBuf), 1, NULL, 0, &(SENDINFO.ioInfo.Overlapped), NULL);
				}
				if (sendfinish)
					break;
				if (error)
				{
					cout << "something wrong ! client close !\n";
					handleInfo->socket.Close();
					break;
				}
			}

		}//if READ

		ReleaseMutex(hMutex);

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