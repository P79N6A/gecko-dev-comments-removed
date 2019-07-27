




#include "perf/jsperf.h"

#include "jscntxt.h" 
#include "jsobj.h" 

using namespace js;
using JS::PerfMeasurement;

using mozilla::UniquePtr;



static PerfMeasurement* GetPM(JSContext* cx, JS::HandleValue value, const char* fname);



#define GETTER(name)                                                    \
    static bool                                                         \
    pm_get_##name(JSContext* cx, unsigned argc, Value* vp)              \
    {                                                                   \
        CallArgs args = CallArgsFromVp(argc, vp);                       \
        PerfMeasurement* p = GetPM(cx, args.thisv(), #name);            \
        if (!p)                                                         \
            return false;                                               \
        args.rval().setNumber(double(p->name));                         \
        return true;                                                    \
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



static bool
pm_start(JSContext* cx, unsigned argc, jsval* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    PerfMeasurement* p = GetPM(cx, args.thisv(), "start");
    if (!p)
        return false;

    p->start();
    args.rval().setUndefined();
    return true;
}

static bool
pm_stop(JSContext* cx, unsigned argc, jsval* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    PerfMeasurement* p = GetPM(cx, args.thisv(), "stop");
    if (!p)
        return false;

    p->stop();
    args.rval().setUndefined();
    return true;
}

static bool
pm_reset(JSContext* cx, unsigned argc, jsval* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    PerfMeasurement* p = GetPM(cx, args.thisv(), "reset");
    if (!p)
        return false;

    p->reset();
    args.rval().setUndefined();
    return true;
}

static bool
pm_canMeasureSomething(JSContext* cx, unsigned argc, jsval* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    PerfMeasurement* p = GetPM(cx, args.thisv(), "canMeasureSomething");
    if (!p)
        return false;

    args.rval().setBoolean(p->canMeasureSomething());
    return true;
}

static const uint8_t PM_FATTRS = JSPROP_READONLY | JSPROP_PERMANENT;
static const JSFunctionSpec pm_fns[] = {
    JS_FN("start",               pm_start,               0, PM_FATTRS),
    JS_FN("stop",                pm_stop,                0, PM_FATTRS),
    JS_FN("reset",               pm_reset,               0, PM_FATTRS),
    JS_FN("canMeasureSomething", pm_canMeasureSomething, 0, PM_FATTRS),
    JS_FS_END
};

static const uint8_t PM_PATTRS =
    JSPROP_ENUMERATE | JSPROP_PERMANENT;

#define GETTER(name)                            \
    JS_PSG(#name, pm_get_##name, PM_PATTRS)

static const JSPropertySpec pm_props[] = {
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
    JS_PS_END
};

#undef GETTER



#define CONSTANT(name) { #name, PerfMeasurement::name }

static const struct pm_const {
    const char* name;
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

static bool pm_construct(JSContext* cx, unsigned argc, jsval* vp);
static void pm_finalize(JSFreeOp* fop, JSObject* obj);

static const JSClass pm_class = {
    "PerfMeasurement", JSCLASS_HAS_PRIVATE,
    nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, pm_finalize
};



static bool
pm_construct(JSContext* cx, unsigned argc, jsval* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    uint32_t mask;
    if (!args.hasDefined(0)) {
        ReportMissingArg(cx, args.calleev(), 0);
        return false;
    }
    if (!JS::ToUint32(cx, args[0], &mask))
        return false;

    JS::RootedObject obj(cx, JS_NewObjectForConstructor(cx, &pm_class, args));
    if (!obj)
        return false;

    if (!JS_FreezeObject(cx, obj))
        return false;

    PerfMeasurement* p = cx->new_<PerfMeasurement>(PerfMeasurement::EventMask(mask));
    if (!p) {
        JS_ReportOutOfMemory(cx);
        return false;
    }

    JS_SetPrivate(obj, p);
    args.rval().setObject(*obj);
    return true;
}

static void
pm_finalize(JSFreeOp* fop, JSObject* obj)
{
    js::FreeOp::get(fop)->delete_(static_cast<PerfMeasurement*>(JS_GetPrivate(obj)));
}



static PerfMeasurement*
GetPM(JSContext* cx, JS::HandleValue value, const char* fname)
{
    if (!value.isObject()) {
        UniquePtr<char[], JS::FreePolicy> bytes =
            DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, value, NullPtr());
        if (!bytes)
            return nullptr;
        JS_ReportErrorNumber(cx, GetErrorMessage, 0, JSMSG_NOT_NONNULL_OBJECT, bytes.get());
        return nullptr;
    }
    RootedObject obj(cx, &value.toObject());
    PerfMeasurement* p = (PerfMeasurement*)
        JS_GetInstancePrivate(cx, obj, &pm_class, nullptr);
    if (p)
        return p;

    
    
    JS_ReportErrorNumber(cx, GetErrorMessage, 0, JSMSG_INCOMPATIBLE_PROTO,
                         pm_class.name, fname, JS_GetClass(obj)->name);
    return nullptr;
}

namespace JS {

JSObject*
RegisterPerfMeasurement(JSContext* cx, HandleObject globalArg)
{
    static const uint8_t PM_CATTRS = JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT;

    RootedObject global(cx, globalArg);
    RootedObject prototype(cx);
    prototype = JS_InitClass(cx, global, js::NullPtr() ,
                             &pm_class, pm_construct, 1,
                             pm_props, pm_fns, 0, 0);
    if (!prototype)
        return 0;

    RootedObject ctor(cx);
    ctor = JS_GetConstructor(cx, prototype);
    if (!ctor)
        return 0;

    for (const pm_const* c = pm_consts; c->name; c++) {
        if (!JS_DefineProperty(cx, ctor, c->name, c->value, PM_CATTRS,
                               JS_STUBGETTER, JS_STUBSETTER))
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
    if (wrapper.isPrimitive())
        return 0;

    
    
    JSObject* obj = wrapper.toObjectOrNull();
    if (obj->getClass() != js::Valueify(&pm_class))
        return 0;

    return (PerfMeasurement*) obj->as<js::NativeObject>().getPrivate();
}

} 
