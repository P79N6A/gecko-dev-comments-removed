









#include "jsmath.h"

#include "mozilla/Constants.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/MemoryReporting.h"

#include <algorithm>  
#include <fcntl.h>

#ifdef XP_UNIX
# include <unistd.h>
#endif

#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jslibmath.h"
#include "jstypes.h"
#include "prmjtime.h"

#include "jsobjinlines.h"

using namespace js;

using mozilla::Abs;
using mozilla::NumberEqualsInt32;
using mozilla::NumberIsInt32;
using mozilla::ExponentComponent;
using mozilla::FloatingPoint;
using mozilla::IsFinite;
using mozilla::IsInfinite;
using mozilla::IsNaN;
using mozilla::IsNegative;
using mozilla::IsNegativeZero;
using mozilla::PositiveInfinity;
using mozilla::NegativeInfinity;
using JS::ToNumber;
using JS::GenericNaN;

static const JSConstDoubleSpec math_constants[] = {
    {M_E,       "E",            0, {0,0,0}},
    {M_LOG2E,   "LOG2E",        0, {0,0,0}},
    {M_LOG10E,  "LOG10E",       0, {0,0,0}},
    {M_LN2,     "LN2",          0, {0,0,0}},
    {M_LN10,    "LN10",         0, {0,0,0}},
    {M_PI,      "PI",           0, {0,0,0}},
    {M_SQRT2,   "SQRT2",        0, {0,0,0}},
    {M_SQRT1_2, "SQRT1_2",      0, {0,0,0}},
    {0,0,0,{0,0,0}}
};

MathCache::MathCache() {
    memset(table, 0, sizeof(table));

    
    JS_ASSERT(IsNegativeZero(-0.0));
    JS_ASSERT(!IsNegativeZero(+0.0));
    JS_ASSERT(hash(-0.0, MathCache::Sin) != hash(+0.0, MathCache::Sin));
}

size_t
MathCache::sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf)
{
    return mallocSizeOf(this);
}

const Class js::MathClass = {
    js_Math_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_Math),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

bool
js_math_abs(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setNaN();
        return true;
    }

    double x;
    if (!ToNumber(cx, args[0], &x))
        return false;

    double z = Abs(x);
    args.rval().setNumber(z);
    return true;
}

#if defined(SOLARIS) && defined(__GNUC__)
#define ACOS_IF_OUT_OF_RANGE(x) if (x < -1 || 1 < x) return GenericNaN();
#else
#define ACOS_IF_OUT_OF_RANGE(x)
#endif

double
js::math_acos_impl(MathCache *cache, double x)
{
    ACOS_IF_OUT_OF_RANGE(x);
    return cache->lookup(acos, x, MathCache::Acos);
}

double
js::math_acos_uncached(double x)
{
    ACOS_IF_OUT_OF_RANGE(x);
    return acos(x);
}

#undef ACOS_IF_OUT_OF_RANGE

bool
js::math_acos(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setNaN();
        return true;
    }

    double x;
    if (!ToNumber(cx, args[0], &x))
        return false;

    MathCache *mathCache = cx->runtime()->getMathCache(cx);
    if (!mathCache)
        return false;

    double z = math_acos_impl(mathCache, x);
    args.rval().setDouble(z);
    return true;
}

#if defined(SOLARIS) && defined(__GNUC__)
#define ASIN_IF_OUT_OF_RANGE(x) if (x < -1 || 1 < x) return GenericNaN();
#else
#define ASIN_IF_OUT_OF_RANGE(x)
#endif

double
js::math_asin_impl(MathCache *cache, double x)
{
    ASIN_IF_OUT_OF_RANGE(x);
    return cache->lookup(asin, x, MathCache::Asin);
}

double
js::math_asin_uncached(double x)
{
    ASIN_IF_OUT_OF_RANGE(x);
    return asin(x);
}

#undef ASIN_IF_OUT_OF_RANGE

bool
js::math_asin(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setNaN();
        return true;
    }

    double x;
    if (!ToNumber(cx, args[0], &x))
        return false;

    MathCache *mathCache = cx->runtime()->getMathCache(cx);
    if (!mathCache)
        return false;

    double z = math_asin_impl(mathCache, x);
    args.rval().setDouble(z);
    return true;
}

double
js::math_atan_impl(MathCache *cache, double x)
{
    return cache->lookup(atan, x, MathCache::Atan);
}

double
js::math_atan_uncached(double x)
{
    return atan(x);
}

bool
js::math_atan(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setNaN();
        return true;
    }

    double x;
    if (!ToNumber(cx, args[0], &x))
        return false;

    MathCache *mathCache = cx->runtime()->getMathCache(cx);
    if (!mathCache)
        return false;

    double z = math_atan_impl(mathCache, x);
    args.rval().setDouble(z);
    return true;
}

double
js::ecmaAtan2(double y, double x)
{
#if defined(_MSC_VER)
    






    if (IsInfinite(y) && IsInfinite(x)) {
        double z = js_copysign(M_PI / 4, y);
        if (x < 0)
            z *= 3;
        return z;
    }
#endif

#if defined(SOLARIS) && defined(__GNUC__)
    if (y == 0) {
        if (IsNegativeZero(x))
            return js_copysign(M_PI, y);
        if (x == 0)
            return y;
    }
#endif
    return atan2(y, x);
}

bool
js::math_atan2(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    double y;
    if (!ToNumber(cx, args.get(0), &y))
        return false;

    double x;
    if (!ToNumber(cx, args.get(1), &x))
        return false;

    double z = ecmaAtan2(y, x);
    args.rval().setDouble(z);
    return true;
}

double
js::math_ceil_impl(double x)
{
#ifdef __APPLE__
    if (x < 0 && x > -1.0)
        return js_copysign(0, -1);
#endif
    return ceil(x);
}

bool
js::math_ceil(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setNaN();
        return true;
    }

    double x;
    if (!ToNumber(cx, args[0], &x))
        return false;

    double z = math_ceil_impl(x);
    args.rval().setNumber(z);
    return true;
}

bool
js::math_clz32(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setInt32(32);
        return true;
    }

    uint32_t n;
    if (!ToUint32(cx, args[0], &n))
        return false;

    if (n == 0) {
        args.rval().setInt32(32);
        return true;
    }

    args.rval().setInt32(mozilla::CountLeadingZeroes32(n));
    return true;
}

double
js::math_cos_impl(MathCache *cache, double x)
{
    return cache->lookup(cos, x, MathCache::Cos);
}

double
js::math_cos_uncached(double x)
{
    return cos(x);
}

bool
js::math_cos(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setNaN();
        return true;
    }

    double x;
    if (!ToNumber(cx, args[0], &x))
        return false;

    MathCache *mathCache = cx->runtime()->getMathCache(cx);
    if (!mathCache)
        return false;

    double z = math_cos_impl(mathCache, x);
    args.rval().setDouble(z);
    return true;
}

#ifdef _WIN32
#define EXP_IF_OUT_OF_RANGE(x)                  \
    if (!IsNaN(x)) {                            \
        if (x == PositiveInfinity<double>())    \
            return PositiveInfinity<double>();  \
        if (x == NegativeInfinity<double>())    \
            return 0.0;                         \
    }
#else
#define EXP_IF_OUT_OF_RANGE(x)
#endif

double
js::math_exp_impl(MathCache *cache, double x)
{
    EXP_IF_OUT_OF_RANGE(x);
    return cache->lookup(exp, x, MathCache::Exp);
}

double
js::math_exp_uncached(double x)
{
    EXP_IF_OUT_OF_RANGE(x);
    return exp(x);
}

#undef EXP_IF_OUT_OF_RANGE

bool
js::math_exp(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setNaN();
        return true;
    }

    double x;
    if (!ToNumber(cx, args[0], &x))
        return false;

    MathCache *mathCache = cx->runtime()->getMathCache(cx);
    if (!mathCache)
        return false;

    double z = math_exp_impl(mathCache, x);
    args.rval().setNumber(z);
    return true;
}

double
js::math_floor_impl(double x)
{
    return floor(x);
}

bool
js::math_floor(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setNaN();
        return true;
    }

    double x;
    if (!ToNumber(cx, args[0], &x))
        return false;

    double z = math_floor_impl(x);
    args.rval().setNumber(z);
    return true;
}

bool
js::math_imul(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    uint32_t a = 0, b = 0;
    if (args.hasDefined(0) && !ToUint32(cx, args[0], &a))
        return false;
    if (args.hasDefined(1) && !ToUint32(cx, args[1], &b))
        return false;

    uint32_t product = a * b;
    args.rval().setInt32(product > INT32_MAX
                         ? int32_t(INT32_MIN + (product - INT32_MAX - 1))
                         : int32_t(product));
    return true;
}


bool
js::RoundFloat32(JSContext *cx, HandleValue v, float *out)
{
    double d;
    bool success = ToNumber(cx, v, &d);
    *out = static_cast<float>(d);
    return success;
}

bool
js::RoundFloat32(JSContext *cx, HandleValue arg, MutableHandleValue res)
{
    float f;
    if (!RoundFloat32(cx, arg, &f))
        return false;

    res.setDouble(static_cast<double>(f));
    return true;
}

bool
js::math_fround(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setNaN();
        return true;
    }

    float f;
    if (!RoundFloat32(cx, args[0], &f))
        return false;

    args.rval().setDouble(static_cast<double>(f));
    return true;
}

#if defined(SOLARIS) && defined(__GNUC__)
#define LOG_IF_OUT_OF_RANGE(x) if (x < 0) return GenericNaN();
#else
#define LOG_IF_OUT_OF_RANGE(x)
#endif

double
js::math_log_impl(MathCache *cache, double x)
{
    LOG_IF_OUT_OF_RANGE(x);
    return cache->lookup(log, x, MathCache::Log);
}

double
js::math_log_uncached(double x)
{
    LOG_IF_OUT_OF_RANGE(x);
    return log(x);
}

#undef LOG_IF_OUT_OF_RANGE

bool
js::math_log(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setNaN();
        return true;
    }

    double x;
    if (!ToNumber(cx, args[0], &x))
        return false;

    MathCache *mathCache = cx->runtime()->getMathCache(cx);
    if (!mathCache)
        return false;

    double z = math_log_impl(mathCache, x);
    args.rval().setNumber(z);
    return true;
}

bool
js_math_max(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    double maxval = NegativeInfinity<double>();
    for (unsigned i = 0; i < args.length(); i++) {
        double x;
        if (!ToNumber(cx, args[i], &x))
            return false;
        
        if (x > maxval || IsNaN(x) || (x == maxval && IsNegative(maxval)))
            maxval = x;
    }
    args.rval().setNumber(maxval);
    return true;
}

bool
js_math_min(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    double minval = PositiveInfinity<double>();
    for (unsigned i = 0; i < args.length(); i++) {
        double x;
        if (!ToNumber(cx, args[i], &x))
            return false;
        
        if (x < minval || IsNaN(x) || (x == minval && IsNegativeZero(x)))
            minval = x;
    }
    args.rval().setNumber(minval);
    return true;
}


#if defined(_MSC_VER)
# pragma optimize("g", off)
#endif
double
js::powi(double x, int y)
{
    unsigned n = (y < 0) ? -y : y;
    double m = x;
    double p = 1;
    while (true) {
        if ((n & 1) != 0) p *= m;
        n >>= 1;
        if (n == 0) {
            if (y < 0) {
                
                
                
                

                double result = 1.0 / p;
                return (result == 0 && IsInfinite(p))
                       ? pow(x, static_cast<double>(y))  
                       : result;
            }

            return p;
        }
        m *= m;
    }
}
#if defined(_MSC_VER)
# pragma optimize("", on)
#endif


#if defined(_MSC_VER)
# pragma optimize("g", off)
#endif
double
js::ecmaPow(double x, double y)
{
    



    int32_t yi;
    if (NumberEqualsInt32(y, &yi))
        return powi(x, yi);

    



    if (!IsFinite(y) && (x == 1.0 || x == -1.0))
        return GenericNaN();

    
    if (y == 0)
        return 1;

    



    if (IsFinite(x) && x != 0.0) {
        if (y == 0.5)
            return sqrt(x);
        if (y == -0.5)
            return 1.0 / sqrt(x);
    }
    return pow(x, y);
}
#if defined(_MSC_VER)
# pragma optimize("", on)
#endif


#if defined(_MSC_VER)
# pragma optimize("g", off)
#endif
bool
js_math_pow(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    double x;
    if (!ToNumber(cx, args.get(0), &x))
        return false;

    double y;
    if (!ToNumber(cx, args.get(1), &y))
        return false;

    double z = ecmaPow(x, y);
    args.rval().setNumber(z);
    return true;
}
#if defined(_MSC_VER)
# pragma optimize("", on)
#endif

static uint64_t
random_generateSeed()
{
    union {
        uint8_t     u8[8];
        uint32_t    u32[2];
        uint64_t    u64;
    } seed;
    seed.u64 = 0;

#if defined(XP_WIN)
    



    rand_s(&seed.u32[0]);
#elif defined(XP_UNIX)
    



    int fd = open("/dev/urandom", O_RDONLY);
    MOZ_ASSERT(fd >= 0, "Can't open /dev/urandom");
    if (fd >= 0) {
        read(fd, seed.u8, mozilla::ArrayLength(seed.u8));
        close(fd);
    }
    seed.u32[0] ^= fd;
#else
# error "Platform needs to implement random_generateSeed()"
#endif

    seed.u32[1] ^= PRMJ_Now();
    return seed.u64;
}

static const uint64_t RNG_MULTIPLIER = 0x5DEECE66DLL;
static const uint64_t RNG_ADDEND = 0xBLL;
static const uint64_t RNG_MASK = (1LL << 48) - 1;
static const double RNG_DSCALE = double(1LL << 53);




static void
random_initState(uint64_t *rngState)
{
    
    uint64_t seed = random_generateSeed();
    seed ^= (seed >> 16);
    *rngState = (seed ^ RNG_MULTIPLIER) & RNG_MASK;
}

uint64_t
random_next(uint64_t *rngState, int bits)
{
    MOZ_ASSERT((*rngState & 0xffff000000000000ULL) == 0, "Bad rngState");
    MOZ_ASSERT(bits > 0 && bits <= 48, "bits is out of range");

    if (*rngState == 0) {
        random_initState(rngState);
    }

    uint64_t nextstate = *rngState * RNG_MULTIPLIER;
    nextstate += RNG_ADDEND;
    nextstate &= RNG_MASK;
    *rngState = nextstate;
    return nextstate >> (48 - bits);
}

static inline double
random_nextDouble(JSContext *cx)
{
    uint64_t *rng = &cx->compartment()->rngState;
    return double((random_next(rng, 26) << 27) + random_next(rng, 27)) / RNG_DSCALE;
}

double
math_random_no_outparam(JSContext *cx)
{
    
    return random_nextDouble(cx);
}

bool
js_math_random(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    double z = random_nextDouble(cx);
    args.rval().setDouble(z);
    return true;
}

double
js::math_round_impl(double x)
{
    int32_t ignored;
    if (NumberIsInt32(x, &ignored))
        return x;

    
    if (ExponentComponent(x) >= int_fast16_t(FloatingPoint<double>::kExponentShift))
        return x;

    return js_copysign(floor(x + 0.5), x);
}

float
js::math_roundf_impl(float x)
{
    int32_t ignored;
    if (NumberIsInt32(x, &ignored))
        return x;

    
    if (ExponentComponent(x) >= int_fast16_t(FloatingPoint<float>::kExponentShift))
        return x;

    return js_copysign(floorf(x + 0.5f), x);
}

bool 
js::math_round(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setNaN();
        return true;
    }

    double x;
    if (!ToNumber(cx, args[0], &x))
        return false;

    double z = math_round_impl(x);
    args.rval().setNumber(z);
    return true;
}

double
js::math_sin_impl(MathCache *cache, double x)
{
    return cache->lookup(sin, x, MathCache::Sin);
}

double
js::math_sin_uncached(double x)
{
    return sin(x);
}

bool
js::math_sin(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setNaN();
        return true;
    }

    double x;
    if (!ToNumber(cx, args[0], &x))
        return false;

    MathCache *mathCache = cx->runtime()->getMathCache(cx);
    if (!mathCache)
        return false;

    double z = math_sin_impl(mathCache, x);
    args.rval().setDouble(z);
    return true;
}

bool
js_math_sqrt(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setNaN();
        return true;
    }

    double x;
    if (!ToNumber(cx, args[0], &x))
        return false;

    MathCache *mathCache = cx->runtime()->getMathCache(cx);
    if (!mathCache)
        return false;

    double z = mathCache->lookup(sqrt, x, MathCache::Sqrt);
    args.rval().setDouble(z);
    return true;
}

double
js::math_tan_impl(MathCache *cache, double x)
{
    return cache->lookup(tan, x, MathCache::Tan);
}

double
js::math_tan_uncached(double x)
{
    return tan(x);
}

bool
js::math_tan(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setNaN();
        return true;
    }

    double x;
    if (!ToNumber(cx, args[0], &x))
        return false;

    MathCache *mathCache = cx->runtime()->getMathCache(cx);
    if (!mathCache)
        return false;

    double z = math_tan_impl(mathCache, x);
    args.rval().setDouble(z);
    return true;
}


typedef double (*UnaryMathFunctionType)(MathCache *cache, double);

template <UnaryMathFunctionType F>
static bool math_function(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() == 0) {
        args.rval().setNumber(GenericNaN());
        return true;
    }

    double x;
    if (!ToNumber(cx, args[0], &x))
        return false;

    MathCache *mathCache = cx->runtime()->getMathCache(cx);
    if (!mathCache)
        return false;
    double z = F(mathCache, x);
    args.rval().setNumber(z);

    return true;
}



double
js::math_log10_impl(MathCache *cache, double x)
{
    return cache->lookup(log10, x, MathCache::Log10);
}

double
js::math_log10_uncached(double x)
{
    return log10(x);
}

bool
js::math_log10(JSContext *cx, unsigned argc, Value *vp)
{
    return math_function<math_log10_impl>(cx, argc, vp);
}

#if !HAVE_LOG2
double log2(double x)
{
    return log(x) / M_LN2;
}
#endif

double
js::math_log2_impl(MathCache *cache, double x)
{
    return cache->lookup(log2, x, MathCache::Log2);
}

double
js::math_log2_uncached(double x)
{
    return log2(x);
}

bool
js::math_log2(JSContext *cx, unsigned argc, Value *vp)
{
    return math_function<math_log2_impl>(cx, argc, vp);
}

#if !HAVE_LOG1P
double log1p(double x)
{
    if (fabs(x) < 1e-4) {
        



        double z = -(x * x * x * x) / 4 + (x * x * x) / 3 - (x * x) / 2 + x;
        return z;
    } else {
        
        return log(1.0 + x);
    }
}
#endif

#ifdef __APPLE__

#define LOG1P_IF_OUT_OF_RANGE(x) if (x == 0) return x;
#else
#define LOG1P_IF_OUT_OF_RANGE(x)
#endif

double
js::math_log1p_impl(MathCache *cache, double x)
{
    LOG1P_IF_OUT_OF_RANGE(x);
    return cache->lookup(log1p, x, MathCache::Log1p);
}

double
js::math_log1p_uncached(double x)
{
    LOG1P_IF_OUT_OF_RANGE(x);
    return log1p(x);
}

#undef LOG1P_IF_OUT_OF_RANGE

bool
js::math_log1p(JSContext *cx, unsigned argc, Value *vp)
{
    return math_function<math_log1p_impl>(cx, argc, vp);
}

#if !HAVE_EXPM1
double expm1(double x)
{
    
    if (x == 0.0)
        return x;

    if (fabs(x) < 1e-5) {
        



        double z = (x * x * x) / 6 + (x * x) / 2 + x;
        return z;
    } else {
        
        return exp(x) - 1.0;
    }
}
#endif

double
js::math_expm1_impl(MathCache *cache, double x)
{
    return cache->lookup(expm1, x, MathCache::Expm1);
}

double
js::math_expm1_uncached(double x)
{
    return expm1(x);
}

bool
js::math_expm1(JSContext *cx, unsigned argc, Value *vp)
{
    return math_function<math_expm1_impl>(cx, argc, vp);
}

#if !HAVE_SQRT1PM1

double sqrt1pm1(double x)
{
    if (fabs(x) > 0.75)
        return sqrt(1 + x) - 1;

    return expm1(log1p(x) / 2);
}
#endif


double
js::math_cosh_impl(MathCache *cache, double x)
{
    return cache->lookup(cosh, x, MathCache::Cosh);
}

double
js::math_cosh_uncached(double x)
{
    return cosh(x);
}

bool
js::math_cosh(JSContext *cx, unsigned argc, Value *vp)
{
    return math_function<math_cosh_impl>(cx, argc, vp);
}

double
js::math_sinh_impl(MathCache *cache, double x)
{
    return cache->lookup(sinh, x, MathCache::Sinh);
}

double
js::math_sinh_uncached(double x)
{
    return sinh(x);
}

bool
js::math_sinh(JSContext *cx, unsigned argc, Value *vp)
{
    return math_function<math_sinh_impl>(cx, argc, vp);
}

double
js::math_tanh_impl(MathCache *cache, double x)
{
    return cache->lookup(tanh, x, MathCache::Tanh);
}

double
js::math_tanh_uncached(double x)
{
    return tanh(x);
}

bool
js::math_tanh(JSContext *cx, unsigned argc, Value *vp)
{
    return math_function<math_tanh_impl>(cx, argc, vp);
}

#if !HAVE_ACOSH
double acosh(double x)
{
    const double SQUARE_ROOT_EPSILON = sqrt(std::numeric_limits<double>::epsilon());

    if ((x - 1) >= SQUARE_ROOT_EPSILON) {
        if (x > 1 / SQUARE_ROOT_EPSILON) {
            



            return log(x) + M_LN2;
        } else if (x < 1.5) {
            
            
            double y = x - 1;
            return log1p(y + sqrt(y * y + 2 * y));
        } else {
            
            return log(x + sqrt(x * x - 1));
        }
    } else {
        
        double y = x - 1;
        
        
        return sqrt(2 * y) * (1 - y / 12 + 3 * y * y / 160);
    }
}
#endif

double
js::math_acosh_impl(MathCache *cache, double x)
{
    return cache->lookup(acosh, x, MathCache::Acosh);
}

double
js::math_acosh_uncached(double x)
{
    return acosh(x);
}

bool
js::math_acosh(JSContext *cx, unsigned argc, Value *vp)
{
    return math_function<math_acosh_impl>(cx, argc, vp);
}

#if !HAVE_ASINH


static double my_asinh(double x)
{
    const double SQUARE_ROOT_EPSILON = sqrt(std::numeric_limits<double>::epsilon());
    const double FOURTH_ROOT_EPSILON = sqrt(SQUARE_ROOT_EPSILON);

    if (x >= FOURTH_ROOT_EPSILON) {
        if (x > 1 / SQUARE_ROOT_EPSILON)
            
            
            return M_LN2 + log(x) + 1 / (4 * x * x);
        else if (x < 0.5)
            return log1p(x + sqrt1pm1(x * x));
        else
            return log(x + sqrt(x * x + 1));
    } else if (x <= -FOURTH_ROOT_EPSILON) {
        return -my_asinh(-x);
    } else {
        
        
        double result = x;

        if (fabs(x) >= SQUARE_ROOT_EPSILON) {
            double x3 = x * x * x;
            
            result -= x3 / 6;
        }

        return result;
    }
}
#endif

double
js::math_asinh_impl(MathCache *cache, double x)
{
#ifdef HAVE_ASINH
    return cache->lookup(asinh, x, MathCache::Asinh);
#else
    return cache->lookup(my_asinh, x, MathCache::Asinh);
#endif
}

double
js::math_asinh_uncached(double x)
{
#ifdef HAVE_ASINH
    return asinh(x);
#else
    return my_asinh(x);
#endif
}

bool
js::math_asinh(JSContext *cx, unsigned argc, Value *vp)
{
    return math_function<math_asinh_impl>(cx, argc, vp);
}

#if !HAVE_ATANH
double atanh(double x)
{
    const double EPSILON = std::numeric_limits<double>::epsilon();
    const double SQUARE_ROOT_EPSILON = sqrt(EPSILON);
    const double FOURTH_ROOT_EPSILON = sqrt(SQUARE_ROOT_EPSILON);

    if (fabs(x) >= FOURTH_ROOT_EPSILON) {
        
        if (fabs(x) < 0.5)
            return (log1p(x) - log1p(-x)) / 2;

        return log((1 + x) / (1 - x)) / 2;
    } else {
        
        
        double result = x;

        if (fabs(x) >= SQUARE_ROOT_EPSILON) {
            double x3 = x * x * x;
            result += x3 / 3;
        }

        return result;
    }
}
#endif

double
js::math_atanh_impl(MathCache *cache, double x)
{
    return cache->lookup(atanh, x, MathCache::Atanh);
}

double
js::math_atanh_uncached(double x)
{
    return atanh(x);
}

bool
js::math_atanh(JSContext *cx, unsigned argc, Value *vp)
{
    return math_function<math_atanh_impl>(cx, argc, vp);
}


double
js::ecmaHypot(double x, double y)
{
#ifdef XP_WIN
    



    if (mozilla::IsInfinite(x) || mozilla::IsInfinite(y)) {
        return mozilla::PositiveInfinity<double>();
    }
#endif
    return hypot(x, y);
}

bool
js::math_hypot(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    
    if (args.length() == 2) {
        double x, y;
        if (!ToNumber(cx, args[0], &x))
            return false;
        if (!ToNumber(cx, args[1], &y))
            return false;

        double result = ecmaHypot(x, y);
        args.rval().setNumber(result);
        return true;
    }

    bool isInfinite = false;
    bool isNaN = false;

    double scale = 0;
    double sumsq = 1;

    for (unsigned i = 0; i < args.length(); i++) {
        double x;
        if (!ToNumber(cx, args[i], &x))
            return false;

        isInfinite |= mozilla::IsInfinite(x);
        isNaN |= mozilla::IsNaN(x);

        double xabs = mozilla::Abs(x);

        if (scale < xabs) {
            sumsq = 1 + sumsq * (scale / xabs) * (scale / xabs);
            scale = xabs;
        } else if (scale != 0) {
            sumsq += (xabs / scale) * (xabs / scale);
        }
    }

    double result = isInfinite ? PositiveInfinity<double>() :
                    isNaN ? GenericNaN() :
                    scale * sqrt(sumsq);
    args.rval().setNumber(result);
    return true;
}

#if !HAVE_TRUNC
double trunc(double x)
{
    return x > 0 ? floor(x) : ceil(x);
}
#endif

double
js::math_trunc_impl(MathCache *cache, double x)
{
    return cache->lookup(trunc, x, MathCache::Trunc);
}

double
js::math_trunc_uncached(double x)
{
    return trunc(x);
}

bool
js::math_trunc(JSContext *cx, unsigned argc, Value *vp)
{
    return math_function<math_trunc_impl>(cx, argc, vp);
}

static double sign(double x)
{
    if (mozilla::IsNaN(x))
        return GenericNaN();

    return x == 0 ? x : x < 0 ? -1 : 1;
}

double
js::math_sign_impl(MathCache *cache, double x)
{
    return cache->lookup(sign, x, MathCache::Sign);
}

double
js::math_sign_uncached(double x)
{
    return sign(x);
}

bool
js::math_sign(JSContext *cx, unsigned argc, Value *vp)
{
    return math_function<math_sign_impl>(cx, argc, vp);
}

#if !HAVE_CBRT
double cbrt(double x)
{
    if (x > 0) {
        return pow(x, 1.0 / 3.0);
    } else if (x == 0) {
        return x;
    } else {
        return -pow(-x, 1.0 / 3.0);
    }
}
#endif

double
js::math_cbrt_impl(MathCache *cache, double x)
{
    return cache->lookup(cbrt, x, MathCache::Cbrt);
}

double
js::math_cbrt_uncached(double x)
{
    return cbrt(x);
}

bool
js::math_cbrt(JSContext *cx, unsigned argc, Value *vp)
{
    return math_function<math_cbrt_impl>(cx, argc, vp);
}

#if JS_HAS_TOSOURCE
static bool
math_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setString(cx->names().Math);
    return true;
}
#endif

static const JSFunctionSpec math_static_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,  math_toSource,        0, 0),
#endif
    JS_FN("abs",            js_math_abs,          1, 0),
    JS_FN("acos",           math_acos,            1, 0),
    JS_FN("asin",           math_asin,            1, 0),
    JS_FN("atan",           math_atan,            1, 0),
    JS_FN("atan2",          math_atan2,           2, 0),
    JS_FN("ceil",           math_ceil,            1, 0),
    JS_FN("clz32",          math_clz32,           1, 0),
    JS_FN("cos",            math_cos,             1, 0),
    JS_FN("exp",            math_exp,             1, 0),
    JS_FN("floor",          math_floor,           1, 0),
    JS_FN("imul",           math_imul,            2, 0),
    JS_FN("fround",         math_fround,          1, 0),
    JS_FN("log",            math_log,             1, 0),
    JS_FN("max",            js_math_max,          2, 0),
    JS_FN("min",            js_math_min,          2, 0),
    JS_FN("pow",            js_math_pow,          2, 0),
    JS_FN("random",         js_math_random,       0, 0),
    JS_FN("round",          math_round,           1, 0),
    JS_FN("sin",            math_sin,             1, 0),
    JS_FN("sqrt",           js_math_sqrt,         1, 0),
    JS_FN("tan",            math_tan,             1, 0),
    JS_FN("log10",          math_log10,           1, 0),
    JS_FN("log2",           math_log2,            1, 0),
    JS_FN("log1p",          math_log1p,           1, 0),
    JS_FN("expm1",          math_expm1,           1, 0),
    JS_FN("cosh",           math_cosh,            1, 0),
    JS_FN("sinh",           math_sinh,            1, 0),
    JS_FN("tanh",           math_tanh,            1, 0),
    JS_FN("acosh",          math_acosh,           1, 0),
    JS_FN("asinh",          math_asinh,           1, 0),
    JS_FN("atanh",          math_atanh,           1, 0),
    JS_FN("hypot",          math_hypot,           2, 0),
    JS_FN("trunc",          math_trunc,           1, 0),
    JS_FN("sign",           math_sign,            1, 0),
    JS_FN("cbrt",           math_cbrt,            1, 0),
    JS_FS_END
};

JSObject *
js_InitMathClass(JSContext *cx, HandleObject obj)
{
    RootedObject proto(cx, obj->as<GlobalObject>().getOrCreateObjectPrototype(cx));
    if (!proto)
        return nullptr;
    RootedObject Math(cx, NewObjectWithGivenProto(cx, &MathClass, proto, obj, SingletonObject));
    if (!Math)
        return nullptr;

    if (!JS_DefineProperty(cx, obj, js_Math_str, Math, 0,
                           JS_PropertyStub, JS_StrictPropertyStub))
    {
        return nullptr;
    }

    if (!JS_DefineFunctions(cx, Math, math_static_methods))
        return nullptr;
    if (!JS_DefineConstDoubles(cx, Math, math_constants))
        return nullptr;

    obj->as<GlobalObject>().setConstructor(JSProto_Math, ObjectValue(*Math));

    return Math;
}
