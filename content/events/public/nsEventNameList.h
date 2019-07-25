




































































#ifdef DEFINED_FORWARDED_EVENT
#error "Don't define DEFINED_FORWARDED_EVENT"
#endif 

#ifndef FORWARDED_EVENT
#define FORWARDED_EVENT(_name) EVENT(_name)
#define DEFINED_FORWARDED_EVENT
#endif 

#ifdef DEFINED_WINDOW_ONLY_EVENT
#error "Don't define DEFINED_WINDOW_ONLY_EVENT"
#endif 

#ifndef WINDOW_ONLY_EVENT
#define WINDOW_ONLY_EVENT(_name)
#define DEFINED_WINDOW_ONLY_EVENT
#endif 

#ifdef DEFINED_WINDOW_EVENT
#error "Don't define DEFINED_WINDOW_EVENT"
#endif 

#ifndef WINDOW_EVENT
#define WINDOW_EVENT(_name) WINDOW_ONLY_EVENT(_name)
#define DEFINED_WINDOW_EVENT
#endif 

#ifdef DEFINED_TOUCH_EVENT
#error "Don't define DEFINED_TOUCH_EVENT"
#endif 

#ifndef TOUCH_EVENT
#define TOUCH_EVENT(_name)
#define DEFINED_TOUCH_EVENT
#endif 

EVENT(abort)
EVENT(canplay)
EVENT(canplaythrough)
EVENT(change)
EVENT(click)
EVENT(contextmenu)
EVENT(cuechange)
EVENT(dblclick)
EVENT(drag)
EVENT(dragend)
EVENT(dragenter)
EVENT(dragleave)
EVENT(dragover)
EVENT(dragstart)
EVENT(drop)
EVENT(durationchange)
EVENT(emptied)
EVENT(ended)
EVENT(input)
EVENT(invalid)
EVENT(keydown)
EVENT(keypress)
EVENT(keyup)
EVENT(loadeddata)
EVENT(loadedmetadata)
EVENT(loadstart)
EVENT(mousedown)
EVENT(mousemove)
EVENT(mouseout)
EVENT(mouseover)
EVENT(mouseup)
EVENT(mousewheel)
EVENT(pause)
EVENT(play)
EVENT(playing)
EVENT(progress)
EVENT(ratechange)
EVENT(readystatechange)
EVENT(reset)
EVENT(seeked)
EVENT(seeking)
EVENT(select)
EVENT(show)
EVENT(stalled)
EVENT(submit)
EVENT(suspend)
EVENT(timeupdate)
EVENT(volumechange)
EVENT(waiting)

EVENT(copy)
EVENT(cut)
EVENT(paste)
EVENT(beforescriptexecute)
EVENT(afterscriptexecute)

FORWARDED_EVENT(blur)
FORWARDED_EVENT(error)
FORWARDED_EVENT(focus)
FORWARDED_EVENT(load)
FORWARDED_EVENT(scroll)

WINDOW_EVENT(afterprint)
WINDOW_EVENT(beforeprint)
WINDOW_EVENT(beforeunload)
WINDOW_EVENT(hashchange)
WINDOW_EVENT(message)
WINDOW_EVENT(offline)
WINDOW_EVENT(online)
WINDOW_EVENT(pagehide)
WINDOW_EVENT(pageshow)
WINDOW_EVENT(popstate)
WINDOW_EVENT(redo)
WINDOW_EVENT(resize)
WINDOW_EVENT(storage)
WINDOW_EVENT(undo)
WINDOW_EVENT(unload)

WINDOW_ONLY_EVENT(devicemotion)
WINDOW_ONLY_EVENT(deviceorientation)

TOUCH_EVENT(touchstart)
TOUCH_EVENT(touchend)
TOUCH_EVENT(touchmove)
TOUCH_EVENT(touchenter)
TOUCH_EVENT(touchleave)
TOUCH_EVENT(touchcancel)

#ifdef DEFINED_FORWARDED_EVENT
#undef DEFINED_FORWARDED_EVENT
#undef FORWARDED_EVENT
#endif 

#ifdef DEFINED_WINDOW_EVENT
#undef DEFINED_WINDOW_EVENT
#undef WINDOW_EVENT
#endif 

#ifdef DEFINED_WINDOW_ONLY_EVENT
#undef DEFINED_WINDOW_ONLY_EVENT
#undef WINDOW_ONLY_EVENT
#endif 

#ifdef DEFINED_TOUCH_EVENT
#undef DEFINED_TOUCH_EVENT
#undef TOUCH_EVENT
#endif 

