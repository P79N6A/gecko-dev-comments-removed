















#ifndef HTTP_STREAM_H_

#define HTTP_STREAM_H_

#include "stagefright_string.h"

#include <sys/types.h>

#include <media/stagefright/MediaErrors.h>
#include <utils/KeyedVector.h>
#include <utils/threads.h>

namespace android {

class HTTPStream {
public:
    HTTPStream();
    ~HTTPStream();

    status_t connect(const char *server, int port = 80);
    status_t disconnect();

    status_t send(const char *data, size_t size);

    
    status_t send(const char *data);

    
    ssize_t receive(void *data, size_t size);

    status_t receive_header(int *http_status);

    
    static const char *kStatusKey;

    bool find_header_value(
            const string &key, string *value) const;

    
    void setReceiveTimeout(int seconds);

    
    
    status_t receive_line(char *line, size_t size);

private:
    enum State {
        READY,
        CONNECTING,
        CONNECTED
    };

    State mState;
    Mutex mLock;
    int mSocket;

    KeyedVector<string, string> mHeaders;

    HTTPStream(const HTTPStream &);
    HTTPStream &operator=(const HTTPStream &);
};

}  

#endif
