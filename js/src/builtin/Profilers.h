









#ifndef builtin_Profilers_h
#define builtin_Profilers_h

#include "jstypes.h"












extern JS_PUBLIC_API(bool)
JS_StartProfiling(const char *profileName);





extern JS_PUBLIC_API(bool)
JS_StopProfiling(const char *profileName);





extern JS_PUBLIC_API(bool)
JS_DumpProfile(const char *outfile, const char *profileName);






extern JS_PUBLIC_API(bool)
JS_PauseProfilers(const char *profileName);




extern JS_PUBLIC_API(bool)
JS_ResumeProfilers(const char *profileName);






JS_PUBLIC_API(const char *)
JS_UnsafeGetLastProfilingError();

#ifdef MOZ_CALLGRIND

extern JS_FRIEND_API(bool)
js_StopCallgrind();

extern JS_FRIEND_API(bool)
js_StartCallgrind();

extern JS_FRIEND_API(bool)
js_DumpCallgrind(const char *outfile);

#endif 

#ifdef __linux__

extern JS_FRIEND_API(bool)
js_StartPerf();

extern JS_FRIEND_API(bool)
js_StopPerf();

#endif 

#endif 
