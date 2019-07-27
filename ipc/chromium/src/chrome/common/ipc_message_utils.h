



#ifndef CHROME_COMMON_IPC_MESSAGE_UTILS_H_
#define CHROME_COMMON_IPC_MESSAGE_UTILS_H_

#include <string>
#include <vector>
#include <map>

#include "base/file_path.h"
#include "base/string_util.h"
#include "base/string16.h"
#include "base/tuple.h"
#include "base/time.h"

#if defined(OS_POSIX)
#include "chrome/common/file_descriptor_set_posix.h"
#endif
#include "chrome/common/ipc_sync_message.h"
#include "chrome/common/transport_dib.h"

namespace IPC {




class MessageIterator {
 public:
  explicit MessageIterator(const Message& m) : msg_(m), iter_(NULL) {
  }
  int NextInt() const {
    int val;
    if (!msg_.ReadInt(&iter_, &val))
      NOTREACHED();
    return val;
  }
  intptr_t NextIntPtr() const {
    intptr_t val;
    if (!msg_.ReadIntPtr(&iter_, &val))
      NOTREACHED();
    return val;
  }
  const std::string NextString() const {
    std::string val;
    if (!msg_.ReadString(&iter_, &val))
      NOTREACHED();
    return val;
  }
  const std::wstring NextWString() const {
    std::wstring val;
    if (!msg_.ReadWString(&iter_, &val))
      NOTREACHED();
    return val;
  }
  const void NextData(const char** data, int* length) const {
    if (!msg_.ReadData(&iter_, data, length)) {
      NOTREACHED();
    }
  }
 private:
  const Message& msg_;
  mutable void* iter_;
};













































template <class P> struct ParamTraits;

template <class P>
static inline void WriteParam(Message* m, const P& p) {
  ParamTraits<P>::Write(m, p);
}

template <class P>
static inline bool WARN_UNUSED_RESULT ReadParam(const Message* m, void** iter,
                                                P* p) {
  return ParamTraits<P>::Read(m, iter, p);
}

template <class P>
static inline void LogParam(const P& p, std::wstring* l) {
  ParamTraits<P>::Log(p, l);
}



template <class P>
struct ParamTraitsFundamental {};

template <>
struct ParamTraitsFundamental<bool> {
  typedef bool param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteBool(p);
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    return m->ReadBool(iter, r);
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(p ? L"true" : L"false");
  }
};

template <>
struct ParamTraitsFundamental<int> {
  typedef int param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteInt(p);
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    return m->ReadInt(iter, r);
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(StringPrintf(L"%d", p));
  }
};

template <>
struct ParamTraitsFundamental<long> {
  typedef long param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteLong(p);
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    return m->ReadLong(iter, r);
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(StringPrintf(L"%l", p));
  }
};

template <>
struct ParamTraitsFundamental<unsigned long> {
  typedef unsigned long param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteULong(p);
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    return m->ReadULong(iter, r);
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(StringPrintf(L"%ul", p));
  }
};

template <>
struct ParamTraitsFundamental<long long> {
  typedef long long param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteData(reinterpret_cast<const char*>(&p), sizeof(param_type));
 }
  static bool Read(const Message* m, void** iter, param_type* r) {
    const char *data;
    int data_size = 0;
    bool result = m->ReadData(iter, &data, &data_size);
    if (result && data_size == sizeof(param_type)) {
      memcpy(r, data, sizeof(param_type));
    } else {
      result = false;
      NOTREACHED();
    }
    return result;
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(StringPrintf(L"%ll", p));
  }
};

template <>
struct ParamTraitsFundamental<unsigned long long> {
  typedef unsigned long long param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteData(reinterpret_cast<const char*>(&p), sizeof(param_type));
 }
  static bool Read(const Message* m, void** iter, param_type* r) {
    const char *data;
    int data_size = 0;
    bool result = m->ReadData(iter, &data, &data_size);
    if (result && data_size == sizeof(param_type)) {
      memcpy(r, data, sizeof(param_type));
    } else {
      result = false;
      NOTREACHED();
    }
    return result;
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(StringPrintf(L"%ull", p));
  }
};

template <>
struct ParamTraitsFundamental<double> {
  typedef double param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteData(reinterpret_cast<const char*>(&p), sizeof(param_type));
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    const char *data;
    int data_size = 0;
    bool result = m->ReadData(iter, &data, &data_size);
    if (result && data_size == sizeof(param_type)) {
      memcpy(r, data, sizeof(param_type));
    } else {
      result = false;
      NOTREACHED();
    }

    return result;
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(StringPrintf(L"e", p));
  }
};



template <class P>
struct ParamTraitsFixed : ParamTraitsFundamental<P> {};

template <>
struct ParamTraitsFixed<int16_t> {
  typedef int16_t param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteInt16(p);
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    return m->ReadInt16(iter, r);
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(StringPrintf(L"%hd", p));
  }
};

template <>
struct ParamTraitsFixed<uint16_t> {
  typedef uint16_t param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteUInt16(p);
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    return m->ReadUInt16(iter, r);
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(StringPrintf(L"%hu", p));
  }
};

template <>
struct ParamTraitsFixed<uint32_t> {
  typedef uint32_t param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteUInt32(p);
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    return m->ReadUInt32(iter, r);
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(StringPrintf(L"%u", p));
  }
};

template <>
struct ParamTraitsFixed<int64_t> {
  typedef int64_t param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteInt64(p);
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    return m->ReadInt64(iter, r);
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(StringPrintf(L"%" PRId64L, p));
  }
};

template <>
struct ParamTraitsFixed<uint64_t> {
  typedef uint64_t param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteInt64(static_cast<int64_t>(p));
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    return m->ReadInt64(iter, reinterpret_cast<int64_t*>(r));
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(StringPrintf(L"%" PRIu64L, p));
  }
};



template <class P>
struct ParamTraitsLibC : ParamTraitsFixed<P> {};

template <>
struct ParamTraitsLibC<size_t> {
  typedef size_t param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteSize(p);
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    return m->ReadSize(iter, r);
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(StringPrintf(L"%u", p));
  }
};



template <class P>
struct ParamTraitsStd : ParamTraitsLibC<P> {};

template <>
struct ParamTraitsStd<std::string> {
  typedef std::string param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteString(p);
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    return m->ReadString(iter, r);
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(UTF8ToWide(p));
  }
};

template <>
struct ParamTraitsStd<std::wstring> {
  typedef std::wstring param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteWString(p);
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    return m->ReadWString(iter, r);
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(p);
  }
};

template <class K, class V>
struct ParamTraitsStd<std::map<K, V> > {
  typedef std::map<K, V> param_type;
  static void Write(Message* m, const param_type& p) {
    WriteParam(m, static_cast<int>(p.size()));
    typename param_type::const_iterator iter;
    for (iter = p.begin(); iter != p.end(); ++iter) {
      WriteParam(m, iter->first);
      WriteParam(m, iter->second);
    }
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    int size;
    if (!ReadParam(m, iter, &size) || size < 0)
      return false;
    for (int i = 0; i < size; ++i) {
      K k;
      if (!ReadParam(m, iter, &k))
        return false;
      V& value = (*r)[k];
      if (!ReadParam(m, iter, &value))
        return false;
    }
    return true;
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(L"<std::map>");
  }
};



template <class P>
struct ParamTraitsWindows : ParamTraitsStd<P> {};

#if defined(OS_WIN)
template <>
struct ParamTraitsWindows<HANDLE> {
  typedef HANDLE param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteIntPtr(reinterpret_cast<intptr_t>(p));
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    DCHECK_EQ(sizeof(param_type), sizeof(intptr_t));
    return m->ReadIntPtr(iter, reinterpret_cast<intptr_t*>(r));
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(StringPrintf(L"0x%X", p));
  }
};

template <>
struct ParamTraitsWindows<HWND> {
  typedef HWND param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteIntPtr(reinterpret_cast<intptr_t>(p));
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    DCHECK_EQ(sizeof(param_type), sizeof(intptr_t));
    return m->ReadIntPtr(iter, reinterpret_cast<intptr_t*>(r));
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(StringPrintf(L"0x%X", p));
  }
};
#endif  



template <class P>
struct ParamTraitsIPC : ParamTraitsWindows<P> {};

#if defined(OS_POSIX)















template<>
struct ParamTraitsIPC<base::FileDescriptor> {
  typedef base::FileDescriptor param_type;
  static void Write(Message* m, const param_type& p) {
    const bool valid = p.fd >= 0;
    WriteParam(m, valid);

    if (valid) {
      if (!m->WriteFileDescriptor(p)) {
        NOTREACHED() << "Too many file descriptors for one message!";
      }
    }
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    bool valid;
    if (!ReadParam(m, iter, &valid))
      return false;

    if (!valid) {
      r->fd = -1;
      r->auto_close = false;
      return true;
    }

    return m->ReadFileDescriptor(iter, r);
  }
  static void Log(const param_type& p, std::wstring* l) {
    if (p.auto_close) {
      l->append(StringPrintf(L"FD(%d auto-close)", p.fd));
    } else {
      l->append(StringPrintf(L"FD(%d)", p.fd));
    }
  }
};
#endif 

#if defined(OS_WIN)
template<>
struct ParamTraitsIPC<TransportDIB::Id> {
  typedef TransportDIB::Id param_type;
  static void Write(Message* m, const param_type& p) {
    WriteParam(m, p.handle);
    WriteParam(m, p.sequence_num);
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    return (ReadParam(m, iter, &r->handle) &&
            ReadParam(m, iter, &r->sequence_num));
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(L"TransportDIB(");
    LogParam(p.handle, l);
    l->append(L", ");
    LogParam(p.sequence_num, l);
    l->append(L")");
  }
};
#endif



template <class P>
struct ParamTraitsMozilla : ParamTraitsIPC<P> {};

template <>
struct ParamTraitsMozilla<nsresult> {
  typedef nsresult param_type;
  static void Write(Message* m, const param_type& p) {
    m->WriteUInt32(static_cast<uint32_t>(p));
  }
  static bool Read(const Message* m, void** iter, param_type* r) {
    return m->ReadUInt32(iter, reinterpret_cast<uint32_t*>(r));
  }
  static void Log(const param_type& p, std::wstring* l) {
    l->append(StringPrintf(L"%u", static_cast<uint32_t>(p)));
  }
};



template <class P> struct ParamTraits : ParamTraitsMozilla<P> {};

}  

#endif  
