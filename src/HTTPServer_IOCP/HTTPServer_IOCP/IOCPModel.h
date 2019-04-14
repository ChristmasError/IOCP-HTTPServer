#pragma once
#include<WinSocket.h>		//��װ�õ�socket���,��Ŀ���������dllλ������Ŀ¼��lib�ļ�����
#include<WinSock2.h>
#include<thread>
#include<iostream>
#include<vector>
#include<stdio.h>
#include"XHttpResponse.h"

#pragma comment(lib, "Kernel32.lib")	// IOCP��Ҫ�õ��Ķ�̬���ӿ�

// ��־Ͷ�ݵ��ǽ��ղ���
#define RECV_POSTED 3
// ��־Ͷ�ݵ��Ƿ��Ͳ���
#define SEND_POSTED 5
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
// ��IO���ݽṹ�嶨��(����ÿһ���ص������Ĳ���)
//========================================================
typedef struct
{
	WSAOVERLAPPED Overlapped;			//OVERLAPPED�ṹ���ýṹ�����һ��event�¼�����,������ڽṹ����λ����Ϊ�׵�ַ
	WSABUF DataBuf;						//WSABUF�ṹ��������Ա��һ��ָ��ָ��buf����һ��buf�ĳ���len
	char buffer[DATABUF_SIZE];			//��Ϣ����
	DWORD rmMode;						//��־λREAD or WRITE
} PER_IO_DATA, *LPPER_IO_DATA;


//========================================================
// ��IO���ݽṹ�嶨��(����ÿһ���ͻ��˵��ص������Ĳ���)
//========================================================
typedef struct 
{
	WinSocket socket;					//�ͻ���socket��Ϣ		 
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;


//========================================================
//					IOCPModel�ඨ��
//========================================================
class IOCPModel
{
public:
	IOCPModel(void);
	~IOCPModel(void);
public:
	// ��ʼ��������
	bool InitializeServer();

	// ����������
	bool Start();

	// �ر�IOCPserver
	bool Stop();

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
	bool _PostRecv();

	// �̺߳�����ΪIOCP������������߳�
	static DWORD WINAPI _WorkerThread(LPVOID lpParam);

	// ��ӡ��Ϣ
	void _ShowMessage(const char*, ...) const;
	

private:
	SYSTEM_INFO					  m_SysInfo;					// ����ϵͳ��Ϣ

	HANDLE					      m_WorkThreadShutdownEvent;	// ֪ͨ�߳�ϵͳ�Ƴ��¼�

	HANDLE						  m_IOCompletionPort;			// ��ɶ˿ھ��

	HANDLE*						  m_WorkerThreadsHandleArray;	// �������߳̾������

	unsigned int				  m_nThreads;				    // �����߳�����

	WinSocket					  m_ServerSocket;				// ��װ��socket���

	vector<LPPER_HANDLE_DATA>     m_ClientContext;				// �ͻ���socket��context��Ϣ

	LPPER_HANDLE_DATA		      m_pListenContext;				// ���ڼ����ͻ���Socket��context��Ϣ

};

