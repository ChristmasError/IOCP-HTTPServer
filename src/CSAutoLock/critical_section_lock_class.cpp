#pragma once

#include "cs_auto_lock_class.h"

CSLock::CSLock()
{
	::InitializeCriticalSection(&m_cs);
}
CSLock::~CSLock()
{
	::LeaveCriticalSection(&m_cs);
	::DeleteCriticalSection(&m_cs);
}

void CSLock::Lock()
{
	::EnterCriticalSection(&m_cs);
	// �������������������߳��� ����
}

void CSLock::UnLock()
{
	::LeaveCriticalSection(&m_cs);
}