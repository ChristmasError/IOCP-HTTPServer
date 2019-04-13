//#include"WinSocket.h"
//#include"XHttpResponse.h"
//#include<thread>
//#include<iostream>
//#include <WinSock2.h>
//#include <vector>
//#include<queue>
//#include<cstdlib>
//
//using namespace std;
//
//#pragma comment(lib, "Ws2_32.lib")		// Socket������õĶ�̬���ӿ�
//#pragma comment(lib, "Kernel32.lib")	// IOCP��Ҫ�õ��Ķ�̬���ӿ�
//
//#define READ 3
//#define WRITE 5
//
//typedef struct {
//	WinSocket socket;//�ͻ���socket
//}PER_HANDLE_DATA;
//
////�ص�IO�õĽṹ�壬��ʱ��¼IO����
//typedef struct {
//	WSAOVERLAPPED Overlapped;   //OVERLAPPED�ṹ���ýṹ�����һ��event�¼�����,������ڽṹ����λ����Ϊ�׵�ַ
//								//���¿��Զ�����չ
//	WSABUF DataBuf;				//WSABUF�ṹ��������Ա��һ��ָ��ָ��buf����һ��buf�ĳ���len
//	char buffer[10240];			//��Ϣ����
//	DWORD rmMode;		//��־λREAD OR WRITE
//}PER_IO_DATA;
//
//typedef struct SEND_INFO {
//	PER_HANDLE_DATA handleInfo;
//	PER_IO_DATA ioInfo;
//	SEND_INFO(PER_HANDLE_DATA h, PER_IO_DATA l) :handleInfo(h), ioInfo(l) {
//	};
//};
//
//queue<SEND_INFO>Queue;
////vector < PER_IO_SOCKET* > clientGroup;		// ��¼�ͻ��˵�������
//
//HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);
//
//DWORD WINAPI ServerWorkThread(LPVOID completionPort);
//DWORD WINAPI ServerSendThread(LPVOID IpParam);
//
//int main(int argc, char* argv[])
//{
//	HANDLE completionPort;
//	SYSTEM_INFO mySysInfo;	// ȷ���������ĺ�������	
//	WinSocket serverSock;	// ������������ʽ�׽���
//	WinSocket acceptSock; //listen���յ��׽���
//	PER_HANDLE_DATA* PerSocketData;
//
//	DWORD RecvBytes, Flags = 0;
//	// ����IOCP���ں˶���
//	/**
//	* ��Ҫ�õ��ĺ�����ԭ�ͣ�
//	* HANDLE WINAPI CreateIoCompletionPort(
//	*    __in   HANDLE FileHandle,		// �Ѿ��򿪵��ļ�������߿վ����һ���ǿͻ��˵ľ��
//	*    __in   HANDLE ExistingCompletionPort,	// �Ѿ����ڵ�IOCP���
//	*    __in   ULONG_PTR CompletionKey,	// ��ɼ���������ָ��I/O��ɰ���ָ���ļ�
//	*    __in   DWORD NumberOfConcurrentThreads // ��������ͬʱִ������߳�����һ���ƽ���CPU������*2
//	* );
//	**/
//	completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
//	if (completionPort == NULL)
//		cout << "������ɶ˿�ʧ��!\n";
//
//	// ����IOCP�߳�--�߳����洴���̳߳�
//
//	GetSystemInfo(&mySysInfo);
//
//	// ���ڴ������ĺ������������߳�
//	for (DWORD i = 0; i < (mySysInfo.dwNumberOfProcessors * 2); ++i) {
//		// �����������������̣߳�������ɶ˿ڴ��ݵ����߳�
//		// DWORD WINAPI ServerWorkThread(LPVOID completionPort)
//		HANDLE WORKThread = CreateThread(NULL, 0, ServerWorkThread, completionPort, 0, NULL);
//		if (NULL == WORKThread) {
//			cerr << "�����߳̾��ʧ�ܣ�Error:" << GetLastError() << endl;
//			system("pause");
//			return -1;
//		}
//		CloseHandle(WORKThread);
//	}
//	//                   (mySysInfo.dwNumberOfProcessors)
//	for (DWORD i = 0; i <(mySysInfo.dwNumberOfProcessors); ++i) {
//		HANDLE SENDThread = CreateThread(NULL, 0, ServerSendThread, 0, 0, NULL);
//		if (NULL == SENDThread) {
//			cerr << "�������;��ʧ�ܣ�Error:" << GetLastError() << endl;
//			system("pause");
//			return -1;
//		}
//		CloseHandle(SENDThread);
//	}
//
//	serverSock.CreateSocket();
//	/*u_long mode = 1;
//	ioctlsocket(httpserver.socket, FIONBIO,&mode );*/
//	unsigned port = 8888;
//	if (argc > 1)
//		port = atoi(argv[1]);
//	serverSock.Bind(port);
//
//	int listenResult = listen(serverSock.socket, 5);
//	if (SOCKET_ERROR == listenResult) {
//		cerr << "����ʧ��. Error: " << GetLastError() << endl;
//		system("pause");
//		return -1;
//	}
//	else
//		cout << "����������׼�����������ڵȴ��ͻ��˵Ľ���...\n";
//
//	//�������ڷ������ݵ��߳�
//	//HANDLE sendThread = CreateThread(NULL, 0, ServerSendThread, 0, 0, NULL);
//
//	while (true)
//	{
//		// �������ӣ���������ɶˣ����������AcceptEx()
//		acceptSock = serverSock.Accept();
//		acceptSock.SetBlock(false);
//		if (INVALID_SOCKET == acceptSock.socket)
//		{
//			if (WSAGetLastError() == WSAEWOULDBLOCK)
//				continue;
//			else
//			{
//				cerr << "Accept Socket Error: " << GetLastError() << endl;
//				system("pause");
//				return -1;
//			}
//		}
//
//		PerSocketData = new PER_HANDLE_DATA();
//		PerSocketData->socket = acceptSock;
//
//		CreateIoCompletionPort((HANDLE)(PerSocketData->socket.socket), completionPort, (DWORD)PerSocketData, 0);
//		//���ܵ��׽�������ɶ˿ڹ���
//
//		// ��ʼ�ڽ����׽����ϴ���I/Oʹ���ص�I/O����,���½����׽�����Ͷ��һ�������첽,WSARecv��WSASend����
//		// ��ЩI/O������ɺ󣬹������̻߳�ΪI/O�����ṩ����	
//		// ��I/O��������(I/O�ص�)
//		PER_IO_DATA* PerIoData = new PER_IO_DATA();
//		ZeroMemory(&(PerIoData->Overlapped), sizeof(WSAOVERLAPPED));
//		PerIoData->DataBuf.len = 1024;
//		PerIoData->DataBuf.buf = PerIoData->buffer;
//		PerIoData->rmMode = READ;	// read
//		WSARecv(PerSocketData->socket.socket, &(PerIoData->DataBuf), 1, &RecvBytes, &Flags, &(PerIoData->Overlapped), NULL);
//	}
//	serverSock.Close();
//	acceptSock.Close();
//	WSACleanup();
//	return 0;
//}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DWORD WINAPI ServerWorkThread(LPVOID IpParam)
//{
//	HANDLE CompletionPort = (HANDLE)IpParam;
//	DWORD BytesTrans;
//	LPOVERLAPPED IpOverlapped;
//	PER_HANDLE_DATA* handleInfo = NULL;
//	PER_IO_DATA* ioInfo = NULL;
//	//����WSARecv()
//	DWORD RecvBytes;
//	DWORD Flags = 0;
//
//	BOOL bRet = false;
//
//	while (true)
//	{
//		if (GetQueuedCompletionStatus(
//			CompletionPort,					//�����IO��Ϣ����ɶ˿ھ��
//			&BytesTrans,					//���ڱ���I/O�����д�ˮ���ݴ�С�ı�����ַ
//			(PULONG_PTR)&(handleInfo),		//����CreateIoCompletionPort()�����������ı�����ֵַ
//			(LPOVERLAPPED*)&ioInfo,			//�������WSASend(),WSARecv()ʱ���ݵ�OVERLAPPED�ṹ���ַ�ı�����ֵַ
//			INFINITE)						//��ʱ��Ϣ
//			== false)
//		{
//			cerr << "GetQueuedCompletionStatus Error: " << GetLastError() << endl;
//			return -1;
//		}
//		if (ioInfo->rmMode == WRITE)
//			continue;
//		if (0 == BytesTrans) {
//			closesocket(handleInfo->socket.socket);
//			//GlobalFree(handleInfo);//������
//			//GlobalFree(ioInfo);
//			//delete handleInfo;
//			//delete ioInfo;
//			continue;
//		}// ������׽������Ƿ��д�����
//
//
//		if (ioInfo->rmMode == READ)
//		{
//			Queue.push(SEND_INFO(*handleInfo, *ioInfo));
//
//		}//if READ
//
//		 // Ϊ��һ���ص����ý�����I/O��������
//		ZeroMemory(&(ioInfo->Overlapped), sizeof(OVERLAPPED)); // ����ڴ�
//		ioInfo->DataBuf.len = 1024;
//		ioInfo->DataBuf.buf = ioInfo->buffer;
//		ioInfo->rmMode = READ;
//		WSARecv(handleInfo->socket.socket,		//client socket
//			&(ioInfo->DataBuf),				//*WSABUF
//			1,								//number of BUFFER	
//			&RecvBytes,						//���ܵ��ֽ���
//			&Flags,							//��־λ
//			&(ioInfo->Overlapped),			//overlaopped�ṹ��ַ
//			NULL);							//ûɶ��
//	}//while
//
//}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DWORD WINAPI ServerSendThread(LPVOID IpParam)
//{
//	XHttpResponse res;     //���ڴ���http����
//	while (1)
//	{
//		WaitForSingleObject(hMutex, INFINITE);
//		//EnterCriticalSection(&g_cs);
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
//			//LeaveCriticalSection(&g_cs);
//			continue;
//		}
//
//	}
//	return 0;
//};