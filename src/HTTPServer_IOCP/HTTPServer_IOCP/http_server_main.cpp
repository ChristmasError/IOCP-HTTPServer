#include "iocp_model_class.h"
#include "http_response_class.h"
#include "clog_class.h"
using namespace std;
class HTTPServer :public IOCPModel
{
public:
	HTTPServer()
	{
		CREATE_LOG("HTTPServerLog.log", ".\\log\\");
	}
private:
	// ������
	void ConnectionEstablished(LPPER_SOCKET_CONTEXT socketInfo)
	{
		LOG_INFO("[Connect] Accept a connection from:%s ,Current connects:%d\n", socketInfo->m_Sock.ip, GetConnectCnt());
		return;
	}
	// ���ӹر�
	void ConnectionClosed(LPPER_SOCKET_CONTEXT socketInfo)
	{
		LOG_INFO("[Disconnect] A connection closed ip:%s ,Current connects:%d\n", socketInfo->m_Sock.ip, GetConnectCnt());
		return;
	}
	// ���ӷ�������
	void ConnectionError(LPPER_SOCKET_CONTEXT socketInfo, DWORD errorNum)
	{
		LOG_INFO("[Error] A connection error:%d from ip:%s, Current connects:%d\n", errorNum , socketInfo->m_Sock.ip ,GetConnectCnt());
	}
	// Recv�������
	void RecvCompleted(LPPER_SOCKET_CONTEXT socketInfo, LPPER_IO_CONTEXT IoInfo)
	{
		HttpResponse res;     //���ڴ���http����

		bool error = false;
		bool sendfinish = false;
		//���´���GET����
		int buflend = strlen(IoInfo->m_buffer);
		if (buflend <= 0) {
			return;
		}
		string responseType;
		if (!res.SetRequest(IoInfo->m_buffer,responseType)) {
			LOG_WARN("Set request failed! client ip:%s\n",socketInfo->m_Sock.ip) ;
			error = true;
			return;
		}

		string head = res.GetHead();

		// Ͷ��SEND��������Ϣͷ
		strcpy_s(IoInfo->m_buffer, head.c_str());
		IoInfo->m_wsaBuf.len = head.size();
		if (PostSend(socketInfo, IoInfo) == false)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				LOG_WARN("Send http headers failed! client ip:%s\n",socketInfo->m_Sock.ip) ;
				//DoClose(sockContext);
				return;
			}
		}
		// Ͷ��SEND��������������
		int i = 0;
		for (;; i++)
		{
			char buf[10240];
			//���ͻ���������ļ�����buf�в������ļ�����_len
			int file_len = res.Read(IoInfo->m_buffer, 102400);
			IoInfo->m_wsaBuf.len = file_len;
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
			if (PostSend(socketInfo, IoInfo) == false)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					LOG_WARN("Send data failed! client ip:%s\n", socketInfo->m_Sock.ip);
					//DoClose(sockContext);
					return;
				}
			}
		}
		if (sendfinish)
		{
			LOG_INFO("[Recv] Complete response to '%s' request from ip:%s\n", responseType.c_str(), socketInfo->m_Sock.ip);
			return;
		}
		if (error)
		{
			LOG_WARN("Send file happen wrong! client close! client ip:%s", socketInfo->m_Sock.ip);
			socketInfo->m_Sock.Close();
			delete socketInfo;
			delete IoInfo;
			return;
		}
	}
	// Send�������
	void SendCompleted(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT IoInfo)
	{
		if(IoInfo->m_wsaBuf.len != 0)
			LOG_INFO("[Send] Send data successd, filesize:%d, client ip:%s\n", IoInfo->m_wsaBuf.len,SocketInfo->m_Sock.ip);
		return;
	}
};
int main()
{
	HTTPServer IOCPserver;

	IOCPserver.StartServer();
	//IOCPserver.StopServer();

	return 0;
}