#pragma once

#include "operation_type_enum.h"
#include <WinSock2.h>
#include <list>

// DATABUFĬ�ϴ�С
#define BUF_SIZE 102400

//====================================================================================
//
//				��IO���ݽṹ�嶨��(����ÿһ���ص������Ĳ���)
//
//====================================================================================
typedef struct _PER_IO_CONTEXT
{
public:
	WSAOVERLAPPED	m_Overlapped;					// OVERLAPPED�ṹ���ýṹ�����һ��event�¼�����,������ڽṹ����λ����Ϊ�׵�ַ
	SOCKET			m_AcceptSocket;					// ��IO������Ӧ��socket
	WSABUF			m_wsaBuf;						// WSABUF�ṹ��������Ա��һ��ָ��ָ��buf����һ��buf�ĳ���len
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
		printf("�ͷ�\n");
	}
	// ����
	void Reset()
	{
		ZeroMemory(m_buffer, BUF_SIZE);
		//ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));
		//
		//if (m_wsaBuf.buf != NULL)
		//	ZeroMemory(m_wsaBuf.buf, BUF_SIZE);
		//else
		//	m_wsaBuf.buf = (char *)::HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUF_SIZE);

		//m_wsaBuf.buf = m_buffer;
		//m_OpType = NULL_POSTED;
	}

} PER_IO_CONTEXT, *LPPER_IO_CONTEXT;

