// WinSocket.cpp: ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "socket.h"


// ���ǵ���������һ��ʾ��
WINSOCKET_API int nWinSocket=0;

// ���ǵ���������һ��ʾ����
WINSOCKET_API int fnWinSocket(void)
{
    return 42;
}

// �����ѵ�����Ĺ��캯����
// �й��ඨ�����Ϣ������� WinSocket.h
CWinSocket::CWinSocket()
{
    return;
}