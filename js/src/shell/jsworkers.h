







































#ifndef jsworkers_h___
#define jsworkers_h___

#ifdef JS_THREADSAFE

#include "jsapi.h"






namespace js {
    namespace workers {
        class ThreadPool;

        class WorkerHooks {
        public:
            virtual JSObject *newGlobalObject(JSContext *cx) = 0;
            virtual ~WorkerHooks() {}
        };

        







        ThreadPool *init(JSContext *cx, WorkerHooks *hooks, JSObject *global, JSObject **rootp);

        




        void terminateAll(ThreadPool *tp);

	






	void finish(JSContext *cx, ThreadPool *tp);
    }
}

#endif 

#endif 
