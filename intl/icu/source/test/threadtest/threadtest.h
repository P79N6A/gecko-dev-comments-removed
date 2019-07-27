







#ifndef ABSTRACTTHREADTEST_H
#define ABSTRACTTHREADTEST_H


























class AbstractThreadTest {
public:
                     AbstractThreadTest() {};
    virtual         ~AbstractThreadTest();
    virtual void     check()   = 0;
    virtual void     runOnce() = 0;
};

#endif 
