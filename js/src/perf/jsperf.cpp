





































#include "jsperf.h"
#include "jscntxt.h" 
#include "jsobj.h" 

#include "jsobjinlines.h"

using JS::PerfMeasurement;



static PerfMeasurement* GetPM(JSContext* cx, JSObject* obj, const char* fname);
static PerfMeasurement* GetPMFromThis(JSContext* cx, jsval* vp);



#define GETTER(name)                                                    \
    static JSBool                                                       \
    pm_get_##name(JSContext* cx, JS::HandleObject obj, JS::HandleId /*unused*/, jsval* vp) \
    {                                                                   \
        PerfMeasurement* p = GetPM(cx, obj, #name);                     \
        if (!p)                                                         \
            return JS_FALSE;                                            \
        return JS_NewNumberValue(cx, double(p->name), vp);              \
    }

GETTER(cpu_cycles)
GETTER(instructions)
GETTER(cache_references)
GETTER(cache_misses)
GETTER(branch_instructions)
GETTER(branch_misses)
GETTER(bus_cycles)
GETTER(page_faults)
GETTER(major_page_faults)
GETTER(context_switches)
GETTER(cpu_migrations)
GETTER(eventsMeasured)

#undef GETTER



static JSBool
pm_start(JSContext* cx, unsigned , jsval* vp)
{
    PerfMeasurement* p = GetPMFromThis(cx, vp);
    if (!p)
        return JS_FALSE;

    p->start();
    return JS_TRUE;
}

static JSBool
pm_stop(JSContext* cx, unsigned , jsval* vp)
{
    PerfMeasurement* p = GetPMFromThis(cx, vp);
    if (!p)
        return JS_FALSE;

    p->stop();
    return JS_TRUE;
}

static JSBool
pm_reset(JSContext* cx, unsigned , jsval* vp)
{
    PerfMeasurement* p = GetPMFromThis(cx, vp);
    if (!p)
        return JS_FALSE;

    p->reset();
    return JS_TRUE;
}

static JSBool
pm_canMeasureSomething(JSContext* cx, unsigned , jsval* vp)
{
    PerfMeasurement* p = GetPMFromThis(cx, vp);
    if (!p)
        return JS_FALSE;

    JS_SET_RVAL(cx, vp, BOOLEAN_TO_JSVAL(p->canMeasureSomething()));
    return JS_TRUE;
}

const uint8_t PM_FATTRS = JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED;
static JSFunctionSpec pm_fns[] = {
    JS_FN("start",               pm_start,               0, PM_FATTRS),
    JS_FN("stop",                pm_stop,                0, PM_FATTRS),
    JS_FN("reset",               pm_reset,               0, PM_FATTRS),
    JS_FN("canMeasureSomething", pm_canMeasureSomething, 0, PM_FATTRS),
    JS_FS_END
};

const uint8_t PM_PATTRS =
    JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED;

#define GETTER(name)                            \
    { #name, 0, PM_PATTRS, pm_get_##name, 0 }

static JSPropertySpec pm_props[] = {
    GETTER(cpu_cycles),
    GETTER(instructions),
    GETTER(cache_references),
    GETTER(cache_misses),
    GETTER(branch_instructions),
    GETTER(branch_misses),
    GETTER(bus_cycles),
    GETTER(page_faults),
    GETTER(major_page_faults),
    GETTER(context_switches),
    GETTER(cpu_migrations),
    GETTER(eventsMeasured),
    {0,0,0,0,0}
};

#undef GETTER



const uint8_t PM_CATTRS = JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT;

#define CONSTANT(name) { #name, PerfMeasurement::name }

static const struct pm_const {
    const char *name;
    PerfMeasurement::EventMask value;
} pm_consts[] = {
    CONSTANT(CPU_CYCLES),
    CONSTANT(INSTRUCTIONS),
    CONSTANT(CACHE_REFERENCES),
    CONSTANT(CACHE_MISSES),
    CONSTANT(BRANCH_INSTRUCTIONS),
    CONSTANT(BRANCH_MISSES),
    CONSTANT(BUS_CYCLES),
    CONSTANT(PAGE_FAULTS),
    CONSTANT(MAJOR_PAGE_FAULTS),
    CONSTANT(CONTEXT_SWITCHES),
    CONSTANT(CPU_MIGRATIONS),
    CONSTANT(ALL),
    CONSTANT(NUM_MEASURABLE_EVENTS),
    { 0, PerfMeasurement::EventMask(0) }
};

#undef CONSTANT

static JSBool pm_construct(JSContext* cx, unsigned argc, jsval* vp);
static void pm_finalize(JSFreeOp* fop, JSObject* obj);

static JSClass pm_class = {
    "PerfMeasurement", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, pm_finalize
};



static JSBool
pm_construct(JSContext* cx, unsigned argc, jsval* vp)
{
    uint32_t mask;
    if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx, vp), "u", &mask))
        return JS_FALSE;

    js::RootedVarObject obj(cx, JS_NewObjectForConstructor(cx, &pm_class, vp));
    if (!obj)
        return JS_FALSE;

    if (!JS_FreezeObject(cx, obj))
        return JS_FALSE;

    PerfMeasurement* p = cx->new_<PerfMeasurement>(PerfMeasurement::EventMask(mask));
    if (!p) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    JS_SetPrivate(obj, p);
    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

static void
pm_finalize(JSFreeOp* fop, JSObject* obj)
{
    js::FreeOp::get(fop)->delete_(static_cast<PerfMeasurement*>(JS_GetPrivate(obj)));
}



static PerfMeasurement*
GetPM(JSContext* cx, JSObject* obj, const char* fname)
{
    PerfMeasurement* p = (PerfMeasurement*)
        JS_GetInstancePrivate(cx, obj, &pm_class, 0);
    if (p)
        return p;

    
    
    JS_ReportErrorNumber(cx, js_GetErrorMessage, 0, JSMSG_INCOMPATIBLE_PROTO,
                         pm_class.name, fname, JS_GetClass(obj)->name);
    return 0;
}

static PerfMeasurement*
GetPMFromThis(JSContext* cx, jsval* vp)
{
    JSObject* this_ = JS_THIS_OBJECT(cx, vp);
    if (!this_)
        return 0;
    return (PerfMeasurement*)
        JS_GetInstancePrivate(cx, this_, &pm_class, JS_ARGV(cx, vp));
}

namespace JS {

JSObject*
RegisterPerfMeasurement(JSContext *cx, JSObject *global)
{
    js::RootedVarObject prototype(cx);
    prototype = JS_InitClass(cx, global, 0 ,
                             &pm_class, pm_construct, 1,
                             pm_props, pm_fns, 0, 0);
    if (!prototype)
        return 0;

    js::RootedVarObject ctor(cx);
    ctor = JS_GetConstructor(cx, prototype);
    if (!ctor)
        return 0;

    for (const pm_const *c = pm_consts; c->name; c++) {
        if (!JS_DefineProperty(cx, ctor, c->name, INT_TO_JSVAL(c->value),
                               JS_PropertyStub, JS_StrictPropertyStub, PM_CATTRS))
            return 0;
    }

    if (!JS_FreezeObject(cx, prototype) ||
        !JS_FreezeObject(cx, ctor)) {
        return 0;
    }

    return prototype;
}

PerfMeasurement*
ExtractPerfMeasurement(jsval wrapper)
{
    if (JSVAL_IS_PRIMITIVE(wrapper))
        return 0;

    
    
    JSObject *obj = JSVAL_TO_OBJECT(wrapper);
    if (obj->getClass() != js::Valueify(&pm_class))
        return 0;

    return (PerfMeasurement*) obj->getPrivate();
}

} 
