






#ifndef SkRunnable_DEFINED
#define SkRunnable_DEFINED

class SkRunnable {
public:
    virtual ~SkRunnable() {};
    virtual void run() = 0;
};

#endif
