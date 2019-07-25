


















#include "Static.h"

#include <utils/BufferedTextOutput.h>
#include "utils_Log.h"

namespace android {

class LibUtilsFirstStatics
{
public:
    LibUtilsFirstStatics()
    {
        initialize_string8();
        initialize_string16();
    }
    
    ~LibUtilsFirstStatics()
    {
        terminate_string16();
        terminate_string8();
    }
};

static LibUtilsFirstStatics gFirstStatics;
int gDarwinCantLoadAllObjects = 1;


#if 0
Vector<int32_t> gTextBuffers;

class LogTextOutput : public BufferedTextOutput
{
public:
    LogTextOutput() : BufferedTextOutput(MULTITHREADED) { }
    virtual ~LogTextOutput() { };

protected:
    virtual status_t writeLines(const struct iovec& vec, size_t N)
    {
        
        if (N != 1) ALOGI("WARNING: writeLines N=%d\n", N);
        ALOGI("%.*s", vec.iov_len, (const char*) vec.iov_base);
        return NO_ERROR;
    }
};

class FdTextOutput : public BufferedTextOutput
{
public:
    FdTextOutput(int fd) : BufferedTextOutput(MULTITHREADED), mFD(fd) { }
    virtual ~FdTextOutput() { };

protected:
    virtual status_t writeLines(const struct iovec& vec, size_t N)
    {
        writev(mFD, &vec, N);
        return NO_ERROR;
    }

private:
    int mFD;
};

static LogTextOutput gLogTextOutput;
static FdTextOutput gStdoutTextOutput(STDOUT_FILENO);
static FdTextOutput gStderrTextOutput(STDERR_FILENO);

TextOutput& alog(gLogTextOutput);
TextOutput& aout(gStdoutTextOutput);
TextOutput& aerr(gStderrTextOutput);
#endif
}   
