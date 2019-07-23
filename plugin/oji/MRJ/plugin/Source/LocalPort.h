











































#pragma once

#include <Quickdraw.h>

class LocalPort {
public:
	LocalPort(GrafPtr port)
	{
		fPort = port;
		fOrigin.h = fOrigin.v = 0;
	}
	
	LocalPort(GrafPtr port, Point origin)
	{
		fPort = port;
		fOrigin.h = origin.h;
		fOrigin.v = origin.v;
	}

	void Enter();
	void Exit();

private:
	GrafPtr fPort;
	Point fOrigin;
	GrafPtr fOldPort;
	Point fOldOrigin;
};
