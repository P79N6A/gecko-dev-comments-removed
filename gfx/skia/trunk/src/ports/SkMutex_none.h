






#ifndef SkMutex_none_DEFINED
#define SkMutex_none_DEFINED



struct SkBaseMutex {
    void acquire() { }
    void release() { }
};

class SkMutex : public SkBaseMutex {
public:
    SkMutex() { }
    ~SkMutex() { }

private:
    SkMutex(const SkMutex&);
    SkMutex& operator=(const SkMutex&);
};


#define SK_DECLARE_STATIC_MUTEX(name) static SkBaseMutex name = { }


#define SK_DECLARE_GLOBAL_MUTEX(name) SkBaseMutex name = { }

#endif
