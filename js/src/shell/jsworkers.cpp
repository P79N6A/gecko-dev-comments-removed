







































#ifdef JS_THREADSAFE

#include <string.h>
#include "prthread.h"
#include "prlock.h"
#include "prcvar.h"
#include "jsapi.h"
#include "jscntxt.h"
#include "jshashtable.h"
#include "jsstdint.h"
#include "jslock.h"
#include "jsvector.h"
#include "jsworkers.h"

extern size_t gMaxStackSize;








































namespace js {
namespace workers {

template <class T, class AllocPolicy>
class Queue {
  private:
    typedef Vector<T, 4, AllocPolicy> Vec;
    Vec v1;
    Vec v2;
    Vec *front;
    Vec *back;

    
    Queue(const Queue &);
    Queue & operator=(const Queue &);

  public:
    Queue() : front(&v1), back(&v2) {}
    bool push(T t) { return back->append(t); }
    bool empty() { return front->empty() && back->empty(); }

    T pop() {
        if (front->empty()) {
            js::Reverse(back->begin(), back->end());
            Vec *tmp = front;
            front = back;
            back = tmp;
        }
        T item = front->back();
        front->popBack();
        return item;
    }        

    void clear() {
        v1.clear();
        v2.clear();
    }

    void trace(JSTracer *trc) {
        for (T *p = v1.begin(); p != v1.end(); p++)
            (*p)->trace(trc);
        for (T *p = v2.begin(); p != v2.end(); p++)
            (*p)->trace(trc);
    }
};

class Event;
class ThreadPool;
class Worker;

class WorkerParent {
  protected:
    typedef HashSet<Worker *, DefaultHasher<Worker *>, SystemAllocPolicy> ChildSet;
    ChildSet children;

    bool initWorkerParent() { return children.init(8); }

  public:
    virtual JSLock *getLock() = 0;
    virtual ThreadPool *getThreadPool() = 0;
    virtual bool post(Event *item) = 0;  
    virtual void trace(JSTracer *trc) = 0;

    bool addChild(Worker *w) {
        AutoLock hold(getLock());
        return children.put(w) != NULL;
    }

    
    
    void removeChild(Worker *w) {
        ChildSet::Ptr p = children.lookup(w);
        JS_ASSERT(p);
        children.remove(p);
    }

    void disposeChildren();
};

template <class T>
class ThreadSafeQueue
{
  protected:
    Queue<T, SystemAllocPolicy> queue;
    JSLock *lock;
    PRCondVar *condvar;
    bool closed;

  private:
    Vector<T, 8, SystemAllocPolicy> busy;

  protected:
    ThreadSafeQueue() : lock(NULL), condvar(NULL), closed(false) {}

    ~ThreadSafeQueue() {
        if (condvar)
            JS_DESTROY_CONDVAR(condvar);
        if (lock)
            JS_DESTROY_LOCK(lock);
    }

    
    virtual bool shouldStop() { return closed; }

  public:
    bool initThreadSafeQueue() {
        JS_ASSERT(!lock);
        JS_ASSERT(!condvar);
        return (lock = JS_NEW_LOCK()) && (condvar = JS_NEW_CONDVAR(lock));
    }

    bool post(T t) {
        AutoLock hold(lock);
        if (closed)
            return false;
        if (queue.empty())
            JS_NOTIFY_ALL_CONDVAR(condvar);
        return queue.push(t);
    }

    void close() {
        AutoLock hold(lock);
        closed = true;
        queue.clear();
        JS_NOTIFY_ALL_CONDVAR(condvar);
    }

    
    bool take(T *t) {
        while (queue.empty()) {
            if (shouldStop())
                return false;
            JS_WAIT_CONDVAR(condvar, JS_NO_TIMEOUT);
        }
        *t = queue.pop();
        busy.append(*t);
        return true;
    }

    
    void drop(T item) {
        for (T *p = busy.begin(); p != busy.end(); p++) {
            if (*p == item) {
                *p = busy.back();
                busy.popBack();
                return;
            }
        }
        JS_NOT_REACHED("removeBusy");
    }

    bool lockedIsIdle() { return busy.empty() && queue.empty(); }

    bool isIdle() {
        AutoLock hold(lock);
        return lockedIsIdle();
    }

    void wake() {
        AutoLock hold(lock);
        JS_NOTIFY_ALL_CONDVAR(condvar);
    }

    void trace(JSTracer *trc) {
        AutoLock hold(lock);
        for (T *p = busy.begin(); p != busy.end(); p++)
            (*p)->trace(trc);
        queue.trace(trc);
    }
};

class MainQueue;

class Event
{
  protected:
    virtual ~Event() { JS_ASSERT(!data); }

    WorkerParent *recipient;
    Worker *child;
    uint64 *data;
    size_t nbytes;

  public:
    enum Result { fail = JS_FALSE, ok = JS_TRUE, forwardToParent };

    virtual void destroy(JSContext *cx) { 
        JS_free(cx, data);
#ifdef DEBUG
        data = NULL;
#endif
        delete this;
    }

    void setChildAndRecipient(Worker *aChild, WorkerParent *aRecipient) {
        child = aChild;
        recipient = aRecipient;
    }

    bool deserializeData(JSContext *cx, jsval *vp) {
        return !!JS_ReadStructuredClone(cx, data, nbytes, JS_STRUCTURED_CLONE_VERSION, vp,
                                        NULL, NULL);
    }

    virtual Result process(JSContext *cx) = 0;

    inline void trace(JSTracer *trc);

    template <class EventType>
    static EventType *createEvent(JSContext *cx, WorkerParent *recipient, Worker *child,
                                  jsval v)
    {
        uint64 *data;
        size_t nbytes;
        if (!JS_WriteStructuredClone(cx, v, &data, &nbytes, NULL, NULL))
            return NULL;

        EventType *event = new EventType;
        if (!event) {
            JS_ReportOutOfMemory(cx);
            return NULL;
        }
        event->recipient = recipient;
        event->child = child;
        event->data = data;
        event->nbytes = nbytes;
        return event;
    }

    Result dispatch(JSContext *cx, JSObject *thisobj, const char *dataPropName,
                    const char *methodName, Result noHandler)
    {
        if (!data)
            return fail;

        JSBool found;
        if (!JS_HasProperty(cx, thisobj, methodName, &found))
            return fail;
        if (!found)
            return noHandler;

        
        jsval v;
        if (!deserializeData(cx, &v))
            return fail;
        JSObject *obj = JS_NewObject(cx, NULL, NULL, NULL);
        if (!obj || !JS_DefineProperty(cx, obj, dataPropName, v, NULL, NULL, 0))
            return fail;

        
        jsval argv[1] = { OBJECT_TO_JSVAL(obj) };
        jsval rval = JSVAL_VOID;
        return Result(JS_CallFunctionName(cx, thisobj, methodName, 1, argv, &rval));
    }
};

typedef ThreadSafeQueue<Event *> EventQueue;

class MainQueue : public EventQueue, public WorkerParent
{
  private:
    ThreadPool *threadPool;

  public:
    explicit MainQueue(ThreadPool *tp) : threadPool(tp) {}

    ~MainQueue() {
        JS_ASSERT(queue.empty());
    }

    bool init() { return initThreadSafeQueue() && initWorkerParent(); }

    void destroy(JSContext *cx) {
        while (!queue.empty())
            queue.pop()->destroy(cx);
        delete this;
    }

    virtual JSLock *getLock() { return lock; }
    virtual ThreadPool *getThreadPool() { return threadPool; }

  protected:
    virtual bool shouldStop();

  public:
    virtual bool post(Event *event) { return EventQueue::post(event); }

    virtual void trace(JSTracer *trc);

    void traceChildren(JSTracer *trc) { EventQueue::trace(trc); }

    JSBool mainThreadWork(JSContext *cx, bool continueOnError) {
        JSAutoSuspendRequest suspend(cx);
        AutoLock hold(lock);

        Event *event;
        while (take(&event)) {
            JS_RELEASE_LOCK(lock);
            Event::Result result;
            {
                JSAutoRequest req(cx);
                result = event->process(cx);
                if (result == Event::forwardToParent) {
                    
                    jsval data;
                    JSAutoByteString bytes;
                    if (event->deserializeData(cx, &data) &&
                        JSVAL_IS_STRING(data) &&
                        bytes.encode(cx, JSVAL_TO_STRING(data))) {
                        JS_ReportError(cx, "%s", bytes.ptr());
                    } else {
                        JS_ReportOutOfMemory(cx);
                    }
                    result = Event::fail;
                }
                if (result == Event::fail && continueOnError) {
                    if (JS_IsExceptionPending(cx) && !JS_ReportPendingException(cx))
                        JS_ClearPendingException(cx);
                    result = Event::ok;
                }
            }
            JS_ACQUIRE_LOCK(lock);
            drop(event);
            event->destroy(cx);
            if (result != Event::ok)
                return false;
        }
        return true;
    }
};







class WorkerQueue : public ThreadSafeQueue<Worker *>
{
  private:
    MainQueue *main;

  public:
    explicit WorkerQueue(MainQueue *main) : main(main) {}

    void work();
};


class ThreadPool
{
  private:
    enum { threadCount = 6 };

    JSObject *obj;
    WorkerHooks *hooks;
    MainQueue *mq;
    WorkerQueue *wq;
    PRThread *threads[threadCount];
    int32_t terminating;

    static JSClass jsClass;

    static void start(void* arg) {
        ((WorkerQueue *) arg)->work();
    }

    explicit ThreadPool(WorkerHooks *hooks) : hooks(hooks), mq(NULL), wq(NULL), terminating(0) {
        for (int i = 0; i < threadCount; i++)
            threads[i] = NULL;
    }

  public:
    ~ThreadPool() {
        JS_ASSERT(!mq);
        JS_ASSERT(!wq);
        JS_ASSERT(!threads[0]);
    }

    static ThreadPool *create(JSContext *cx, WorkerHooks *hooks) {
        ThreadPool *tp = new ThreadPool(hooks);
        if (!tp) {
            JS_ReportOutOfMemory(cx);
            return NULL;
        }

        JSObject *obj = JS_NewObject(cx, &jsClass, NULL, NULL);
        if (!obj || !JS_SetPrivate(cx, obj, tp)) {
            delete tp;
            return NULL;
        }
        tp->obj = obj;
        return tp;
    }

    JSObject *asObject() { return obj; }
    WorkerHooks *getHooks() { return hooks; }
    WorkerQueue *getWorkerQueue() { return wq; }
    MainQueue *getMainQueue() { return mq; }
    bool isTerminating() { return terminating != 0; }

    



    bool start(JSContext *cx) {
        JS_ASSERT(!mq && !wq);
        mq = new MainQueue(this);
        if (!mq || !mq->init()) {
            mq->destroy(cx);
            mq = NULL;
            return false;
        }
        wq = new WorkerQueue(mq);
        if (!wq || !wq->initThreadSafeQueue()) {
            delete wq;
            wq = NULL;
            mq->destroy(cx);
            mq = NULL;
            return false;
        }
        JSAutoSuspendRequest suspend(cx);
        bool ok = true;
        for (int i = 0; i < threadCount; i++) {
            threads[i] = PR_CreateThread(PR_USER_THREAD, start, wq, PR_PRIORITY_NORMAL,
                                         PR_LOCAL_THREAD, PR_JOINABLE_THREAD, 0);
            if (!threads[i]) {
                shutdown(cx);
                ok = false;
                break;
            }
        }
        return ok;
    }

    void terminateAll(JSRuntime *rt) {
        
        
        JS_ATOMIC_SET(&terminating, 1);
        JS_TriggerAllOperationCallbacks(rt);
    }

    
    void shutdown(JSContext *cx) {
        wq->close();
        for (int i = 0; i < threadCount; i++) {
            if (threads[i]) {
                PR_JoinThread(threads[i]);
                threads[i] = NULL;
            }
        }

        delete wq;
        wq = NULL;

        mq->disposeChildren();
        mq->destroy(cx);
        mq = NULL;
        terminating = 0;
    }

  private:
    static void jsTraceThreadPool(JSTracer *trc, JSObject *obj) {
        ThreadPool *tp = unwrap(trc->context, obj);
        if (tp->mq) {
            tp->mq->traceChildren(trc);
            tp->wq->trace(trc);
        }
    }


    static void jsFinalize(JSContext *cx, JSObject *obj) {
        if (ThreadPool *tp = unwrap(cx, obj))
            delete tp;
    }

  public:
    static ThreadPool *unwrap(JSContext *cx, JSObject *obj) {
        JS_ASSERT(JS_GET_CLASS(cx, obj) == &jsClass);
        return (ThreadPool *) JS_GetPrivate(cx, obj);
    }
};












class Worker : public WorkerParent
{
  private:
    ThreadPool *threadPool;
    WorkerParent *parent;
    JSObject *object;  
    JSContext *context;
    JSLock *lock;
    Queue<Event *, SystemAllocPolicy> events;  
    Event *current;
    bool terminated;
    int32_t terminateFlag;

    static JSClass jsWorkerClass;

    Worker()
        : threadPool(NULL), parent(NULL), object(NULL),
          context(NULL), lock(NULL), current(NULL), terminated(false), terminateFlag(0) {}

    bool init(JSContext *parentcx, WorkerParent *parent, JSObject *obj) {
        JS_ASSERT(!threadPool && !this->parent && !object && !lock);

        if (!initWorkerParent() || !parent->addChild(this))
            return false;
        threadPool = parent->getThreadPool();
        this->parent = parent;
        this->object = obj;
        lock = JS_NEW_LOCK();
        return lock &&
               createContext(parentcx, parent) &&
               JS_SetPrivate(parentcx, obj, this);
    }

    bool createContext(JSContext *parentcx, WorkerParent *parent) {
        JSRuntime *rt = JS_GetRuntime(parentcx);
        context = JS_NewContext(rt, 8192);
        if (!context)
            return false;

        
        
        
        
        JS_SetOptions(context, JS_GetOptions(parentcx) | JSOPTION_UNROOTED_GLOBAL |
                                                         JSOPTION_DONT_REPORT_UNCAUGHT);
        JS_SetVersion(context, JS_GetVersion(parentcx));
        JS_SetContextPrivate(context, this);
        JS_SetOperationCallback(context, jsOperationCallback);
        JS_BeginRequest(context);

        JSObject *global = threadPool->getHooks()->newGlobalObject(context);
        JSObject *post, *proto, *ctor;
        if (!global)
            goto bad;
        JS_SetGlobalObject(context, global);

        
        
        
        
        
        post = JS_GetFunctionObject(JS_DefineFunction(context, global, "postMessage",
                                                      (JSNative) jsPostMessageToParent, 1, 0));
        if (!post || !JS_SetReservedSlot(context, post, 0, PRIVATE_TO_JSVAL(this)))
            goto bad;

        proto = JS_InitClass(context, global, NULL, &jsWorkerClass, jsConstruct, 1,
                             NULL, jsMethods, NULL, NULL);
        if (!proto)
            goto bad;

        ctor = JS_GetConstructor(context, proto);
        if (!ctor || !JS_SetReservedSlot(context, ctor, 0, PRIVATE_TO_JSVAL(this)))
            goto bad;

        JS_EndRequest(context);
        JS_ClearContextThread(context);
        return true;

    bad:
        JS_EndRequest(context);
        JS_DestroyContext(context);
        context = NULL;
        return false;
    }

    static void jsTraceWorker(JSTracer *trc, JSObject *obj) {
        JS_ASSERT(JS_GET_CLASS(trc->context, obj) == &jsWorkerClass);
        if (Worker *w = (Worker *) JS_GetPrivate(trc->context, obj)) {
            w->parent->trace(trc);
            w->events.trace(trc);
            if (w->current)
                w->current->trace(trc);
            JS_CALL_OBJECT_TRACER(trc, JS_GetGlobalObject(w->context), "Worker global");
        }
    }

    static void jsFinalize(JSContext *cx, JSObject *obj) {
        JS_ASSERT(JS_GET_CLASS(cx, obj) == &jsWorkerClass);
        if (Worker *w = (Worker *) JS_GetPrivate(cx, obj))
            delete w;
    }

    static JSBool jsOperationCallback(JSContext *cx) {
        Worker *w = (Worker *) JS_GetContextPrivate(cx);
        JSAutoSuspendRequest suspend(cx);  
        return !w->checkTermination();
    }

    static JSBool jsResolveGlobal(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                                  JSObject **objp)
    {
        JSBool resolved;

        if (!JS_ResolveStandardClass(cx, obj, id, &resolved))
            return false;
        if (resolved)
            *objp = obj;

        return true;
    }

    static JSBool jsPostMessageToParent(JSContext *cx, uintN argc, jsval *vp);
    static JSBool jsPostMessageToChild(JSContext *cx, uintN argc, jsval *vp);
    static JSBool jsTerminate(JSContext *cx, uintN argc, jsval *vp);

    bool checkTermination() {
        AutoLock hold(lock);
        return lockedCheckTermination();
    }

    bool lockedCheckTermination() {
        if (terminateFlag || threadPool->isTerminating()) {
            terminateSelf();
            terminateFlag = 0;
        }
        return terminated;
    }

    
    void terminateSelf() {
        terminated = true;
        while (!events.empty())
            events.pop()->destroy(context);

        
        
        
        for (ChildSet::Enum e(children); !e.empty(); e.popFront())
            e.front()->setTerminateFlag();  
    }

  public:
    ~Worker() {
        if (parent)
            parent->removeChild(this);
        dispose();
    }

    void dispose() {
        JS_ASSERT(!current);
        while (!events.empty())
            events.pop()->destroy(context);
        if (lock) {
            JS_DESTROY_LOCK(lock);
            lock = NULL;
        }
        if (context) {
            JS_SetContextThread(context);
            JS_DestroyContextNoGC(context);
            context = NULL;
        }
        object = NULL;

        
        
        
        parent = NULL;
        disposeChildren();
    }

    static Worker *create(JSContext *parentcx, WorkerParent *parent,
                          JSString *scriptName, JSObject *obj);

    JSObject *asObject() { return object; }

    JSObject *getGlobal() { return JS_GetGlobalObject(context); }

    WorkerParent *getParent() { return parent; }

    virtual JSLock *getLock() { return lock; }

    virtual ThreadPool *getThreadPool() { return threadPool; }

    bool post(Event *event) {
        AutoLock hold(lock);
        if (terminated)
            return false;
        if (!current && events.empty() && !threadPool->getWorkerQueue()->post(this))
            return false;
        return events.push(event);
    }

    void setTerminateFlag() {
        AutoLock hold(lock);
        terminateFlag = true;
        if (current)
            JS_TriggerOperationCallback(context);
    }

    void processOneEvent();

    
    void trace(JSTracer *trc) {
        
        
        JS_CALL_OBJECT_TRACER(trc, object, "queued Worker");
    }

    static bool getWorkerParentFromConstructor(JSContext *cx, JSObject *ctor, WorkerParent **p) {
        jsval v;
        if (!JS_GetReservedSlot(cx, ctor, 0, &v))
            return false;
        if (JSVAL_IS_VOID(v)) {
            
            
            
            if (!JS_GetReservedSlot(cx, ctor, 1, &v))
                return false;
            ThreadPool *threadPool = (ThreadPool *) JSVAL_TO_PRIVATE(v);
            if (!threadPool->start(cx))
                return false;
            WorkerParent *parent = threadPool->getMainQueue();
            if (!JS_SetReservedSlot(cx, ctor, 0, PRIVATE_TO_JSVAL(parent))) {
                threadPool->shutdown(cx);
                return false;
            }
            *p = parent;
            return true;
        }
        *p = (WorkerParent *) JSVAL_TO_PRIVATE(v);
        return true;
    }

    static JSBool jsConstruct(JSContext *cx, uintN argc, jsval *vp) {
        WorkerParent *parent;
        if (!getWorkerParentFromConstructor(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)), &parent))
            return false;


        JSString *scriptName = JS_ValueToString(cx, argc ? JS_ARGV(cx, vp)[0] : JSVAL_VOID);
        if (!scriptName)
            return false;

        JSObject *obj = JS_NewObject(cx, &jsWorkerClass, NULL, NULL);
        if (!obj || !create(cx, parent, scriptName, obj))
            return false;
        JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
        return true;
    }

    static JSFunctionSpec jsMethods[3];
    static JSFunctionSpec jsStaticMethod[2];

    static ThreadPool *initWorkers(JSContext *cx, WorkerHooks *hooks, JSObject *global,
                                   JSObject **objp) {
        
        ThreadPool *threadPool = ThreadPool::create(cx, hooks);
        if (!threadPool)
            return NULL;

        
        *objp = threadPool->asObject();

        
        JSObject *proto = JS_InitClass(cx, global, NULL, &jsWorkerClass,
                                       jsConstruct, 1,
                                       NULL, jsMethods, NULL, NULL);
        if (!proto)
            return NULL;

        
        
        JSObject *ctor = JS_GetConstructor(cx, proto);
        if (!JS_SetReservedSlot(cx, ctor, 1, PRIVATE_TO_JSVAL(threadPool)))
            return NULL;

        return threadPool;
    }
};

class InitEvent : public Event
{
  public:
    static InitEvent *create(JSContext *cx, Worker *worker, JSString *scriptName) {
        return createEvent<InitEvent>(cx, worker, worker, STRING_TO_JSVAL(scriptName));
    }

    Result process(JSContext *cx) {
        jsval s;
        if (!deserializeData(cx, &s))
            return fail;
        JS_ASSERT(JSVAL_IS_STRING(s));
        JSAutoByteString filename(cx, JSVAL_TO_STRING(s));
        if (!filename)
            return fail;

        JSObject *scriptObj = JS_CompileFile(cx, child->getGlobal(), filename.ptr());
        if (!scriptObj)
            return fail;

        AutoValueRooter rval(cx);
        JSBool ok = JS_ExecuteScript(cx, child->getGlobal(), scriptObj, Jsvalify(rval.addr()));
        return Result(ok);
    }
};

class DownMessageEvent : public Event
{
  public:
    static DownMessageEvent *create(JSContext *cx, Worker *child, jsval data) {
        return createEvent<DownMessageEvent>(cx, child, child, data);
    }

    Result process(JSContext *cx) {
        return dispatch(cx, child->getGlobal(), "data", "onmessage", ok);
    }
};

class UpMessageEvent : public Event
{
  public:
    static UpMessageEvent *create(JSContext *cx, Worker *child, jsval data) {
        return createEvent<UpMessageEvent>(cx, child->getParent(), child, data);
    }

    Result process(JSContext *cx) {
        return dispatch(cx, child->asObject(), "data", "onmessage", ok);
    }
};

class ErrorEvent : public Event
{
  public:
    static ErrorEvent *create(JSContext *cx, Worker *child) {
        JSString *data = NULL;
        jsval exc;
        if (JS_GetPendingException(cx, &exc)) {
            AutoValueRooter tvr(cx, Valueify(exc));
            JS_ClearPendingException(cx);

            
            
            
            if (JSVAL_IS_OBJECT(exc)) {
                jsval msg;
                if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(exc), "message", &msg))
                    JS_ClearPendingException(cx);
                else if (JSVAL_IS_STRING(msg))
                    data = JSVAL_TO_STRING(msg);
            }
            if (!data) {
                data = JS_ValueToString(cx, exc);
                if (!data)
                    return NULL;
            }
        }
        return createEvent<ErrorEvent>(cx, child->getParent(), child,
                                       data ? STRING_TO_JSVAL(data) : JSVAL_VOID);
    }

    Result process(JSContext *cx) {
        return dispatch(cx, child->asObject(), "message", "onerror", forwardToParent);
    }
};

} 
} 

using namespace js::workers;

void
WorkerParent::disposeChildren()
{
    for (ChildSet::Enum e(children); !e.empty(); e.popFront()) {
        e.front()->dispose();
        e.removeFront();
    }
}

bool
MainQueue::shouldStop()
{
    
    
    
    
    
    return closed || threadPool->getWorkerQueue()->isIdle();
}

void
MainQueue::trace(JSTracer *trc)
{
     JS_CALL_OBJECT_TRACER(trc, threadPool->asObject(), "MainQueue");
}

void
WorkerQueue::work() {
    AutoLock hold(lock);

    Worker *w;
    while (take(&w)) {  
        JS_RELEASE_LOCK(lock);
        w->processOneEvent();     
        JS_ACQUIRE_LOCK(lock);
        drop(w);

        if (lockedIsIdle()) {
            JS_RELEASE_LOCK(lock);
            main->wake();
            JS_ACQUIRE_LOCK(lock);
        }
    }
}

const bool mswin =
#ifdef XP_WIN
    true
#else
    false
#endif
    ;

template <class Ch> bool
IsAbsolute(const Ch *filename)
{
    return filename[0] == '/' ||
           (mswin && (filename[0] == '\\' || (filename[0] != '\0' && filename[1] == ':')));
}


static JSString *
ResolveRelativePath(JSContext *cx, const char *base, JSString *filename)
{
    size_t fileLen = JS_GetStringLength(filename);
    const jschar *fileChars = JS_GetStringCharsZ(cx, filename);
    if (!fileChars)
        return NULL;

    if (IsAbsolute(fileChars))
        return filename;

    
    size_t dirLen = -1;
    for (size_t i = 0; base[i]; i++) {
        if (base[i] == '/' || (mswin && base[i] == '\\'))
            dirLen = i;
    }

    
    if (!IsAbsolute(base) && dirLen == (size_t) -1)
        return filename;

    
    js::Vector<jschar, 0> result(cx);
    size_t nchars;
    if (!JS_DecodeBytes(cx, base, dirLen + 1, NULL, &nchars))
        return NULL;
    if (!result.reserve(dirLen + 1 + fileLen))
        return NULL;
    JS_ALWAYS_TRUE(result.resize(dirLen + 1));
    if (!JS_DecodeBytes(cx, base, dirLen + 1, result.begin(), &nchars))
        return NULL;
    result.infallibleAppend(fileChars, fileLen);
    return JS_NewUCStringCopyN(cx, result.begin(), result.length());
}

Worker *
Worker::create(JSContext *parentcx, WorkerParent *parent, JSString *scriptName, JSObject *obj)
{
    Worker *w = new Worker();
    if (!w || !w->init(parentcx, parent, obj)) {
        delete w;
        return NULL;
    }

    JSStackFrame *frame = JS_GetScriptedCaller(parentcx, NULL);
    const char *base = JS_GetScriptFilename(parentcx, JS_GetFrameScript(parentcx, frame));
    JSString *scriptPath = ResolveRelativePath(parentcx, base, scriptName);
    if (!scriptPath)
        return NULL;

    
    Event *event = InitEvent::create(parentcx, w, scriptPath);
    if (!event)
        return NULL;
    if (!w->events.push(event) || !w->threadPool->getWorkerQueue()->post(w)) {
        event->destroy(parentcx);
        JS_ReportOutOfMemory(parentcx);
        w->dispose();
        return NULL;
    }
    return w;
}

void
Worker::processOneEvent()
{
    Event *event = NULL;    
    {
        AutoLock hold1(lock);
        if (lockedCheckTermination() || events.empty())
            return;

        event = current = events.pop();
    }

    JS_SetContextThread(context);
    JS_SetNativeStackQuota(context, gMaxStackSize);

    Event::Result result;
    {
        JSAutoRequest req(context);
        result = event->process(context);
    }

    
    
    if (result == Event::forwardToParent) {
        event->setChildAndRecipient(this, parent);
        if (parent->post(event)) {
            event = NULL;  
        } else {
            JS_ReportOutOfMemory(context);
            result = Event::fail;
        }
    }
    if (result == Event::fail && !checkTermination()) {
        JSAutoRequest req(context);
        Event *err = ErrorEvent::create(context, this);
        if (err && !parent->post(err)) {
            JS_ReportOutOfMemory(context);
            err->destroy(context);
            err = NULL;
        }
        if (!err) {
            
        }
    }

    if (event)
        event->destroy(context);
    JS_ClearContextThread(context);

    {
        AutoLock hold2(lock);
        current = NULL;
        if (!lockedCheckTermination() && !events.empty()) {
            
            if (!threadPool->getWorkerQueue()->post(this))
                JS_ReportOutOfMemory(context);
        }
    }
}

JSBool
Worker::jsPostMessageToParent(JSContext *cx, uintN argc, jsval *vp)
{
    jsval workerval;
    if (!JS_GetReservedSlot(cx, JSVAL_TO_OBJECT(JS_CALLEE(cx, vp)), 0, &workerval))
        return false;
    Worker *w = (Worker *) JSVAL_TO_PRIVATE(workerval);

    {
        JSAutoSuspendRequest suspend(cx);  
        if (w->checkTermination())
            return false;
    }

    jsval data = argc > 0 ? JS_ARGV(cx, vp)[0] : JSVAL_VOID;
    Event *event = UpMessageEvent::create(cx, w, data);
    if (!event)
        return false;
    if (!w->parent->post(event)) {
        event->destroy(cx);
        JS_ReportOutOfMemory(cx);
        return false;
    }
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return true;
}

JSBool
Worker::jsPostMessageToChild(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *workerobj = JS_THIS_OBJECT(cx, vp);
    if (!workerobj)
        return false;
    Worker *w = (Worker *) JS_GetInstancePrivate(cx, workerobj, &jsWorkerClass, JS_ARGV(cx, vp));
    if (!w) {
        if (!JS_IsExceptionPending(cx))
            JS_ReportError(cx, "Worker was shut down");
        return false;
    }
    
    jsval data = argc > 0 ? JS_ARGV(cx, vp)[0] : JSVAL_VOID;
    Event *event = DownMessageEvent::create(cx, w, data);
    if (!event)
        return false;
    if (!w->post(event)) {
        JS_ReportOutOfMemory(cx);
        return false;
    }
    JS_SET_RVAL(cx, vp, JSVAL_VOID);
    return true;
}

JSBool
Worker::jsTerminate(JSContext *cx, uintN argc, jsval *vp)
{
    JS_SET_RVAL(cx, vp, JSVAL_VOID);

    JSObject *workerobj = JS_THIS_OBJECT(cx, vp);
    if (!workerobj)
        return false;
    Worker *w = (Worker *) JS_GetInstancePrivate(cx, workerobj, &jsWorkerClass, JS_ARGV(cx, vp));
    if (!w)
        return !JS_IsExceptionPending(cx);  

    JSAutoSuspendRequest suspend(cx);
    w->setTerminateFlag();
    return true;
}

void
Event::trace(JSTracer *trc)
{
    if (recipient)
        recipient->trace(trc);
    if (child)
        JS_CALL_OBJECT_TRACER(trc, child->asObject(), "worker");
}

JSClass ThreadPool::jsClass = {
    "ThreadPool", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, jsFinalize,
    NULL, NULL, NULL, NULL,
    NULL, NULL, jsTraceThreadPool, NULL
};

JSClass Worker::jsWorkerClass = {
    "Worker", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, jsFinalize,
    NULL, NULL, NULL, NULL,
    NULL, NULL, jsTraceWorker, NULL
};

JSFunctionSpec Worker::jsMethods[3] = {
    JS_FN("postMessage", Worker::jsPostMessageToChild, 1, 0),
    JS_FN("terminate", Worker::jsTerminate, 0, 0),
    JS_FS_END
};

ThreadPool *
js::workers::init(JSContext *cx, WorkerHooks *hooks, JSObject *global, JSObject **rootp)
{
    return Worker::initWorkers(cx, hooks, global, rootp);
}

void
js::workers::terminateAll(JSRuntime *rt, ThreadPool *tp)
{
    tp->terminateAll(rt);
}

void
js::workers::finish(JSContext *cx, ThreadPool *tp)
{
    if (MainQueue *mq = tp->getMainQueue()) {
        JS_ALWAYS_TRUE(mq->mainThreadWork(cx, true));
        tp->shutdown(cx);
    }
}

#endif 
