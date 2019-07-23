




































#pragma once





typedef struct RegisteredWindow {
        RegisteredWindow* mNext;
        nsIEventHandler* mHandler;
        nsPluginPlatformWindowRef mWindow;

        RegisteredWindow(RegisteredWindow* next, nsIEventHandler* handler, nsPluginPlatformWindowRef window)
            : mNext(next), mHandler(handler), mWindow(window)
        {
            NS_ADDREF(mHandler);
        }

        ~RegisteredWindow()
        {
            NS_RELEASE(mHandler);
        }
    } RegisteredWindow;

NS_METHOD
AltRegisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window);

NS_METHOD
AltUnregisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window);
