#pragma once

#include "per_io_context_struct.h"
#include "cs_auto_lock_class.h"

//========================================================
//                 
//		       IOContextPool��IOContext��
//
//========================================================

class IOContextPool
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


