#pragma once

#define _WINSOCKAPI_
#include <windows.h>

////////////////////////////////////////////////////////
// �ٽ�����
class CSLock
{ 
public:

	CSLock();
	~CSLock();

	void Lock();
	void UnLock();

private:
	CRITICAL_SECTION m_cs;
};

////////////////////////////////////////////////////////
// �Զ���

class CSAutoLock
{
public:

	CSAutoLock(CSLock& cslock);
	~CSAutoLock();

	void Lock();
	void UnLock();

private:
	CSLock & m_lock;
};