







#ifndef SkTrace_DEFINED
#define SkTrace_DEFINED

#ifdef SK_USER_TRACE_INCLUDE_FILE






















    #include SK_USER_TRACE_INCLUDE_FILE

#else

    #define SK_TRACE_EVENT0(event)
    #define SK_TRACE_EVENT1(event, name1, value1)
    #define SK_TRACE_EVENT2(event, name1, value1, name2, value2)

#endif

#endif


