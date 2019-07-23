











































#pragma once

class Monitor {
public:
	virtual ~Monitor() {}
	
	virtual void enter() = 0;
	virtual void exit() = 0;
	
	virtual void wait() = 0;
	virtual void wait(long long millis) = 0;
	virtual void notify() = 0;
	virtual void notifyAll() = 0;
};
