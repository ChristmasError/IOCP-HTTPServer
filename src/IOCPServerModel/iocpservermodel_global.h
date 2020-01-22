#ifndef IOCPSERVERMODEL_H
#define IOCPSERVERMODEL_H

#ifdef IOCPSERVERMODEL_EXPORTS
#define IOCPSERVERMODEL_API __declspec(dllexport)
#else
#define IOCPSERVERMODEL_API __declspec(dllimport)
#endif

#ifdef _DEBUG
#pragma comment(lib,"SERVERDATASTRUCTUREd.lib")
#else
#pragma comment(lib,"SERVERDATASTRUCTURE.lib")
#endif // DEBUG

#define WIN32_LEAN_AND_MEAN             // �� Windows ͷ�ļ����ų�����ʹ�õ�����

#include <Winsock2.h>
#include <mstcpip.h>
#include <MSWSock.h>

#endif //IOCPSERVERMODEL_H