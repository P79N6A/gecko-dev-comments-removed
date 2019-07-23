




































#ifndef SWITCHTOUITHREAD_H
#define SWITCHTOUITHREAD_H

#include "nsISupports.h"


struct MethodInfo;


#define WM_CALLMETHOD   'CAme'





class nsSwitchToUIThread {

public:
    virtual bool CallMethod(MethodInfo *info) = 0;

	
	
	
	enum
	{
	    CREATE       = 0x0101,
	    CREATE_NATIVE,
	    DESTROY,
	    SET_FOCUS,
	    GOT_FOCUS,
	    KILL_FOCUS,
	    ONMOUSE,
	    ONDROP,
	    ONWHEEL,
	    ONPAINT,
	    ONRESIZE,
	    CLOSEWINDOW,
	    ONKEY,
	    BTNCLICK,
	    ONACTIVATE,
	    ONMOVE,
	    ONWORKSPACE
#if defined(BeIME)
	    ,
		ONIME
#endif
	};

};





struct MethodInfo {
	nsISupports			*widget;
    nsSwitchToUIThread	*target;
    uint32				methodId;
    int					nArgs;
    uint32				*args;

    MethodInfo(nsISupports *ref, nsSwitchToUIThread *obj, uint32 id, int numArgs = 0, uint32 *arguments = 0)
    {
		widget = ref;
		NS_ADDREF(ref);
		target   = obj;
		methodId = id;
		nArgs    = numArgs;
		args = new uint32 [numArgs];
		memcpy(args, arguments, sizeof(uint32) * numArgs);
    }

	~MethodInfo()
	{
		delete [] args;
		NS_RELEASE(widget);
	}

    bool Invoke() { return target->CallMethod(this); }
};

#endif 

