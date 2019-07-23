






































#ifndef __CIconServicesIcon_h__
#define __CIconServicesIcon_h__

#include <LControl.h>

class CIconServicesIcon : public LControl
{
public:
	enum { class_ID = FOUR_CHAR_CODE('CISC') };

                           CIconServicesIcon(const SPaneInfo&	inPaneInfo,
	                                         MessageT			inValueMessage,
                                             OSType             inIconType,
                                             SInt16             inIconResID);
						   CIconServicesIcon(LStream*	inStream);

	virtual				   ~CIconServicesIcon();

    
    virtual void            DrawSelf();
    virtual void            EnableSelf();
    virtual void            DisableSelf();
    
    
    SInt16                  FindHotSpot(Point	inPoint) const;
    Boolean                 PointInHotSpot(Point		inPoint,
								           SInt16	    inHotSpot) const;
    void                    HotSpotAction(SInt16    inHotSpot,
	                                      Boolean	inCurrInside,
	                                      Boolean	inPrevInside);
    void		            HotSpotResult(SInt16 inHotSpot);	
	
	
protected:
    void                    Init();              

    void                    AdjustIconRect(Rect& ioRect) const;

	void                    GetIconRef();
	void                    ReleaseIconRef();
	    
protected:
    OSType                  mIconType;
    SInt16                  mIconResID;
    IconAlignmentType       mAlignmentType;
    
    IconRef                 mIconRef;
    bool                    mbIsPressed;

    static OSType           mgAppCreator;
    static FSSpec           mgIconFileSpec;
};

#endif
