#pragma once

#include "csautolock_global.h"

////////////////////////////////////////////////////////
// �ٽ�����
class CSAUTOLOCK_API CSLock
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

class CSAUTOLOCK_API CSAutoLock
{
public:

	CSAutoLock(CSLock& cslock);
	~CSAutoLock();

	void Lock();
	void UnLock();

private:
	CSLock & m_lock;
};