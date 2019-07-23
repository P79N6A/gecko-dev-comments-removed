










































#if !defined(AFX_NSIOBSERSERV_H__96D90A39_FFC1_11D5_9B13_00C04FA02BE6__INCLUDED_)
#define AFX_NSIOBSERSERV_H__96D90A39_FFC1_11D5_9B13_00C04FA02BE6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 

class CnsIObserServ : public nsIObserver, public nsSupportsWeakReference
 
{
public:
	CnsIObserServ();

	virtual ~CnsIObserServ();

	NS_DECL_ISUPPORTS
	NS_DECL_NSIOBSERVER

	

	void RunAllTests();
	void AddObserversTest(int);
	void RemoveObserversTest(int);
	void NotifyObserversTest(int);
	void EnumerateObserversTest(int);
	void OnStartTests(UINT nMenuID);
};

typedef struct
{
   char			theTopic[1024];
   bool			theOwnsWeak;
} ObserverElement;

#endif 
