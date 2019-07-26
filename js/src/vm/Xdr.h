





#ifndef vm_Xdr_h
#define vm_Xdr_h

#include "mozilla/Endian.h"

#include "jsatom.h"

namespace js {










static const uint32_t XDR_BYTECODE_VERSION = uint32_t(0xb973c0de - 152);

class XDRBuffer {
  public:
    XDRBuffer(JSContext *cx)
      : context(cx), base(NULL), cursor(NULL), limit(NULL) { }

    JSContext *cx() const {
        return context;
    }

    void *getData(uint32_t *lengthp) const {
        JS_ASSERT(size_t(cursor - base) <= size_t(UINT32_MAX));
        *lengthp = uint32_t(cursor - base);
        return base;
    }

    void setData(const void *data, uint32_t length) {
        base = static_cast<uint8_t *>(const_cast<void *>(data));
        cursor = base;
        limit = base + length;
    }

    const uint8_t *read(size_t n) {
        JS_ASSERT(n <= size_t(limit - cursor));
        uint8_t *ptr = cursor;
        cursor += n;
        return ptr;
    }

    const char *readCString() {
        char *ptr = reinterpret_cast<char *>(cursor);
        cursor = reinterpret_cast<uint8_t *>(strchr(ptr, '\0')) + 1;
        JS_ASSERT(base < cursor);
        JS_ASSERT(cursor <= limit);
        return ptr;
    }

    uint8_t *write(size_t n) {
        if (n > size_t(limit - cursor)) {
            if (!grow(n))
                return NULL;
        }
        uint8_t *ptr = cursor;
        cursor += n;
        return ptr;
    }

    static bool isUint32Overflow(size_t n) {
        return size_t(-1) > size_t(UINT32_MAX) && n > size_t(UINT32_MAX);
    }

    void freeBuffer();

  private:
    bool grow(size_t n);

    JSContext   *const context;
    uint8_t     *base;
    uint8_t     *cursor;
    uint8_t     *limit;
};




template <XDRMode mode>
class XDRState {
  public:
    XDRBuffer buf;

  protected:
    JSPrincipals *principals_;
    JSPrincipals *originPrincipals_;

    XDRState(JSContext *cx)
      : buf(cx), principals_(NULL), originPrincipals_(NULL) {
    }

  public:
    JSContext *cx() const {
        return buf.cx();
    }

    JSPrincipals *originPrincipals() const {
        return originPrincipals_;
    }

    bool codeUint8(uint8_t *n) {
        if (mode == XDR_ENCODE) {
            uint8_t *ptr = buf.write(sizeof *n);
            if (!ptr)
                return false;
            *ptr = *n;
        } else {
            *n = *buf.read(sizeof *n);
        }
        return true;
    }

    bool codeUint16(uint16_t *n) {
        if (mode == XDR_ENCODE) {
            uint8_t *ptr = buf.write(sizeof *n);
            if (!ptr)
                return false;
            mozilla::LittleEndian::writeUint16(ptr, *n);
        } else {
            const uint8_t *ptr = buf.read(sizeof *n);
            *n = mozilla::LittleEndian::readUint16(ptr);
        }
        return true;
    }

    bool codeUint32(uint32_t *n) {
        if (mode == XDR_ENCODE) {
            uint8_t *ptr = buf.write(sizeof *n);
            if (!ptr)
                return false;
            mozilla::LittleEndian::writeUint32(ptr, *n);
        } else {
            const uint8_t *ptr = buf.read(sizeof *n);
            *n = mozilla::LittleEndian::readUint32(ptr);
        }
        return true;
    }

    bool codeUint64(uint64_t *n) {
        if (mode == XDR_ENCODE) {
            uint8_t *ptr = buf.write(sizeof(*n));
            if (!ptr)
                return false;
            mozilla::LittleEndian::writeUint64(ptr, *n);
        } else {
            const uint8_t *ptr = buf.read(sizeof(*n));
            *n = mozilla::LittleEndian::readUint64(ptr);
        }
        return true;
    }

    bool codeDouble(double *dp) {
        union DoublePun {
            double d;
            uint64_t u;
        } pun;
        if (mode == XDR_ENCODE)
            pun.d = *dp;
        if (!codeUint64(&pun.u))
            return false;
        if (mode == XDR_DECODE)
            *dp = pun.d;
        return true;
    }

    bool codeBytes(void *bytes, size_t len) {
        if (mode == XDR_ENCODE) {
            uint8_t *ptr = buf.write(len);
            if (!ptr)
                return false;
            memcpy(ptr, bytes, len);
        } else {
            memcpy(bytes, buf.read(len), len);
        }
        return true;
    }

    





    bool codeCString(const char **sp) {
        if (mode == XDR_ENCODE) {
            size_t n = strlen(*sp) + 1;
            uint8_t *ptr = buf.write(n);
            if (!ptr)
                return false;
            memcpy(ptr, *sp, n);
        } else {
            *sp = buf.readCString();
        }
        return true;
    }

    bool codeChars(jschar *chars, size_t nchars);

    bool codeFunction(JS::MutableHandleObject objp);
    bool codeScript(MutableHandleScript scriptp);
};

class XDREncoder : public XDRState<XDR_ENCODE> {
  public:
    XDREncoder(JSContext *cx)
      : XDRState<XDR_ENCODE>(cx) {
    }

    ~XDREncoder() {
        buf.freeBuffer();
    }

    const void *getData(uint32_t *lengthp) const {
        return buf.getData(lengthp);
    }

    void *forgetData(uint32_t *lengthp) {
        void *data = buf.getData(lengthp);
        buf.setData(NULL, 0);
        return data;
    }
};

class XDRDecoder : public XDRState<XDR_DECODE> {
  public:
    XDRDecoder(JSContext *cx, const void *data, uint32_t length,
               JSPrincipals *principals, JSPrincipals *originPrincipals);

};

} 

#endif 
