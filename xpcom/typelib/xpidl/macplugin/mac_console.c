



































#include "mac_console.h"

#ifndef __CONSOLE__
#include <console.h>
#endif

extern CWPluginContext gPluginContext;

UInt32 mac_console_count = 0;
CWMemHandle mac_console_handle = NULL;

















short InstallConsole(short fd)
{
#pragma unused (fd)
	mac_console_count = 0;
	CWAllocMemHandle(gPluginContext, 8192, false, &mac_console_handle);
	return 0;
}










void RemoveConsole(void)
{
	if (mac_console_handle != NULL) {
		CWFreeMemHandle(gPluginContext, mac_console_handle);
		mac_console_handle = NULL;
	}
}













long WriteCharsToConsole(char *buffer, long n)
{
	long size = 0;
	void* ptr = NULL;

	if (CWGetMemHandleSize(gPluginContext, mac_console_handle, &size) == noErr) {
		if (mac_console_count + n >= size) {
			size += 8192;
			if (CWResizeMemHandle(gPluginContext, mac_console_handle, size) != noErr)
				return -1;
		}
	}

	if (CWLockMemHandle(gPluginContext, mac_console_handle, false, &ptr) == noErr) {
		BlockMoveData(buffer, (char *)ptr + mac_console_count, n);
		mac_console_count += n;
		CWUnlockMemHandle(gPluginContext, mac_console_handle);
	}
	
	return 0;
}














long ReadCharsFromConsole(char *buffer, long n)
{
	return 0;
}
