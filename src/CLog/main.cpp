#include "clog_class.h"

int main()
{
	CLog::LogInstance()->init("mylog.log");
	// Ĭ��log·��Ϊ��Ŀ·��
	// �Զ���log·��������·������ log �ļ���
	// CLog::LogInstance()->init("mylog.log","..\\log\\");
	LOG_INFO("%s\n", "test!!");

	LOG_WARN("%s\n", "test!!");

	LOG_ERROR("%s\n", "test!!");

	LOG_DEBUG("%s\n", "test!!");
	
	while (1);
	return 0;
}