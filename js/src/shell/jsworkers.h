







































#ifndef jsworkers_h___
#define jsworkers_h___

#ifdef JS_THREADSAFE

#include "jsapi.h"






namespace js {
    namespace workers {
        class WorkerHooks {
        public:
            virtual JSObject *newGlobalObject(JSContext *cx) = 0;
            virtual ~WorkerHooks() {}
        };

        







        JSBool init(JSContext *cx, WorkerHooks *hooks, JSObject *global, JSObject **rootp);

        




        void terminateAll(JSContext *cx, JSObject *workersobj);

	






	void finish(JSContext *cx, JSObject *workersobj);
    }
}

#endif 

#endif 
