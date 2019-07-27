





#ifndef vm_Xdr_h
#define vm_Xdr_h

#include "mozilla/Endian.h"
#include "mozilla/TypeTraits.h"

#include "jsatom.h"
#include "jsfriendapi.h"

namespace js {





















static const uint32_t XDR_BYTECODE_VERSION_SUBTRAHEND = 222;
static_assert(XDR_BYTECODE_VERSION_SUBTRAHEND % 2 == 0, "see the comment above");
static const uint32_t XDR_BYTECODE_VERSION =
    uint32_t(0xb973c0de - (XDR_BYTECODE_VERSION_SUBTRAHEND
#ifdef JS_HAS_SYMBOLS
                                                           + 1
#endif
                                                              ));

static_assert(JSErr_Limit == 369,
              "GREETINGS, POTENTIAL SUBTRAHEND INCREMENTER! If you added or "
              "removed MSG_DEFs from js.msg, you should increment "
              "XDR_BYTECODE_VERSION_SUBTRAHEND and update this assertion's "
              "expected JSErr_Limit value.");

class XDRBuffer {
  public:
    explicit XDRBuffer(JSContext *cx)
      : context(cx), base(nullptr), cursor(nullptr), limit(nullptr) { }

    JSContext *cx() const {
        return context;
    }

    void *getData(uint32_t *lengthp) const {
        MOZ_ASSERT(size_t(cursor - base) <= size_t(UINT32_MAX));
        *lengthp = uint32_t(cursor - base);
        return base;
    }

    void setData(const void *data, uint32_t length) {
        base = static_cast<uint8_t *>(const_cast<void *>(data));
        cursor = base;
        limit = base + length;
    }

    const uint8_t *read(size_t n) {
        MOZ_ASSERT(n <= size_t(limit - cursor));
        uint8_t *ptr = cursor;
        cursor += n;
        return ptr;
    }

    const char *readCString() {
        char *ptr = reinterpret_cast<char *>(cursor);
        cursor = reinterpret_cast<uint8_t *>(strchr(ptr, '\0')) + 1;
        MOZ_ASSERT(base < cursor);
        MOZ_ASSERT(cursor <= limit);
        return ptr;
    }

    uint8_t *write(size_t n) {
        if (n > size_t(limit - cursor)) {
            if (!grow(n))
                return nullptr;
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
    explicit XDRState(JSContext *cx)
      : buf(cx) { }

  public:
    JSContext *cx() const {
        return buf.cx();
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

    




    template <typename T>
    bool codeEnum32(T *val, typename mozilla::EnableIf<mozilla::IsEnum<T>::value, T>::Type * = NULL)
    {
        uint32_t tmp;
        if (mode == XDR_ENCODE)
            tmp = *val;
        if (!codeUint32(&tmp))
            return false;
        if (mode == XDR_DECODE)
            *val = T(tmp);
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

    bool codeChars(const JS::Latin1Char *chars, size_t nchars);
    bool codeChars(char16_t *chars, size_t nchars);

    bool codeFunction(JS::MutableHandleFunction objp);
    bool codeScript(MutableHandleScript scriptp);
    bool codeConstValue(MutableHandleValue vp);
};

class XDREncoder : public XDRState<XDR_ENCODE> {
  public:
    explicit XDREncoder(JSContext *cx)
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
        buf.setData(nullptr, 0);
        return data;
    }
};

class XDRDecoder : public XDRState<XDR_DECODE> {
  public:
    XDRDecoder(JSContext *cx, const void *data, uint32_t length);

};

} 

#endif 
