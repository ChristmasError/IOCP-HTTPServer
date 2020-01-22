#pragma once
#include <cstdio>
#include <cstdint>
#include <ctime>

//��־�������ڵ�-ѭ������ڵ�
class LogBuffer
{
public:
    enum BufferStatus
    {
        FREE,
        FULL
    };

    LogBuffer(uint32_t len) :
        mStatus(FREE),
        mPrev(NULL),
        mNext(NULL),
        _mTotalLen(len),
        _mUsedLen(0)
    {
        _mTotalLen = len;
        mCurLogName = new char[MAX_PATH];
        memset(mCurLogName, 0, MAX_PATH);
        _mCacheData = new char[len];
        memset(_mCacheData, 0, len);
        if (!_mCacheData)
        {
            std::cerr << "Error : no space to allocate _mCacheData\n";
            exit(1);
        }
    }

    void Clear()
    {
        memset(_mCacheData, 0, _mTotalLen);
        memset(mCurLogName, 0, MAX_PATH);
        _mUsedLen = 0;
        mStatus = FREE;
    }

    uint32_t AvailableLen() const
    {
        return this->_mTotalLen - this->_mUsedLen;
    }

    bool Empty() const
    {
        return _mUsedLen == 0 && mCurLogName[0] == '\0';
    }

    void AppendLog(const char* log_line, uint32_t len)
    {
        if (AvailableLen() < len)
            return;
        memcpy(_mCacheData + _mUsedLen, log_line, len);
        _mUsedLen += len;
    }

    void WriteFile(FILE* fp)
    {
        uint32_t writeLen = fwrite(_mCacheData, 1, _mUsedLen, fp);
        if (writeLen != _mUsedLen) //д�볤���뻺�����ı����Ȳ�һ�����ж�д���ı�ʧ��
        {
            std::cerr << "Error : write log to disk error,writeLen: "<< writeLen<<std::endl;
        }
    }

public:
    BufferStatus    mStatus;   //������״̬
    LogBuffer* mPrev;   //��������ǰһ��������ָ��
    LogBuffer* mNext;   //����������һ��������ָ��
    char* mCurLogName;    //���������Ӧ��־�ļ�����

private:
    uint32_t _mTotalLen;  //������ռ��С
    uint32_t _mUsedLen;   //�������ÿռ�
    char* _mCacheData; //�������� 

};
