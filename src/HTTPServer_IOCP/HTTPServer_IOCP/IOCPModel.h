#pragma once
#include<WinSocket.h>		//��װ�õ�socket���,��Ŀ���������dllλ������Ŀ¼��lib�ļ�����
#include<WinSock2.h>
#include<thread>
#include<iostream>
#include<vector>
#include<stdio.h>
#include"XHttpResponse.h"

#pragma comment(lib, "Kernel32.lib")	// IOCP��Ҫ�õ��Ķ�̬���ӿ�

// DATABUFĬ�ϴ�С
#define DATABUF_SIZE 1024
// ���ݸ�Worker�̵߳��˳��ź�
#define WORK_THREADS_EXIT_CODE NULL
// Ĭ�϶˿�
#define DEFAULT_PORT  8888
// Ĭ��IP
#define DEFAULT_IP "10.11.147.70"

// �ͷž��
inline void RELEASE_HANDLE(HANDLE handle)
{
	if ( handle != NULL && handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(handle);
		handle = NULL;
	}
}
// �ͷ�ָ��
inline void RELEASE_POINT(void* point)
{
	if (point != NULL)
	{
		delete point;
		point = NULL;
	}
}

//========================================================
// ����ɶ˿���Ͷ�ݵ�I/O����������
//========================================================
typedef enum _OPERATION_TYPE
{
	RECV_POSTED,		// ��־Ͷ�ݵ��ǽ��ղ���
	SEND_POSTED,		// ��־Ͷ�ݵ��Ƿ��Ͳ���
	NULL_POSTED			// ��ʼ����
} OPERATION_TYPE;

#define READ 3
#define WRITE 5
//========================================================
// ��IO���ݽṹ�嶨��(����ÿһ���ص������Ĳ���)
//========================================================
typedef struct _PER_IO_DATA
{
	WSAOVERLAPPED	m_Overlapped;					// OVERLAPPED�ṹ���ýṹ�����һ��event�¼�����,������ڽṹ����λ����Ϊ�׵�ַ
	//SOCKET			socket;							// ���IO����ʹ�õ�socket
	WSABUF			m_wsaBuf;						// WSABUF�ṹ��������Ա��һ��ָ��ָ��buf����һ��buf�ĳ���len
	char			m_buffer[DATABUF_SIZE];			// WSABUF�����ַ�������
	DWORD rmMode;
	//OPERATION_TYPE  m_OpType;						// ��־λ

	//// ��ʼ��
	//_PER_IO_DATA()
	//{
	//	ZeroMemory(&(m_Overlapped), sizeof(WSAOVERLAPPED));
	//	//socket = INVALID_SOCKET;
	//	ZeroMemory(m_buffer, DATABUF_SIZE);
	//	m_wsaBuf.buf = m_buffer;
	//	m_wsaBuf.len = DATABUF_SIZE;
	//	m_OpType = NULL_POSTED;
	//}
	//// �ͷ�
	//~_PER_IO_DATA()
	//{
	//	if (socket != INVALID_SOCKET)
	//	{
	//		closesocket(socket);
	//		socket = INVALID_SOCKET;
	//	}
	//}

	// ����buf������
	void ResetBuffer()
	{
		ZeroMemory(m_buffer, DATABUF_SIZE);
	}
} PER_IO_DATA, *LPPER_IO_DATA;

//========================================================
// ��IO���ݽṹ�ⶨ��(ÿһ���ͻ���socket������
//========================================================
typedef struct  _PER_HANDLE_DATA
{
	WinSocket				m_Sock;		//ÿһ��socket
	//vector<PER_IO_DATA*>    m_vecIoContex;  //ÿ��socket���յ�������IO��������

	//// ��ʼ��
	//_PER_HANDLE_DATA()
	//{
	//}
	//// �ͷ���Դ
	//~_PER_HANDLE_DATA()
	//{
	//	if (m_Sock.socket != INVALID_SOCKET)
	//	{
	//		m_Sock.Close();
	//		m_Sock.socket = INVALID_SOCKET;
	//	}
	//	for (int i = 0; i < m_vecIoContex.size(); i++)
	//	{
	//		delete m_vecIoContex[i];
	//	}
	//}
} PER_HANDLE_DATA,*LPPER_HANDLE_DATA;

//========================================================
//					IOCPModel�ඨ��
//========================================================
class IOCPModel
{
public:
	//IOCPModel()
	//{
	//	InitializeServer();
	//}
	//~IOCPModel()
	//{
	//	//this->Stop();
	//}
public:
	// ����������
	bool Start();

	// �رշ�����
	//bool Stop();

	// ��ʼ��������
	bool InitializeServer();

	// ����Socket��
	bool LoadSocketLib();

	// ж��Socket��
	bool UnloadSocketLib() 
	{ 
		WSACleanup(); 
	}

private:
	// ��ʼ��IOCP:��ɶ˿��빤���߳��̳߳ش���
	bool _InitializeIOCP();

	// ��ʼ��������Socket
	bool _InitializeListenSocket();

	// �ͷ�������Դ
	void _Deinitialize();

	// Ͷ��WSARecv()����
	//bool _PostRecv();

	// �̺߳�����ΪIOCP������������߳�
	static DWORD WINAPI _WorkerThread(LPVOID lpParam);

	// ��ӡ��Ϣ
	void _ShowMessage(const char*, ...) const;
	

private:
	SYSTEM_INFO					  m_SysInfo;					// ����ϵͳ��Ϣ

	HANDLE					      m_WorkThreadShutdownEvent;	// ֪ͨ�߳�ϵͳ�Ƴ��¼�

	HANDLE						  m_IOCompletionPort;			// ��ɶ˿ھ��

	HANDLE*						  m_WorkerThreadsHandleArray;	// �������߳̾������

	int							  m_nThreads;				    // �����߳�����

	WinSocket					  m_ServerSocket;				// ��װ��socket���

	vector<LPPER_HANDLE_DATA>     m_ClientContext;				// �ͻ���socket��context��Ϣ

	LPPER_HANDLE_DATA		      m_pListenContext;				// ���ڼ����ͻ���Socket��context��Ϣ

};
