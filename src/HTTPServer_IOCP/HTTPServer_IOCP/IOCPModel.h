#pragma once
#include<WinSocket.h>		//��װ�õ�socket���,��Ŀ���������dllλ������Ŀ¼��lib�ļ�����
#include<WinSock2.h>
#include<thread>
#include<iostream>
#include<vector>
#include<stdio.h>
#include"XHttpResponse.h"

#pragma comment(lib, "Kernel32.lib")	// IOCP��Ҫ�õ��Ķ�̬���ӿ�

// ����������״̬
#define RUNNING true
#define STOP false

// DATABUFĬ�ϴ�С
#define BUF_SIZE 1024
// ���ݸ�Worker�̵߳��˳��ź�
#define WORK_THREADS_EXIT NULL
// Ĭ�϶˿�
#define DEFAULT_PORT  8888
// Ĭ��IP
#define DEFAULT_IP "10.11.147.70"
// �˳���־
#define EXIT_CODE (-1)

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

//========================================================
// ��IO���ݽṹ�嶨��(����ÿһ���ص������Ĳ���)
//========================================================
typedef struct _PER_IO_DATA
{
	WSAOVERLAPPED	m_Overlapped;					// OVERLAPPED�ṹ���ýṹ�����һ��event�¼�����,������ڽṹ����λ����Ϊ�׵�ַ
	WSABUF			m_wsaBuf;						// WSABUF�ṹ��������Ա��һ��ָ��ָ��buf����һ��buf�ĳ���len
	char			m_buffer[BUF_SIZE];			// WSABUF�����ַ�������
	OPERATION_TYPE           m_OpType;
	//OPERATION_TYPE  m_OpType;						// ��־λ

	// ��ʼ��
	_PER_IO_DATA()
	{
		ZeroMemory(&(m_Overlapped), sizeof(WSAOVERLAPPED));
		ZeroMemory(m_buffer, BUF_SIZE);
		m_wsaBuf.buf = m_buffer;
		m_wsaBuf.len = BUF_SIZE;
		m_OpType = NULL_POSTED;
	}
	// �ͷ�
	~_PER_IO_DATA()
	{

	}
	// ����
	void Reset()
	{
		ZeroMemory(&m_Overlapped, sizeof(WSAOVERLAPPED));
		if (m_wsaBuf.buf != NULL)
			ZeroMemory(m_wsaBuf.buf, BUF_SIZE);
		else
			m_wsaBuf.buf= (char *)::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUF_SIZE);
		m_OpType = NULL_POSTED;
		ZeroMemory(m_buffer, BUF_SIZE);
	}
} PER_IO_DATA, *LPPER_IO_DATA;

//========================================================
// ��IO���ݽṹ�ⶨ��(ÿһ���ͻ���socket������
//========================================================
typedef struct  _PER_HANDLE_DATA
{
	WinSocket				m_Sock;		//ÿһ��socket
	//vector<PER_IO_DATA*>    m_vecIoContex;  //ÿ��socket���յ�������IO��������

	// ��ʼ��
	_PER_HANDLE_DATA()
	{
	}
	// �ͷ���Դ
	~_PER_HANDLE_DATA()
	{
	}
} PER_HANDLE_DATA,*LPPER_HANDLE_DATA;

//========================================================
//					IOCPModel�ඨ��
//========================================================
class IOCPModel
{
public:
	// ����������Դ��ʼ��
	IOCPModel()
	{
		m_WorkerShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		InitializeServerResource();
	}
	~IOCPModel()
	{
		_Deinitialize();
	}
public:
	// ����������
	bool Start();

	// �رշ�����
	//void Stop();

	// ��ʼ����������Դ
	// 1.��ʼ��Winsock����
	// 2.��ʼ��IOCP + ���������̳߳�
	// 3.�����������׽���
	// 4.accept()
	bool InitializeServerResource();

	// ����Winsock��:public
	bool LoadSocketLib();

	// ж��Socket��
	bool UnloadSocketLib() 
	{ 
		WSACleanup(); 
	}

private:
	// ����Winsock��:private
	bool _LoadSocketLib();

	// ��ʼ��IOCP:��ɶ˿��빤���߳��̳߳ش���
	bool _InitializeIOCP();

	// �����������׽���
	bool _InitializeListenSocket();

	// �̺߳�����ΪIOCP������������߳�
	static DWORD WINAPI _WorkerThread(LPVOID lpParam);

	// Ͷ��WSARecv()����
	bool _PostRecv();

	// Ͷ��WSARecv()����
	//bool _PostRecv();

	// ��ӡ��Ϣ
	void _ShowMessage(const char*, ...) const;

	// �ͷ�������Դ
	void _Deinitialize();

protected:
	bool						  _ServerRunning;				// �����������ж�

	WSADATA						  wsaData;						// Winsock�����ʼ��

	SYSTEM_INFO					  m_SysInfo;					// ����ϵͳ��Ϣ

	HANDLE					      m_WorkerShutdownEvent;	// ֪ͨ�߳�ϵͳ�Ƴ��¼�

	HANDLE						  m_IOCompletionPort;			// ��ɶ˿ھ��

	WinSocket					  m_ServerSocket;				// ��װ��socket���

	HANDLE*						  m_WorkerThreadsHandleArray;	// �������߳̾������

	int							  m_nThreads;				    // �����߳�����

	

};
