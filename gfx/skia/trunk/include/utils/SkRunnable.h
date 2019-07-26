






#ifndef SkRunnable_DEFINED
#define SkRunnable_DEFINED

template <typename T>
struct SkTRunnable {
    virtual ~SkTRunnable() {};
    virtual void run(T&) = 0;
};

template <>
struct SkTRunnable<void> {
    virtual ~SkTRunnable() {};
    virtual void run() = 0;
};

typedef SkTRunnable<void> SkRunnable;

#endif
