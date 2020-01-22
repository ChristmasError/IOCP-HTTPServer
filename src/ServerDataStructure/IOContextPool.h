#pragma once

#include "serverdatastructure_global.h"
#include "PerIOContext.h"
#include "../CSAutoLock/CSAutoLock.h"

//========================================================
//                 
//		       IOContextPool��IOContext��
//
//========================================================

class SERVERDATASTRUCTURE_API IOContextPool
{
private:
	std::list<LPPER_IO_CONTEXT> contextList;
	unsigned int   m_nFreeIoContext;
	unsigned int   m_nActiveIoContext;
	CSLock     m_csLock;
public:
	IOContextPool();
	~IOContextPool();

	// ����һ��IOContxt
	LPPER_IO_CONTEXT AllocateIoContext();

	// ����һ��IOContxt
	void ReleaseIOContext(LPPER_IO_CONTEXT pContext);

	void ShowIOContextPoolInfo();
};


