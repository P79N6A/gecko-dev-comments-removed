









#include "engine_configurations.h"

#if defined(CARBON_RENDERING)
#include "vie_autotest_mac_carbon.h"
#include "vie_autotest_defines.h"
#include "vie_autotest.h"
#include "vie_autotest_main.h"

ViEAutoTestWindowManager::ViEAutoTestWindowManager() :
    _carbonWindow1(new WindowRef()),
    _carbonWindow2(new WindowRef()),
    _hiView1(new HIViewRef()),
    _hiView2(new HIViewRef())
{
}

ViEAutoTestWindowManager::~ViEAutoTestWindowManager()
{
    if (_carbonWindow1EventHandlerRef)
        RemoveEventHandler(_carbonWindow1EventHandlerRef);

    if (_carbonWindow2EventHandlerRef)
        RemoveEventHandler(_carbonWindow2EventHandlerRef);

    if (_carbonHIView1EventHandlerRef)
        RemoveEventHandler(_carbonHIView1EventHandlerRef);

    if (_carbonHIView2EventHandlerRef)
        RemoveEventHandler(_carbonHIView2EventHandlerRef);

    delete _carbonWindow1;
    delete _carbonWindow2;
    delete _hiView1;
    delete _hiView2;
}

int ViEAutoTestWindowManager::CreateWindows(AutoTestRect window1Size,
                                            AutoTestRect window2Size,
                                            char* window1Title,
                                            char* window2Title)
{

    WindowAttributes windowAttributes = kWindowStandardDocumentAttributes
        | kWindowStandardHandlerAttribute | kWindowCompositingAttribute;
    Rect windowContentRect;
    static const EventTypeSpec
        windowEventTypes[] = { kEventClassWindow, kEventWindowBoundsChanged,
            kEventClassWindow, kEventWindowBoundsChanging, kEventClassWindow,
            kEventWindowZoomed, kEventClassWindow, kEventWindowExpanded,
            kEventClassWindow, kEventWindowClickResizeRgn, kEventClassWindow,
            kEventWindowClickDragRgn };

    

    SetRect(&windowContentRect, window1Size.origin.x, window1Size.origin.y,
            window1Size.origin.x + window1Size.size.width, window1Size.origin.y
                + window1Size.size.height);

    CreateNewWindow(kDocumentWindowClass, windowAttributes, &windowContentRect,
                    _carbonWindow1);
    SetWindowTitleWithCFString(*_carbonWindow1, CFSTR("Carbon Window 1"));
    ShowWindow(*_carbonWindow1);
    InitCursor();
    InstallWindowEventHandler(*_carbonWindow1,
                              NewEventHandlerUPP(HandleWindowEvent),
                              GetEventTypeCount(windowEventTypes),
                              windowEventTypes, (void*) this,
                              &_carbonWindow1EventHandlerRef);

    

    SetRect(&windowContentRect, window2Size.origin.x, window2Size.origin.y,
            window2Size.origin.x + window2Size.size.width, window2Size.origin.y
                + window2Size.size.height);

    CreateNewWindow(kDocumentWindowClass, windowAttributes, &windowContentRect,
                    _carbonWindow2);
    SetWindowTitleWithCFString(*_carbonWindow2, CFSTR("Carbon Window 2"));
    ShowWindow(*_carbonWindow2);
    InitCursor();
    InstallWindowEventHandler(*_carbonWindow2,
                              NewEventHandlerUPP(HandleWindowEvent),
                              GetEventTypeCount(windowEventTypes),
                              windowEventTypes, (void*) this,
                              &_carbonWindow2EventHandlerRef);

#if defined(HIVIEWREF_MODE)
    OSStatus status;
    static const EventTypeSpec hiviewEventTypes[] = { kEventClassControl,
        kEventControlBoundsChanged, kEventClassControl, kEventControlDraw };

    HIRect hiView1Rect = { 10, 10, 200, 200 };
    status = HICreateCustomView(&hiView1Rect, &_hiView1);
    status = HIViewAddSubview(&_carbonWindow1, _hiView1);
    HIViewSetZOrder(_hiView1, kHIViewZOrderAbove, NULL);
    HIViewSetVisible(_hiView1, true);

    HIViewInstallEventHandler(_hiView1, NewEventHandlerUPP(HandleHIViewEvent),
                              GetEventTypeCount(hiviewEventTypes),
                              hiviewEventTypes, (void *) this,
                              &_carbonHIView1EventHandlerRef);

    HIRect hiView2Rect = { 10, 10, 200, 200 };
    status = HICreateCustomView(&hiView2Rect, &_hiView2);
    status = HIViewAddSubview(&_carbonWindow2, _hiView2);
    HIViewSetZOrder(_hiView2, kHIViewZOrderAbove, NULL);
    HIViewSetVisible(_hiView2, true);

    HIViewInstallEventHandler(_hiView2, NewEventHandlerUPP(HandleHIViewEvent),
                              GetEventTypeCount(hiviewEventTypes),
                              hiviewEventTypes, (void *) this,
                              &_carbonHIView2EventHandlerRef);
#endif

    return 0;
}

pascal OSStatus ViEAutoTestWindowManager::HandleWindowEvent(
    EventHandlerCallRef nextHandler, EventRef theEvent, void* userData)
{

    WindowRef windowRef = NULL;

    int eventType = GetEventKind(theEvent);

    
    
    GetEventParameter(theEvent, kEventParamDirectObject, typeWindowRef, NULL,
                      sizeof(WindowRef), NULL, &windowRef);

    ViEAutoTestWindowManager* obj = (ViEAutoTestWindowManager*) (userData);

    if (windowRef == obj->GetWindow1())
    {
        
    }
    else if (windowRef == obj->GetWindow2())
    {
        
    }

    if (kEventWindowBoundsChanged == eventType)
    {
    }
    else if (kEventWindowBoundsChanging == eventType)
    {
    }
    else if (kEventWindowZoomed == eventType)
    {
    }
    else if (kEventWindowExpanding == eventType)
    {
    }
    else if (kEventWindowExpanded == eventType)
    {
    }
    else if (kEventWindowClickResizeRgn == eventType)
    {
    }
    else if (kEventWindowClickDragRgn == eventType)
    {
    }
    else
    {
    }

    return noErr;
}

pascal OSStatus ViEAutoTestWindowManager::HandleHIViewEvent(
    EventHandlerCallRef nextHandler, EventRef theEvent, void* userData)
{
    HIViewRef hiviewRef = NULL;

    
    
    int eventType = GetEventKind(theEvent);
    OSStatus status = noErr;
    status = GetEventParameter(theEvent, kEventParamDirectObject,
                               typeControlRef, NULL, sizeof(ControlRef), NULL,
                               &hiviewRef);

    if (GetEventClass(theEvent) == kEventClassControl)
    {
        if (GetEventKind(theEvent) == kEventControlDraw)
        {
            ViEAutoTestWindowManager* obj =
                (ViEAutoTestWindowManager*) (userData);

            CGContextRef context;
            status = GetEventParameter(theEvent, kEventParamCGContextRef,
                                       typeCGContextRef, NULL, sizeof(context),
                                       NULL, &context);
            HIRect viewBounds;

            HIViewRef* ptrHIViewRef =
                static_cast<HIViewRef*> (obj->GetWindow1());
            if (hiviewRef == *ptrHIViewRef)
            {
                
                CGContextSetRGBFillColor(context, 1, 0, 0, 1);
                HIViewGetBounds(hiviewRef, &viewBounds);
                CGContextFillRect(context, viewBounds);
            }

            ptrHIViewRef = static_cast<HIViewRef*> (obj->GetWindow1());
            if (hiviewRef == *ptrHIViewRef)
            {
                CGContextSetRGBFillColor(context, 0, 1, 0, 1);
                HIViewGetBounds(hiviewRef, &viewBounds);
                CGContextFillRect(context, viewBounds);
            }

        }
    }

    





















    return status;
}

int ViEAutoTestWindowManager::TerminateWindows()
{
    return 0;
}

void* ViEAutoTestWindowManager::GetWindow1()
{
#if defined(HIVIEWREF_MODE)
    return (void*)_hiView1;
#else
    return (void*) _carbonWindow1;
#endif

}
void* ViEAutoTestWindowManager::GetWindow2()
{
#if defined(HIVIEWREF_MODE)
    return (void*)_hiView2;
#else
    return (void*) _carbonWindow2;
#endif

}

bool ViEAutoTestWindowManager::SetTopmostWindow()
{
    return true;
}

















int main(int argc, const char * argv[])
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    [NSApplication sharedApplication];

    
    
    if (argc > 1)
    {
AutoTestClass    * autoTestClass = [[AutoTestClass alloc]init];
    [NSThread detachNewThreadSelector:@selector(autoTestWithArg:)
     toTarget:autoTestClass withObject:[NSString stringWithFormat:@"%s",
                                        argv[1]]];
}
else
{
    AutoTestClass* autoTestClass = [[AutoTestClass alloc]init];
    [NSThread detachNewThreadSelector:@selector(autoTestWithArg:)
     toTarget:autoTestClass withObject:nil];
}


[[NSRunLoop currentRunLoop]run];
[pool release];
}

@implementation AutoTestClass

-(void)autoTestWithArg:(NSString*)answerFile;
{
    
    
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    ViEAutoTestMain autoTest;

    if(NSOrderedSame != [answerFile compare:@""])
    {
        char answerFileUTF8[1024] = "";
        strcpy(answerFileUTF8, (char*)[answerFileUTF8 UTF8]);
        autoTest.UseAnswerFile(answerFileUTF8);
    }

    int success = autoTest.BeginOSIndependentTesting();

    [pool release];
    return;
}






@end

#endif

