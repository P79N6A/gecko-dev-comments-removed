








































#if defined(_RCTIME_H)
#else
#define _RCTIME_H

#include "rcbase.h"

#include <prtime.h>












class PR_IMPLEMENT(RCTime): public RCBase
{
public:
    typedef enum {now} Current;

    RCTime();                       
    RCTime(Current);                
    RCTime(const RCTime&);          
    RCTime(const PRExplodedTime&);  

    virtual ~RCTime();

    
    void operator=(const RCTime&); 
    void operator=(const PRExplodedTime&);

    
    PRBool operator<(const RCTime&);
    PRBool operator>(const RCTime&);
    PRBool operator<=(const RCTime&);
    PRBool operator>=(const RCTime&);
    PRBool operator==(const RCTime&);

    
    RCTime operator+(const RCTime&);
    RCTime operator-(const RCTime&);
    RCTime& operator+=(const RCTime&);
    RCTime& operator-=(const RCTime&);

    
    RCTime operator/(PRUint64);
    RCTime operator*(PRUint64);
    RCTime& operator/=(PRUint64);
    RCTime& operator*=(PRUint64);

    void Now();                     

private:
    PRTime gmt;

public:

    RCTime(PRTime);                 
    void operator=(PRTime);         
    operator PRTime() const;        
};  

inline RCTime::RCTime(): RCBase() { }

inline void RCTime::Now() { gmt = PR_Now(); }
inline RCTime::operator PRTime() const { return gmt; }

inline void RCTime::operator=(PRTime his) { gmt = his; }
inline void RCTime::operator=(const RCTime& his) { gmt = his.gmt; }

inline PRBool RCTime::operator<(const RCTime& his)
    { return (gmt < his.gmt) ? PR_TRUE : PR_FALSE; }
inline PRBool RCTime::operator>(const RCTime& his)
    { return (gmt > his.gmt) ? PR_TRUE : PR_FALSE; }
inline PRBool RCTime::operator<=(const RCTime& his)
    { return (gmt <= his.gmt) ? PR_TRUE : PR_FALSE; }
inline PRBool RCTime::operator>=(const RCTime& his)
    { return (gmt >= his.gmt) ? PR_TRUE : PR_FALSE; }
inline PRBool RCTime::operator==(const RCTime& his)
    { return (gmt == his.gmt) ? PR_TRUE : PR_FALSE; }

inline RCTime& RCTime::operator+=(const RCTime& his)
    { gmt += his.gmt; return *this; }
inline RCTime& RCTime::operator-=(const RCTime& his)
    { gmt -= his.gmt; return *this; }
inline RCTime& RCTime::operator/=(PRUint64 his)
    { gmt /= his; return *this; }
inline RCTime& RCTime::operator*=(PRUint64 his)
    { gmt *= his; return *this; }

#endif 


