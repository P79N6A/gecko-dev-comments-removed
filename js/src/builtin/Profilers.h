










#ifndef Profilers_h___
#define Profilers_h___

#include "jsapi.h"












extern JS_PUBLIC_API(JSBool)
JS_StartProfiling(const char *profileName);





extern JS_PUBLIC_API(JSBool)
JS_StopProfiling(const char *profileName);





extern JS_PUBLIC_API(JSBool)
JS_DumpProfile(const char *outfile, const char *profileName);






extern JS_PUBLIC_API(JSBool)
JS_PauseProfilers(const char *profileName);




extern JS_PUBLIC_API(JSBool)
JS_ResumeProfilers(const char *profileName);






JS_PUBLIC_API(const char *)
JS_UnsafeGetLastProfilingError();

#ifdef MOZ_CALLGRIND

extern JS_FRIEND_API(JSBool)
js_StopCallgrind();

extern JS_FRIEND_API(JSBool)
js_StartCallgrind();

extern JS_FRIEND_API(JSBool)
js_DumpCallgrind(const char *outfile);

#endif 

#ifdef __linux__

extern JS_FRIEND_API(JSBool)
js_StartPerf();

extern JS_FRIEND_API(JSBool)
js_StopPerf();

#endif 

#endif 
