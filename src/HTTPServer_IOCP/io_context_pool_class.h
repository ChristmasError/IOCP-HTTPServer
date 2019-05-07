#pragma once

#include<per_io_context_struct.cpp>

//========================================================
//                 
//		       IOContextPool������IOContext��
//
//========================================================

class IOContextPool
{
private:
	std::list<LPPER_IO_CONTEXT> contextList;
	CRITICAL_SECTION csLock;

public:
	IOContextPool();
	~IOContextPool();

	// ����һ��IOContxt
	LPPER_IO_CONTEXT AllocateIoContext();
	
	// ����һ��IOContxt
	void ReleaseIOContext(LPPER_IO_CONTEXT pContext);

};


