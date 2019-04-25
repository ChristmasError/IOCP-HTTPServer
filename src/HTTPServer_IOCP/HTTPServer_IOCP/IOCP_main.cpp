#include<IOCPModel.h>
using namespace std;
class HTTPServer :public IOCPModel
{
	void RecvCompleted(PER_HANDLE_DATA *handleInfo, PER_IO_DATA *ioInfo) 
	{
		XHttpResponse res;     //���ڴ���http����

		switch (ioInfo->m_OpType)
		{
		case SEND_POSTED:
			break;
			//case WRITE
		case RECV_POSTED:
			bool error = false;
			bool sendfinish = false;
			for (;;)
			{
				//���´���GET����
				int buflend = strlen(ioInfo->m_buffer);

				if (buflend <= 0) {
					break;
				}
				if (!res.SetRequest(ioInfo->m_buffer)) {
					cerr << "SetRequest failed!" << endl;
					error = true;
					break;
				}
				string head = res.GetHead();
				if (handleInfo->m_Sock.Send(head.c_str(), head.size()) <= 0)
				{
					break;
				}//��Ӧ��ͷ
				for (;;)
				{
					char buf[10240];
					//���ͻ���������ļ�����buf�в������ļ�����_len
					int file_len = res.Read(buf, 10240);
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
					if (handleInfo->m_Sock.Send(buf, file_len) <= 0)
					{
						break;
					}
				}//for(;;)
				if (sendfinish)
				{
					break;
				}
				if (error)
				{
					cerr << "send file happen wrong ! client close !" << endl;
					handleInfo->m_Sock.Close();
					delete handleInfo;
					delete ioInfo;
					break;
				}
			}
			break;
			//case READ
		}//switch
	}
};
int main()
{
	HTTPServer IOCPserver;
	IOCPserver.StartServer(EX);
	IOCPserver.StopServer();
	return 0;
}