






































#ifndef __CThrobber__
#define __CThrobber__

#include <LControl.h>
#include <LPeriodical.h>
 

#include <Movies.h>

class CThrobber : public LControl,
                  public LPeriodical
{
public:
	enum { class_ID = FOUR_CHAR_CODE('Thrb') };

                             CThrobber();
                             CThrobber(LStream*	inStream);
 
	virtual				    ~CThrobber();
 
    
 	virtual void            FinishCreateSelf();
	virtual void	        ShowSelf();
	virtual void	        HideSelf();
 	virtual void            DrawSelf();
 
    void                    ResizeFrameBy(SInt16		inWidthDelta,
                            		      SInt16		inHeightDelta,
                            			  Boolean	    inRefresh);
    void                    MoveBy(SInt32		inHorizDelta,
         				           SInt32		inVertDelta,
         						   Boolean      inRefresh);
         						   
    
	virtual void		    SpendTime(const EventRecord &inMacEvent);
 	
	
 	virtual void            Start();
 	virtual void            Stop();
 
protected:
    void                    CreateMovie();
        
protected:
    SInt16                  mMovieResID;
    Handle                  mMovieHandle;
    Movie                   mMovie;
    MovieController         mMovieController;
};


#endif
