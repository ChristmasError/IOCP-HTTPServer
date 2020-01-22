#pragma once

#include "serverdatastructure_global.h"

// DATABUFĬ�ϴ�С
#define BUF_SIZE (163840)


//====================================================================================
//
//			����ɶ˿���Ͷ�ݵ�I/O����������
//
//====================================================================================
typedef enum SERVERDATASTRUCTURE_API _OPERATION_TYPE
{
	ACCEPT_POSTED,		// ��־Ͷ�ݵ��ǽ��ղ���
	RECV_POSTED,		// ��־Ͷ�ݵ��ǽ��ղ���
	SEND_POSTED,		// ��־Ͷ�ݵ��Ƿ��Ͳ���
	NULL_POSTED			// ��ʼ����
} OPERATION_TYPE;

//====================================================================================
//
//				��IO���ݽṹ�嶨��(����ÿһ���ص������Ĳ���)
//
//====================================================================================
typedef class SERVERDATASTRUCTURE_API _PER_IO_CONTEXT
{
public:
	WSAOVERLAPPED	m_Overlapped;					// OVERLAPPED�ṹ���ýṹ�����һ��event�¼�����,������ڽṹ����λ����Ϊ�׵�ַ
	SOCKET			m_AcceptSocket;					// ��IO������Ӧ��socket
	WSABUF			m_wsaBuf;						// WSABUF�ṹ��������Ա:һ��ָ��ָ��buf����һ��buf�ĳ���len
	char			m_buffer[BUF_SIZE];			    // WSABUF�����ַ�������
	OPERATION_TYPE  m_OpType;						// ��־λ

	_PER_IO_CONTEXT()
	{
		ZeroMemory(&(m_Overlapped), sizeof(m_Overlapped));
		ZeroMemory(m_buffer, BUF_SIZE);
		m_AcceptSocket = INVALID_SOCKET;
		m_wsaBuf.buf   = m_buffer;
		m_wsaBuf.len   = BUF_SIZE;
		m_OpType       = NULL_POSTED;
	}
	// �ͷ�
	~_PER_IO_CONTEXT()
	{
		if (m_AcceptSocket != INVALID_SOCKET)
		{
			closesocket(m_AcceptSocket);
			m_AcceptSocket = INVALID_SOCKET;
		}
	}
	// ����
	void Reset()
	{
		ZeroMemory(m_buffer, BUF_SIZE);
		ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
		m_OpType = NULL_POSTED;
	}

} PER_IO_CONTEXT, *LPPER_IO_CONTEXT;

