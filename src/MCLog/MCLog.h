#pragma once
//#pragma warning(disable:4996) //close SDL check

#include "mclog_global.h"

//��־Ĭ��·������Ŀ¼\Log\��������\��־�����磺".\\Log\\2019-12-12(GetSystemDate()��ȡ)\\LogName.txt",��־����".txt"��ȱʡ
//�Զ�����־·����ʽ: ".\\·��\\" , ".\\·��" , "./·��" , "./·��/"
#define LOG_DEFAULT_PATH ".\\Log\\"
//��־�ļ�����С����    1Gb
#define MEM_USE_LIMIT (1u * 1024 * 1024 * 1024)
//������־��������        4Kb
#define PER_LOG_LEN_LIMIT (4 * 1024)
//�������̵߳ȴ��ź�ʱ��   millisecond
#define BUFF_WAIT_TIME (500)
//�����������ȶ�Ӧ��
#define PER_BUFFER_SIZE (_mPerBufSize)
//����������־��дʱ��   seconds
#define RELOG_TIME_THRESOLD (5) 

class MCLOG_API MCLog
{
public:
    static MCLog* LogInstance()
    {
        Init();
        return _mInstance;
    }
private:
    static void Init()
    {
        if (!_mInstance)
        {
            _mInstance = new MCLog();
            memcpy(_mInstance->_mLogPath, LOG_DEFAULT_PATH, strlen(LOG_DEFAULT_PATH) + 1);
            _mInstance->_hWriteFileSemaphore = CreateSemaphore(NULL, 0, 1, NULL);		//��ʼsignal״̬:  unsignnal;
            ::InitializeCriticalSection(&(_mInstance->_hCS_CurBufferLock));
            HANDLE LogConsumerThread = CreateThread(NULL, NULL, CachePersistThreadFunc, NULL, 0, NULL);
            if (LogConsumerThread != 0)
                CloseHandle(LogConsumerThread);
        }
    }

public:
    //������־·��,�����øú�����ʹ��Ĭ��·��
    void SetLogPath(const char* log_path = LOG_DEFAULT_PATH);
    //����־д�뻺��
    void LogWriteBuffer(const char* log_name, const char* log_str);

private:
    MCLog();
    ~MCLog();

private:
    static MCLog*    _mInstance;           //����
    static uint32_t  _mPerBufSize;         //�����������ȴ�С 1024 * 1024 == 1Mb

    HANDLE           _hWriteFileSemaphore; //ĳһ�����������ź��������������߳̽����ļ�д��
    CRITICAL_SECTION _hCS_CurBufferLock;   //�ٽ��� ͬ��_mCurBuffer,_mSecBuffer
    SYSTEMTIME       _mSys;
    uint32_t         _mPid;
    FILE* _mFp;
    char* _mLogPath;            //��־�ļ�·��
    char* _mSysDate;            //ϵͳ����,��:2019-12-12
    char* _mLogFileLocation;    //�����������߳�����д�����־����·��,�磺".\\Log\\2019-12-12(����FlushLogPath())\\LogName.txt"
    int              _mBufCnt;             //������������
    LogBuffer*       _mCurBuffer;          //��������ָ��,ָ��ֻ�� 1.�������� 2.���濪ʼ���й̻����� 
                                           //������������Ż����˳���ƶ�,��_mCurBuffer=_mCurBuffer->mNext
    LogBuffer*       _mSecBuffer;          //�λ�����ָ��,ָ���������,��_mCurBuffer����־������Ŀ��д����־�ļ�����һ�²Ż���б��
    LogBuffer*       _mPrstBuffer;         //��ǰ���ڽ���������ת¼���ļ��Ļ�����ָ��
    uint64_t         _mLastErrorTime;      //��־���������ʱ��,��־��������ֵΪ0

    bool CreateFilePath(const char* log_path);
    bool OpenFile(const char* log_name);

private:  //�����߳����
    //�����̺߳������ӻ���������д���ı��ļ�
    static DWORD WINAPI CachePersistThreadFunc(LPVOID lpParam);
    //����д���ı��ļ�
    void PersistBuffer();
    //��ȡϵͳ����,��:2019-12-12
    void GetSystemDate(char* log_date);
};

#define LOG_INIT()\
do{\
    MCLog::LogInstance();\
}while (0)

#define SET_LOGPATH(log_path)\
do{\
    MCLog::LogInstance()->SetLogPath(log_path); \
}while (0)

#define WRITE_LOG(log_name,log_str)\
do{\
    MCLog::LogInstance()->LogWriteBuffer(log_name, log_str);\
}while (0)




