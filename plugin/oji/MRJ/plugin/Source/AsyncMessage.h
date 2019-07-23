











































#pragma once

#include "MRJSession.h"

class AsyncMessage : public NativeMessage {
public:
	AsyncMessage(MRJSession* session) : mSession(session) {}
	virtual ~AsyncMessage() {}

	void send(Boolean async = false);

protected:
	MRJSession* mSession;
};
