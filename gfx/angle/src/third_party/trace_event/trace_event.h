












































































































































#ifndef COMMON_TRACE_EVENT_H_
#define COMMON_TRACE_EVENT_H_

#include <string>

#include "common/event_tracer.h"



#define TRACE_STR_COPY(str) \
    WebCore::TraceEvent::TraceStringWithCopy(str)






#define TRACE_EVENT0(category, name) \
    INTERNAL_TRACE_EVENT_ADD_SCOPED(category, name)
#define TRACE_EVENT1(category, name, arg1_name, arg1_val) \
    INTERNAL_TRACE_EVENT_ADD_SCOPED(category, name, arg1_name, arg1_val)
#define TRACE_EVENT2(category, name, arg1_name, arg1_val, arg2_name, arg2_val) \
    INTERNAL_TRACE_EVENT_ADD_SCOPED(category, name, arg1_name, arg1_val, \
        arg2_name, arg2_val)






#define TRACE_EVENT_INSTANT0(category, name) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_INSTANT, \
        category, name, TRACE_EVENT_FLAG_NONE)
#define TRACE_EVENT_INSTANT1(category, name, arg1_name, arg1_val) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_INSTANT, \
        category, name, TRACE_EVENT_FLAG_NONE, arg1_name, arg1_val)
#define TRACE_EVENT_INSTANT2(category, name, arg1_name, arg1_val, \
        arg2_name, arg2_val) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_INSTANT, \
        category, name, TRACE_EVENT_FLAG_NONE, arg1_name, arg1_val, \
        arg2_name, arg2_val)
#define TRACE_EVENT_COPY_INSTANT0(category, name) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_INSTANT, \
        category, name, TRACE_EVENT_FLAG_COPY)
#define TRACE_EVENT_COPY_INSTANT1(category, name, arg1_name, arg1_val) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_INSTANT, \
        category, name, TRACE_EVENT_FLAG_COPY, arg1_name, arg1_val)
#define TRACE_EVENT_COPY_INSTANT2(category, name, arg1_name, arg1_val, \
        arg2_name, arg2_val) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_INSTANT, \
        category, name, TRACE_EVENT_FLAG_COPY, arg1_name, arg1_val, \
        arg2_name, arg2_val)






#define TRACE_EVENT_BEGIN0(category, name) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_BEGIN, \
        category, name, TRACE_EVENT_FLAG_NONE)
#define TRACE_EVENT_BEGIN1(category, name, arg1_name, arg1_val) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_BEGIN, \
        category, name, TRACE_EVENT_FLAG_NONE, arg1_name, arg1_val)
#define TRACE_EVENT_BEGIN2(category, name, arg1_name, arg1_val, \
        arg2_name, arg2_val) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_BEGIN, \
        category, name, TRACE_EVENT_FLAG_NONE, arg1_name, arg1_val, \
        arg2_name, arg2_val)
#define TRACE_EVENT_COPY_BEGIN0(category, name) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_BEGIN, \
        category, name, TRACE_EVENT_FLAG_COPY)
#define TRACE_EVENT_COPY_BEGIN1(category, name, arg1_name, arg1_val) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_BEGIN, \
        category, name, TRACE_EVENT_FLAG_COPY, arg1_name, arg1_val)
#define TRACE_EVENT_COPY_BEGIN2(category, name, arg1_name, arg1_val, \
        arg2_name, arg2_val) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_BEGIN, \
        category, name, TRACE_EVENT_FLAG_COPY, arg1_name, arg1_val, \
        arg2_name, arg2_val)





#define TRACE_EVENT_END0(category, name) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_END, \
        category, name, TRACE_EVENT_FLAG_NONE)
#define TRACE_EVENT_END1(category, name, arg1_name, arg1_val) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_END, \
        category, name, TRACE_EVENT_FLAG_NONE, arg1_name, arg1_val)
#define TRACE_EVENT_END2(category, name, arg1_name, arg1_val, \
        arg2_name, arg2_val) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_END, \
        category, name, TRACE_EVENT_FLAG_NONE, arg1_name, arg1_val, \
        arg2_name, arg2_val)
#define TRACE_EVENT_COPY_END0(category, name) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_END, \
        category, name, TRACE_EVENT_FLAG_COPY)
#define TRACE_EVENT_COPY_END1(category, name, arg1_name, arg1_val) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_END, \
        category, name, TRACE_EVENT_FLAG_COPY, arg1_name, arg1_val)
#define TRACE_EVENT_COPY_END2(category, name, arg1_name, arg1_val, \
        arg2_name, arg2_val) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_END, \
        category, name, TRACE_EVENT_FLAG_COPY, arg1_name, arg1_val, \
        arg2_name, arg2_val)





#define TRACE_COUNTER1(category, name, value) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_COUNTER, \
        category, name, TRACE_EVENT_FLAG_NONE, \
        "value", static_cast<int>(value))
#define TRACE_COPY_COUNTER1(category, name, value) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_COUNTER, \
        category, name, TRACE_EVENT_FLAG_COPY, \
        "value", static_cast<int>(value))






#define TRACE_COUNTER2(category, name, value1_name, value1_val, \
        value2_name, value2_val) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_COUNTER, \
        category, name, TRACE_EVENT_FLAG_NONE, \
        value1_name, static_cast<int>(value1_val), \
        value2_name, static_cast<int>(value2_val))
#define TRACE_COPY_COUNTER2(category, name, value1_name, value1_val, \
        value2_name, value2_val) \
    INTERNAL_TRACE_EVENT_ADD(TRACE_EVENT_PHASE_COUNTER, \
        category, name, TRACE_EVENT_FLAG_COPY, \
        value1_name, static_cast<int>(value1_val), \
        value2_name, static_cast<int>(value2_val))









#define TRACE_COUNTER_ID1(category, name, id, value) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_COUNTER, \
        category, name, id, TRACE_EVENT_FLAG_NONE, \
        "value", static_cast<int>(value))
#define TRACE_COPY_COUNTER_ID1(category, name, id, value) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_COUNTER, \
        category, name, id, TRACE_EVENT_FLAG_COPY, \
        "value", static_cast<int>(value))










#define TRACE_COUNTER_ID2(category, name, id, value1_name, value1_val, \
        value2_name, value2_val) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_COUNTER, \
        category, name, id, TRACE_EVENT_FLAG_NONE, \
        value1_name, static_cast<int>(value1_val), \
        value2_name, static_cast<int>(value2_val))
#define TRACE_COPY_COUNTER_ID2(category, name, id, value1_name, value1_val, \
        value2_name, value2_val) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_COUNTER, \
        category, name, id, TRACE_EVENT_FLAG_COPY, \
        value1_name, static_cast<int>(value1_val), \
        value2_name, static_cast<int>(value2_val))

















#define TRACE_EVENT_ASYNC_BEGIN0(category, name, id) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_BEGIN, \
        category, name, id, TRACE_EVENT_FLAG_NONE)
#define TRACE_EVENT_ASYNC_BEGIN1(category, name, id, arg1_name, arg1_val) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_BEGIN, \
        category, name, id, TRACE_EVENT_FLAG_NONE, arg1_name, arg1_val)
#define TRACE_EVENT_ASYNC_BEGIN2(category, name, id, arg1_name, arg1_val, \
        arg2_name, arg2_val) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_BEGIN, \
        category, name, id, TRACE_EVENT_FLAG_NONE, \
        arg1_name, arg1_val, arg2_name, arg2_val)
#define TRACE_EVENT_COPY_ASYNC_BEGIN0(category, name, id) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_BEGIN, \
        category, name, id, TRACE_EVENT_FLAG_COPY)
#define TRACE_EVENT_COPY_ASYNC_BEGIN1(category, name, id, arg1_name, arg1_val) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_BEGIN, \
        category, name, id, TRACE_EVENT_FLAG_COPY, \
        arg1_name, arg1_val)
#define TRACE_EVENT_COPY_ASYNC_BEGIN2(category, name, id, arg1_name, arg1_val, \
        arg2_name, arg2_val) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_BEGIN, \
        category, name, id, TRACE_EVENT_FLAG_COPY, \
        arg1_name, arg1_val, arg2_name, arg2_val)






#define TRACE_EVENT_ASYNC_STEP0(category, name, id, step) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_STEP, \
        category, name, id, TRACE_EVENT_FLAG_NONE, "step", step)
#define TRACE_EVENT_ASYNC_STEP1(category, name, id, step, \
                                      arg1_name, arg1_val) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_STEP, \
        category, name, id, TRACE_EVENT_FLAG_NONE, "step", step, \
        arg1_name, arg1_val)
#define TRACE_EVENT_COPY_ASYNC_STEP0(category, name, id, step) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_STEP, \
        category, name, id, TRACE_EVENT_FLAG_COPY, "step", step)
#define TRACE_EVENT_COPY_ASYNC_STEP1(category, name, id, step, \
        arg1_name, arg1_val) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_STEP, \
        category, name, id, TRACE_EVENT_FLAG_COPY, "step", step, \
        arg1_name, arg1_val)



#define TRACE_EVENT_ASYNC_END0(category, name, id) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_END, \
        category, name, id, TRACE_EVENT_FLAG_NONE)
#define TRACE_EVENT_ASYNC_END1(category, name, id, arg1_name, arg1_val) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_END, \
        category, name, id, TRACE_EVENT_FLAG_NONE, arg1_name, arg1_val)
#define TRACE_EVENT_ASYNC_END2(category, name, id, arg1_name, arg1_val, \
        arg2_name, arg2_val) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_END, \
        category, name, id, TRACE_EVENT_FLAG_NONE, \
        arg1_name, arg1_val, arg2_name, arg2_val)
#define TRACE_EVENT_COPY_ASYNC_END0(category, name, id) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_END, \
        category, name, id, TRACE_EVENT_FLAG_COPY)
#define TRACE_EVENT_COPY_ASYNC_END1(category, name, id, arg1_name, arg1_val) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_END, \
        category, name, id, TRACE_EVENT_FLAG_COPY, \
        arg1_name, arg1_val)
#define TRACE_EVENT_COPY_ASYNC_END2(category, name, id, arg1_name, arg1_val, \
        arg2_name, arg2_val) \
    INTERNAL_TRACE_EVENT_ADD_WITH_ID(TRACE_EVENT_PHASE_ASYNC_END, \
        category, name, id, TRACE_EVENT_FLAG_COPY, \
        arg1_name, arg1_val, arg2_name, arg2_val)












#define TRACE_EVENT_SCOPED_SAMPLING_STATE_FOR_BUCKET(bucket_number, category, name) \
    TraceEvent::SamplingStateScope<bucket_number> traceEventSamplingScope(category "\0" name);



#define TRACE_EVENT_GET_SAMPLING_STATE_FOR_BUCKET(bucket_number) \
    TraceEvent::SamplingStateScope<bucket_number>::current()



#define TRACE_EVENT_SET_SAMPLING_STATE_FOR_BUCKET(bucket_number, category, name) \
    TraceEvent::SamplingStateScope<bucket_number>::set(category "\0" name)




#define TRACE_EVENT_SET_NONCONST_SAMPLING_STATE_FOR_BUCKET(bucket_number, categoryAndName) \
    TraceEvent::SamplingStateScope<bucket_number>::set(categoryAndName)


#define TRACE_EVENT_SCOPED_SAMPLING_STATE(category, name) \
    TRACE_EVENT_SCOPED_SAMPLING_STATE_FOR_BUCKET(0, category, name)
#define TRACE_EVENT_GET_SAMPLING_STATE() \
    TRACE_EVENT_GET_SAMPLING_STATE_FOR_BUCKET(0)
#define TRACE_EVENT_SET_SAMPLING_STATE(category, name) \
    TRACE_EVENT_SET_SAMPLING_STATE_FOR_BUCKET(0, category, name)
#define TRACE_EVENT_SET_NONCONST_SAMPLING_STATE(categoryAndName) \
    TRACE_EVENT_SET_NONCONST_SAMPLING_STATE_FOR_BUCKET(0, categoryAndName)














#define TRACE_EVENT_API_GET_CATEGORY_ENABLED \
    gl::TraceGetTraceCategoryEnabledFlag












#define TRACE_EVENT_API_ADD_TRACE_EVENT \
    gl::TraceAddTraceEvent






#define INTERNAL_TRACE_EVENT_UID3(a, b) \
    trace_event_unique_##a##b
#define INTERNAL_TRACE_EVENT_UID2(a, b) \
    INTERNAL_TRACE_EVENT_UID3(a, b)
#define INTERNALTRACEEVENTUID(name_prefix) \
    INTERNAL_TRACE_EVENT_UID2(name_prefix, __LINE__)


#define INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO(category) \
    static const unsigned char* INTERNALTRACEEVENTUID(catstatic) = 0; \
    if (!INTERNALTRACEEVENTUID(catstatic)) \
      INTERNALTRACEEVENTUID(catstatic) = \
          TRACE_EVENT_API_GET_CATEGORY_ENABLED(category);



#define INTERNAL_TRACE_EVENT_ADD(phase, category, name, flags, ...) \
    do { \
        INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO(category); \
        if (*INTERNALTRACEEVENTUID(catstatic)) { \
            gl::TraceEvent::addTraceEvent( \
                phase, INTERNALTRACEEVENTUID(catstatic), name, \
                gl::TraceEvent::noEventId, flags, ##__VA_ARGS__); \
        } \
    } while (0)




#define INTERNAL_TRACE_EVENT_ADD_SCOPED(category, name, ...) \
    INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO(category); \
    gl::TraceEvent::TraceEndOnScopeClose \
        INTERNALTRACEEVENTUID(profileScope); \
    if (*INTERNALTRACEEVENTUID(catstatic)) { \
      gl::TraceEvent::addTraceEvent( \
          TRACE_EVENT_PHASE_BEGIN, \
          INTERNALTRACEEVENTUID(catstatic), \
          name, gl::TraceEvent::noEventId, \
          TRACE_EVENT_FLAG_NONE, ##__VA_ARGS__); \
      INTERNALTRACEEVENTUID(profileScope).initialize( \
          INTERNALTRACEEVENTUID(catstatic), name); \
    }



#define INTERNAL_TRACE_EVENT_ADD_WITH_ID(phase, category, name, id, flags, \
                                         ...) \
    do { \
        INTERNAL_TRACE_EVENT_GET_CATEGORY_INFO(category); \
        if (*INTERNALTRACEEVENTUID(catstatic)) { \
            unsigned char traceEventFlags = flags | TRACE_EVENT_FLAG_HAS_ID; \
            gl::TraceEvent::TraceID traceEventTraceID( \
                id, &traceEventFlags); \
            gl::TraceEvent::addTraceEvent( \
                phase, INTERNALTRACEEVENTUID(catstatic), \
                name, traceEventTraceID.data(), traceEventFlags, \
                ##__VA_ARGS__); \
        } \
    } while (0)







#define TRACE_EVENT_PHASE_BEGIN    ('B')
#define TRACE_EVENT_PHASE_END      ('E')
#define TRACE_EVENT_PHASE_INSTANT  ('I')
#define TRACE_EVENT_PHASE_ASYNC_BEGIN ('S')
#define TRACE_EVENT_PHASE_ASYNC_STEP  ('T')
#define TRACE_EVENT_PHASE_ASYNC_END   ('F')
#define TRACE_EVENT_PHASE_METADATA ('M')
#define TRACE_EVENT_PHASE_COUNTER  ('C')
#define TRACE_EVENT_PHASE_SAMPLE  ('P')


#define TRACE_EVENT_FLAG_NONE        (static_cast<unsigned char>(0))
#define TRACE_EVENT_FLAG_COPY        (static_cast<unsigned char>(1 << 0))
#define TRACE_EVENT_FLAG_HAS_ID      (static_cast<unsigned char>(1 << 1))
#define TRACE_EVENT_FLAG_MANGLE_ID   (static_cast<unsigned char>(1 << 2))


#define TRACE_VALUE_TYPE_BOOL         (static_cast<unsigned char>(1))
#define TRACE_VALUE_TYPE_UINT         (static_cast<unsigned char>(2))
#define TRACE_VALUE_TYPE_INT          (static_cast<unsigned char>(3))
#define TRACE_VALUE_TYPE_DOUBLE       (static_cast<unsigned char>(4))
#define TRACE_VALUE_TYPE_POINTER      (static_cast<unsigned char>(5))
#define TRACE_VALUE_TYPE_STRING       (static_cast<unsigned char>(6))
#define TRACE_VALUE_TYPE_COPY_STRING  (static_cast<unsigned char>(7))


namespace gl {

namespace TraceEvent {



const int zeroNumArgs = 0;
const unsigned long long noEventId = 0;




class TraceID {
public:
    explicit TraceID(const void* id, unsigned char* flags) :
        m_data(static_cast<unsigned long long>(reinterpret_cast<unsigned long>(id)))
    {
        *flags |= TRACE_EVENT_FLAG_MANGLE_ID;
    }
    explicit TraceID(unsigned long long id, unsigned char* flags) : m_data(id) { (void)flags; }
    explicit TraceID(unsigned long id, unsigned char* flags) : m_data(id) { (void)flags; }
    explicit TraceID(unsigned int id, unsigned char* flags) : m_data(id) { (void)flags; }
    explicit TraceID(unsigned short id, unsigned char* flags) : m_data(id) { (void)flags; }
    explicit TraceID(unsigned char id, unsigned char* flags) : m_data(id) { (void)flags; }
    explicit TraceID(long long id, unsigned char* flags) :
        m_data(static_cast<unsigned long long>(id)) { (void)flags; }
    explicit TraceID(long id, unsigned char* flags) :
        m_data(static_cast<unsigned long long>(id)) { (void)flags; }
    explicit TraceID(int id, unsigned char* flags) :
        m_data(static_cast<unsigned long long>(id)) { (void)flags; }
    explicit TraceID(short id, unsigned char* flags) :
        m_data(static_cast<unsigned long long>(id)) { (void)flags; }
    explicit TraceID(signed char id, unsigned char* flags) :
        m_data(static_cast<unsigned long long>(id)) { (void)flags; }

    unsigned long long data() const { return m_data; }

private:
    unsigned long long m_data;
};


union TraceValueUnion {
    bool m_bool;
    unsigned long long m_uint;
    long long m_int;
    double m_double;
    const void* m_pointer;
    const char* m_string;
};


class TraceStringWithCopy {
public:
    explicit TraceStringWithCopy(const char* str) : m_str(str) { }
    operator const char* () const { return m_str; }
private:
    const char* m_str;
};




#define INTERNAL_DECLARE_SET_TRACE_VALUE(actual_type, \
                                         union_member, \
                                         value_type_id) \
    static inline void setTraceValue(actual_type arg, \
                                     unsigned char* type, \
                                     unsigned long long* value) { \
        TraceValueUnion typeValue; \
        typeValue.union_member = arg; \
        *type = value_type_id; \
        *value = typeValue.m_uint; \
    }

#define INTERNAL_DECLARE_SET_TRACE_VALUE_INT(actual_type, \
                                             value_type_id) \
    static inline void setTraceValue(actual_type arg, \
                                     unsigned char* type, \
                                     unsigned long long* value) { \
        *type = value_type_id; \
        *value = static_cast<unsigned long long>(arg); \
    }

INTERNAL_DECLARE_SET_TRACE_VALUE_INT(unsigned long long, TRACE_VALUE_TYPE_UINT)
INTERNAL_DECLARE_SET_TRACE_VALUE_INT(unsigned int, TRACE_VALUE_TYPE_UINT)
INTERNAL_DECLARE_SET_TRACE_VALUE_INT(unsigned short, TRACE_VALUE_TYPE_UINT)
INTERNAL_DECLARE_SET_TRACE_VALUE_INT(unsigned char, TRACE_VALUE_TYPE_UINT)
INTERNAL_DECLARE_SET_TRACE_VALUE_INT(long long, TRACE_VALUE_TYPE_INT)
INTERNAL_DECLARE_SET_TRACE_VALUE_INT(int, TRACE_VALUE_TYPE_INT)
INTERNAL_DECLARE_SET_TRACE_VALUE_INT(short, TRACE_VALUE_TYPE_INT)
INTERNAL_DECLARE_SET_TRACE_VALUE_INT(signed char, TRACE_VALUE_TYPE_INT)
INTERNAL_DECLARE_SET_TRACE_VALUE(bool, m_bool, TRACE_VALUE_TYPE_BOOL)
INTERNAL_DECLARE_SET_TRACE_VALUE(double, m_double, TRACE_VALUE_TYPE_DOUBLE)
INTERNAL_DECLARE_SET_TRACE_VALUE(const void*, m_pointer,
                                 TRACE_VALUE_TYPE_POINTER)
INTERNAL_DECLARE_SET_TRACE_VALUE(const char*, m_string,
                                 TRACE_VALUE_TYPE_STRING)
INTERNAL_DECLARE_SET_TRACE_VALUE(const TraceStringWithCopy&, m_string,
                                 TRACE_VALUE_TYPE_COPY_STRING)

#undef INTERNAL_DECLARE_SET_TRACE_VALUE
#undef INTERNAL_DECLARE_SET_TRACE_VALUE_INT

static inline void setTraceValue(const std::string& arg,
                                 unsigned char* type,
                                 unsigned long long* value) {
    TraceValueUnion typeValue;
    typeValue.m_string = arg.data();
    *type = TRACE_VALUE_TYPE_COPY_STRING;
    *value = typeValue.m_uint;
}






static inline void addTraceEvent(char phase,
                                const unsigned char* categoryEnabled,
                                const char* name,
                                unsigned long long id,
                                unsigned char flags) {
    TRACE_EVENT_API_ADD_TRACE_EVENT(
        phase, categoryEnabled, name, id,
        zeroNumArgs, 0, 0, 0,
        flags);
}

template<class ARG1_TYPE>
static inline void addTraceEvent(char phase,
                                const unsigned char* categoryEnabled,
                                const char* name,
                                unsigned long long id,
                                unsigned char flags,
                                const char* arg1Name,
                                const ARG1_TYPE& arg1Val) {
    const int numArgs = 1;
    unsigned char argTypes[1];
    unsigned long long argValues[1];
    setTraceValue(arg1Val, &argTypes[0], &argValues[0]);
    TRACE_EVENT_API_ADD_TRACE_EVENT(
        phase, categoryEnabled, name, id,
        numArgs, &arg1Name, argTypes, argValues,
        flags);
}

template<class ARG1_TYPE, class ARG2_TYPE>
static inline void addTraceEvent(char phase,
                                const unsigned char* categoryEnabled,
                                const char* name,
                                unsigned long long id,
                                unsigned char flags,
                                const char* arg1Name,
                                const ARG1_TYPE& arg1Val,
                                const char* arg2Name,
                                const ARG2_TYPE& arg2Val) {
    const int numArgs = 2;
    const char* argNames[2] = { arg1Name, arg2Name };
    unsigned char argTypes[2];
    unsigned long long argValues[2];
    setTraceValue(arg1Val, &argTypes[0], &argValues[0]);
    setTraceValue(arg2Val, &argTypes[1], &argValues[1]);
    return TRACE_EVENT_API_ADD_TRACE_EVENT(
        phase, categoryEnabled, name, id,
        numArgs, argNames, argTypes, argValues,
        flags);
}


class TraceEndOnScopeClose {
public:
    
    TraceEndOnScopeClose() : m_pdata(0) { }
    ~TraceEndOnScopeClose()
    {
        if (m_pdata)
            addEventIfEnabled();
    }

    void initialize(const unsigned char* categoryEnabled,
                    const char* name)
    {
        m_data.categoryEnabled = categoryEnabled;
        m_data.name = name;
        m_pdata = &m_data;
    }

private:
    
    void addEventIfEnabled()
    {
        
        if (*m_pdata->categoryEnabled) {
            TRACE_EVENT_API_ADD_TRACE_EVENT(
                TRACE_EVENT_PHASE_END,
                m_pdata->categoryEnabled,
                m_pdata->name, noEventId,
                zeroNumArgs, 0, 0, 0,
                TRACE_EVENT_FLAG_NONE);
        }
    }

    
    
    
    
    
    struct Data {
        const unsigned char* categoryEnabled;
        const char* name;
    };
    Data* m_pdata;
    Data m_data;
};




template<size_t BucketNumber>
class SamplingStateScope {
public:
    SamplingStateScope(const char* categoryAndName)
    {
        m_previousState = SamplingStateScope<BucketNumber>::current();
        SamplingStateScope<BucketNumber>::set(categoryAndName);
    }

    ~SamplingStateScope()
    {
        SamplingStateScope<BucketNumber>::set(m_previousState);
    }

    
    static inline const char* current()
    {
        return reinterpret_cast<const char*>(*gl::traceSamplingState[BucketNumber]);
    }
    static inline void set(const char* categoryAndName)
    {
        *gl::traceSamplingState[BucketNumber] = reinterpret_cast<long>(const_cast<char*>(categoryAndName));
    }

private:
    const char* m_previousState;
};

} 

} 

#endif
