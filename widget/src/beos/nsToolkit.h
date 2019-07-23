





































#ifndef TOOLKIT_H      
#define TOOLKIT_H

#include "nsIToolkit.h"

#include <OS.h>

struct MethodInfo;





 

class nsToolkit : public nsIToolkit
{
public:
            NS_DECL_ISUPPORTS

                            nsToolkit();
            NS_IMETHOD      Init(PRThread *aThread);
            bool            CallMethod(MethodInfo *info);
			bool			CallMethodAsync(MethodInfo *info);
            
            PRBool          IsGuiThread(void)      { return (PRBool)(mGuiThread == PR_GetCurrentThread());}
            PRThread*       GetGuiThread(void)       { return mGuiThread;   }
			void			Kill();
private:
            virtual         ~nsToolkit();
            void            CreateUIThread(void);

protected:
    
    PRThread    *mGuiThread;
	static void	RunPump(void* arg);
	void		GetInterface();
	bool		cached;
	bool		localthread;
	port_id		eventport;
};

#endif  
