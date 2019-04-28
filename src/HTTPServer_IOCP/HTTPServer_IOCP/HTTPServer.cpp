#include<IOCPModel.h>
using namespace std;
class HTTPServer :public IOCPModel
{
	// ����RecvCompleted()
	void RecvCompleted(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT IoInfo)
	{
		HttpResponse res;     //���ڴ���http����

		bool error = false;
		bool sendfinish = false;
		//���´���GET����
		int buflend = strlen(IoInfo->m_buffer);
		if (buflend <= 0) {
			return;
		}
		if (!res.SetRequest(IoInfo->m_buffer)) {
			cerr << "SetRequest failed!" << endl;
			error = true;
			return;
		}
		
		string head = res.GetHead();
		
		// Ͷ��SEND������ͷ
		strcpy_s(IoInfo->m_buffer, head.c_str());
		IoInfo->m_wsaBuf.len = head.size();
		if (PostSend(SocketInfo,IoInfo)==false)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				cout << "send false!\n";
				//DoClose(sockContext);
				return ;
			}
		}
		int i = 0;
		for (;;i++)
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
			// Ͷ��SEND����������
			if (PostSend(SocketInfo, IoInfo) == false)
			{
				if (WSAGetLastError() != WSA_IO_PENDING)
				{
					cout << "send false!\n";
					//DoClose(sockContext);
					return;
				}
			}
		}
		if (sendfinish)
		{
			//cout << "!!!!!!!!!!!!!!!!!ѭ��"<<i<<"��,send finish!!!!!!!!!!!!!!!!!\n";
			return;
		}
		if (error)
		{
			cerr << "send file happen wrong ! client close !" << endl;
			SocketInfo->m_Sock.Close();
			delete SocketInfo;
			delete IoInfo;
			return;
		}
	}
	// ����SendCompleted()
	void SendCompleted(LPPER_SOCKET_CONTEXT SocketInfo, LPPER_IO_CONTEXT IoInfo)
	{
		std::cout << "�������ݳɹ���\n";
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