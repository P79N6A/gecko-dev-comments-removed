

#define PY_SSIZE_T_CLEAN
#ifndef CYTHON_USE_PYLONG_INTERNALS
#ifdef PYLONG_BITS_IN_DIGIT
#define CYTHON_USE_PYLONG_INTERNALS 0
#else
#include "pyconfig.h"
#ifdef PYLONG_BITS_IN_DIGIT
#define CYTHON_USE_PYLONG_INTERNALS 1
#else
#define CYTHON_USE_PYLONG_INTERNALS 0
#endif
#endif
#endif
#include "Python.h"
#ifndef Py_PYTHON_H
    #error Python headers needed to compile C extensions, please install development version of Python.
#elif PY_VERSION_HEX < 0x02040000
    #error Cython requires Python 2.4+.
#else
#define CYTHON_ABI "0_20_1"
#include <stddef.h> 
#ifndef offsetof
#define offsetof(type, member) ( (size_t) & ((type*)0) -> member )
#endif
#if !defined(WIN32) && !defined(MS_WINDOWS)
  #ifndef __stdcall
    #define __stdcall
  #endif
  #ifndef __cdecl
    #define __cdecl
  #endif
  #ifndef __fastcall
    #define __fastcall
  #endif
#endif
#ifndef DL_IMPORT
  #define DL_IMPORT(t) t
#endif
#ifndef DL_EXPORT
  #define DL_EXPORT(t) t
#endif
#ifndef PY_LONG_LONG
  #define PY_LONG_LONG LONG_LONG
#endif
#ifndef Py_HUGE_VAL
  #define Py_HUGE_VAL HUGE_VAL
#endif
#ifdef PYPY_VERSION
#define CYTHON_COMPILING_IN_PYPY 1
#define CYTHON_COMPILING_IN_CPYTHON 0
#else
#define CYTHON_COMPILING_IN_PYPY 0
#define CYTHON_COMPILING_IN_CPYTHON 1
#endif
#if CYTHON_COMPILING_IN_PYPY
#define Py_OptimizeFlag 0
#endif
#if PY_VERSION_HEX < 0x02050000
  typedef int Py_ssize_t;
  #define PY_SSIZE_T_MAX INT_MAX
  #define PY_SSIZE_T_MIN INT_MIN
  #define PY_FORMAT_SIZE_T ""
  #define CYTHON_FORMAT_SSIZE_T ""
  #define PyInt_FromSsize_t(z) PyInt_FromLong(z)
  #define PyInt_AsSsize_t(o)   __Pyx_PyInt_As_int(o)
  #define PyNumber_Index(o)    ((PyNumber_Check(o) && !PyFloat_Check(o)) ? PyNumber_Int(o) : \
                                (PyErr_Format(PyExc_TypeError, \
                                              "expected index value, got %.200s", Py_TYPE(o)->tp_name), \
                                 (PyObject*)0))
  #define __Pyx_PyIndex_Check(o) (PyNumber_Check(o) && !PyFloat_Check(o) && \
                                  !PyComplex_Check(o))
  #define PyIndex_Check __Pyx_PyIndex_Check
  #define PyErr_WarnEx(category, message, stacklevel) PyErr_Warn(category, message)
  #define __PYX_BUILD_PY_SSIZE_T "i"
#else
  #define __PYX_BUILD_PY_SSIZE_T "n"
  #define CYTHON_FORMAT_SSIZE_T "z"
  #define __Pyx_PyIndex_Check PyIndex_Check
#endif
#if PY_VERSION_HEX < 0x02060000
  #define Py_REFCNT(ob) (((PyObject*)(ob))->ob_refcnt)
  #define Py_TYPE(ob)   (((PyObject*)(ob))->ob_type)
  #define Py_SIZE(ob)   (((PyVarObject*)(ob))->ob_size)
  #define PyVarObject_HEAD_INIT(type, size) \
          PyObject_HEAD_INIT(type) size,
  #define PyType_Modified(t)
  typedef struct {
     void *buf;
     PyObject *obj;
     Py_ssize_t len;
     Py_ssize_t itemsize;
     int readonly;
     int ndim;
     char *format;
     Py_ssize_t *shape;
     Py_ssize_t *strides;
     Py_ssize_t *suboffsets;
     void *internal;
  } Py_buffer;
  #define PyBUF_SIMPLE 0
  #define PyBUF_WRITABLE 0x0001
  #define PyBUF_FORMAT 0x0004
  #define PyBUF_ND 0x0008
  #define PyBUF_STRIDES (0x0010 | PyBUF_ND)
  #define PyBUF_C_CONTIGUOUS (0x0020 | PyBUF_STRIDES)
  #define PyBUF_F_CONTIGUOUS (0x0040 | PyBUF_STRIDES)
  #define PyBUF_ANY_CONTIGUOUS (0x0080 | PyBUF_STRIDES)
  #define PyBUF_INDIRECT (0x0100 | PyBUF_STRIDES)
  #define PyBUF_RECORDS (PyBUF_STRIDES | PyBUF_FORMAT | PyBUF_WRITABLE)
  #define PyBUF_FULL (PyBUF_INDIRECT | PyBUF_FORMAT | PyBUF_WRITABLE)
  typedef int (*getbufferproc)(PyObject *, Py_buffer *, int);
  typedef void (*releasebufferproc)(PyObject *, Py_buffer *);
#endif
#if PY_MAJOR_VERSION < 3
  #define __Pyx_BUILTIN_MODULE_NAME "__builtin__"
  #define __Pyx_PyCode_New(a, k, l, s, f, code, c, n, v, fv, cell, fn, name, fline, lnos) \
          PyCode_New(a+k, l, s, f, code, c, n, v, fv, cell, fn, name, fline, lnos)
  #define __Pyx_DefaultClassType PyClass_Type
#else
  #define __Pyx_BUILTIN_MODULE_NAME "builtins"
  #define __Pyx_PyCode_New(a, k, l, s, f, code, c, n, v, fv, cell, fn, name, fline, lnos) \
          PyCode_New(a, k, l, s, f, code, c, n, v, fv, cell, fn, name, fline, lnos)
  #define __Pyx_DefaultClassType PyType_Type
#endif
#if PY_VERSION_HEX < 0x02060000
  #define PyUnicode_FromString(s) PyUnicode_Decode(s, strlen(s), "UTF-8", "strict")
#endif
#if PY_MAJOR_VERSION >= 3
  #define Py_TPFLAGS_CHECKTYPES 0
  #define Py_TPFLAGS_HAVE_INDEX 0
#endif
#if (PY_VERSION_HEX < 0x02060000) || (PY_MAJOR_VERSION >= 3)
  #define Py_TPFLAGS_HAVE_NEWBUFFER 0
#endif
#if PY_VERSION_HEX < 0x02060000
  #define Py_TPFLAGS_HAVE_VERSION_TAG 0
#endif
#if PY_VERSION_HEX < 0x02060000 && !defined(Py_TPFLAGS_IS_ABSTRACT)
  #define Py_TPFLAGS_IS_ABSTRACT 0
#endif
#if PY_VERSION_HEX < 0x030400a1 && !defined(Py_TPFLAGS_HAVE_FINALIZE)
  #define Py_TPFLAGS_HAVE_FINALIZE 0
#endif
#if PY_VERSION_HEX > 0x03030000 && defined(PyUnicode_KIND)
  #define CYTHON_PEP393_ENABLED 1
  #define __Pyx_PyUnicode_READY(op)       (likely(PyUnicode_IS_READY(op)) ? \
                                              0 : _PyUnicode_Ready((PyObject *)(op)))
  #define __Pyx_PyUnicode_GET_LENGTH(u)   PyUnicode_GET_LENGTH(u)
  #define __Pyx_PyUnicode_READ_CHAR(u, i) PyUnicode_READ_CHAR(u, i)
  #define __Pyx_PyUnicode_KIND(u)         PyUnicode_KIND(u)
  #define __Pyx_PyUnicode_DATA(u)         PyUnicode_DATA(u)
  #define __Pyx_PyUnicode_READ(k, d, i)   PyUnicode_READ(k, d, i)
#else
  #define CYTHON_PEP393_ENABLED 0
  #define __Pyx_PyUnicode_READY(op)       (0)
  #define __Pyx_PyUnicode_GET_LENGTH(u)   PyUnicode_GET_SIZE(u)
  #define __Pyx_PyUnicode_READ_CHAR(u, i) ((Py_UCS4)(PyUnicode_AS_UNICODE(u)[i]))
  #define __Pyx_PyUnicode_KIND(u)         (sizeof(Py_UNICODE))
  #define __Pyx_PyUnicode_DATA(u)         ((void*)PyUnicode_AS_UNICODE(u))
  #define __Pyx_PyUnicode_READ(k, d, i)   ((void)(k), (Py_UCS4)(((Py_UNICODE*)d)[i]))
#endif
#if CYTHON_COMPILING_IN_PYPY
  #define __Pyx_PyUnicode_Concat(a, b)      PyNumber_Add(a, b)
  #define __Pyx_PyUnicode_ConcatSafe(a, b)  PyNumber_Add(a, b)
#else
  #define __Pyx_PyUnicode_Concat(a, b)      PyUnicode_Concat(a, b)
  #define __Pyx_PyUnicode_ConcatSafe(a, b)  ((unlikely((a) == Py_None) || unlikely((b) == Py_None)) ? \
      PyNumber_Add(a, b) : __Pyx_PyUnicode_Concat(a, b))
#endif
#define __Pyx_PyString_FormatSafe(a, b)  ((unlikely((a) == Py_None)) ? PyNumber_Remainder(a, b) : __Pyx_PyString_Format(a, b))
#define __Pyx_PyUnicode_FormatSafe(a, b)  ((unlikely((a) == Py_None)) ? PyNumber_Remainder(a, b) : PyUnicode_Format(a, b))
#if PY_MAJOR_VERSION >= 3
  #define __Pyx_PyString_Format(a, b)  PyUnicode_Format(a, b)
#else
  #define __Pyx_PyString_Format(a, b)  PyString_Format(a, b)
#endif
#if PY_MAJOR_VERSION >= 3
  #define PyBaseString_Type            PyUnicode_Type
  #define PyStringObject               PyUnicodeObject
  #define PyString_Type                PyUnicode_Type
  #define PyString_Check               PyUnicode_Check
  #define PyString_CheckExact          PyUnicode_CheckExact
#endif
#if PY_VERSION_HEX < 0x02060000
  #define PyBytesObject                PyStringObject
  #define PyBytes_Type                 PyString_Type
  #define PyBytes_Check                PyString_Check
  #define PyBytes_CheckExact           PyString_CheckExact
  #define PyBytes_FromString           PyString_FromString
  #define PyBytes_FromStringAndSize    PyString_FromStringAndSize
  #define PyBytes_FromFormat           PyString_FromFormat
  #define PyBytes_DecodeEscape         PyString_DecodeEscape
  #define PyBytes_AsString             PyString_AsString
  #define PyBytes_AsStringAndSize      PyString_AsStringAndSize
  #define PyBytes_Size                 PyString_Size
  #define PyBytes_AS_STRING            PyString_AS_STRING
  #define PyBytes_GET_SIZE             PyString_GET_SIZE
  #define PyBytes_Repr                 PyString_Repr
  #define PyBytes_Concat               PyString_Concat
  #define PyBytes_ConcatAndDel         PyString_ConcatAndDel
#endif
#if PY_MAJOR_VERSION >= 3
  #define __Pyx_PyBaseString_Check(obj) PyUnicode_Check(obj)
  #define __Pyx_PyBaseString_CheckExact(obj) PyUnicode_CheckExact(obj)
#else
  #define __Pyx_PyBaseString_Check(obj) (PyString_CheckExact(obj) || PyUnicode_CheckExact(obj) || \
                                         PyString_Check(obj) || PyUnicode_Check(obj))
  #define __Pyx_PyBaseString_CheckExact(obj) (PyString_CheckExact(obj) || PyUnicode_CheckExact(obj))
#endif
#if PY_VERSION_HEX < 0x02060000
  #define PySet_Check(obj)             PyObject_TypeCheck(obj, &PySet_Type)
  #define PyFrozenSet_Check(obj)       PyObject_TypeCheck(obj, &PyFrozenSet_Type)
#endif
#ifndef PySet_CheckExact
  #define PySet_CheckExact(obj)        (Py_TYPE(obj) == &PySet_Type)
#endif
#define __Pyx_TypeCheck(obj, type) PyObject_TypeCheck(obj, (PyTypeObject *)type)
#if PY_MAJOR_VERSION >= 3
  #define PyIntObject                  PyLongObject
  #define PyInt_Type                   PyLong_Type
  #define PyInt_Check(op)              PyLong_Check(op)
  #define PyInt_CheckExact(op)         PyLong_CheckExact(op)
  #define PyInt_FromString             PyLong_FromString
  #define PyInt_FromUnicode            PyLong_FromUnicode
  #define PyInt_FromLong               PyLong_FromLong
  #define PyInt_FromSize_t             PyLong_FromSize_t
  #define PyInt_FromSsize_t            PyLong_FromSsize_t
  #define PyInt_AsLong                 PyLong_AsLong
  #define PyInt_AS_LONG                PyLong_AS_LONG
  #define PyInt_AsSsize_t              PyLong_AsSsize_t
  #define PyInt_AsUnsignedLongMask     PyLong_AsUnsignedLongMask
  #define PyInt_AsUnsignedLongLongMask PyLong_AsUnsignedLongLongMask
  #define PyNumber_Int                 PyNumber_Long
#endif
#if PY_MAJOR_VERSION >= 3
  #define PyBoolObject                 PyLongObject
#endif
#if PY_VERSION_HEX < 0x030200A4
  typedef long Py_hash_t;
  #define __Pyx_PyInt_FromHash_t PyInt_FromLong
  #define __Pyx_PyInt_AsHash_t   PyInt_AsLong
#else
  #define __Pyx_PyInt_FromHash_t PyInt_FromSsize_t
  #define __Pyx_PyInt_AsHash_t   PyInt_AsSsize_t
#endif
#if (PY_MAJOR_VERSION < 3) || (PY_VERSION_HEX >= 0x03010300)
  #define __Pyx_PySequence_GetSlice(obj, a, b) PySequence_GetSlice(obj, a, b)
  #define __Pyx_PySequence_SetSlice(obj, a, b, value) PySequence_SetSlice(obj, a, b, value)
  #define __Pyx_PySequence_DelSlice(obj, a, b) PySequence_DelSlice(obj, a, b)
#else
  #define __Pyx_PySequence_GetSlice(obj, a, b) (unlikely(!(obj)) ? \
        (PyErr_SetString(PyExc_SystemError, "null argument to internal routine"), (PyObject*)0) : \
        (likely((obj)->ob_type->tp_as_mapping) ? (PySequence_GetSlice(obj, a, b)) : \
            (PyErr_Format(PyExc_TypeError, "'%.200s' object is unsliceable", (obj)->ob_type->tp_name), (PyObject*)0)))
  #define __Pyx_PySequence_SetSlice(obj, a, b, value) (unlikely(!(obj)) ? \
        (PyErr_SetString(PyExc_SystemError, "null argument to internal routine"), -1) : \
        (likely((obj)->ob_type->tp_as_mapping) ? (PySequence_SetSlice(obj, a, b, value)) : \
            (PyErr_Format(PyExc_TypeError, "'%.200s' object doesn't support slice assignment", (obj)->ob_type->tp_name), -1)))
  #define __Pyx_PySequence_DelSlice(obj, a, b) (unlikely(!(obj)) ? \
        (PyErr_SetString(PyExc_SystemError, "null argument to internal routine"), -1) : \
        (likely((obj)->ob_type->tp_as_mapping) ? (PySequence_DelSlice(obj, a, b)) : \
            (PyErr_Format(PyExc_TypeError, "'%.200s' object doesn't support slice deletion", (obj)->ob_type->tp_name), -1)))
#endif
#if PY_MAJOR_VERSION >= 3
  #define PyMethod_New(func, self, klass) ((self) ? PyMethod_New(func, self) : PyInstanceMethod_New(func))
#endif
#if PY_VERSION_HEX < 0x02050000
  #define __Pyx_GetAttrString(o,n)   PyObject_GetAttrString((o),((char *)(n)))
  #define __Pyx_SetAttrString(o,n,a) PyObject_SetAttrString((o),((char *)(n)),(a))
  #define __Pyx_DelAttrString(o,n)   PyObject_DelAttrString((o),((char *)(n)))
#else
  #define __Pyx_GetAttrString(o,n)   PyObject_GetAttrString((o),(n))
  #define __Pyx_SetAttrString(o,n,a) PyObject_SetAttrString((o),(n),(a))
  #define __Pyx_DelAttrString(o,n)   PyObject_DelAttrString((o),(n))
#endif
#if PY_VERSION_HEX < 0x02050000
  #define __Pyx_NAMESTR(n) ((char *)(n))
  #define __Pyx_DOCSTR(n)  ((char *)(n))
#else
  #define __Pyx_NAMESTR(n) (n)
  #define __Pyx_DOCSTR(n)  (n)
#endif
#ifndef CYTHON_INLINE
  #if defined(__GNUC__)
    #define CYTHON_INLINE __inline__
  #elif defined(_MSC_VER)
    #define CYTHON_INLINE __inline
  #elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define CYTHON_INLINE inline
  #else
    #define CYTHON_INLINE
  #endif
#endif
#ifndef CYTHON_RESTRICT
  #if defined(__GNUC__)
    #define CYTHON_RESTRICT __restrict__
  #elif defined(_MSC_VER) && _MSC_VER >= 1400
    #define CYTHON_RESTRICT __restrict
  #elif defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
    #define CYTHON_RESTRICT restrict
  #else
    #define CYTHON_RESTRICT
  #endif
#endif
#ifdef NAN
#define __PYX_NAN() ((float) NAN)
#else
static CYTHON_INLINE float __PYX_NAN() {
  


  float value;
  memset(&value, 0xFF, sizeof(value));
  return value;
}
#endif


#if PY_MAJOR_VERSION >= 3
  #define __Pyx_PyNumber_Divide(x,y)         PyNumber_TrueDivide(x,y)
  #define __Pyx_PyNumber_InPlaceDivide(x,y)  PyNumber_InPlaceTrueDivide(x,y)
#else
  #define __Pyx_PyNumber_Divide(x,y)         PyNumber_Divide(x,y)
  #define __Pyx_PyNumber_InPlaceDivide(x,y)  PyNumber_InPlaceDivide(x,y)
#endif

#ifndef __PYX_EXTERN_C
  #ifdef __cplusplus
    #define __PYX_EXTERN_C extern "C"
  #else
    #define __PYX_EXTERN_C extern
  #endif
#endif

#if defined(WIN32) || defined(MS_WINDOWS)
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#define __PYX_HAVE___yaml
#define __PYX_HAVE_API___yaml
#include "_yaml.h"
#ifdef _OPENMP
#include <omp.h>
#endif 

#ifdef PYREX_WITHOUT_ASSERTIONS
#define CYTHON_WITHOUT_ASSERTIONS
#endif

#ifndef CYTHON_UNUSED
# if defined(__GNUC__)
#   if !(defined(__cplusplus)) || (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#     define CYTHON_UNUSED __attribute__ ((__unused__))
#   else
#     define CYTHON_UNUSED
#   endif
# elif defined(__ICC) || (defined(__INTEL_COMPILER) && !defined(_MSC_VER))
#   define CYTHON_UNUSED __attribute__ ((__unused__))
# else
#   define CYTHON_UNUSED
# endif
#endif
typedef struct {PyObject **p; char *s; const Py_ssize_t n; const char* encoding;
                const char is_unicode; const char is_str; const char intern; } __Pyx_StringTabEntry; 

#define __PYX_DEFAULT_STRING_ENCODING_IS_ASCII 0
#define __PYX_DEFAULT_STRING_ENCODING_IS_DEFAULT 0
#define __PYX_DEFAULT_STRING_ENCODING ""
#define __Pyx_PyObject_FromString __Pyx_PyBytes_FromString
#define __Pyx_PyObject_FromStringAndSize __Pyx_PyBytes_FromStringAndSize
#define __Pyx_fits_Py_ssize_t(v, type, is_signed)  (    \
    (sizeof(type) < sizeof(Py_ssize_t))  ||             \
    (sizeof(type) > sizeof(Py_ssize_t) &&               \
          likely(v < (type)PY_SSIZE_T_MAX ||            \
                 v == (type)PY_SSIZE_T_MAX)  &&         \
          (!is_signed || likely(v > (type)PY_SSIZE_T_MIN ||       \
                                v == (type)PY_SSIZE_T_MIN)))  ||  \
    (sizeof(type) == sizeof(Py_ssize_t) &&              \
          (is_signed || likely(v < (type)PY_SSIZE_T_MAX ||        \
                               v == (type)PY_SSIZE_T_MAX)))  )
static CYTHON_INLINE char* __Pyx_PyObject_AsString(PyObject*);
static CYTHON_INLINE char* __Pyx_PyObject_AsStringAndSize(PyObject*, Py_ssize_t* length);
#define __Pyx_PyByteArray_FromString(s) PyByteArray_FromStringAndSize((const char*)s, strlen((const char*)s))
#define __Pyx_PyByteArray_FromStringAndSize(s, l) PyByteArray_FromStringAndSize((const char*)s, l)
#define __Pyx_PyBytes_FromString        PyBytes_FromString
#define __Pyx_PyBytes_FromStringAndSize PyBytes_FromStringAndSize
static CYTHON_INLINE PyObject* __Pyx_PyUnicode_FromString(char*);
#if PY_MAJOR_VERSION < 3
    #define __Pyx_PyStr_FromString        __Pyx_PyBytes_FromString
    #define __Pyx_PyStr_FromStringAndSize __Pyx_PyBytes_FromStringAndSize
#else
    #define __Pyx_PyStr_FromString        __Pyx_PyUnicode_FromString
    #define __Pyx_PyStr_FromStringAndSize __Pyx_PyUnicode_FromStringAndSize
#endif
#define __Pyx_PyObject_AsSString(s)    ((signed char*) __Pyx_PyObject_AsString(s))
#define __Pyx_PyObject_AsUString(s)    ((unsigned char*) __Pyx_PyObject_AsString(s))
#define __Pyx_PyObject_FromUString(s)  __Pyx_PyObject_FromString((char*)s)
#define __Pyx_PyBytes_FromUString(s)   __Pyx_PyBytes_FromString((char*)s)
#define __Pyx_PyByteArray_FromUString(s)   __Pyx_PyByteArray_FromString((char*)s)
#define __Pyx_PyStr_FromUString(s)     __Pyx_PyStr_FromString((char*)s)
#define __Pyx_PyUnicode_FromUString(s) __Pyx_PyUnicode_FromString((char*)s)
#if PY_MAJOR_VERSION < 3
static CYTHON_INLINE size_t __Pyx_Py_UNICODE_strlen(const Py_UNICODE *u)
{
    const Py_UNICODE *u_end = u;
    while (*u_end++) ;
    return u_end - u - 1;
}
#else
#define __Pyx_Py_UNICODE_strlen Py_UNICODE_strlen
#endif
#define __Pyx_PyUnicode_FromUnicode(u)       PyUnicode_FromUnicode(u, __Pyx_Py_UNICODE_strlen(u))
#define __Pyx_PyUnicode_FromUnicodeAndLength PyUnicode_FromUnicode
#define __Pyx_PyUnicode_AsUnicode            PyUnicode_AsUnicode
#define __Pyx_Owned_Py_None(b) (Py_INCREF(Py_None), Py_None)
#define __Pyx_PyBool_FromLong(b) ((b) ? (Py_INCREF(Py_True), Py_True) : (Py_INCREF(Py_False), Py_False))
static CYTHON_INLINE int __Pyx_PyObject_IsTrue(PyObject*);
static CYTHON_INLINE PyObject* __Pyx_PyNumber_Int(PyObject* x);
static CYTHON_INLINE Py_ssize_t __Pyx_PyIndex_AsSsize_t(PyObject*);
static CYTHON_INLINE PyObject * __Pyx_PyInt_FromSize_t(size_t);
#if CYTHON_COMPILING_IN_CPYTHON
#define __pyx_PyFloat_AsDouble(x) (PyFloat_CheckExact(x) ? PyFloat_AS_DOUBLE(x) : PyFloat_AsDouble(x))
#else
#define __pyx_PyFloat_AsDouble(x) PyFloat_AsDouble(x)
#endif
#define __pyx_PyFloat_AsFloat(x) ((float) __pyx_PyFloat_AsDouble(x))
#if PY_MAJOR_VERSION < 3 && __PYX_DEFAULT_STRING_ENCODING_IS_ASCII
static int __Pyx_sys_getdefaultencoding_not_ascii;
static int __Pyx_init_sys_getdefaultencoding_params(void) {
    PyObject* sys = NULL;
    PyObject* default_encoding = NULL;
    PyObject* ascii_chars_u = NULL;
    PyObject* ascii_chars_b = NULL;
    sys = PyImport_ImportModule("sys");
    if (sys == NULL) goto bad;
    default_encoding = PyObject_CallMethod(sys, (char*) (const char*) "getdefaultencoding", NULL);
    if (default_encoding == NULL) goto bad;
    if (strcmp(PyBytes_AsString(default_encoding), "ascii") == 0) {
        __Pyx_sys_getdefaultencoding_not_ascii = 0;
    } else {
        const char* default_encoding_c = PyBytes_AS_STRING(default_encoding);
        char ascii_chars[128];
        int c;
        for (c = 0; c < 128; c++) {
            ascii_chars[c] = c;
        }
        __Pyx_sys_getdefaultencoding_not_ascii = 1;
        ascii_chars_u = PyUnicode_DecodeASCII(ascii_chars, 128, NULL);
        if (ascii_chars_u == NULL) goto bad;
        ascii_chars_b = PyUnicode_AsEncodedString(ascii_chars_u, default_encoding_c, NULL);
        if (ascii_chars_b == NULL || strncmp(ascii_chars, PyBytes_AS_STRING(ascii_chars_b), 128) != 0) {
            PyErr_Format(
                PyExc_ValueError,
                "This module compiled with c_string_encoding=ascii, but default encoding '%.200s' is not a superset of ascii.",
                default_encoding_c);
            goto bad;
        }
    }
    Py_XDECREF(sys);
    Py_XDECREF(default_encoding);
    Py_XDECREF(ascii_chars_u);
    Py_XDECREF(ascii_chars_b);
    return 0;
bad:
    Py_XDECREF(sys);
    Py_XDECREF(default_encoding);
    Py_XDECREF(ascii_chars_u);
    Py_XDECREF(ascii_chars_b);
    return -1;
}
#endif
#if __PYX_DEFAULT_STRING_ENCODING_IS_DEFAULT && PY_MAJOR_VERSION >= 3
#define __Pyx_PyUnicode_FromStringAndSize(c_str, size) PyUnicode_DecodeUTF8(c_str, size, NULL)
#else
#define __Pyx_PyUnicode_FromStringAndSize(c_str, size) PyUnicode_Decode(c_str, size, __PYX_DEFAULT_STRING_ENCODING, NULL)
#if __PYX_DEFAULT_STRING_ENCODING_IS_DEFAULT
static char* __PYX_DEFAULT_STRING_ENCODING;
static int __Pyx_init_sys_getdefaultencoding_params(void) {
    PyObject* sys = NULL;
    PyObject* default_encoding = NULL;
    char* default_encoding_c;
    sys = PyImport_ImportModule("sys");
    if (sys == NULL) goto bad;
    default_encoding = PyObject_CallMethod(sys, (char*) (const char*) "getdefaultencoding", NULL);
    if (default_encoding == NULL) goto bad;
    default_encoding_c = PyBytes_AS_STRING(default_encoding);
    __PYX_DEFAULT_STRING_ENCODING = (char*) malloc(strlen(default_encoding_c));
    strcpy(__PYX_DEFAULT_STRING_ENCODING, default_encoding_c);
    Py_DECREF(sys);
    Py_DECREF(default_encoding);
    return 0;
bad:
    Py_XDECREF(sys);
    Py_XDECREF(default_encoding);
    return -1;
}
#endif
#endif


#ifdef __GNUC__
  
  #if __GNUC__ > 2 || (__GNUC__ == 2 && (__GNUC_MINOR__ > 95))
    #define likely(x)   __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
  #else 
    #define likely(x)   (x)
    #define unlikely(x) (x)
  #endif 
#else 
  #define likely(x)   (x)
  #define unlikely(x) (x)
#endif 

static PyObject *__pyx_m;
static PyObject *__pyx_d;
static PyObject *__pyx_b;
static PyObject *__pyx_empty_tuple;
static PyObject *__pyx_empty_bytes;
static int __pyx_lineno;
static int __pyx_clineno = 0;
static const char * __pyx_cfilenm= __FILE__;
static const char *__pyx_filename;


static const char *__pyx_f[] = {
  "_yaml.pyx",
};


struct __pyx_obj_5_yaml_Mark;
struct __pyx_obj_5_yaml_CParser;
struct __pyx_obj_5_yaml_CEmitter;








struct __pyx_obj_5_yaml_Mark {
  PyObject_HEAD
  PyObject *name;
  int index;
  int line;
  int column;
  PyObject *buffer;
  PyObject *pointer;
};









struct __pyx_obj_5_yaml_CParser {
  PyObject_HEAD
  struct __pyx_vtabstruct_5_yaml_CParser *__pyx_vtab;
  yaml_parser_t parser;
  yaml_event_t parsed_event;
  PyObject *stream;
  PyObject *stream_name;
  PyObject *current_token;
  PyObject *current_event;
  PyObject *anchors;
  PyObject *stream_cache;
  int stream_cache_len;
  int stream_cache_pos;
  int unicode_source;
};









struct __pyx_obj_5_yaml_CEmitter {
  PyObject_HEAD
  struct __pyx_vtabstruct_5_yaml_CEmitter *__pyx_vtab;
  yaml_emitter_t emitter;
  PyObject *stream;
  int document_start_implicit;
  int document_end_implicit;
  PyObject *use_version;
  PyObject *use_tags;
  PyObject *serialized_nodes;
  PyObject *anchors;
  int last_alias_id;
  int closed;
  int dump_unicode;
  PyObject *use_encoding;
};











struct __pyx_vtabstruct_5_yaml_CParser {
  PyObject *(*_parser_error)(struct __pyx_obj_5_yaml_CParser *);
  PyObject *(*_scan)(struct __pyx_obj_5_yaml_CParser *);
  PyObject *(*_token_to_object)(struct __pyx_obj_5_yaml_CParser *, yaml_token_t *);
  PyObject *(*_parse)(struct __pyx_obj_5_yaml_CParser *);
  PyObject *(*_event_to_object)(struct __pyx_obj_5_yaml_CParser *, yaml_event_t *);
  PyObject *(*_compose_document)(struct __pyx_obj_5_yaml_CParser *);
  PyObject *(*_compose_node)(struct __pyx_obj_5_yaml_CParser *, PyObject *, PyObject *);
  PyObject *(*_compose_scalar_node)(struct __pyx_obj_5_yaml_CParser *, PyObject *);
  PyObject *(*_compose_sequence_node)(struct __pyx_obj_5_yaml_CParser *, PyObject *);
  PyObject *(*_compose_mapping_node)(struct __pyx_obj_5_yaml_CParser *, PyObject *);
  int (*_parse_next_event)(struct __pyx_obj_5_yaml_CParser *);
};
static struct __pyx_vtabstruct_5_yaml_CParser *__pyx_vtabptr_5_yaml_CParser;










struct __pyx_vtabstruct_5_yaml_CEmitter {
  PyObject *(*_emitter_error)(struct __pyx_obj_5_yaml_CEmitter *);
  int (*_object_to_event)(struct __pyx_obj_5_yaml_CEmitter *, PyObject *, yaml_event_t *);
  int (*_anchor_node)(struct __pyx_obj_5_yaml_CEmitter *, PyObject *);
  int (*_serialize_node)(struct __pyx_obj_5_yaml_CEmitter *, PyObject *, PyObject *, PyObject *);
};
static struct __pyx_vtabstruct_5_yaml_CEmitter *__pyx_vtabptr_5_yaml_CEmitter;
#ifndef CYTHON_REFNANNY
  #define CYTHON_REFNANNY 0
#endif
#if CYTHON_REFNANNY
  typedef struct {
    void (*INCREF)(void*, PyObject*, int);
    void (*DECREF)(void*, PyObject*, int);
    void (*GOTREF)(void*, PyObject*, int);
    void (*GIVEREF)(void*, PyObject*, int);
    void* (*SetupContext)(const char*, int, const char*);
    void (*FinishContext)(void**);
  } __Pyx_RefNannyAPIStruct;
  static __Pyx_RefNannyAPIStruct *__Pyx_RefNanny = NULL;
  static __Pyx_RefNannyAPIStruct *__Pyx_RefNannyImportAPI(const char *modname); 
  #define __Pyx_RefNannyDeclarations void *__pyx_refnanny = NULL;
#ifdef WITH_THREAD
  #define __Pyx_RefNannySetupContext(name, acquire_gil) \
          if (acquire_gil) { \
              PyGILState_STATE __pyx_gilstate_save = PyGILState_Ensure(); \
              __pyx_refnanny = __Pyx_RefNanny->SetupContext((name), __LINE__, __FILE__); \
              PyGILState_Release(__pyx_gilstate_save); \
          } else { \
              __pyx_refnanny = __Pyx_RefNanny->SetupContext((name), __LINE__, __FILE__); \
          }
#else
  #define __Pyx_RefNannySetupContext(name, acquire_gil) \
          __pyx_refnanny = __Pyx_RefNanny->SetupContext((name), __LINE__, __FILE__)
#endif
  #define __Pyx_RefNannyFinishContext() \
          __Pyx_RefNanny->FinishContext(&__pyx_refnanny)
  #define __Pyx_INCREF(r)  __Pyx_RefNanny->INCREF(__pyx_refnanny, (PyObject *)(r), __LINE__)
  #define __Pyx_DECREF(r)  __Pyx_RefNanny->DECREF(__pyx_refnanny, (PyObject *)(r), __LINE__)
  #define __Pyx_GOTREF(r)  __Pyx_RefNanny->GOTREF(__pyx_refnanny, (PyObject *)(r), __LINE__)
  #define __Pyx_GIVEREF(r) __Pyx_RefNanny->GIVEREF(__pyx_refnanny, (PyObject *)(r), __LINE__)
  #define __Pyx_XINCREF(r)  do { if((r) != NULL) {__Pyx_INCREF(r); }} while(0)
  #define __Pyx_XDECREF(r)  do { if((r) != NULL) {__Pyx_DECREF(r); }} while(0)
  #define __Pyx_XGOTREF(r)  do { if((r) != NULL) {__Pyx_GOTREF(r); }} while(0)
  #define __Pyx_XGIVEREF(r) do { if((r) != NULL) {__Pyx_GIVEREF(r);}} while(0)
#else
  #define __Pyx_RefNannyDeclarations
  #define __Pyx_RefNannySetupContext(name, acquire_gil)
  #define __Pyx_RefNannyFinishContext()
  #define __Pyx_INCREF(r) Py_INCREF(r)
  #define __Pyx_DECREF(r) Py_DECREF(r)
  #define __Pyx_GOTREF(r)
  #define __Pyx_GIVEREF(r)
  #define __Pyx_XINCREF(r) Py_XINCREF(r)
  #define __Pyx_XDECREF(r) Py_XDECREF(r)
  #define __Pyx_XGOTREF(r)
  #define __Pyx_XGIVEREF(r)
#endif 
#define __Pyx_XDECREF_SET(r, v) do {                            \
        PyObject *tmp = (PyObject *) r;                         \
        r = v; __Pyx_XDECREF(tmp);                              \
    } while (0)
#define __Pyx_DECREF_SET(r, v) do {                             \
        PyObject *tmp = (PyObject *) r;                         \
        r = v; __Pyx_DECREF(tmp);                               \
    } while (0)
#define __Pyx_CLEAR(r)    do { PyObject* tmp = ((PyObject*)(r)); r = NULL; __Pyx_DECREF(tmp);} while(0)
#define __Pyx_XCLEAR(r)   do { if((r) != NULL) {PyObject* tmp = ((PyObject*)(r)); r = NULL; __Pyx_DECREF(tmp);}} while(0)

#if CYTHON_COMPILING_IN_CPYTHON
static CYTHON_INLINE PyObject* __Pyx_PyObject_GetAttrStr(PyObject* obj, PyObject* attr_name) {
    PyTypeObject* tp = Py_TYPE(obj);
    if (likely(tp->tp_getattro))
        return tp->tp_getattro(obj, attr_name);
#if PY_MAJOR_VERSION < 3
    if (likely(tp->tp_getattr))
        return tp->tp_getattr(obj, PyString_AS_STRING(attr_name));
#endif
    return PyObject_GetAttr(obj, attr_name);
}
#else
#define __Pyx_PyObject_GetAttrStr(o,n) PyObject_GetAttr(o,n)
#endif

static PyObject *__Pyx_GetBuiltinName(PyObject *name); 

static void __Pyx_RaiseArgtupleInvalid(const char* func_name, int exact,
    Py_ssize_t num_min, Py_ssize_t num_max, Py_ssize_t num_found); 

static void __Pyx_RaiseDoubleKeywordsError(const char* func_name, PyObject* kw_name); 

static int __Pyx_ParseOptionalKeywords(PyObject *kwds, PyObject **argnames[], \
    PyObject *kwds2, PyObject *values[], Py_ssize_t num_pos_args, \
    const char* function_name); 

static CYTHON_INLINE void __Pyx_ExceptionSave(PyObject **type, PyObject **value, PyObject **tb); 
static void __Pyx_ExceptionReset(PyObject *type, PyObject *value, PyObject *tb); 

static int __Pyx_GetException(PyObject **type, PyObject **value, PyObject **tb); 

#if CYTHON_COMPILING_IN_CPYTHON
static CYTHON_INLINE PyObject* __Pyx_PyObject_Call(PyObject *func, PyObject *arg, PyObject *kw); 
#else
#define __Pyx_PyObject_Call(func, arg, kw) PyObject_Call(func, arg, kw)
#endif

static CYTHON_INLINE void __Pyx_ErrRestore(PyObject *type, PyObject *value, PyObject *tb); 
static CYTHON_INLINE void __Pyx_ErrFetch(PyObject **type, PyObject **value, PyObject **tb); 

static void __Pyx_Raise(PyObject *type, PyObject *value, PyObject *tb, PyObject *cause); 

static CYTHON_INLINE PyObject *__Pyx_GetModuleGlobalName(PyObject *name); 

static CYTHON_INLINE int __Pyx_CheckKeywordStrings(PyObject *kwdict, const char* function_name, int kw_allowed); 

static CYTHON_INLINE int __Pyx_PySequence_Contains(PyObject* item, PyObject* seq, int eq) {
    int result = PySequence_Contains(seq, item);
    return unlikely(result < 0) ? result : (result == (eq == Py_EQ));
}

static CYTHON_INLINE void __Pyx_RaiseUnboundLocalError(const char *varname);

#if CYTHON_COMPILING_IN_CPYTHON
static CYTHON_INLINE int __Pyx_PyList_Append(PyObject* list, PyObject* x) {
    PyListObject* L = (PyListObject*) list;
    Py_ssize_t len = Py_SIZE(list);
    if (likely(L->allocated > len) & likely(len > (L->allocated >> 1))) {
        Py_INCREF(x);
        PyList_SET_ITEM(list, len, x);
        Py_SIZE(list) = len+1;
        return 0;
    }
    return PyList_Append(list, x);
}
#else
#define __Pyx_PyList_Append(L,x) PyList_Append(L,x)
#endif

#if CYTHON_COMPILING_IN_CPYTHON
#define __Pyx_PyObject_DelAttrStr(o,n) __Pyx_PyObject_SetAttrStr(o,n,NULL)
static CYTHON_INLINE int __Pyx_PyObject_SetAttrStr(PyObject* obj, PyObject* attr_name, PyObject* value) {
    PyTypeObject* tp = Py_TYPE(obj);
    if (likely(tp->tp_setattro))
        return tp->tp_setattro(obj, attr_name, value);
#if PY_MAJOR_VERSION < 3
    if (likely(tp->tp_setattr))
        return tp->tp_setattr(obj, PyString_AS_STRING(attr_name), value);
#endif
    return PyObject_SetAttr(obj, attr_name, value);
}
#else
#define __Pyx_PyObject_DelAttrStr(o,n)   PyObject_DelAttr(o,n)
#define __Pyx_PyObject_SetAttrStr(o,n,v) PyObject_SetAttr(o,n,v)
#endif

static CYTHON_INLINE PyObject *__Pyx_GetAttr(PyObject *, PyObject *); 

static CYTHON_INLINE PyObject *__Pyx_GetAttr3(PyObject *, PyObject *, PyObject *); 

#include <string.h>

static CYTHON_INLINE int __Pyx_PyBytes_Equals(PyObject* s1, PyObject* s2, int equals); 

static CYTHON_INLINE int __Pyx_PyUnicode_Equals(PyObject* s1, PyObject* s2, int equals); 

#if PY_MAJOR_VERSION >= 3
#define __Pyx_PyString_Equals __Pyx_PyUnicode_Equals
#else
#define __Pyx_PyString_Equals __Pyx_PyBytes_Equals
#endif

#define __Pyx_GetItemInt(o, i, type, is_signed, to_py_func, is_list, wraparound, boundscheck) \
    (__Pyx_fits_Py_ssize_t(i, type, is_signed) ? \
    __Pyx_GetItemInt_Fast(o, (Py_ssize_t)i, is_list, wraparound, boundscheck) : \
    (is_list ? (PyErr_SetString(PyExc_IndexError, "list index out of range"), (PyObject*)NULL) : \
               __Pyx_GetItemInt_Generic(o, to_py_func(i))))
#define __Pyx_GetItemInt_List(o, i, type, is_signed, to_py_func, is_list, wraparound, boundscheck) \
    (__Pyx_fits_Py_ssize_t(i, type, is_signed) ? \
    __Pyx_GetItemInt_List_Fast(o, (Py_ssize_t)i, wraparound, boundscheck) : \
    (PyErr_SetString(PyExc_IndexError, "list index out of range"), (PyObject*)NULL))
static CYTHON_INLINE PyObject *__Pyx_GetItemInt_List_Fast(PyObject *o, Py_ssize_t i,
                                                              int wraparound, int boundscheck);
#define __Pyx_GetItemInt_Tuple(o, i, type, is_signed, to_py_func, is_list, wraparound, boundscheck) \
    (__Pyx_fits_Py_ssize_t(i, type, is_signed) ? \
    __Pyx_GetItemInt_Tuple_Fast(o, (Py_ssize_t)i, wraparound, boundscheck) : \
    (PyErr_SetString(PyExc_IndexError, "tuple index out of range"), (PyObject*)NULL))
static CYTHON_INLINE PyObject *__Pyx_GetItemInt_Tuple_Fast(PyObject *o, Py_ssize_t i,
                                                              int wraparound, int boundscheck);
static CYTHON_INLINE PyObject *__Pyx_GetItemInt_Generic(PyObject *o, PyObject* j);
static CYTHON_INLINE PyObject *__Pyx_GetItemInt_Fast(PyObject *o, Py_ssize_t i,
                                                     int is_list, int wraparound, int boundscheck);

static CYTHON_INLINE void __Pyx_RaiseTooManyValuesError(Py_ssize_t expected);

static CYTHON_INLINE void __Pyx_RaiseNeedMoreValuesError(Py_ssize_t index);

static CYTHON_INLINE int __Pyx_IterFinish(void); 

static int __Pyx_IternextUnpackEndCheck(PyObject *retval, Py_ssize_t expected); 

static int __Pyx_SetVtable(PyObject *dict, void *vtable); 

static CYTHON_INLINE PyObject* __Pyx_PyInt_From_int(int value);

static PyObject *__Pyx_Import(PyObject *name, PyObject *from_list, int level); 

static CYTHON_INLINE int __Pyx_PyInt_As_int(PyObject *);

static CYTHON_INLINE PyObject* __Pyx_PyInt_From_long(long value);

static CYTHON_INLINE long __Pyx_PyInt_As_long(PyObject *);

static int __Pyx_check_binary_version(void);

typedef struct {
    int code_line;
    PyCodeObject* code_object;
} __Pyx_CodeObjectCacheEntry;
struct __Pyx_CodeObjectCache {
    int count;
    int max_count;
    __Pyx_CodeObjectCacheEntry* entries;
};
static struct __Pyx_CodeObjectCache __pyx_code_cache = {0,0,NULL};
static int __pyx_bisect_code_objects(__Pyx_CodeObjectCacheEntry* entries, int count, int code_line);
static PyCodeObject *__pyx_find_code_object(int code_line);
static void __pyx_insert_code_object(int code_line, PyCodeObject* code_object);

static void __Pyx_AddTraceback(const char *funcname, int c_line,
                               int py_line, const char *filename); 

static int __Pyx_InitStrings(__Pyx_StringTabEntry *t); 



static PyTypeObject *__pyx_ptype_5_yaml_Mark = 0;
static PyTypeObject *__pyx_ptype_5_yaml_CParser = 0;
static PyTypeObject *__pyx_ptype_5_yaml_CEmitter = 0;
static int __pyx_f_5_yaml_input_handler(void *, char *, int, int *); 
static int __pyx_f_5_yaml_output_handler(void *, char *, int); 
#define __Pyx_MODULE_NAME "_yaml"
int __pyx_module_is_main__yaml = 0;


static PyObject *__pyx_builtin_MemoryError;
static PyObject *__pyx_builtin_AttributeError;
static PyObject *__pyx_builtin_TypeError;
static PyObject *__pyx_builtin_ValueError;
static PyObject *__pyx_pf_5_yaml_get_version_string(CYTHON_UNUSED PyObject *__pyx_self); 
static PyObject *__pyx_pf_5_yaml_2get_version(CYTHON_UNUSED PyObject *__pyx_self); 
static int __pyx_pf_5_yaml_4Mark___init__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self, PyObject *__pyx_v_name, int __pyx_v_index, int __pyx_v_line, int __pyx_v_column, PyObject *__pyx_v_buffer, PyObject *__pyx_v_pointer); 
static PyObject *__pyx_pf_5_yaml_4Mark_2get_snippet(CYTHON_UNUSED struct __pyx_obj_5_yaml_Mark *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_4Mark_4__str__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_4Mark_4name___get__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_4Mark_5index___get__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_4Mark_4line___get__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_4Mark_6column___get__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_4Mark_6buffer___get__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_4Mark_7pointer___get__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self); 
static int __pyx_pf_5_yaml_7CParser___init__(struct __pyx_obj_5_yaml_CParser *__pyx_v_self, PyObject *__pyx_v_stream); 
static void __pyx_pf_5_yaml_7CParser_2__dealloc__(struct __pyx_obj_5_yaml_CParser *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_7CParser_4dispose(CYTHON_UNUSED struct __pyx_obj_5_yaml_CParser *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_7CParser_6raw_scan(struct __pyx_obj_5_yaml_CParser *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_7CParser_8get_token(struct __pyx_obj_5_yaml_CParser *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_7CParser_10peek_token(struct __pyx_obj_5_yaml_CParser *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_7CParser_12check_token(struct __pyx_obj_5_yaml_CParser *__pyx_v_self, PyObject *__pyx_v_choices); 
static PyObject *__pyx_pf_5_yaml_7CParser_14raw_parse(struct __pyx_obj_5_yaml_CParser *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_7CParser_16get_event(struct __pyx_obj_5_yaml_CParser *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_7CParser_18peek_event(struct __pyx_obj_5_yaml_CParser *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_7CParser_20check_event(struct __pyx_obj_5_yaml_CParser *__pyx_v_self, PyObject *__pyx_v_choices); 
static PyObject *__pyx_pf_5_yaml_7CParser_22check_node(struct __pyx_obj_5_yaml_CParser *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_7CParser_24get_node(struct __pyx_obj_5_yaml_CParser *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_7CParser_26get_single_node(struct __pyx_obj_5_yaml_CParser *__pyx_v_self); 
static int __pyx_pf_5_yaml_8CEmitter___init__(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self, PyObject *__pyx_v_stream, PyObject *__pyx_v_canonical, PyObject *__pyx_v_indent, PyObject *__pyx_v_width, PyObject *__pyx_v_allow_unicode, PyObject *__pyx_v_line_break, PyObject *__pyx_v_encoding, PyObject *__pyx_v_explicit_start, PyObject *__pyx_v_explicit_end, PyObject *__pyx_v_version, PyObject *__pyx_v_tags); 
static void __pyx_pf_5_yaml_8CEmitter_2__dealloc__(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_8CEmitter_4dispose(CYTHON_UNUSED struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_8CEmitter_6emit(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self, PyObject *__pyx_v_event_object); 
static PyObject *__pyx_pf_5_yaml_8CEmitter_8open(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_8CEmitter_10close(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self); 
static PyObject *__pyx_pf_5_yaml_8CEmitter_12serialize(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self, PyObject *__pyx_v_node); 
static PyObject *__pyx_tp_new_5_yaml_Mark(PyTypeObject *t, PyObject *a, PyObject *k); 
static PyObject *__pyx_tp_new_5_yaml_CParser(PyTypeObject *t, PyObject *a, PyObject *k); 
static PyObject *__pyx_tp_new_5_yaml_CEmitter(PyTypeObject *t, PyObject *a, PyObject *k); 
static char __pyx_k__3[] = "?";
static char __pyx_k__6[] = "";
static char __pyx_k__7[] = "'";
static char __pyx_k__8[] = "\"";
static char __pyx_k__9[] = "|";
static char __pyx_k_TAG[] = "TAG";
static char __pyx_k__10[] = ">";
static char __pyx_k__17[] = "\r";
static char __pyx_k__18[] = "\n";
static char __pyx_k__19[] = "\r\n";
static char __pyx_k_tag[] = "tag";
static char __pyx_k_YAML[] = "YAML";
static char __pyx_k_file[] = "<file>";
static char __pyx_k_line[] = "line";
static char __pyx_k_main[] = "__main__";
static char __pyx_k_name[] = "name";
static char __pyx_k_read[] = "read";
static char __pyx_k_tags[] = "tags";
static char __pyx_k_test[] = "__test__";
static char __pyx_k_yaml[] = "yaml";
static char __pyx_k_class[] = "__class__";
static char __pyx_k_error[] = "error";
static char __pyx_k_index[] = "index";
static char __pyx_k_major[] = "major";
static char __pyx_k_minor[] = "minor";
static char __pyx_k_nodes[] = "nodes";
static char __pyx_k_patch[] = "patch";
static char __pyx_k_style[] = "style";
static char __pyx_k_utf_8[] = "utf-8";
static char __pyx_k_value[] = "value";
static char __pyx_k_width[] = "width";
static char __pyx_k_write[] = "write";
static char __pyx_k_anchor[] = "anchor";
static char __pyx_k_buffer[] = "buffer";
static char __pyx_k_column[] = "column";
static char __pyx_k_events[] = "events";
static char __pyx_k_id_03d[] = "id%03d";
static char __pyx_k_import[] = "__import__";
static char __pyx_k_indent[] = "indent";
static char __pyx_k_parser[] = "parser";
static char __pyx_k_reader[] = "reader";
static char __pyx_k_stream[] = "stream";
static char __pyx_k_strict[] = "strict";
static char __pyx_k_tokens[] = "tokens";
static char __pyx_k_yaml_2[] = "_yaml";
static char __pyx_k_emitter[] = "emitter";
static char __pyx_k_pointer[] = "pointer";
static char __pyx_k_resolve[] = "resolve";
static char __pyx_k_scanner[] = "scanner";
static char __pyx_k_version[] = "version";
static char __pyx_k_KeyToken[] = "KeyToken";
static char __pyx_k_TagToken[] = "TagToken";
static char __pyx_k_composer[] = "composer";
static char __pyx_k_encoding[] = "encoding";
static char __pyx_k_end_mark[] = "end_mark";
static char __pyx_k_explicit[] = "explicit";
static char __pyx_k_implicit[] = "implicit";
static char __pyx_k_TypeError[] = "TypeError";
static char __pyx_k_YAMLError[] = "YAMLError";
static char __pyx_k_canonical[] = "canonical";
static char __pyx_k_utf_16_be[] = "utf-16-be";
static char __pyx_k_utf_16_le[] = "utf-16-le";
static char __pyx_k_AliasEvent[] = "AliasEvent";
static char __pyx_k_AliasToken[] = "AliasToken";
static char __pyx_k_ScalarNode[] = "ScalarNode";
static char __pyx_k_ValueError[] = "ValueError";
static char __pyx_k_ValueToken[] = "ValueToken";
static char __pyx_k_flow_style[] = "flow_style";
static char __pyx_k_line_break[] = "line_break";
static char __pyx_k_pyx_vtable[] = "__pyx_vtable__";
static char __pyx_k_serializer[] = "serializer";
static char __pyx_k_start_mark[] = "start_mark";
static char __pyx_k_AnchorToken[] = "AnchorToken";
static char __pyx_k_MappingNode[] = "MappingNode";
static char __pyx_k_MemoryError[] = "MemoryError";
static char __pyx_k_ParserError[] = "ParserError";
static char __pyx_k_ReaderError[] = "ReaderError";
static char __pyx_k_ScalarEvent[] = "ScalarEvent";
static char __pyx_k_ScalarToken[] = "ScalarToken";
static char __pyx_k_byte_string[] = "<byte string>";
static char __pyx_k_constructor[] = "constructor";
static char __pyx_k_get_version[] = "get_version";
static char __pyx_k_representer[] = "representer";
static char __pyx_k_EmitterError[] = "EmitterError";
static char __pyx_k_ScannerError[] = "ScannerError";
static char __pyx_k_SequenceNode[] = "SequenceNode";
static char __pyx_k_explicit_end[] = "explicit_end";
static char __pyx_k_BlockEndToken[] = "BlockEndToken";
static char __pyx_k_ComposerError[] = "ComposerError";
static char __pyx_k_allow_unicode[] = "allow_unicode";
static char __pyx_k_too_many_tags[] = "too many tags";
static char __pyx_k_AttributeError[] = "AttributeError";
static char __pyx_k_DirectiveToken[] = "DirectiveToken";
static char __pyx_k_FlowEntryToken[] = "FlowEntryToken";
static char __pyx_k_StreamEndEvent[] = "StreamEndEvent";
static char __pyx_k_StreamEndToken[] = "StreamEndToken";
static char __pyx_k_explicit_start[] = "explicit_start";
static char __pyx_k_unicode_string[] = "<unicode string>";
static char __pyx_k_BlockEntryToken[] = "BlockEntryToken";
static char __pyx_k_MappingEndEvent[] = "MappingEndEvent";
static char __pyx_k_SerializerError[] = "SerializerError";
static char __pyx_k_ascend_resolver[] = "ascend_resolver";
static char __pyx_k_invalid_event_s[] = "invalid event %s";
static char __pyx_k_no_parser_error[] = "no parser error";
static char __pyx_k_ConstructorError[] = "ConstructorError";
static char __pyx_k_DocumentEndEvent[] = "DocumentEndEvent";
static char __pyx_k_DocumentEndToken[] = "DocumentEndToken";
static char __pyx_k_RepresenterError[] = "RepresenterError";
static char __pyx_k_SequenceEndEvent[] = "SequenceEndEvent";
static char __pyx_k_StreamStartEvent[] = "StreamStartEvent";
static char __pyx_k_StreamStartToken[] = "StreamStartToken";
static char __pyx_k_descend_resolver[] = "descend_resolver";
static char __pyx_k_no_emitter_error[] = "no emitter error";
static char __pyx_k_second_occurence[] = "second occurence";
static char __pyx_k_MappingStartEvent[] = "MappingStartEvent";
static char __pyx_k_DocumentStartEvent[] = "DocumentStartEvent";
static char __pyx_k_DocumentStartToken[] = "DocumentStartToken";
static char __pyx_k_SequenceStartEvent[] = "SequenceStartEvent";
static char __pyx_k_get_version_string[] = "get_version_string";
static char __pyx_k_unknown_event_type[] = "unknown event type";
static char __pyx_k_unknown_token_type[] = "unknown token type";
static char __pyx_k_FlowMappingEndToken[] = "FlowMappingEndToken";
static char __pyx_k_FlowSequenceEndToken[] = "FlowSequenceEndToken";
static char __pyx_k_in_s_line_d_column_d[] = "  in \"%s\", line %d, column %d";
static char __pyx_k_serializer_is_closed[] = "serializer is closed";
static char __pyx_k_tag_must_be_a_string[] = "tag must be a string";
static char __pyx_k_FlowMappingStartToken[] = "FlowMappingStartToken";
static char __pyx_k_found_undefined_alias[] = "found undefined alias";
static char __pyx_k_BlockMappingStartToken[] = "BlockMappingStartToken";
static char __pyx_k_FlowSequenceStartToken[] = "FlowSequenceStartToken";
static char __pyx_k_value_must_be_a_string[] = "value must be a string";
static char __pyx_k_BlockSequenceStartToken[] = "BlockSequenceStartToken";
static char __pyx_k_anchor_must_be_a_string[] = "anchor must be a string";
static char __pyx_k_serializer_is_not_opened[] = "serializer is not opened";
static char __pyx_k_a_string_value_is_expected[] = "a string value is expected";
static char __pyx_k_but_found_another_document[] = "but found another document";
static char __pyx_k_tag_handle_must_be_a_string[] = "tag handle must be a string";
static char __pyx_k_tag_prefix_must_be_a_string[] = "tag prefix must be a string";
static char __pyx_k_serializer_is_already_opened[] = "serializer is already opened";
static char __pyx_k_root_src_pyyaml_ext__yaml_pyx[] = "/root/src/pyyaml/ext/_yaml.pyx";
static char __pyx_k_a_string_or_stream_input_is_requ[] = "a string or stream input is required";
static char __pyx_k_expected_a_single_document_in_th[] = "expected a single document in the stream";
static char __pyx_k_found_duplicate_anchor_first_occ[] = "found duplicate anchor; first occurence";
static PyObject *__pyx_n_s_AliasEvent;
static PyObject *__pyx_n_s_AliasToken;
static PyObject *__pyx_n_s_AnchorToken;
static PyObject *__pyx_n_s_AttributeError;
static PyObject *__pyx_n_s_BlockEndToken;
static PyObject *__pyx_n_s_BlockEntryToken;
static PyObject *__pyx_n_s_BlockMappingStartToken;
static PyObject *__pyx_n_s_BlockSequenceStartToken;
static PyObject *__pyx_n_s_ComposerError;
static PyObject *__pyx_n_s_ConstructorError;
static PyObject *__pyx_n_s_DirectiveToken;
static PyObject *__pyx_n_s_DocumentEndEvent;
static PyObject *__pyx_n_s_DocumentEndToken;
static PyObject *__pyx_n_s_DocumentStartEvent;
static PyObject *__pyx_n_s_DocumentStartToken;
static PyObject *__pyx_n_s_EmitterError;
static PyObject *__pyx_n_s_FlowEntryToken;
static PyObject *__pyx_n_s_FlowMappingEndToken;
static PyObject *__pyx_n_s_FlowMappingStartToken;
static PyObject *__pyx_n_s_FlowSequenceEndToken;
static PyObject *__pyx_n_s_FlowSequenceStartToken;
static PyObject *__pyx_n_s_KeyToken;
static PyObject *__pyx_n_s_MappingEndEvent;
static PyObject *__pyx_n_s_MappingNode;
static PyObject *__pyx_n_s_MappingStartEvent;
static PyObject *__pyx_n_s_MemoryError;
static PyObject *__pyx_n_s_ParserError;
static PyObject *__pyx_n_s_ReaderError;
static PyObject *__pyx_n_s_RepresenterError;
static PyObject *__pyx_n_s_ScalarEvent;
static PyObject *__pyx_n_s_ScalarNode;
static PyObject *__pyx_n_s_ScalarToken;
static PyObject *__pyx_n_s_ScannerError;
static PyObject *__pyx_n_s_SequenceEndEvent;
static PyObject *__pyx_n_s_SequenceNode;
static PyObject *__pyx_n_s_SequenceStartEvent;
static PyObject *__pyx_n_s_SerializerError;
static PyObject *__pyx_n_s_StreamEndEvent;
static PyObject *__pyx_n_s_StreamEndToken;
static PyObject *__pyx_n_s_StreamStartEvent;
static PyObject *__pyx_n_s_StreamStartToken;
static PyObject *__pyx_n_u_TAG;
static PyObject *__pyx_n_s_TagToken;
static PyObject *__pyx_n_s_TypeError;
static PyObject *__pyx_n_s_ValueError;
static PyObject *__pyx_n_s_ValueToken;
static PyObject *__pyx_n_u_YAML;
static PyObject *__pyx_n_s_YAMLError;
static PyObject *__pyx_kp_s__10;
static PyObject *__pyx_kp_u__10;
static PyObject *__pyx_kp_s__17;
static PyObject *__pyx_kp_s__18;
static PyObject *__pyx_kp_s__19;
static PyObject *__pyx_kp_s__3;
static PyObject *__pyx_kp_u__3;
static PyObject *__pyx_kp_u__6;
static PyObject *__pyx_kp_s__7;
static PyObject *__pyx_kp_u__7;
static PyObject *__pyx_kp_s__8;
static PyObject *__pyx_kp_u__8;
static PyObject *__pyx_kp_s__9;
static PyObject *__pyx_kp_u__9;
static PyObject *__pyx_kp_s_a_string_or_stream_input_is_requ;
static PyObject *__pyx_kp_u_a_string_or_stream_input_is_requ;
static PyObject *__pyx_kp_s_a_string_value_is_expected;
static PyObject *__pyx_kp_u_a_string_value_is_expected;
static PyObject *__pyx_n_s_allow_unicode;
static PyObject *__pyx_n_s_anchor;
static PyObject *__pyx_kp_s_anchor_must_be_a_string;
static PyObject *__pyx_kp_u_anchor_must_be_a_string;
static PyObject *__pyx_n_s_ascend_resolver;
static PyObject *__pyx_n_s_buffer;
static PyObject *__pyx_kp_s_but_found_another_document;
static PyObject *__pyx_kp_u_but_found_another_document;
static PyObject *__pyx_kp_s_byte_string;
static PyObject *__pyx_kp_u_byte_string;
static PyObject *__pyx_n_s_canonical;
static PyObject *__pyx_n_s_class;
static PyObject *__pyx_n_s_column;
static PyObject *__pyx_n_s_composer;
static PyObject *__pyx_n_s_constructor;
static PyObject *__pyx_n_s_descend_resolver;
static PyObject *__pyx_n_s_emitter;
static PyObject *__pyx_n_s_encoding;
static PyObject *__pyx_n_u_encoding;
static PyObject *__pyx_n_s_end_mark;
static PyObject *__pyx_n_s_error;
static PyObject *__pyx_n_s_events;
static PyObject *__pyx_kp_s_expected_a_single_document_in_th;
static PyObject *__pyx_kp_u_expected_a_single_document_in_th;
static PyObject *__pyx_n_s_explicit;
static PyObject *__pyx_n_s_explicit_end;
static PyObject *__pyx_n_s_explicit_start;
static PyObject *__pyx_kp_s_file;
static PyObject *__pyx_kp_u_file;
static PyObject *__pyx_n_s_flow_style;
static PyObject *__pyx_kp_s_found_duplicate_anchor_first_occ;
static PyObject *__pyx_kp_u_found_duplicate_anchor_first_occ;
static PyObject *__pyx_kp_s_found_undefined_alias;
static PyObject *__pyx_kp_u_found_undefined_alias;
static PyObject *__pyx_n_s_get_version;
static PyObject *__pyx_n_s_get_version_string;
static PyObject *__pyx_kp_u_id_03d;
static PyObject *__pyx_n_s_implicit;
static PyObject *__pyx_n_s_import;
static PyObject *__pyx_kp_s_in_s_line_d_column_d;
static PyObject *__pyx_n_s_indent;
static PyObject *__pyx_n_s_index;
static PyObject *__pyx_kp_s_invalid_event_s;
static PyObject *__pyx_kp_u_invalid_event_s;
static PyObject *__pyx_n_s_line;
static PyObject *__pyx_n_s_line_break;
static PyObject *__pyx_n_s_main;
static PyObject *__pyx_n_s_major;
static PyObject *__pyx_n_s_minor;
static PyObject *__pyx_n_s_name;
static PyObject *__pyx_kp_s_no_emitter_error;
static PyObject *__pyx_kp_u_no_emitter_error;
static PyObject *__pyx_kp_s_no_parser_error;
static PyObject *__pyx_kp_u_no_parser_error;
static PyObject *__pyx_n_s_nodes;
static PyObject *__pyx_n_s_parser;
static PyObject *__pyx_n_s_patch;
static PyObject *__pyx_n_s_pointer;
static PyObject *__pyx_n_s_pyx_vtable;
static PyObject *__pyx_n_s_read;
static PyObject *__pyx_n_s_reader;
static PyObject *__pyx_n_s_representer;
static PyObject *__pyx_n_s_resolve;
static PyObject *__pyx_kp_s_root_src_pyyaml_ext__yaml_pyx;
static PyObject *__pyx_n_s_scanner;
static PyObject *__pyx_kp_s_second_occurence;
static PyObject *__pyx_kp_u_second_occurence;
static PyObject *__pyx_n_s_serializer;
static PyObject *__pyx_kp_s_serializer_is_already_opened;
static PyObject *__pyx_kp_u_serializer_is_already_opened;
static PyObject *__pyx_kp_s_serializer_is_closed;
static PyObject *__pyx_kp_u_serializer_is_closed;
static PyObject *__pyx_kp_s_serializer_is_not_opened;
static PyObject *__pyx_kp_u_serializer_is_not_opened;
static PyObject *__pyx_n_s_start_mark;
static PyObject *__pyx_n_s_stream;
static PyObject *__pyx_n_s_style;
static PyObject *__pyx_n_s_tag;
static PyObject *__pyx_kp_s_tag_handle_must_be_a_string;
static PyObject *__pyx_kp_u_tag_handle_must_be_a_string;
static PyObject *__pyx_kp_s_tag_must_be_a_string;
static PyObject *__pyx_kp_u_tag_must_be_a_string;
static PyObject *__pyx_kp_s_tag_prefix_must_be_a_string;
static PyObject *__pyx_kp_u_tag_prefix_must_be_a_string;
static PyObject *__pyx_n_s_tags;
static PyObject *__pyx_n_s_test;
static PyObject *__pyx_n_s_tokens;
static PyObject *__pyx_kp_s_too_many_tags;
static PyObject *__pyx_kp_u_too_many_tags;
static PyObject *__pyx_kp_s_unicode_string;
static PyObject *__pyx_kp_u_unicode_string;
static PyObject *__pyx_kp_s_unknown_event_type;
static PyObject *__pyx_kp_u_unknown_event_type;
static PyObject *__pyx_kp_s_unknown_token_type;
static PyObject *__pyx_kp_u_unknown_token_type;
static PyObject *__pyx_kp_s_utf_16_be;
static PyObject *__pyx_kp_u_utf_16_be;
static PyObject *__pyx_kp_s_utf_16_le;
static PyObject *__pyx_kp_u_utf_16_le;
static PyObject *__pyx_kp_u_utf_8;
static PyObject *__pyx_n_s_value;
static PyObject *__pyx_kp_s_value_must_be_a_string;
static PyObject *__pyx_kp_u_value_must_be_a_string;
static PyObject *__pyx_n_s_version;
static PyObject *__pyx_n_s_width;
static PyObject *__pyx_n_s_write;
static PyObject *__pyx_n_s_yaml;
static PyObject *__pyx_n_s_yaml_2;
static PyObject *__pyx_int_0;
static PyObject *__pyx_int_1;
static PyObject *__pyx_tuple_;
static PyObject *__pyx_tuple__2;
static PyObject *__pyx_tuple__4;
static PyObject *__pyx_tuple__5;
static PyObject *__pyx_tuple__11;
static PyObject *__pyx_tuple__12;
static PyObject *__pyx_tuple__13;
static PyObject *__pyx_tuple__14;
static PyObject *__pyx_tuple__15;
static PyObject *__pyx_tuple__16;
static PyObject *__pyx_tuple__20;
static PyObject *__pyx_tuple__21;
static PyObject *__pyx_tuple__22;
static PyObject *__pyx_tuple__23;
static PyObject *__pyx_tuple__24;
static PyObject *__pyx_tuple__25;
static PyObject *__pyx_tuple__26;
static PyObject *__pyx_tuple__27;
static PyObject *__pyx_tuple__28;
static PyObject *__pyx_tuple__29;
static PyObject *__pyx_tuple__30;
static PyObject *__pyx_tuple__31;
static PyObject *__pyx_tuple__32;
static PyObject *__pyx_tuple__33;
static PyObject *__pyx_tuple__34;
static PyObject *__pyx_tuple__35;
static PyObject *__pyx_tuple__36;
static PyObject *__pyx_tuple__37;
static PyObject *__pyx_tuple__38;
static PyObject *__pyx_tuple__39;
static PyObject *__pyx_tuple__40;
static PyObject *__pyx_tuple__41;
static PyObject *__pyx_tuple__42;
static PyObject *__pyx_tuple__43;
static PyObject *__pyx_tuple__44;
static PyObject *__pyx_tuple__45;
static PyObject *__pyx_tuple__46;
static PyObject *__pyx_tuple__47;
static PyObject *__pyx_tuple__48;
static PyObject *__pyx_tuple__49;
static PyObject *__pyx_tuple__50;
static PyObject *__pyx_tuple__51;
static PyObject *__pyx_tuple__52;
static PyObject *__pyx_tuple__53;
static PyObject *__pyx_tuple__54;
static PyObject *__pyx_tuple__55;
static PyObject *__pyx_tuple__56;
static PyObject *__pyx_tuple__57;
static PyObject *__pyx_tuple__58;
static PyObject *__pyx_tuple__59;
static PyObject *__pyx_tuple__60;
static PyObject *__pyx_tuple__61;
static PyObject *__pyx_tuple__62;
static PyObject *__pyx_tuple__63;
static PyObject *__pyx_tuple__64;
static PyObject *__pyx_tuple__65;
static PyObject *__pyx_tuple__66;
static PyObject *__pyx_tuple__67;
static PyObject *__pyx_tuple__68;
static PyObject *__pyx_tuple__69;
static PyObject *__pyx_tuple__70;
static PyObject *__pyx_tuple__71;
static PyObject *__pyx_tuple__72;
static PyObject *__pyx_tuple__74;
static PyObject *__pyx_codeobj__73;
static PyObject *__pyx_codeobj__75;










static PyObject *__pyx_pw_5_yaml_1get_version_string(PyObject *__pyx_self, CYTHON_UNUSED PyObject *unused); 
static PyMethodDef __pyx_mdef_5_yaml_1get_version_string = {__Pyx_NAMESTR("get_version_string"), (PyCFunction)__pyx_pw_5_yaml_1get_version_string, METH_NOARGS, __Pyx_DOCSTR(0)};
static PyObject *__pyx_pw_5_yaml_1get_version_string(PyObject *__pyx_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("get_version_string (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_get_version_string(__pyx_self);

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_get_version_string(CYTHON_UNUSED PyObject *__pyx_self) {
  char *__pyx_v_value;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  PyObject *__pyx_t_2 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("get_version_string", 0);

  






  __pyx_v_value = yaml_get_version_string();

  






  __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
  if (__pyx_t_1) {

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_2 = __Pyx_PyBytes_FromString(__pyx_v_value); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 8; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_r = __pyx_t_2;
    __pyx_t_2 = 0;
    goto __pyx_L0;
  }
   {

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_2 = PyUnicode_FromString(__pyx_v_value); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 10; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_r = __pyx_t_2;
    __pyx_t_2 = 0;
    goto __pyx_L0;
  }

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_2);
  __Pyx_AddTraceback("_yaml.get_version_string", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_3get_version(PyObject *__pyx_self, CYTHON_UNUSED PyObject *unused); 
static PyMethodDef __pyx_mdef_5_yaml_3get_version = {__Pyx_NAMESTR("get_version"), (PyCFunction)__pyx_pw_5_yaml_3get_version, METH_NOARGS, __Pyx_DOCSTR(0)};
static PyObject *__pyx_pw_5_yaml_3get_version(PyObject *__pyx_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("get_version (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_2get_version(__pyx_self);

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_2get_version(CYTHON_UNUSED PyObject *__pyx_self) {
  int __pyx_v_major;
  int __pyx_v_minor;
  int __pyx_v_patch;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  PyObject *__pyx_t_1 = NULL;
  PyObject *__pyx_t_2 = NULL;
  PyObject *__pyx_t_3 = NULL;
  PyObject *__pyx_t_4 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("get_version", 0);

  






  yaml_get_version((&__pyx_v_major), (&__pyx_v_minor), (&__pyx_v_patch));

  






  __Pyx_XDECREF(__pyx_r);
  __pyx_t_1 = __Pyx_PyInt_From_int(__pyx_v_major); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 15; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyInt_From_int(__pyx_v_minor); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 15; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_patch); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 15; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __pyx_t_4 = PyTuple_New(3); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 15; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_4);
  PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_t_1);
  __Pyx_GIVEREF(__pyx_t_1);
  PyTuple_SET_ITEM(__pyx_t_4, 1, __pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  PyTuple_SET_ITEM(__pyx_t_4, 2, __pyx_t_3);
  __Pyx_GIVEREF(__pyx_t_3);
  __pyx_t_1 = 0;
  __pyx_t_2 = 0;
  __pyx_t_3 = 0;
  __pyx_r = __pyx_t_4;
  __pyx_t_4 = 0;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_XDECREF(__pyx_t_2);
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_XDECREF(__pyx_t_4);
  __Pyx_AddTraceback("_yaml.get_version", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static int __pyx_pw_5_yaml_4Mark_1__init__(PyObject *__pyx_v_self, PyObject *__pyx_args, PyObject *__pyx_kwds); 
static int __pyx_pw_5_yaml_4Mark_1__init__(PyObject *__pyx_v_self, PyObject *__pyx_args, PyObject *__pyx_kwds) {
  PyObject *__pyx_v_name = 0;
  int __pyx_v_index;
  int __pyx_v_line;
  int __pyx_v_column;
  PyObject *__pyx_v_buffer = 0;
  PyObject *__pyx_v_pointer = 0;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  int __pyx_r;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__init__ (wrapper)", 0);
  {
    static PyObject **__pyx_pyargnames[] = {&__pyx_n_s_name,&__pyx_n_s_index,&__pyx_n_s_line,&__pyx_n_s_column,&__pyx_n_s_buffer,&__pyx_n_s_pointer,0};
    PyObject* values[6] = {0,0,0,0,0,0};
    if (unlikely(__pyx_kwds)) {
      Py_ssize_t kw_args;
      const Py_ssize_t pos_args = PyTuple_GET_SIZE(__pyx_args);
      switch (pos_args) {
        case  6: values[5] = PyTuple_GET_ITEM(__pyx_args, 5);
        case  5: values[4] = PyTuple_GET_ITEM(__pyx_args, 4);
        case  4: values[3] = PyTuple_GET_ITEM(__pyx_args, 3);
        case  3: values[2] = PyTuple_GET_ITEM(__pyx_args, 2);
        case  2: values[1] = PyTuple_GET_ITEM(__pyx_args, 1);
        case  1: values[0] = PyTuple_GET_ITEM(__pyx_args, 0);
        case  0: break;
        default: goto __pyx_L5_argtuple_error;
      }
      kw_args = PyDict_Size(__pyx_kwds);
      switch (pos_args) {
        case  0:
        if (likely((values[0] = PyDict_GetItem(__pyx_kwds, __pyx_n_s_name)) != 0)) kw_args--;
        else goto __pyx_L5_argtuple_error;
        case  1:
        if (likely((values[1] = PyDict_GetItem(__pyx_kwds, __pyx_n_s_index)) != 0)) kw_args--;
        else {
          __Pyx_RaiseArgtupleInvalid("__init__", 1, 6, 6, 1); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 72; __pyx_clineno = __LINE__; goto __pyx_L3_error;}
        }
        case  2:
        if (likely((values[2] = PyDict_GetItem(__pyx_kwds, __pyx_n_s_line)) != 0)) kw_args--;
        else {
          __Pyx_RaiseArgtupleInvalid("__init__", 1, 6, 6, 2); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 72; __pyx_clineno = __LINE__; goto __pyx_L3_error;}
        }
        case  3:
        if (likely((values[3] = PyDict_GetItem(__pyx_kwds, __pyx_n_s_column)) != 0)) kw_args--;
        else {
          __Pyx_RaiseArgtupleInvalid("__init__", 1, 6, 6, 3); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 72; __pyx_clineno = __LINE__; goto __pyx_L3_error;}
        }
        case  4:
        if (likely((values[4] = PyDict_GetItem(__pyx_kwds, __pyx_n_s_buffer)) != 0)) kw_args--;
        else {
          __Pyx_RaiseArgtupleInvalid("__init__", 1, 6, 6, 4); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 72; __pyx_clineno = __LINE__; goto __pyx_L3_error;}
        }
        case  5:
        if (likely((values[5] = PyDict_GetItem(__pyx_kwds, __pyx_n_s_pointer)) != 0)) kw_args--;
        else {
          __Pyx_RaiseArgtupleInvalid("__init__", 1, 6, 6, 5); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 72; __pyx_clineno = __LINE__; goto __pyx_L3_error;}
        }
      }
      if (unlikely(kw_args > 0)) {
        if (unlikely(__Pyx_ParseOptionalKeywords(__pyx_kwds, __pyx_pyargnames, 0, values, pos_args, "__init__") < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 72; __pyx_clineno = __LINE__; goto __pyx_L3_error;}
      }
    } else if (PyTuple_GET_SIZE(__pyx_args) != 6) {
      goto __pyx_L5_argtuple_error;
    } else {
      values[0] = PyTuple_GET_ITEM(__pyx_args, 0);
      values[1] = PyTuple_GET_ITEM(__pyx_args, 1);
      values[2] = PyTuple_GET_ITEM(__pyx_args, 2);
      values[3] = PyTuple_GET_ITEM(__pyx_args, 3);
      values[4] = PyTuple_GET_ITEM(__pyx_args, 4);
      values[5] = PyTuple_GET_ITEM(__pyx_args, 5);
    }
    __pyx_v_name = values[0];
    __pyx_v_index = __Pyx_PyInt_As_int(values[1]); if (unlikely((__pyx_v_index == (int)-1) && PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 72; __pyx_clineno = __LINE__; goto __pyx_L3_error;}
    __pyx_v_line = __Pyx_PyInt_As_int(values[2]); if (unlikely((__pyx_v_line == (int)-1) && PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 72; __pyx_clineno = __LINE__; goto __pyx_L3_error;}
    __pyx_v_column = __Pyx_PyInt_As_int(values[3]); if (unlikely((__pyx_v_column == (int)-1) && PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 72; __pyx_clineno = __LINE__; goto __pyx_L3_error;}
    __pyx_v_buffer = values[4];
    __pyx_v_pointer = values[5];
  }
  goto __pyx_L4_argument_unpacking_done;
  __pyx_L5_argtuple_error:;
  __Pyx_RaiseArgtupleInvalid("__init__", 1, 6, 6, PyTuple_GET_SIZE(__pyx_args)); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 72; __pyx_clineno = __LINE__; goto __pyx_L3_error;}
  __pyx_L3_error:;
  __Pyx_AddTraceback("_yaml.Mark.__init__", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __Pyx_RefNannyFinishContext();
  return -1;
  __pyx_L4_argument_unpacking_done:;
  __pyx_r = __pyx_pf_5_yaml_4Mark___init__(((struct __pyx_obj_5_yaml_Mark *)__pyx_v_self), __pyx_v_name, __pyx_v_index, __pyx_v_line, __pyx_v_column, __pyx_v_buffer, __pyx_v_pointer);

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static int __pyx_pf_5_yaml_4Mark___init__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self, PyObject *__pyx_v_name, int __pyx_v_index, int __pyx_v_line, int __pyx_v_column, PyObject *__pyx_v_buffer, PyObject *__pyx_v_pointer) {
  int __pyx_r;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__init__", 0);

  






  __Pyx_INCREF(__pyx_v_name);
  __Pyx_GIVEREF(__pyx_v_name);
  __Pyx_GOTREF(__pyx_v_self->name);
  __Pyx_DECREF(__pyx_v_self->name);
  __pyx_v_self->name = __pyx_v_name;

  






  __pyx_v_self->index = __pyx_v_index;

  






  __pyx_v_self->line = __pyx_v_line;

  






  __pyx_v_self->column = __pyx_v_column;

  






  __Pyx_INCREF(__pyx_v_buffer);
  __Pyx_GIVEREF(__pyx_v_buffer);
  __Pyx_GOTREF(__pyx_v_self->buffer);
  __Pyx_DECREF(__pyx_v_self->buffer);
  __pyx_v_self->buffer = __pyx_v_buffer;

  






  __Pyx_INCREF(__pyx_v_pointer);
  __Pyx_GIVEREF(__pyx_v_pointer);
  __Pyx_GOTREF(__pyx_v_self->pointer);
  __Pyx_DECREF(__pyx_v_self->pointer);
  __pyx_v_self->pointer = __pyx_v_pointer;

  







  
  __pyx_r = 0;
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_4Mark_3get_snippet(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused); 
static PyObject *__pyx_pw_5_yaml_4Mark_3get_snippet(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("get_snippet (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_4Mark_2get_snippet(((struct __pyx_obj_5_yaml_Mark *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_4Mark_2get_snippet(CYTHON_UNUSED struct __pyx_obj_5_yaml_Mark *__pyx_v_self) {
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("get_snippet", 0);

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(Py_None);
  __pyx_r = Py_None;
  goto __pyx_L0;

  







  
  __pyx_L0:;
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_4Mark_5__str__(PyObject *__pyx_v_self); 
static PyObject *__pyx_pw_5_yaml_4Mark_5__str__(PyObject *__pyx_v_self) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__str__ (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_4Mark_4__str__(((struct __pyx_obj_5_yaml_Mark *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_4Mark_4__str__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self) {
  PyObject *__pyx_v_where = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  PyObject *__pyx_t_1 = NULL;
  PyObject *__pyx_t_2 = NULL;
  PyObject *__pyx_t_3 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("__str__", 0);

  






  __pyx_t_1 = __Pyx_PyInt_From_long((__pyx_v_self->line + 1)); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 86; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyInt_From_long((__pyx_v_self->column + 1)); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 86; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __pyx_t_3 = PyTuple_New(3); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 86; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __Pyx_INCREF(__pyx_v_self->name);
  PyTuple_SET_ITEM(__pyx_t_3, 0, __pyx_v_self->name);
  __Pyx_GIVEREF(__pyx_v_self->name);
  PyTuple_SET_ITEM(__pyx_t_3, 1, __pyx_t_1);
  __Pyx_GIVEREF(__pyx_t_1);
  PyTuple_SET_ITEM(__pyx_t_3, 2, __pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  __pyx_t_1 = 0;
  __pyx_t_2 = 0;
  __pyx_t_2 = __Pyx_PyString_Format(__pyx_kp_s_in_s_line_d_column_d, __pyx_t_3); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 86; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
  __pyx_v_where = ((PyObject*)__pyx_t_2);
  __pyx_t_2 = 0;

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(__pyx_v_where);
  __pyx_r = __pyx_v_where;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_XDECREF(__pyx_t_2);
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_AddTraceback("_yaml.Mark.__str__", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_where);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_4Mark_4name_1__get__(PyObject *__pyx_v_self); 
static PyObject *__pyx_pw_5_yaml_4Mark_4name_1__get__(PyObject *__pyx_v_self) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__get__ (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_4Mark_4name___get__(((struct __pyx_obj_5_yaml_Mark *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_4Mark_4name___get__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self) {
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__get__", 0);
  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(__pyx_v_self->name);
  __pyx_r = __pyx_v_self->name;
  goto __pyx_L0;

  
  __pyx_L0:;
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_4Mark_5index_1__get__(PyObject *__pyx_v_self); 
static PyObject *__pyx_pw_5_yaml_4Mark_5index_1__get__(PyObject *__pyx_v_self) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__get__ (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_4Mark_5index___get__(((struct __pyx_obj_5_yaml_Mark *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_4Mark_5index___get__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self) {
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  PyObject *__pyx_t_1 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("__get__", 0);
  __Pyx_XDECREF(__pyx_r);
  __pyx_t_1 = __Pyx_PyInt_From_int(__pyx_v_self->index); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 66; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_r = __pyx_t_1;
  __pyx_t_1 = 0;
  goto __pyx_L0;

  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_AddTraceback("_yaml.Mark.index.__get__", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_4Mark_4line_1__get__(PyObject *__pyx_v_self); 
static PyObject *__pyx_pw_5_yaml_4Mark_4line_1__get__(PyObject *__pyx_v_self) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__get__ (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_4Mark_4line___get__(((struct __pyx_obj_5_yaml_Mark *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_4Mark_4line___get__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self) {
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  PyObject *__pyx_t_1 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("__get__", 0);
  __Pyx_XDECREF(__pyx_r);
  __pyx_t_1 = __Pyx_PyInt_From_int(__pyx_v_self->line); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 67; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_r = __pyx_t_1;
  __pyx_t_1 = 0;
  goto __pyx_L0;

  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_AddTraceback("_yaml.Mark.line.__get__", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_4Mark_6column_1__get__(PyObject *__pyx_v_self); 
static PyObject *__pyx_pw_5_yaml_4Mark_6column_1__get__(PyObject *__pyx_v_self) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__get__ (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_4Mark_6column___get__(((struct __pyx_obj_5_yaml_Mark *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_4Mark_6column___get__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self) {
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  PyObject *__pyx_t_1 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("__get__", 0);
  __Pyx_XDECREF(__pyx_r);
  __pyx_t_1 = __Pyx_PyInt_From_int(__pyx_v_self->column); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 68; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_r = __pyx_t_1;
  __pyx_t_1 = 0;
  goto __pyx_L0;

  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_AddTraceback("_yaml.Mark.column.__get__", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_4Mark_6buffer_1__get__(PyObject *__pyx_v_self); 
static PyObject *__pyx_pw_5_yaml_4Mark_6buffer_1__get__(PyObject *__pyx_v_self) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__get__ (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_4Mark_6buffer___get__(((struct __pyx_obj_5_yaml_Mark *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_4Mark_6buffer___get__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self) {
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__get__", 0);
  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(__pyx_v_self->buffer);
  __pyx_r = __pyx_v_self->buffer;
  goto __pyx_L0;

  
  __pyx_L0:;
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_4Mark_7pointer_1__get__(PyObject *__pyx_v_self); 
static PyObject *__pyx_pw_5_yaml_4Mark_7pointer_1__get__(PyObject *__pyx_v_self) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__get__ (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_4Mark_7pointer___get__(((struct __pyx_obj_5_yaml_Mark *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_4Mark_7pointer___get__(struct __pyx_obj_5_yaml_Mark *__pyx_v_self) {
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__get__", 0);
  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(__pyx_v_self->pointer);
  __pyx_r = __pyx_v_self->pointer;
  goto __pyx_L0;

  
  __pyx_L0:;
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static int __pyx_pw_5_yaml_7CParser_1__init__(PyObject *__pyx_v_self, PyObject *__pyx_args, PyObject *__pyx_kwds); 
static int __pyx_pw_5_yaml_7CParser_1__init__(PyObject *__pyx_v_self, PyObject *__pyx_args, PyObject *__pyx_kwds) {
  PyObject *__pyx_v_stream = 0;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  int __pyx_r;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__init__ (wrapper)", 0);
  {
    static PyObject **__pyx_pyargnames[] = {&__pyx_n_s_stream,0};
    PyObject* values[1] = {0};
    if (unlikely(__pyx_kwds)) {
      Py_ssize_t kw_args;
      const Py_ssize_t pos_args = PyTuple_GET_SIZE(__pyx_args);
      switch (pos_args) {
        case  1: values[0] = PyTuple_GET_ITEM(__pyx_args, 0);
        case  0: break;
        default: goto __pyx_L5_argtuple_error;
      }
      kw_args = PyDict_Size(__pyx_kwds);
      switch (pos_args) {
        case  0:
        if (likely((values[0] = PyDict_GetItem(__pyx_kwds, __pyx_n_s_stream)) != 0)) kw_args--;
        else goto __pyx_L5_argtuple_error;
      }
      if (unlikely(kw_args > 0)) {
        if (unlikely(__Pyx_ParseOptionalKeywords(__pyx_kwds, __pyx_pyargnames, 0, values, pos_args, "__init__") < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 262; __pyx_clineno = __LINE__; goto __pyx_L3_error;}
      }
    } else if (PyTuple_GET_SIZE(__pyx_args) != 1) {
      goto __pyx_L5_argtuple_error;
    } else {
      values[0] = PyTuple_GET_ITEM(__pyx_args, 0);
    }
    __pyx_v_stream = values[0];
  }
  goto __pyx_L4_argument_unpacking_done;
  __pyx_L5_argtuple_error:;
  __Pyx_RaiseArgtupleInvalid("__init__", 1, 1, 1, PyTuple_GET_SIZE(__pyx_args)); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 262; __pyx_clineno = __LINE__; goto __pyx_L3_error;}
  __pyx_L3_error:;
  __Pyx_AddTraceback("_yaml.CParser.__init__", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __Pyx_RefNannyFinishContext();
  return -1;
  __pyx_L4_argument_unpacking_done:;
  __pyx_r = __pyx_pf_5_yaml_7CParser___init__(((struct __pyx_obj_5_yaml_CParser *)__pyx_v_self), __pyx_v_stream);

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static int __pyx_pf_5_yaml_7CParser___init__(struct __pyx_obj_5_yaml_CParser *__pyx_v_self, PyObject *__pyx_v_stream) {
  PyObject *__pyx_v_is_readable = 0;
  int __pyx_r;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  PyObject *__pyx_t_2 = NULL;
  PyObject *__pyx_t_3 = NULL;
  PyObject *__pyx_t_4 = NULL;
  PyObject *__pyx_t_5 = NULL;
  int __pyx_t_6;
  PyObject *__pyx_t_7 = NULL;
  PyObject *__pyx_t_8 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("__init__", 0);
  __Pyx_INCREF(__pyx_v_stream);

  






  __pyx_t_1 = ((yaml_parser_initialize((&__pyx_v_self->parser)) == 0) != 0);
  if (__pyx_t_1) {

    






    PyErr_NoMemory(); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 265; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  }

  






  __pyx_v_self->parsed_event.type = YAML_NO_EVENT;

  






  __Pyx_INCREF(__pyx_int_1);
  __pyx_v_is_readable = __pyx_int_1;

  






  {
    __Pyx_ExceptionSave(&__pyx_t_2, &__pyx_t_3, &__pyx_t_4);
    __Pyx_XGOTREF(__pyx_t_2);
    __Pyx_XGOTREF(__pyx_t_3);
    __Pyx_XGOTREF(__pyx_t_4);
     {

      






      __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_stream, __pyx_n_s_read); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 269; __pyx_clineno = __LINE__; goto __pyx_L4_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
    }
    __Pyx_XDECREF(__pyx_t_2); __pyx_t_2 = 0;
    __Pyx_XDECREF(__pyx_t_3); __pyx_t_3 = 0;
    __Pyx_XDECREF(__pyx_t_4); __pyx_t_4 = 0;
    goto __pyx_L11_try_end;
    __pyx_L4_error:;
    __Pyx_XDECREF(__pyx_t_5); __pyx_t_5 = 0;

    






    __pyx_t_6 = PyErr_ExceptionMatches(__pyx_builtin_AttributeError);
    if (__pyx_t_6) {
      __Pyx_AddTraceback("_yaml.CParser.__init__", __pyx_clineno, __pyx_lineno, __pyx_filename);
      if (__Pyx_GetException(&__pyx_t_5, &__pyx_t_7, &__pyx_t_8) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 270; __pyx_clineno = __LINE__; goto __pyx_L6_except_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_GOTREF(__pyx_t_7);
      __Pyx_GOTREF(__pyx_t_8);

      






      __Pyx_INCREF(__pyx_int_0);
      __Pyx_DECREF_SET(__pyx_v_is_readable, __pyx_int_0);
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;
      __Pyx_DECREF(__pyx_t_8); __pyx_t_8 = 0;
      goto __pyx_L5_exception_handled;
    }
    goto __pyx_L6_except_error;
    __pyx_L6_except_error:;
    __Pyx_XGIVEREF(__pyx_t_2);
    __Pyx_XGIVEREF(__pyx_t_3);
    __Pyx_XGIVEREF(__pyx_t_4);
    __Pyx_ExceptionReset(__pyx_t_2, __pyx_t_3, __pyx_t_4);
    goto __pyx_L1_error;
    __pyx_L5_exception_handled:;
    __Pyx_XGIVEREF(__pyx_t_2);
    __Pyx_XGIVEREF(__pyx_t_3);
    __Pyx_XGIVEREF(__pyx_t_4);
    __Pyx_ExceptionReset(__pyx_t_2, __pyx_t_3, __pyx_t_4);
    __pyx_L11_try_end:;
  }

  






  __pyx_v_self->unicode_source = 0;

  






  __pyx_t_1 = __Pyx_PyObject_IsTrue(__pyx_v_is_readable); if (unlikely(__pyx_t_1 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 273; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  if (__pyx_t_1) {

    






    __Pyx_INCREF(__pyx_v_stream);
    __Pyx_GIVEREF(__pyx_v_stream);
    __Pyx_GOTREF(__pyx_v_self->stream);
    __Pyx_DECREF(__pyx_v_self->stream);
    __pyx_v_self->stream = __pyx_v_stream;

    






    {
      __Pyx_ExceptionSave(&__pyx_t_4, &__pyx_t_3, &__pyx_t_2);
      __Pyx_XGOTREF(__pyx_t_4);
      __Pyx_XGOTREF(__pyx_t_3);
      __Pyx_XGOTREF(__pyx_t_2);
       {

        






        __pyx_t_8 = __Pyx_PyObject_GetAttrStr(__pyx_v_stream, __pyx_n_s_name); if (unlikely(!__pyx_t_8)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 276; __pyx_clineno = __LINE__; goto __pyx_L15_error;}
        __Pyx_GOTREF(__pyx_t_8);
        __Pyx_GIVEREF(__pyx_t_8);
        __Pyx_GOTREF(__pyx_v_self->stream_name);
        __Pyx_DECREF(__pyx_v_self->stream_name);
        __pyx_v_self->stream_name = __pyx_t_8;
        __pyx_t_8 = 0;
      }
      __Pyx_XDECREF(__pyx_t_4); __pyx_t_4 = 0;
      __Pyx_XDECREF(__pyx_t_3); __pyx_t_3 = 0;
      __Pyx_XDECREF(__pyx_t_2); __pyx_t_2 = 0;
      goto __pyx_L22_try_end;
      __pyx_L15_error:;
      __Pyx_XDECREF(__pyx_t_5); __pyx_t_5 = 0;
      __Pyx_XDECREF(__pyx_t_7); __pyx_t_7 = 0;
      __Pyx_XDECREF(__pyx_t_8); __pyx_t_8 = 0;

      






      __pyx_t_6 = PyErr_ExceptionMatches(__pyx_builtin_AttributeError);
      if (__pyx_t_6) {
        __Pyx_AddTraceback("_yaml.CParser.__init__", __pyx_clineno, __pyx_lineno, __pyx_filename);
        if (__Pyx_GetException(&__pyx_t_8, &__pyx_t_7, &__pyx_t_5) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 277; __pyx_clineno = __LINE__; goto __pyx_L17_except_error;}
        __Pyx_GOTREF(__pyx_t_8);
        __Pyx_GOTREF(__pyx_t_7);
        __Pyx_GOTREF(__pyx_t_5);

        






        __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
        if (__pyx_t_1) {

          






          __Pyx_INCREF(__pyx_kp_s_file);
          __Pyx_GIVEREF(__pyx_kp_s_file);
          __Pyx_GOTREF(__pyx_v_self->stream_name);
          __Pyx_DECREF(__pyx_v_self->stream_name);
          __pyx_v_self->stream_name = __pyx_kp_s_file;
          goto __pyx_L25;
        }
         {

          






          __Pyx_INCREF(__pyx_kp_u_file);
          __Pyx_GIVEREF(__pyx_kp_u_file);
          __Pyx_GOTREF(__pyx_v_self->stream_name);
          __Pyx_DECREF(__pyx_v_self->stream_name);
          __pyx_v_self->stream_name = __pyx_kp_u_file;
        }
        __pyx_L25:;
        __Pyx_DECREF(__pyx_t_8); __pyx_t_8 = 0;
        __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;
        __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
        goto __pyx_L16_exception_handled;
      }
      goto __pyx_L17_except_error;
      __pyx_L17_except_error:;
      __Pyx_XGIVEREF(__pyx_t_4);
      __Pyx_XGIVEREF(__pyx_t_3);
      __Pyx_XGIVEREF(__pyx_t_2);
      __Pyx_ExceptionReset(__pyx_t_4, __pyx_t_3, __pyx_t_2);
      goto __pyx_L1_error;
      __pyx_L16_exception_handled:;
      __Pyx_XGIVEREF(__pyx_t_4);
      __Pyx_XGIVEREF(__pyx_t_3);
      __Pyx_XGIVEREF(__pyx_t_2);
      __Pyx_ExceptionReset(__pyx_t_4, __pyx_t_3, __pyx_t_2);
      __pyx_L22_try_end:;
    }

    






    __Pyx_INCREF(Py_None);
    __Pyx_GIVEREF(Py_None);
    __Pyx_GOTREF(__pyx_v_self->stream_cache);
    __Pyx_DECREF(__pyx_v_self->stream_cache);
    __pyx_v_self->stream_cache = Py_None;

    






    __pyx_v_self->stream_cache_len = 0;

    






    __pyx_v_self->stream_cache_pos = 0;

    






    yaml_parser_set_input((&__pyx_v_self->parser), __pyx_f_5_yaml_input_handler, ((void *)__pyx_v_self));
    goto __pyx_L14;
  }
   {

    






    __pyx_t_1 = ((PyUnicode_CheckExact(__pyx_v_stream) != 0) != 0);
    if (__pyx_t_1) {

      






      __pyx_t_5 = PyUnicode_AsUTF8String(__pyx_v_stream); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 288; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_DECREF_SET(__pyx_v_stream, __pyx_t_5);
      __pyx_t_5 = 0;

      






      __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
      if (__pyx_t_1) {

        






        __Pyx_INCREF(__pyx_kp_s_unicode_string);
        __Pyx_GIVEREF(__pyx_kp_s_unicode_string);
        __Pyx_GOTREF(__pyx_v_self->stream_name);
        __Pyx_DECREF(__pyx_v_self->stream_name);
        __pyx_v_self->stream_name = __pyx_kp_s_unicode_string;
        goto __pyx_L27;
      }
       {

        






        __Pyx_INCREF(__pyx_kp_u_unicode_string);
        __Pyx_GIVEREF(__pyx_kp_u_unicode_string);
        __Pyx_GOTREF(__pyx_v_self->stream_name);
        __Pyx_DECREF(__pyx_v_self->stream_name);
        __pyx_v_self->stream_name = __pyx_kp_u_unicode_string;
      }
      __pyx_L27:;

      






      __pyx_v_self->unicode_source = 1;
      goto __pyx_L26;
    }
     {

      






      __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
      if (__pyx_t_1) {

        






        __Pyx_INCREF(__pyx_kp_s_byte_string);
        __Pyx_GIVEREF(__pyx_kp_s_byte_string);
        __Pyx_GOTREF(__pyx_v_self->stream_name);
        __Pyx_DECREF(__pyx_v_self->stream_name);
        __pyx_v_self->stream_name = __pyx_kp_s_byte_string;
        goto __pyx_L28;
      }
       {

        






        __Pyx_INCREF(__pyx_kp_u_byte_string);
        __Pyx_GIVEREF(__pyx_kp_u_byte_string);
        __Pyx_GOTREF(__pyx_v_self->stream_name);
        __Pyx_DECREF(__pyx_v_self->stream_name);
        __pyx_v_self->stream_name = __pyx_kp_u_byte_string;
      }
      __pyx_L28:;
    }
    __pyx_L26:;

    






    __pyx_t_1 = ((PyString_CheckExact(__pyx_v_stream) == 0) != 0);
    if (__pyx_t_1) {

      






      __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
      if (__pyx_t_1) {

        






        __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple_, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 301; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_Raise(__pyx_t_5, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 301; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
       {

        






        __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__2, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 303; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_Raise(__pyx_t_5, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 303; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
    }

    






    __Pyx_INCREF(__pyx_v_stream);
    __Pyx_GIVEREF(__pyx_v_stream);
    __Pyx_GOTREF(__pyx_v_self->stream);
    __Pyx_DECREF(__pyx_v_self->stream);
    __pyx_v_self->stream = __pyx_v_stream;

    






    yaml_parser_set_input_string((&__pyx_v_self->parser), PyString_AS_STRING(__pyx_v_stream), PyString_GET_SIZE(__pyx_v_stream));
  }
  __pyx_L14:;

  






  __Pyx_INCREF(Py_None);
  __Pyx_GIVEREF(Py_None);
  __Pyx_GOTREF(__pyx_v_self->current_token);
  __Pyx_DECREF(__pyx_v_self->current_token);
  __pyx_v_self->current_token = Py_None;

  






  __Pyx_INCREF(Py_None);
  __Pyx_GIVEREF(Py_None);
  __Pyx_GOTREF(__pyx_v_self->current_event);
  __Pyx_DECREF(__pyx_v_self->current_event);
  __pyx_v_self->current_event = Py_None;

  






  __pyx_t_5 = PyDict_New(); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 308; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_5);
  __Pyx_GIVEREF(__pyx_t_5);
  __Pyx_GOTREF(__pyx_v_self->anchors);
  __Pyx_DECREF(__pyx_v_self->anchors);
  __pyx_v_self->anchors = __pyx_t_5;
  __pyx_t_5 = 0;

  







  
  __pyx_r = 0;
  goto __pyx_L0;
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_5);
  __Pyx_XDECREF(__pyx_t_7);
  __Pyx_XDECREF(__pyx_t_8);
  __Pyx_AddTraceback("_yaml.CParser.__init__", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = -1;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_is_readable);
  __Pyx_XDECREF(__pyx_v_stream);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static void __pyx_pw_5_yaml_7CParser_3__dealloc__(PyObject *__pyx_v_self); 
static void __pyx_pw_5_yaml_7CParser_3__dealloc__(PyObject *__pyx_v_self) {
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__dealloc__ (wrapper)", 0);
  __pyx_pf_5_yaml_7CParser_2__dealloc__(((struct __pyx_obj_5_yaml_CParser *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
}

static void __pyx_pf_5_yaml_7CParser_2__dealloc__(struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__dealloc__", 0);

  






  yaml_parser_delete((&__pyx_v_self->parser));

  






  yaml_event_delete((&__pyx_v_self->parsed_event));

  







  
  __Pyx_RefNannyFinishContext();
}










static PyObject *__pyx_pw_5_yaml_7CParser_5dispose(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused); 
static PyObject *__pyx_pw_5_yaml_7CParser_5dispose(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("dispose (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_7CParser_4dispose(((struct __pyx_obj_5_yaml_CParser *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_7CParser_4dispose(CYTHON_UNUSED struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("dispose", 0);

  
  __pyx_r = Py_None; __Pyx_INCREF(Py_None);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static PyObject *__pyx_f_5_yaml_7CParser__parser_error(struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  PyObject *__pyx_v_context_mark = NULL;
  PyObject *__pyx_v_problem_mark = NULL;
  PyObject *__pyx_v_context = NULL;
  PyObject *__pyx_v_problem = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  PyObject *__pyx_t_2 = NULL;
  PyObject *__pyx_t_3 = NULL;
  PyObject *__pyx_t_4 = NULL;
  PyObject *__pyx_t_5 = NULL;
  PyObject *__pyx_t_6 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("_parser_error", 0);

  






  switch (__pyx_v_self->parser.error) {

    






    case YAML_MEMORY_ERROR:

    






    __Pyx_XDECREF(__pyx_r);
    __Pyx_INCREF(__pyx_builtin_MemoryError);
    __pyx_r = __pyx_builtin_MemoryError;
    goto __pyx_L0;
    break;

    






    case YAML_READER_ERROR:

    






    __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
    if (__pyx_t_1) {

      






      __Pyx_XDECREF(__pyx_r);
      __pyx_t_2 = __Pyx_GetModuleGlobalName(__pyx_n_s_ReaderError); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 322; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_self->parser.problem_offset); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 322; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);

      






      __pyx_t_4 = __Pyx_PyInt_From_int(__pyx_v_self->parser.problem_value); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 323; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);
      __pyx_t_5 = __Pyx_PyBytes_FromString(__pyx_v_self->parser.problem); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 323; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);

      






      __pyx_t_6 = PyTuple_New(5); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 322; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);
      __Pyx_INCREF(__pyx_v_self->stream_name);
      PyTuple_SET_ITEM(__pyx_t_6, 0, __pyx_v_self->stream_name);
      __Pyx_GIVEREF(__pyx_v_self->stream_name);
      PyTuple_SET_ITEM(__pyx_t_6, 1, __pyx_t_3);
      __Pyx_GIVEREF(__pyx_t_3);
      PyTuple_SET_ITEM(__pyx_t_6, 2, __pyx_t_4);
      __Pyx_GIVEREF(__pyx_t_4);
      __Pyx_INCREF(__pyx_kp_s__3);
      PyTuple_SET_ITEM(__pyx_t_6, 3, __pyx_kp_s__3);
      __Pyx_GIVEREF(__pyx_kp_s__3);
      PyTuple_SET_ITEM(__pyx_t_6, 4, __pyx_t_5);
      __Pyx_GIVEREF(__pyx_t_5);
      __pyx_t_3 = 0;
      __pyx_t_4 = 0;
      __pyx_t_5 = 0;
      __pyx_t_5 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_t_6, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 322; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
      __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
      __pyx_r = __pyx_t_5;
      __pyx_t_5 = 0;
      goto __pyx_L0;
    }
     {

      






      __Pyx_XDECREF(__pyx_r);
      __pyx_t_5 = __Pyx_GetModuleGlobalName(__pyx_n_s_ReaderError); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 325; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __pyx_t_6 = __Pyx_PyInt_From_int(__pyx_v_self->parser.problem_offset); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 325; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);

      






      __pyx_t_2 = __Pyx_PyInt_From_int(__pyx_v_self->parser.problem_value); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 326; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __pyx_t_4 = PyUnicode_FromString(__pyx_v_self->parser.problem); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 326; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);

      






      __pyx_t_3 = PyTuple_New(5); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 325; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __Pyx_INCREF(__pyx_v_self->stream_name);
      PyTuple_SET_ITEM(__pyx_t_3, 0, __pyx_v_self->stream_name);
      __Pyx_GIVEREF(__pyx_v_self->stream_name);
      PyTuple_SET_ITEM(__pyx_t_3, 1, __pyx_t_6);
      __Pyx_GIVEREF(__pyx_t_6);
      PyTuple_SET_ITEM(__pyx_t_3, 2, __pyx_t_2);
      __Pyx_GIVEREF(__pyx_t_2);
      __Pyx_INCREF(__pyx_kp_u__3);
      PyTuple_SET_ITEM(__pyx_t_3, 3, __pyx_kp_u__3);
      __Pyx_GIVEREF(__pyx_kp_u__3);
      PyTuple_SET_ITEM(__pyx_t_3, 4, __pyx_t_4);
      __Pyx_GIVEREF(__pyx_t_4);
      __pyx_t_6 = 0;
      __pyx_t_2 = 0;
      __pyx_t_4 = 0;
      __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_5, __pyx_t_3, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 325; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
      __pyx_r = __pyx_t_4;
      __pyx_t_4 = 0;
      goto __pyx_L0;
    }
    break;

    






    case YAML_SCANNER_ERROR:
    case YAML_PARSER_ERROR:

    






    __Pyx_INCREF(Py_None);
    __pyx_v_context_mark = Py_None;

    






    __Pyx_INCREF(Py_None);
    __pyx_v_problem_mark = Py_None;

    






    __pyx_t_1 = ((__pyx_v_self->parser.context != NULL) != 0);
    if (__pyx_t_1) {

      






      __pyx_t_4 = __Pyx_PyInt_From_int(__pyx_v_self->parser.context_mark.index); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 333; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);

      






      __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_self->parser.context_mark.line); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 334; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);

      






      __pyx_t_5 = __Pyx_PyInt_From_int(__pyx_v_self->parser.context_mark.column); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 335; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);

      






      __pyx_t_2 = PyTuple_New(6); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 332; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __Pyx_INCREF(__pyx_v_self->stream_name);
      PyTuple_SET_ITEM(__pyx_t_2, 0, __pyx_v_self->stream_name);
      __Pyx_GIVEREF(__pyx_v_self->stream_name);
      PyTuple_SET_ITEM(__pyx_t_2, 1, __pyx_t_4);
      __Pyx_GIVEREF(__pyx_t_4);
      PyTuple_SET_ITEM(__pyx_t_2, 2, __pyx_t_3);
      __Pyx_GIVEREF(__pyx_t_3);
      PyTuple_SET_ITEM(__pyx_t_2, 3, __pyx_t_5);
      __Pyx_GIVEREF(__pyx_t_5);
      __Pyx_INCREF(Py_None);
      PyTuple_SET_ITEM(__pyx_t_2, 4, Py_None);
      __Pyx_GIVEREF(Py_None);
      __Pyx_INCREF(Py_None);
      PyTuple_SET_ITEM(__pyx_t_2, 5, Py_None);
      __Pyx_GIVEREF(Py_None);
      __pyx_t_4 = 0;
      __pyx_t_3 = 0;
      __pyx_t_5 = 0;
      __pyx_t_5 = __Pyx_PyObject_Call(((PyObject *)((PyObject*)__pyx_ptype_5_yaml_Mark)), __pyx_t_2, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 332; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
      __Pyx_DECREF_SET(__pyx_v_context_mark, __pyx_t_5);
      __pyx_t_5 = 0;
      goto __pyx_L4;
    }
    __pyx_L4:;

    






    __pyx_t_1 = ((__pyx_v_self->parser.problem != NULL) != 0);
    if (__pyx_t_1) {

      






      __pyx_t_5 = __Pyx_PyInt_From_int(__pyx_v_self->parser.problem_mark.index); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 338; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);

      






      __pyx_t_2 = __Pyx_PyInt_From_int(__pyx_v_self->parser.problem_mark.line); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 339; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);

      






      __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_self->parser.problem_mark.column); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 340; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);

      






      __pyx_t_4 = PyTuple_New(6); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 337; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);
      __Pyx_INCREF(__pyx_v_self->stream_name);
      PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_v_self->stream_name);
      __Pyx_GIVEREF(__pyx_v_self->stream_name);
      PyTuple_SET_ITEM(__pyx_t_4, 1, __pyx_t_5);
      __Pyx_GIVEREF(__pyx_t_5);
      PyTuple_SET_ITEM(__pyx_t_4, 2, __pyx_t_2);
      __Pyx_GIVEREF(__pyx_t_2);
      PyTuple_SET_ITEM(__pyx_t_4, 3, __pyx_t_3);
      __Pyx_GIVEREF(__pyx_t_3);
      __Pyx_INCREF(Py_None);
      PyTuple_SET_ITEM(__pyx_t_4, 4, Py_None);
      __Pyx_GIVEREF(Py_None);
      __Pyx_INCREF(Py_None);
      PyTuple_SET_ITEM(__pyx_t_4, 5, Py_None);
      __Pyx_GIVEREF(Py_None);
      __pyx_t_5 = 0;
      __pyx_t_2 = 0;
      __pyx_t_3 = 0;
      __pyx_t_3 = __Pyx_PyObject_Call(((PyObject *)((PyObject*)__pyx_ptype_5_yaml_Mark)), __pyx_t_4, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 337; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
      __Pyx_DECREF_SET(__pyx_v_problem_mark, __pyx_t_3);
      __pyx_t_3 = 0;
      goto __pyx_L5;
    }
    __pyx_L5:;

    






    __Pyx_INCREF(Py_None);
    __pyx_v_context = Py_None;

    






    __pyx_t_1 = ((__pyx_v_self->parser.context != NULL) != 0);
    if (__pyx_t_1) {

      






      __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
      if (__pyx_t_1) {

        






        __pyx_t_3 = __Pyx_PyBytes_FromString(__pyx_v_self->parser.context); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 344; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_3);
        __Pyx_DECREF_SET(__pyx_v_context, __pyx_t_3);
        __pyx_t_3 = 0;
        goto __pyx_L7;
      }
       {

        






        __pyx_t_3 = PyUnicode_FromString(__pyx_v_self->parser.context); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 346; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_3);
        __Pyx_DECREF_SET(__pyx_v_context, __pyx_t_3);
        __pyx_t_3 = 0;
      }
      __pyx_L7:;
      goto __pyx_L6;
    }
    __pyx_L6:;

    






    __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
    if (__pyx_t_1) {

      






      __pyx_t_3 = __Pyx_PyBytes_FromString(__pyx_v_self->parser.problem); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 348; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __pyx_v_problem = __pyx_t_3;
      __pyx_t_3 = 0;
      goto __pyx_L8;
    }
     {

      






      __pyx_t_3 = PyUnicode_FromString(__pyx_v_self->parser.problem); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 350; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __pyx_v_problem = __pyx_t_3;
      __pyx_t_3 = 0;
    }
    __pyx_L8:;

    






    __pyx_t_1 = ((__pyx_v_self->parser.error == YAML_SCANNER_ERROR) != 0);
    if (__pyx_t_1) {

      






      __Pyx_XDECREF(__pyx_r);
      __pyx_t_3 = __Pyx_GetModuleGlobalName(__pyx_n_s_ScannerError); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 352; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __pyx_t_4 = PyTuple_New(4); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 352; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);
      __Pyx_INCREF(__pyx_v_context);
      PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_v_context);
      __Pyx_GIVEREF(__pyx_v_context);
      __Pyx_INCREF(__pyx_v_context_mark);
      PyTuple_SET_ITEM(__pyx_t_4, 1, __pyx_v_context_mark);
      __Pyx_GIVEREF(__pyx_v_context_mark);
      __Pyx_INCREF(__pyx_v_problem);
      PyTuple_SET_ITEM(__pyx_t_4, 2, __pyx_v_problem);
      __Pyx_GIVEREF(__pyx_v_problem);
      __Pyx_INCREF(__pyx_v_problem_mark);
      PyTuple_SET_ITEM(__pyx_t_4, 3, __pyx_v_problem_mark);
      __Pyx_GIVEREF(__pyx_v_problem_mark);
      __pyx_t_2 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_t_4, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 352; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
      __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
      __pyx_r = __pyx_t_2;
      __pyx_t_2 = 0;
      goto __pyx_L0;
    }
     {

      






      __Pyx_XDECREF(__pyx_r);
      __pyx_t_2 = __Pyx_GetModuleGlobalName(__pyx_n_s_ParserError); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 354; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __pyx_t_4 = PyTuple_New(4); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 354; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);
      __Pyx_INCREF(__pyx_v_context);
      PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_v_context);
      __Pyx_GIVEREF(__pyx_v_context);
      __Pyx_INCREF(__pyx_v_context_mark);
      PyTuple_SET_ITEM(__pyx_t_4, 1, __pyx_v_context_mark);
      __Pyx_GIVEREF(__pyx_v_context_mark);
      __Pyx_INCREF(__pyx_v_problem);
      PyTuple_SET_ITEM(__pyx_t_4, 2, __pyx_v_problem);
      __Pyx_GIVEREF(__pyx_v_problem);
      __Pyx_INCREF(__pyx_v_problem_mark);
      PyTuple_SET_ITEM(__pyx_t_4, 3, __pyx_v_problem_mark);
      __Pyx_GIVEREF(__pyx_v_problem_mark);
      __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_t_4, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 354; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
      __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
      __pyx_r = __pyx_t_3;
      __pyx_t_3 = 0;
      goto __pyx_L0;
    }
    break;
    default: break;
  }

  






  __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
  if (__pyx_t_1) {

    






    __pyx_t_3 = __Pyx_PyObject_Call(__pyx_builtin_ValueError, __pyx_tuple__4, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 356; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_Raise(__pyx_t_3, 0, 0, 0);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    {__pyx_filename = __pyx_f[0]; __pyx_lineno = 356; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  }
   {

    






    __pyx_t_3 = __Pyx_PyObject_Call(__pyx_builtin_ValueError, __pyx_tuple__5, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 358; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_Raise(__pyx_t_3, 0, 0, 0);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    {__pyx_filename = __pyx_f[0]; __pyx_lineno = 358; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  }

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_2);
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_XDECREF(__pyx_t_4);
  __Pyx_XDECREF(__pyx_t_5);
  __Pyx_XDECREF(__pyx_t_6);
  __Pyx_AddTraceback("_yaml.CParser._parser_error", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_context_mark);
  __Pyx_XDECREF(__pyx_v_problem_mark);
  __Pyx_XDECREF(__pyx_v_context);
  __Pyx_XDECREF(__pyx_v_problem);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_7CParser_7raw_scan(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused); 
static PyObject *__pyx_pw_5_yaml_7CParser_7raw_scan(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("raw_scan (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_7CParser_6raw_scan(((struct __pyx_obj_5_yaml_CParser *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_7CParser_6raw_scan(struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  yaml_token_t __pyx_v_token;
  int __pyx_v_done;
  int __pyx_v_count;
  PyObject *__pyx_v_error = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("raw_scan", 0);

  






  __pyx_v_count = 0;

  






  __pyx_v_done = 0;

  






  while (1) {
    __pyx_t_1 = ((__pyx_v_done == 0) != 0);
    if (!__pyx_t_1) break;

    






    __pyx_t_2 = yaml_parser_scan((&__pyx_v_self->parser), (&__pyx_v_token)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 367; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __pyx_t_1 = ((__pyx_t_2 == 0) != 0);
    if (__pyx_t_1) {

      






      __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parser_error(__pyx_v_self); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 368; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __pyx_v_error = __pyx_t_3;
      __pyx_t_3 = 0;

      






      __Pyx_Raise(__pyx_v_error, 0, 0, 0);
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 369; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }

    






    __pyx_t_1 = ((__pyx_v_token.type == YAML_NO_TOKEN) != 0);
    if (__pyx_t_1) {

      






      __pyx_v_done = 1;
      goto __pyx_L6;
    }
     {

      






      __pyx_v_count = (__pyx_v_count + 1);
    }
    __pyx_L6:;

    






    yaml_token_delete((&__pyx_v_token));
  }

  






  __Pyx_XDECREF(__pyx_r);
  __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_count); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 375; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __pyx_r = __pyx_t_3;
  __pyx_t_3 = 0;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_AddTraceback("_yaml.CParser.raw_scan", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_error);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static PyObject *__pyx_f_5_yaml_7CParser__scan(struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  yaml_token_t __pyx_v_token;
  PyObject *__pyx_v_error = NULL;
  PyObject *__pyx_v_token_object = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("_scan", 0);

  






  __pyx_t_1 = yaml_parser_scan((&__pyx_v_self->parser), (&__pyx_v_token)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 379; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_t_2 = ((__pyx_t_1 == 0) != 0);
  if (__pyx_t_2) {

    






    __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parser_error(__pyx_v_self); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 380; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_v_error = __pyx_t_3;
    __pyx_t_3 = 0;

    






    __Pyx_Raise(__pyx_v_error, 0, 0, 0);
    {__pyx_filename = __pyx_f[0]; __pyx_lineno = 381; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  }

  






  __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_token_to_object(__pyx_v_self, (&__pyx_v_token)); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 382; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __pyx_v_token_object = __pyx_t_3;
  __pyx_t_3 = 0;

  






  yaml_token_delete((&__pyx_v_token));

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(__pyx_v_token_object);
  __pyx_r = __pyx_v_token_object;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_AddTraceback("_yaml.CParser._scan", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_error);
  __Pyx_XDECREF(__pyx_v_token_object);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static PyObject *__pyx_f_5_yaml_7CParser__token_to_object(struct __pyx_obj_5_yaml_CParser *__pyx_v_self, yaml_token_t *__pyx_v_token) {
  struct __pyx_obj_5_yaml_Mark *__pyx_v_start_mark = NULL;
  struct __pyx_obj_5_yaml_Mark *__pyx_v_end_mark = NULL;
  PyObject *__pyx_v_encoding = NULL;
  PyObject *__pyx_v_handle = NULL;
  PyObject *__pyx_v_prefix = NULL;
  PyObject *__pyx_v_value = NULL;
  PyObject *__pyx_v_suffix = NULL;
  int __pyx_v_plain;
  PyObject *__pyx_v_style = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  PyObject *__pyx_t_1 = NULL;
  PyObject *__pyx_t_2 = NULL;
  PyObject *__pyx_t_3 = NULL;
  PyObject *__pyx_t_4 = NULL;
  int __pyx_t_5;
  int __pyx_t_6;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("_token_to_object", 0);

  






  __pyx_t_1 = __Pyx_PyInt_From_int(__pyx_v_token->start_mark.index); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 388; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);

  






  __pyx_t_2 = __Pyx_PyInt_From_int(__pyx_v_token->start_mark.line); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 389; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);

  






  __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_token->start_mark.column); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 390; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);

  






  __pyx_t_4 = PyTuple_New(6); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 387; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_4);
  __Pyx_INCREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_v_self->stream_name);
  __Pyx_GIVEREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_4, 1, __pyx_t_1);
  __Pyx_GIVEREF(__pyx_t_1);
  PyTuple_SET_ITEM(__pyx_t_4, 2, __pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  PyTuple_SET_ITEM(__pyx_t_4, 3, __pyx_t_3);
  __Pyx_GIVEREF(__pyx_t_3);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_4, 4, Py_None);
  __Pyx_GIVEREF(Py_None);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_4, 5, Py_None);
  __Pyx_GIVEREF(Py_None);
  __pyx_t_1 = 0;
  __pyx_t_2 = 0;
  __pyx_t_3 = 0;
  __pyx_t_3 = __Pyx_PyObject_Call(((PyObject *)((PyObject*)__pyx_ptype_5_yaml_Mark)), __pyx_t_4, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 387; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
  __pyx_v_start_mark = ((struct __pyx_obj_5_yaml_Mark *)__pyx_t_3);
  __pyx_t_3 = 0;

  






  __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_token->end_mark.index); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 393; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);

  






  __pyx_t_4 = __Pyx_PyInt_From_int(__pyx_v_token->end_mark.line); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 394; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_4);

  






  __pyx_t_2 = __Pyx_PyInt_From_int(__pyx_v_token->end_mark.column); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 395; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);

  






  __pyx_t_1 = PyTuple_New(6); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 392; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_INCREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_1, 0, __pyx_v_self->stream_name);
  __Pyx_GIVEREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_1, 1, __pyx_t_3);
  __Pyx_GIVEREF(__pyx_t_3);
  PyTuple_SET_ITEM(__pyx_t_1, 2, __pyx_t_4);
  __Pyx_GIVEREF(__pyx_t_4);
  PyTuple_SET_ITEM(__pyx_t_1, 3, __pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_1, 4, Py_None);
  __Pyx_GIVEREF(Py_None);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_1, 5, Py_None);
  __Pyx_GIVEREF(Py_None);
  __pyx_t_3 = 0;
  __pyx_t_4 = 0;
  __pyx_t_2 = 0;
  __pyx_t_2 = __Pyx_PyObject_Call(((PyObject *)((PyObject*)__pyx_ptype_5_yaml_Mark)), __pyx_t_1, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 392; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_v_end_mark = ((struct __pyx_obj_5_yaml_Mark *)__pyx_t_2);
  __pyx_t_2 = 0;

  






  switch (__pyx_v_token->type) {

    






    case YAML_NO_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __Pyx_INCREF(Py_None);
    __pyx_r = Py_None;
    goto __pyx_L0;
    break;

    






    case YAML_STREAM_START_TOKEN:

    






    __Pyx_INCREF(Py_None);
    __pyx_v_encoding = Py_None;

    






    __pyx_t_5 = ((__pyx_v_token->data.stream_start.encoding == YAML_UTF8_ENCODING) != 0);
    if (__pyx_t_5) {

      






      __pyx_t_5 = ((__pyx_v_self->unicode_source == 0) != 0);
      if (__pyx_t_5) {

        






        __Pyx_INCREF(__pyx_kp_u_utf_8);
        __Pyx_DECREF_SET(__pyx_v_encoding, __pyx_kp_u_utf_8);
        goto __pyx_L4;
      }
      __pyx_L4:;
      goto __pyx_L3;
    }

    






    __pyx_t_5 = ((__pyx_v_token->data.stream_start.encoding == YAML_UTF16LE_ENCODING) != 0);
    if (__pyx_t_5) {

      






      __Pyx_INCREF(__pyx_kp_u_utf_16_le);
      __Pyx_DECREF_SET(__pyx_v_encoding, __pyx_kp_u_utf_16_le);
      goto __pyx_L3;
    }

    






    __pyx_t_5 = ((__pyx_v_token->data.stream_start.encoding == YAML_UTF16BE_ENCODING) != 0);
    if (__pyx_t_5) {

      






      __Pyx_INCREF(__pyx_kp_u_utf_16_be);
      __Pyx_DECREF_SET(__pyx_v_encoding, __pyx_kp_u_utf_16_be);
      goto __pyx_L3;
    }
    __pyx_L3:;

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_2 = __Pyx_GetModuleGlobalName(__pyx_n_s_StreamStartToken); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 408; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_t_1 = PyTuple_New(3); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 408; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_1, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_1, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __Pyx_INCREF(__pyx_v_encoding);
    PyTuple_SET_ITEM(__pyx_t_1, 2, __pyx_v_encoding);
    __Pyx_GIVEREF(__pyx_v_encoding);
    __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_t_1, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 408; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    __pyx_r = __pyx_t_4;
    __pyx_t_4 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_STREAM_END_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_4 = __Pyx_GetModuleGlobalName(__pyx_n_s_StreamEndToken); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 410; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_1 = PyTuple_New(2); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 410; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_1, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_1, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_2 = __Pyx_PyObject_Call(__pyx_t_4, __pyx_t_1, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 410; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    __pyx_r = __pyx_t_2;
    __pyx_t_2 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_VERSION_DIRECTIVE_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_2 = __Pyx_GetModuleGlobalName(__pyx_n_s_DirectiveToken); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 412; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);

    






    __pyx_t_1 = __Pyx_PyInt_From_int(__pyx_v_token->data.version_directive.major); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 413; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);

    






    __pyx_t_4 = __Pyx_PyInt_From_int(__pyx_v_token->data.version_directive.minor); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 414; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);

    






    __pyx_t_3 = PyTuple_New(2); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 413; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    PyTuple_SET_ITEM(__pyx_t_3, 0, __pyx_t_1);
    __Pyx_GIVEREF(__pyx_t_1);
    PyTuple_SET_ITEM(__pyx_t_3, 1, __pyx_t_4);
    __Pyx_GIVEREF(__pyx_t_4);
    __pyx_t_1 = 0;
    __pyx_t_4 = 0;

    






    __pyx_t_4 = PyTuple_New(4); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 412; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_INCREF(__pyx_n_u_YAML);
    PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_n_u_YAML);
    __Pyx_GIVEREF(__pyx_n_u_YAML);
    PyTuple_SET_ITEM(__pyx_t_4, 1, __pyx_t_3);
    __Pyx_GIVEREF(__pyx_t_3);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_4, 2, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_4, 3, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_3 = 0;
    __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_t_4, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 412; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __pyx_r = __pyx_t_3;
    __pyx_t_3 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_TAG_DIRECTIVE_TOKEN:

    






    __pyx_t_3 = PyUnicode_FromString(__pyx_v_token->data.tag_directive.handle); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 417; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_v_handle = __pyx_t_3;
    __pyx_t_3 = 0;

    






    __pyx_t_3 = PyUnicode_FromString(__pyx_v_token->data.tag_directive.prefix); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 418; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_v_prefix = __pyx_t_3;
    __pyx_t_3 = 0;

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_3 = __Pyx_GetModuleGlobalName(__pyx_n_s_DirectiveToken); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 419; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_4 = PyTuple_New(2); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 419; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_INCREF(__pyx_v_handle);
    PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_v_handle);
    __Pyx_GIVEREF(__pyx_v_handle);
    __Pyx_INCREF(__pyx_v_prefix);
    PyTuple_SET_ITEM(__pyx_t_4, 1, __pyx_v_prefix);
    __Pyx_GIVEREF(__pyx_v_prefix);

    






    __pyx_t_2 = PyTuple_New(4); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 419; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(__pyx_n_u_TAG);
    PyTuple_SET_ITEM(__pyx_t_2, 0, __pyx_n_u_TAG);
    __Pyx_GIVEREF(__pyx_n_u_TAG);
    PyTuple_SET_ITEM(__pyx_t_2, 1, __pyx_t_4);
    __Pyx_GIVEREF(__pyx_t_4);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 2, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 3, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_4 = 0;

    






    __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_t_2, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 419; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_4;
    __pyx_t_4 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_DOCUMENT_START_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_4 = __Pyx_GetModuleGlobalName(__pyx_n_s_DocumentStartToken); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 422; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_2 = PyTuple_New(2); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 422; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_4, __pyx_t_2, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 422; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_3;
    __pyx_t_3 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_DOCUMENT_END_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_3 = __Pyx_GetModuleGlobalName(__pyx_n_s_DocumentEndToken); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 424; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_2 = PyTuple_New(2); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 424; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_t_2, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 424; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_4;
    __pyx_t_4 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_BLOCK_SEQUENCE_START_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_4 = __Pyx_GetModuleGlobalName(__pyx_n_s_BlockSequenceStartToken); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 426; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_2 = PyTuple_New(2); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 426; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_4, __pyx_t_2, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 426; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_3;
    __pyx_t_3 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_BLOCK_MAPPING_START_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_3 = __Pyx_GetModuleGlobalName(__pyx_n_s_BlockMappingStartToken); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 428; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_2 = PyTuple_New(2); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 428; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_t_2, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 428; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_4;
    __pyx_t_4 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_BLOCK_END_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_4 = __Pyx_GetModuleGlobalName(__pyx_n_s_BlockEndToken); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 430; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_2 = PyTuple_New(2); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 430; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_4, __pyx_t_2, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 430; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_3;
    __pyx_t_3 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_FLOW_SEQUENCE_START_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_3 = __Pyx_GetModuleGlobalName(__pyx_n_s_FlowSequenceStartToken); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 432; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_2 = PyTuple_New(2); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 432; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_t_2, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 432; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_4;
    __pyx_t_4 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_FLOW_SEQUENCE_END_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_4 = __Pyx_GetModuleGlobalName(__pyx_n_s_FlowSequenceEndToken); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 434; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_2 = PyTuple_New(2); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 434; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_4, __pyx_t_2, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 434; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_3;
    __pyx_t_3 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_FLOW_MAPPING_START_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_3 = __Pyx_GetModuleGlobalName(__pyx_n_s_FlowMappingStartToken); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 436; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_2 = PyTuple_New(2); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 436; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_t_2, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 436; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_4;
    __pyx_t_4 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_FLOW_MAPPING_END_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_4 = __Pyx_GetModuleGlobalName(__pyx_n_s_FlowMappingEndToken); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 438; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_2 = PyTuple_New(2); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 438; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_4, __pyx_t_2, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 438; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_3;
    __pyx_t_3 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_BLOCK_ENTRY_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_3 = __Pyx_GetModuleGlobalName(__pyx_n_s_BlockEntryToken); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 440; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_2 = PyTuple_New(2); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 440; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_t_2, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 440; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_4;
    __pyx_t_4 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_FLOW_ENTRY_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_4 = __Pyx_GetModuleGlobalName(__pyx_n_s_FlowEntryToken); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 442; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_2 = PyTuple_New(2); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 442; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_4, __pyx_t_2, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 442; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_3;
    __pyx_t_3 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_KEY_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_3 = __Pyx_GetModuleGlobalName(__pyx_n_s_KeyToken); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 444; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_2 = PyTuple_New(2); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 444; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_t_2, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 444; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_4;
    __pyx_t_4 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_VALUE_TOKEN:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_4 = __Pyx_GetModuleGlobalName(__pyx_n_s_ValueToken); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 446; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_2 = PyTuple_New(2); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 446; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_4, __pyx_t_2, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 446; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_3;
    __pyx_t_3 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_ALIAS_TOKEN:

    






    __pyx_t_3 = PyUnicode_FromString(__pyx_v_token->data.alias.value); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 448; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_v_value = __pyx_t_3;
    __pyx_t_3 = 0;

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_3 = __Pyx_GetModuleGlobalName(__pyx_n_s_AliasToken); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 449; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_2 = PyTuple_New(3); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 449; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(__pyx_v_value);
    PyTuple_SET_ITEM(__pyx_t_2, 0, __pyx_v_value);
    __Pyx_GIVEREF(__pyx_v_value);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 2, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_t_2, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 449; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_4;
    __pyx_t_4 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_ANCHOR_TOKEN:

    






    __pyx_t_4 = PyUnicode_FromString(__pyx_v_token->data.anchor.value); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 451; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_v_value = __pyx_t_4;
    __pyx_t_4 = 0;

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_4 = __Pyx_GetModuleGlobalName(__pyx_n_s_AnchorToken); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 452; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_2 = PyTuple_New(3); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 452; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(__pyx_v_value);
    PyTuple_SET_ITEM(__pyx_t_2, 0, __pyx_v_value);
    __Pyx_GIVEREF(__pyx_v_value);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 2, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_4, __pyx_t_2, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 452; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_3;
    __pyx_t_3 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_TAG_TOKEN:

    






    __pyx_t_3 = PyUnicode_FromString(__pyx_v_token->data.tag.handle); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 454; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_v_handle = __pyx_t_3;
    __pyx_t_3 = 0;

    






    __pyx_t_3 = PyUnicode_FromString(__pyx_v_token->data.tag.suffix); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 455; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_v_suffix = __pyx_t_3;
    __pyx_t_3 = 0;

    






    __pyx_t_5 = __Pyx_PyObject_IsTrue(__pyx_v_handle); if (unlikely(__pyx_t_5 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 456; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __pyx_t_6 = ((!__pyx_t_5) != 0);
    if (__pyx_t_6) {

      






      __Pyx_INCREF(Py_None);
      __Pyx_DECREF_SET(__pyx_v_handle, Py_None);
      goto __pyx_L5;
    }
    __pyx_L5:;

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_3 = __Pyx_GetModuleGlobalName(__pyx_n_s_TagToken); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 458; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_2 = PyTuple_New(2); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 458; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(__pyx_v_handle);
    PyTuple_SET_ITEM(__pyx_t_2, 0, __pyx_v_handle);
    __Pyx_GIVEREF(__pyx_v_handle);
    __Pyx_INCREF(__pyx_v_suffix);
    PyTuple_SET_ITEM(__pyx_t_2, 1, __pyx_v_suffix);
    __Pyx_GIVEREF(__pyx_v_suffix);
    __pyx_t_4 = PyTuple_New(3); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 458; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_t_2);
    __Pyx_GIVEREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_4, 1, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_4, 2, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_2 = 0;
    __pyx_t_2 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_t_4, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 458; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __pyx_r = __pyx_t_2;
    __pyx_t_2 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_SCALAR_TOKEN:

    






    __pyx_t_2 = PyUnicode_DecodeUTF8(__pyx_v_token->data.scalar.value, __pyx_v_token->data.scalar.length, __pyx_k_strict); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 460; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_v_value = __pyx_t_2;
    __pyx_t_2 = 0;

    






    __pyx_v_plain = 0;

    






    __Pyx_INCREF(Py_None);
    __pyx_v_style = Py_None;

    






    __pyx_t_6 = ((__pyx_v_token->data.scalar.style == YAML_PLAIN_SCALAR_STYLE) != 0);
    if (__pyx_t_6) {

      






      __pyx_v_plain = 1;

      






      __Pyx_INCREF(__pyx_kp_u__6);
      __Pyx_DECREF_SET(__pyx_v_style, __pyx_kp_u__6);
      goto __pyx_L6;
    }

    






    __pyx_t_6 = ((__pyx_v_token->data.scalar.style == YAML_SINGLE_QUOTED_SCALAR_STYLE) != 0);
    if (__pyx_t_6) {

      






      __Pyx_INCREF(__pyx_kp_u__7);
      __Pyx_DECREF_SET(__pyx_v_style, __pyx_kp_u__7);
      goto __pyx_L6;
    }

    






    __pyx_t_6 = ((__pyx_v_token->data.scalar.style == YAML_DOUBLE_QUOTED_SCALAR_STYLE) != 0);
    if (__pyx_t_6) {

      






      __Pyx_INCREF(__pyx_kp_u__8);
      __Pyx_DECREF_SET(__pyx_v_style, __pyx_kp_u__8);
      goto __pyx_L6;
    }

    






    __pyx_t_6 = ((__pyx_v_token->data.scalar.style == YAML_LITERAL_SCALAR_STYLE) != 0);
    if (__pyx_t_6) {

      






      __Pyx_INCREF(__pyx_kp_u__9);
      __Pyx_DECREF_SET(__pyx_v_style, __pyx_kp_u__9);
      goto __pyx_L6;
    }

    






    __pyx_t_6 = ((__pyx_v_token->data.scalar.style == YAML_FOLDED_SCALAR_STYLE) != 0);
    if (__pyx_t_6) {

      






      __Pyx_INCREF(__pyx_kp_u__10);
      __Pyx_DECREF_SET(__pyx_v_style, __pyx_kp_u__10);
      goto __pyx_L6;
    }
    __pyx_L6:;

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_2 = __Pyx_GetModuleGlobalName(__pyx_n_s_ScalarToken); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 475; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_t_4 = __Pyx_PyBool_FromLong(__pyx_v_plain); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 475; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);

    






    __pyx_t_3 = PyTuple_New(5); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 475; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_INCREF(__pyx_v_value);
    PyTuple_SET_ITEM(__pyx_t_3, 0, __pyx_v_value);
    __Pyx_GIVEREF(__pyx_v_value);
    PyTuple_SET_ITEM(__pyx_t_3, 1, __pyx_t_4);
    __Pyx_GIVEREF(__pyx_t_4);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_3, 2, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_3, 3, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __Pyx_INCREF(__pyx_v_style);
    PyTuple_SET_ITEM(__pyx_t_3, 4, __pyx_v_style);
    __Pyx_GIVEREF(__pyx_v_style);
    __pyx_t_4 = 0;

    






    __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_t_3, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 475; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __pyx_r = __pyx_t_4;
    __pyx_t_4 = 0;
    goto __pyx_L0;
    break;
    default:

    






    __pyx_t_6 = ((PY_MAJOR_VERSION < 3) != 0);
    if (__pyx_t_6) {

      






      __pyx_t_4 = __Pyx_PyObject_Call(__pyx_builtin_ValueError, __pyx_tuple__11, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 479; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);
      __Pyx_Raise(__pyx_t_4, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 479; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
     {

      






      __pyx_t_4 = __Pyx_PyObject_Call(__pyx_builtin_ValueError, __pyx_tuple__12, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 481; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);
      __Pyx_Raise(__pyx_t_4, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 481; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
    break;
  }

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_XDECREF(__pyx_t_2);
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_XDECREF(__pyx_t_4);
  __Pyx_AddTraceback("_yaml.CParser._token_to_object", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF((PyObject *)__pyx_v_start_mark);
  __Pyx_XDECREF((PyObject *)__pyx_v_end_mark);
  __Pyx_XDECREF(__pyx_v_encoding);
  __Pyx_XDECREF(__pyx_v_handle);
  __Pyx_XDECREF(__pyx_v_prefix);
  __Pyx_XDECREF(__pyx_v_value);
  __Pyx_XDECREF(__pyx_v_suffix);
  __Pyx_XDECREF(__pyx_v_style);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_7CParser_9get_token(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused); 
static PyObject *__pyx_pw_5_yaml_7CParser_9get_token(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("get_token (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_7CParser_8get_token(((struct __pyx_obj_5_yaml_CParser *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_7CParser_8get_token(struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  PyObject *__pyx_v_value = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("get_token", 0);

  






  __pyx_t_1 = (__pyx_v_self->current_token != Py_None);
  __pyx_t_2 = (__pyx_t_1 != 0);
  if (__pyx_t_2) {

    






    __pyx_t_3 = __pyx_v_self->current_token;
    __Pyx_INCREF(__pyx_t_3);
    __pyx_v_value = __pyx_t_3;
    __pyx_t_3 = 0;

    






    __Pyx_INCREF(Py_None);
    __Pyx_GIVEREF(Py_None);
    __Pyx_GOTREF(__pyx_v_self->current_token);
    __Pyx_DECREF(__pyx_v_self->current_token);
    __pyx_v_self->current_token = Py_None;
    goto __pyx_L3;
  }
   {

    






    __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_scan(__pyx_v_self); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 488; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_v_value = __pyx_t_3;
    __pyx_t_3 = 0;
  }
  __pyx_L3:;

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(__pyx_v_value);
  __pyx_r = __pyx_v_value;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_AddTraceback("_yaml.CParser.get_token", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_value);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_7CParser_11peek_token(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused); 
static PyObject *__pyx_pw_5_yaml_7CParser_11peek_token(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("peek_token (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_7CParser_10peek_token(((struct __pyx_obj_5_yaml_CParser *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_7CParser_10peek_token(struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("peek_token", 0);

  






  __pyx_t_1 = (__pyx_v_self->current_token == Py_None);
  __pyx_t_2 = (__pyx_t_1 != 0);
  if (__pyx_t_2) {

    






    __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_scan(__pyx_v_self); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 493; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_GIVEREF(__pyx_t_3);
    __Pyx_GOTREF(__pyx_v_self->current_token);
    __Pyx_DECREF(__pyx_v_self->current_token);
    __pyx_v_self->current_token = __pyx_t_3;
    __pyx_t_3 = 0;
    goto __pyx_L3;
  }
  __pyx_L3:;

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(__pyx_v_self->current_token);
  __pyx_r = __pyx_v_self->current_token;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_AddTraceback("_yaml.CParser.peek_token", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_7CParser_13check_token(PyObject *__pyx_v_self, PyObject *__pyx_args, PyObject *__pyx_kwds); 
static PyObject *__pyx_pw_5_yaml_7CParser_13check_token(PyObject *__pyx_v_self, PyObject *__pyx_args, PyObject *__pyx_kwds) {
  PyObject *__pyx_v_choices = 0;
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("check_token (wrapper)", 0);
  if (unlikely(__pyx_kwds) && unlikely(PyDict_Size(__pyx_kwds) > 0) && unlikely(!__Pyx_CheckKeywordStrings(__pyx_kwds, "check_token", 0))) return NULL;
  __Pyx_INCREF(__pyx_args);
  __pyx_v_choices = __pyx_args;
  __pyx_r = __pyx_pf_5_yaml_7CParser_12check_token(((struct __pyx_obj_5_yaml_CParser *)__pyx_v_self), __pyx_v_choices);

  
  __Pyx_XDECREF(__pyx_v_choices);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_7CParser_12check_token(struct __pyx_obj_5_yaml_CParser *__pyx_v_self, PyObject *__pyx_v_choices) {
  PyObject *__pyx_v_token_class = NULL;
  PyObject *__pyx_v_choice = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  Py_ssize_t __pyx_t_4;
  PyObject *__pyx_t_5 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("check_token", 0);

  






  __pyx_t_1 = (__pyx_v_self->current_token == Py_None);
  __pyx_t_2 = (__pyx_t_1 != 0);
  if (__pyx_t_2) {

    






    __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_scan(__pyx_v_self); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 498; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_GIVEREF(__pyx_t_3);
    __Pyx_GOTREF(__pyx_v_self->current_token);
    __Pyx_DECREF(__pyx_v_self->current_token);
    __pyx_v_self->current_token = __pyx_t_3;
    __pyx_t_3 = 0;
    goto __pyx_L3;
  }
  __pyx_L3:;

  






  __pyx_t_2 = (__pyx_v_self->current_token == Py_None);
  __pyx_t_1 = (__pyx_t_2 != 0);
  if (__pyx_t_1) {

    






    __Pyx_XDECREF(__pyx_r);
    __Pyx_INCREF(Py_False);
    __pyx_r = Py_False;
    goto __pyx_L0;
  }

  






  __pyx_t_1 = (__pyx_v_choices != Py_None) && (PyTuple_GET_SIZE(__pyx_v_choices) != 0);
  __pyx_t_2 = ((!__pyx_t_1) != 0);
  if (__pyx_t_2) {

    






    __Pyx_XDECREF(__pyx_r);
    __Pyx_INCREF(Py_True);
    __pyx_r = Py_True;
    goto __pyx_L0;
  }

  






  __pyx_t_3 = __Pyx_PyObject_GetAttrStr(__pyx_v_self->current_token, __pyx_n_s_class); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 503; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __pyx_v_token_class = __pyx_t_3;
  __pyx_t_3 = 0;

  






  __pyx_t_3 = __pyx_v_choices; __Pyx_INCREF(__pyx_t_3); __pyx_t_4 = 0;
  for (;;) {
    if (__pyx_t_4 >= PyTuple_GET_SIZE(__pyx_t_3)) break;
    #if CYTHON_COMPILING_IN_CPYTHON
    __pyx_t_5 = PyTuple_GET_ITEM(__pyx_t_3, __pyx_t_4); __Pyx_INCREF(__pyx_t_5); __pyx_t_4++; if (unlikely(0 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 504; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    #else
    __pyx_t_5 = PySequence_ITEM(__pyx_t_3, __pyx_t_4); __pyx_t_4++; if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 504; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    #endif
    __Pyx_XDECREF_SET(__pyx_v_choice, __pyx_t_5);
    __pyx_t_5 = 0;

    






    __pyx_t_2 = (__pyx_v_token_class == __pyx_v_choice);
    __pyx_t_1 = (__pyx_t_2 != 0);
    if (__pyx_t_1) {

      






      __Pyx_XDECREF(__pyx_r);
      __Pyx_INCREF(Py_True);
      __pyx_r = Py_True;
      __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
      goto __pyx_L0;
    }
  }
  __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(Py_False);
  __pyx_r = Py_False;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_XDECREF(__pyx_t_5);
  __Pyx_AddTraceback("_yaml.CParser.check_token", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_token_class);
  __Pyx_XDECREF(__pyx_v_choice);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_7CParser_15raw_parse(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused); 
static PyObject *__pyx_pw_5_yaml_7CParser_15raw_parse(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("raw_parse (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_7CParser_14raw_parse(((struct __pyx_obj_5_yaml_CParser *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_7CParser_14raw_parse(struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  yaml_event_t __pyx_v_event;
  int __pyx_v_done;
  int __pyx_v_count;
  PyObject *__pyx_v_error = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("raw_parse", 0);

  






  __pyx_v_count = 0;

  






  __pyx_v_done = 0;

  






  while (1) {
    __pyx_t_1 = ((__pyx_v_done == 0) != 0);
    if (!__pyx_t_1) break;

    






    __pyx_t_2 = yaml_parser_parse((&__pyx_v_self->parser), (&__pyx_v_event)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 516; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __pyx_t_1 = ((__pyx_t_2 == 0) != 0);
    if (__pyx_t_1) {

      






      __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parser_error(__pyx_v_self); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 517; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __pyx_v_error = __pyx_t_3;
      __pyx_t_3 = 0;

      






      __Pyx_Raise(__pyx_v_error, 0, 0, 0);
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 518; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }

    






    __pyx_t_1 = ((__pyx_v_event.type == YAML_NO_EVENT) != 0);
    if (__pyx_t_1) {

      






      __pyx_v_done = 1;
      goto __pyx_L6;
    }
     {

      






      __pyx_v_count = (__pyx_v_count + 1);
    }
    __pyx_L6:;

    






    yaml_event_delete((&__pyx_v_event));
  }

  






  __Pyx_XDECREF(__pyx_r);
  __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_count); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 524; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __pyx_r = __pyx_t_3;
  __pyx_t_3 = 0;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_AddTraceback("_yaml.CParser.raw_parse", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_error);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static PyObject *__pyx_f_5_yaml_7CParser__parse(struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  yaml_event_t __pyx_v_event;
  PyObject *__pyx_v_error = NULL;
  PyObject *__pyx_v_event_object = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("_parse", 0);

  






  __pyx_t_1 = yaml_parser_parse((&__pyx_v_self->parser), (&__pyx_v_event)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 528; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_t_2 = ((__pyx_t_1 == 0) != 0);
  if (__pyx_t_2) {

    






    __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parser_error(__pyx_v_self); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 529; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_v_error = __pyx_t_3;
    __pyx_t_3 = 0;

    






    __Pyx_Raise(__pyx_v_error, 0, 0, 0);
    {__pyx_filename = __pyx_f[0]; __pyx_lineno = 530; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  }

  






  __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_event_to_object(__pyx_v_self, (&__pyx_v_event)); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 531; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __pyx_v_event_object = __pyx_t_3;
  __pyx_t_3 = 0;

  






  yaml_event_delete((&__pyx_v_event));

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(__pyx_v_event_object);
  __pyx_r = __pyx_v_event_object;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_AddTraceback("_yaml.CParser._parse", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_error);
  __Pyx_XDECREF(__pyx_v_event_object);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static PyObject *__pyx_f_5_yaml_7CParser__event_to_object(struct __pyx_obj_5_yaml_CParser *__pyx_v_self, yaml_event_t *__pyx_v_event) {
  yaml_tag_directive_t *__pyx_v_tag_directive;
  struct __pyx_obj_5_yaml_Mark *__pyx_v_start_mark = NULL;
  struct __pyx_obj_5_yaml_Mark *__pyx_v_end_mark = NULL;
  PyObject *__pyx_v_encoding = NULL;
  int __pyx_v_explicit;
  PyObject *__pyx_v_version = NULL;
  PyObject *__pyx_v_tags = NULL;
  PyObject *__pyx_v_handle = NULL;
  PyObject *__pyx_v_prefix = NULL;
  PyObject *__pyx_v_anchor = NULL;
  PyObject *__pyx_v_tag = NULL;
  PyObject *__pyx_v_value = NULL;
  int __pyx_v_plain_implicit;
  int __pyx_v_quoted_implicit;
  PyObject *__pyx_v_style = NULL;
  int __pyx_v_implicit;
  PyObject *__pyx_v_flow_style = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  PyObject *__pyx_t_1 = NULL;
  PyObject *__pyx_t_2 = NULL;
  PyObject *__pyx_t_3 = NULL;
  PyObject *__pyx_t_4 = NULL;
  int __pyx_t_5;
  yaml_tag_directive_t *__pyx_t_6;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("_event_to_object", 0);

  






  __pyx_t_1 = __Pyx_PyInt_From_int(__pyx_v_event->start_mark.index); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 538; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);

  






  __pyx_t_2 = __Pyx_PyInt_From_int(__pyx_v_event->start_mark.line); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 539; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);

  






  __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_event->start_mark.column); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 540; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);

  






  __pyx_t_4 = PyTuple_New(6); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 537; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_4);
  __Pyx_INCREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_v_self->stream_name);
  __Pyx_GIVEREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_4, 1, __pyx_t_1);
  __Pyx_GIVEREF(__pyx_t_1);
  PyTuple_SET_ITEM(__pyx_t_4, 2, __pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  PyTuple_SET_ITEM(__pyx_t_4, 3, __pyx_t_3);
  __Pyx_GIVEREF(__pyx_t_3);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_4, 4, Py_None);
  __Pyx_GIVEREF(Py_None);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_4, 5, Py_None);
  __Pyx_GIVEREF(Py_None);
  __pyx_t_1 = 0;
  __pyx_t_2 = 0;
  __pyx_t_3 = 0;
  __pyx_t_3 = __Pyx_PyObject_Call(((PyObject *)((PyObject*)__pyx_ptype_5_yaml_Mark)), __pyx_t_4, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 537; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
  __pyx_v_start_mark = ((struct __pyx_obj_5_yaml_Mark *)__pyx_t_3);
  __pyx_t_3 = 0;

  






  __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_event->end_mark.index); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 543; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);

  






  __pyx_t_4 = __Pyx_PyInt_From_int(__pyx_v_event->end_mark.line); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 544; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_4);

  






  __pyx_t_2 = __Pyx_PyInt_From_int(__pyx_v_event->end_mark.column); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 545; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);

  






  __pyx_t_1 = PyTuple_New(6); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 542; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_INCREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_1, 0, __pyx_v_self->stream_name);
  __Pyx_GIVEREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_1, 1, __pyx_t_3);
  __Pyx_GIVEREF(__pyx_t_3);
  PyTuple_SET_ITEM(__pyx_t_1, 2, __pyx_t_4);
  __Pyx_GIVEREF(__pyx_t_4);
  PyTuple_SET_ITEM(__pyx_t_1, 3, __pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_1, 4, Py_None);
  __Pyx_GIVEREF(Py_None);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_1, 5, Py_None);
  __Pyx_GIVEREF(Py_None);
  __pyx_t_3 = 0;
  __pyx_t_4 = 0;
  __pyx_t_2 = 0;
  __pyx_t_2 = __Pyx_PyObject_Call(((PyObject *)((PyObject*)__pyx_ptype_5_yaml_Mark)), __pyx_t_1, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 542; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_v_end_mark = ((struct __pyx_obj_5_yaml_Mark *)__pyx_t_2);
  __pyx_t_2 = 0;

  






  switch (__pyx_v_event->type) {

    






    case YAML_NO_EVENT:

    






    __Pyx_XDECREF(__pyx_r);
    __Pyx_INCREF(Py_None);
    __pyx_r = Py_None;
    goto __pyx_L0;
    break;

    






    case YAML_STREAM_START_EVENT:

    






    __Pyx_INCREF(Py_None);
    __pyx_v_encoding = Py_None;

    






    __pyx_t_5 = ((__pyx_v_event->data.stream_start.encoding == YAML_UTF8_ENCODING) != 0);
    if (__pyx_t_5) {

      






      __pyx_t_5 = ((__pyx_v_self->unicode_source == 0) != 0);
      if (__pyx_t_5) {

        






        __Pyx_INCREF(__pyx_kp_u_utf_8);
        __Pyx_DECREF_SET(__pyx_v_encoding, __pyx_kp_u_utf_8);
        goto __pyx_L4;
      }
      __pyx_L4:;
      goto __pyx_L3;
    }

    






    __pyx_t_5 = ((__pyx_v_event->data.stream_start.encoding == YAML_UTF16LE_ENCODING) != 0);
    if (__pyx_t_5) {

      






      __Pyx_INCREF(__pyx_kp_u_utf_16_le);
      __Pyx_DECREF_SET(__pyx_v_encoding, __pyx_kp_u_utf_16_le);
      goto __pyx_L3;
    }

    






    __pyx_t_5 = ((__pyx_v_event->data.stream_start.encoding == YAML_UTF16BE_ENCODING) != 0);
    if (__pyx_t_5) {

      






      __Pyx_INCREF(__pyx_kp_u_utf_16_be);
      __Pyx_DECREF_SET(__pyx_v_encoding, __pyx_kp_u_utf_16_be);
      goto __pyx_L3;
    }
    __pyx_L3:;

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_2 = __Pyx_GetModuleGlobalName(__pyx_n_s_StreamStartEvent); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 558; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_t_1 = PyTuple_New(3); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 558; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_1, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_1, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __Pyx_INCREF(__pyx_v_encoding);
    PyTuple_SET_ITEM(__pyx_t_1, 2, __pyx_v_encoding);
    __Pyx_GIVEREF(__pyx_v_encoding);
    __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_t_1, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 558; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    __pyx_r = __pyx_t_4;
    __pyx_t_4 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_STREAM_END_EVENT:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_4 = __Pyx_GetModuleGlobalName(__pyx_n_s_StreamEndEvent); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 560; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_1 = PyTuple_New(2); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 560; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_1, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_1, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_2 = __Pyx_PyObject_Call(__pyx_t_4, __pyx_t_1, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 560; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    __pyx_r = __pyx_t_2;
    __pyx_t_2 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_DOCUMENT_START_EVENT:

    






    __pyx_v_explicit = 0;

    






    __pyx_t_5 = ((__pyx_v_event->data.document_start.implicit == 0) != 0);
    if (__pyx_t_5) {

      






      __pyx_v_explicit = 1;
      goto __pyx_L5;
    }
    __pyx_L5:;

    






    __Pyx_INCREF(Py_None);
    __pyx_v_version = Py_None;

    






    __pyx_t_5 = ((__pyx_v_event->data.document_start.version_directive != NULL) != 0);
    if (__pyx_t_5) {

      






      __pyx_t_2 = __Pyx_PyInt_From_int(__pyx_v_event->data.document_start.version_directive->major); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 567; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);

      






      __pyx_t_1 = __Pyx_PyInt_From_int(__pyx_v_event->data.document_start.version_directive->minor); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 568; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);

      






      __pyx_t_4 = PyTuple_New(2); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 567; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);
      PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_t_2);
      __Pyx_GIVEREF(__pyx_t_2);
      PyTuple_SET_ITEM(__pyx_t_4, 1, __pyx_t_1);
      __Pyx_GIVEREF(__pyx_t_1);
      __pyx_t_2 = 0;
      __pyx_t_1 = 0;
      __Pyx_DECREF_SET(__pyx_v_version, __pyx_t_4);
      __pyx_t_4 = 0;
      goto __pyx_L6;
    }
    __pyx_L6:;

    






    __Pyx_INCREF(Py_None);
    __pyx_v_tags = Py_None;

    






    __pyx_t_5 = ((__pyx_v_event->data.document_start.tag_directives.start != NULL) != 0);
    if (__pyx_t_5) {

      






      __pyx_t_4 = PyDict_New(); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 571; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);
      __Pyx_DECREF_SET(__pyx_v_tags, __pyx_t_4);
      __pyx_t_4 = 0;

      






      __pyx_t_6 = __pyx_v_event->data.document_start.tag_directives.start;
      __pyx_v_tag_directive = __pyx_t_6;

      






      while (1) {
        __pyx_t_5 = ((__pyx_v_tag_directive != __pyx_v_event->data.document_start.tag_directives.end) != 0);
        if (!__pyx_t_5) break;

        






        __pyx_t_4 = PyUnicode_FromString(__pyx_v_tag_directive->handle); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 574; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_4);
        __Pyx_XDECREF_SET(__pyx_v_handle, __pyx_t_4);
        __pyx_t_4 = 0;

        






        __pyx_t_4 = PyUnicode_FromString(__pyx_v_tag_directive->prefix); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 575; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_4);
        __Pyx_XDECREF_SET(__pyx_v_prefix, __pyx_t_4);
        __pyx_t_4 = 0;

        






        if (unlikely(PyObject_SetItem(__pyx_v_tags, __pyx_v_handle, __pyx_v_prefix) < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 576; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

        






        __pyx_v_tag_directive = (__pyx_v_tag_directive + 1);
      }
      goto __pyx_L7;
    }
    __pyx_L7:;

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_4 = __Pyx_GetModuleGlobalName(__pyx_n_s_DocumentStartEvent); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 578; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);

    






    __pyx_t_1 = __Pyx_PyBool_FromLong(__pyx_v_explicit); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 579; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);

    






    __pyx_t_2 = PyTuple_New(5); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 578; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 2, __pyx_t_1);
    __Pyx_GIVEREF(__pyx_t_1);
    __Pyx_INCREF(__pyx_v_version);
    PyTuple_SET_ITEM(__pyx_t_2, 3, __pyx_v_version);
    __Pyx_GIVEREF(__pyx_v_version);
    __Pyx_INCREF(__pyx_v_tags);
    PyTuple_SET_ITEM(__pyx_t_2, 4, __pyx_v_tags);
    __Pyx_GIVEREF(__pyx_v_tags);
    __pyx_t_1 = 0;
    __pyx_t_1 = __Pyx_PyObject_Call(__pyx_t_4, __pyx_t_2, NULL); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 578; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_1;
    __pyx_t_1 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_DOCUMENT_END_EVENT:

    






    __pyx_v_explicit = 0;

    






    __pyx_t_5 = ((__pyx_v_event->data.document_end.implicit == 0) != 0);
    if (__pyx_t_5) {

      






      __pyx_v_explicit = 1;
      goto __pyx_L10;
    }
    __pyx_L10:;

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_DocumentEndEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 584; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __pyx_t_2 = __Pyx_PyBool_FromLong(__pyx_v_explicit); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 584; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_t_4 = PyTuple_New(3); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 584; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_4, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_4, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_4, 2, __pyx_t_2);
    __Pyx_GIVEREF(__pyx_t_2);
    __pyx_t_2 = 0;
    __pyx_t_2 = __Pyx_PyObject_Call(__pyx_t_1, __pyx_t_4, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 584; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __pyx_r = __pyx_t_2;
    __pyx_t_2 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_ALIAS_EVENT:

    






    __pyx_t_2 = PyUnicode_FromString(__pyx_v_event->data.alias.anchor); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 586; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_v_anchor = __pyx_t_2;
    __pyx_t_2 = 0;

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_2 = __Pyx_GetModuleGlobalName(__pyx_n_s_AliasEvent); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 587; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_t_4 = PyTuple_New(3); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 587; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_INCREF(__pyx_v_anchor);
    PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_v_anchor);
    __Pyx_GIVEREF(__pyx_v_anchor);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_4, 1, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_4, 2, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_1 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_t_4, NULL); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 587; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __pyx_r = __pyx_t_1;
    __pyx_t_1 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_SCALAR_EVENT:

    






    __Pyx_INCREF(Py_None);
    __pyx_v_anchor = Py_None;

    






    __pyx_t_5 = ((__pyx_v_event->data.scalar.anchor != NULL) != 0);
    if (__pyx_t_5) {

      






      __pyx_t_1 = PyUnicode_FromString(__pyx_v_event->data.scalar.anchor); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 591; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __Pyx_DECREF_SET(__pyx_v_anchor, __pyx_t_1);
      __pyx_t_1 = 0;
      goto __pyx_L11;
    }
    __pyx_L11:;

    






    __Pyx_INCREF(Py_None);
    __pyx_v_tag = Py_None;

    






    __pyx_t_5 = ((__pyx_v_event->data.scalar.tag != NULL) != 0);
    if (__pyx_t_5) {

      






      __pyx_t_1 = PyUnicode_FromString(__pyx_v_event->data.scalar.tag); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 594; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __Pyx_DECREF_SET(__pyx_v_tag, __pyx_t_1);
      __pyx_t_1 = 0;
      goto __pyx_L12;
    }
    __pyx_L12:;

    






    __pyx_t_1 = PyUnicode_DecodeUTF8(__pyx_v_event->data.scalar.value, __pyx_v_event->data.scalar.length, __pyx_k_strict); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 595; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __pyx_v_value = __pyx_t_1;
    __pyx_t_1 = 0;

    






    __pyx_v_plain_implicit = 0;

    






    __pyx_t_5 = ((__pyx_v_event->data.scalar.plain_implicit == 1) != 0);
    if (__pyx_t_5) {

      






      __pyx_v_plain_implicit = 1;
      goto __pyx_L13;
    }
    __pyx_L13:;

    






    __pyx_v_quoted_implicit = 0;

    






    __pyx_t_5 = ((__pyx_v_event->data.scalar.quoted_implicit == 1) != 0);
    if (__pyx_t_5) {

      






      __pyx_v_quoted_implicit = 1;
      goto __pyx_L14;
    }
    __pyx_L14:;

    






    __Pyx_INCREF(Py_None);
    __pyx_v_style = Py_None;

    






    __pyx_t_5 = ((__pyx_v_event->data.scalar.style == YAML_PLAIN_SCALAR_STYLE) != 0);
    if (__pyx_t_5) {

      






      __Pyx_INCREF(__pyx_kp_u__6);
      __Pyx_DECREF_SET(__pyx_v_style, __pyx_kp_u__6);
      goto __pyx_L15;
    }

    






    __pyx_t_5 = ((__pyx_v_event->data.scalar.style == YAML_SINGLE_QUOTED_SCALAR_STYLE) != 0);
    if (__pyx_t_5) {

      






      __Pyx_INCREF(__pyx_kp_u__7);
      __Pyx_DECREF_SET(__pyx_v_style, __pyx_kp_u__7);
      goto __pyx_L15;
    }

    






    __pyx_t_5 = ((__pyx_v_event->data.scalar.style == YAML_DOUBLE_QUOTED_SCALAR_STYLE) != 0);
    if (__pyx_t_5) {

      






      __Pyx_INCREF(__pyx_kp_u__8);
      __Pyx_DECREF_SET(__pyx_v_style, __pyx_kp_u__8);
      goto __pyx_L15;
    }

    






    __pyx_t_5 = ((__pyx_v_event->data.scalar.style == YAML_LITERAL_SCALAR_STYLE) != 0);
    if (__pyx_t_5) {

      






      __Pyx_INCREF(__pyx_kp_u__9);
      __Pyx_DECREF_SET(__pyx_v_style, __pyx_kp_u__9);
      goto __pyx_L15;
    }

    






    __pyx_t_5 = ((__pyx_v_event->data.scalar.style == YAML_FOLDED_SCALAR_STYLE) != 0);
    if (__pyx_t_5) {

      






      __Pyx_INCREF(__pyx_kp_u__10);
      __Pyx_DECREF_SET(__pyx_v_style, __pyx_kp_u__10);
      goto __pyx_L15;
    }
    __pyx_L15:;

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_ScalarEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 614; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);

    






    __pyx_t_4 = __Pyx_PyBool_FromLong(__pyx_v_plain_implicit); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 615; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_2 = __Pyx_PyBool_FromLong(__pyx_v_quoted_implicit); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 615; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_t_3 = PyTuple_New(2); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 615; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    PyTuple_SET_ITEM(__pyx_t_3, 0, __pyx_t_4);
    __Pyx_GIVEREF(__pyx_t_4);
    PyTuple_SET_ITEM(__pyx_t_3, 1, __pyx_t_2);
    __Pyx_GIVEREF(__pyx_t_2);
    __pyx_t_4 = 0;
    __pyx_t_2 = 0;

    






    __pyx_t_2 = PyTuple_New(7); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 614; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_INCREF(__pyx_v_anchor);
    PyTuple_SET_ITEM(__pyx_t_2, 0, __pyx_v_anchor);
    __Pyx_GIVEREF(__pyx_v_anchor);
    __Pyx_INCREF(__pyx_v_tag);
    PyTuple_SET_ITEM(__pyx_t_2, 1, __pyx_v_tag);
    __Pyx_GIVEREF(__pyx_v_tag);
    PyTuple_SET_ITEM(__pyx_t_2, 2, __pyx_t_3);
    __Pyx_GIVEREF(__pyx_t_3);
    __Pyx_INCREF(__pyx_v_value);
    PyTuple_SET_ITEM(__pyx_t_2, 3, __pyx_v_value);
    __Pyx_GIVEREF(__pyx_v_value);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 4, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_2, 5, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __Pyx_INCREF(__pyx_v_style);
    PyTuple_SET_ITEM(__pyx_t_2, 6, __pyx_v_style);
    __Pyx_GIVEREF(__pyx_v_style);
    __pyx_t_3 = 0;
    __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_1, __pyx_t_2, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 614; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_r = __pyx_t_3;
    __pyx_t_3 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_SEQUENCE_START_EVENT:

    






    __Pyx_INCREF(Py_None);
    __pyx_v_anchor = Py_None;

    






    __pyx_t_5 = ((__pyx_v_event->data.sequence_start.anchor != NULL) != 0);
    if (__pyx_t_5) {

      






      __pyx_t_3 = PyUnicode_FromString(__pyx_v_event->data.sequence_start.anchor); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 620; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __Pyx_DECREF_SET(__pyx_v_anchor, __pyx_t_3);
      __pyx_t_3 = 0;
      goto __pyx_L16;
    }
    __pyx_L16:;

    






    __Pyx_INCREF(Py_None);
    __pyx_v_tag = Py_None;

    






    __pyx_t_5 = ((__pyx_v_event->data.sequence_start.tag != NULL) != 0);
    if (__pyx_t_5) {

      






      __pyx_t_3 = PyUnicode_FromString(__pyx_v_event->data.sequence_start.tag); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 623; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __Pyx_DECREF_SET(__pyx_v_tag, __pyx_t_3);
      __pyx_t_3 = 0;
      goto __pyx_L17;
    }
    __pyx_L17:;

    






    __pyx_v_implicit = 0;

    






    __pyx_t_5 = ((__pyx_v_event->data.sequence_start.implicit == 1) != 0);
    if (__pyx_t_5) {

      






      __pyx_v_implicit = 1;
      goto __pyx_L18;
    }
    __pyx_L18:;

    






    __Pyx_INCREF(Py_None);
    __pyx_v_flow_style = Py_None;

    






    __pyx_t_5 = ((__pyx_v_event->data.sequence_start.style == YAML_FLOW_SEQUENCE_STYLE) != 0);
    if (__pyx_t_5) {

      






      __Pyx_INCREF(Py_True);
      __Pyx_DECREF_SET(__pyx_v_flow_style, Py_True);
      goto __pyx_L19;
    }

    






    __pyx_t_5 = ((__pyx_v_event->data.sequence_start.style == YAML_BLOCK_SEQUENCE_STYLE) != 0);
    if (__pyx_t_5) {

      






      __Pyx_INCREF(Py_False);
      __Pyx_DECREF_SET(__pyx_v_flow_style, Py_False);
      goto __pyx_L19;
    }
    __pyx_L19:;

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_3 = __Pyx_GetModuleGlobalName(__pyx_n_s_SequenceStartEvent); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 632; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_2 = __Pyx_PyBool_FromLong(__pyx_v_implicit); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 632; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);

    






    __pyx_t_1 = PyTuple_New(6); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 632; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __Pyx_INCREF(__pyx_v_anchor);
    PyTuple_SET_ITEM(__pyx_t_1, 0, __pyx_v_anchor);
    __Pyx_GIVEREF(__pyx_v_anchor);
    __Pyx_INCREF(__pyx_v_tag);
    PyTuple_SET_ITEM(__pyx_t_1, 1, __pyx_v_tag);
    __Pyx_GIVEREF(__pyx_v_tag);
    PyTuple_SET_ITEM(__pyx_t_1, 2, __pyx_t_2);
    __Pyx_GIVEREF(__pyx_t_2);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_1, 3, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_1, 4, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __Pyx_INCREF(__pyx_v_flow_style);
    PyTuple_SET_ITEM(__pyx_t_1, 5, __pyx_v_flow_style);
    __Pyx_GIVEREF(__pyx_v_flow_style);
    __pyx_t_2 = 0;

    






    __pyx_t_2 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_t_1, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 632; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    __pyx_r = __pyx_t_2;
    __pyx_t_2 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_MAPPING_START_EVENT:

    






    __Pyx_INCREF(Py_None);
    __pyx_v_anchor = Py_None;

    






    __pyx_t_5 = ((__pyx_v_event->data.mapping_start.anchor != NULL) != 0);
    if (__pyx_t_5) {

      






      __pyx_t_2 = PyUnicode_FromString(__pyx_v_event->data.mapping_start.anchor); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 637; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __Pyx_DECREF_SET(__pyx_v_anchor, __pyx_t_2);
      __pyx_t_2 = 0;
      goto __pyx_L20;
    }
    __pyx_L20:;

    






    __Pyx_INCREF(Py_None);
    __pyx_v_tag = Py_None;

    






    __pyx_t_5 = ((__pyx_v_event->data.mapping_start.tag != NULL) != 0);
    if (__pyx_t_5) {

      






      __pyx_t_2 = PyUnicode_FromString(__pyx_v_event->data.mapping_start.tag); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 640; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __Pyx_DECREF_SET(__pyx_v_tag, __pyx_t_2);
      __pyx_t_2 = 0;
      goto __pyx_L21;
    }
    __pyx_L21:;

    






    __pyx_v_implicit = 0;

    






    __pyx_t_5 = ((__pyx_v_event->data.mapping_start.implicit == 1) != 0);
    if (__pyx_t_5) {

      






      __pyx_v_implicit = 1;
      goto __pyx_L22;
    }
    __pyx_L22:;

    






    __Pyx_INCREF(Py_None);
    __pyx_v_flow_style = Py_None;

    






    __pyx_t_5 = ((__pyx_v_event->data.mapping_start.style == YAML_FLOW_MAPPING_STYLE) != 0);
    if (__pyx_t_5) {

      






      __Pyx_INCREF(Py_True);
      __Pyx_DECREF_SET(__pyx_v_flow_style, Py_True);
      goto __pyx_L23;
    }

    






    __pyx_t_5 = ((__pyx_v_event->data.mapping_start.style == YAML_BLOCK_MAPPING_STYLE) != 0);
    if (__pyx_t_5) {

      






      __Pyx_INCREF(Py_False);
      __Pyx_DECREF_SET(__pyx_v_flow_style, Py_False);
      goto __pyx_L23;
    }
    __pyx_L23:;

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_2 = __Pyx_GetModuleGlobalName(__pyx_n_s_MappingStartEvent); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 649; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_t_1 = __Pyx_PyBool_FromLong(__pyx_v_implicit); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 649; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);

    






    __pyx_t_3 = PyTuple_New(6); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 649; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_INCREF(__pyx_v_anchor);
    PyTuple_SET_ITEM(__pyx_t_3, 0, __pyx_v_anchor);
    __Pyx_GIVEREF(__pyx_v_anchor);
    __Pyx_INCREF(__pyx_v_tag);
    PyTuple_SET_ITEM(__pyx_t_3, 1, __pyx_v_tag);
    __Pyx_GIVEREF(__pyx_v_tag);
    PyTuple_SET_ITEM(__pyx_t_3, 2, __pyx_t_1);
    __Pyx_GIVEREF(__pyx_t_1);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_3, 3, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_3, 4, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __Pyx_INCREF(__pyx_v_flow_style);
    PyTuple_SET_ITEM(__pyx_t_3, 5, __pyx_v_flow_style);
    __Pyx_GIVEREF(__pyx_v_flow_style);
    __pyx_t_1 = 0;

    






    __pyx_t_1 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_t_3, NULL); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 649; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __pyx_r = __pyx_t_1;
    __pyx_t_1 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_SEQUENCE_END_EVENT:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_SequenceEndEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 652; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __pyx_t_3 = PyTuple_New(2); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 652; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_3, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_3, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_2 = __Pyx_PyObject_Call(__pyx_t_1, __pyx_t_3, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 652; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __pyx_r = __pyx_t_2;
    __pyx_t_2 = 0;
    goto __pyx_L0;
    break;

    






    case YAML_MAPPING_END_EVENT:

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_2 = __Pyx_GetModuleGlobalName(__pyx_n_s_MappingEndEvent); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 654; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_t_3 = PyTuple_New(2); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 654; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
    PyTuple_SET_ITEM(__pyx_t_3, 0, ((PyObject *)__pyx_v_start_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
    __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
    PyTuple_SET_ITEM(__pyx_t_3, 1, ((PyObject *)__pyx_v_end_mark));
    __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
    __pyx_t_1 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_t_3, NULL); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 654; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __pyx_r = __pyx_t_1;
    __pyx_t_1 = 0;
    goto __pyx_L0;
    break;
    default:

    






    __pyx_t_5 = ((PY_MAJOR_VERSION < 3) != 0);
    if (__pyx_t_5) {

      






      __pyx_t_1 = __Pyx_PyObject_Call(__pyx_builtin_ValueError, __pyx_tuple__13, NULL); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 657; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __Pyx_Raise(__pyx_t_1, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 657; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
     {

      






      __pyx_t_1 = __Pyx_PyObject_Call(__pyx_builtin_ValueError, __pyx_tuple__14, NULL); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 659; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __Pyx_Raise(__pyx_t_1, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 659; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
    break;
  }

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_XDECREF(__pyx_t_2);
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_XDECREF(__pyx_t_4);
  __Pyx_AddTraceback("_yaml.CParser._event_to_object", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF((PyObject *)__pyx_v_start_mark);
  __Pyx_XDECREF((PyObject *)__pyx_v_end_mark);
  __Pyx_XDECREF(__pyx_v_encoding);
  __Pyx_XDECREF(__pyx_v_version);
  __Pyx_XDECREF(__pyx_v_tags);
  __Pyx_XDECREF(__pyx_v_handle);
  __Pyx_XDECREF(__pyx_v_prefix);
  __Pyx_XDECREF(__pyx_v_anchor);
  __Pyx_XDECREF(__pyx_v_tag);
  __Pyx_XDECREF(__pyx_v_value);
  __Pyx_XDECREF(__pyx_v_style);
  __Pyx_XDECREF(__pyx_v_flow_style);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_7CParser_17get_event(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused); 
static PyObject *__pyx_pw_5_yaml_7CParser_17get_event(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("get_event (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_7CParser_16get_event(((struct __pyx_obj_5_yaml_CParser *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_7CParser_16get_event(struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  PyObject *__pyx_v_value = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("get_event", 0);

  






  __pyx_t_1 = (__pyx_v_self->current_event != Py_None);
  __pyx_t_2 = (__pyx_t_1 != 0);
  if (__pyx_t_2) {

    






    __pyx_t_3 = __pyx_v_self->current_event;
    __Pyx_INCREF(__pyx_t_3);
    __pyx_v_value = __pyx_t_3;
    __pyx_t_3 = 0;

    






    __Pyx_INCREF(Py_None);
    __Pyx_GIVEREF(Py_None);
    __Pyx_GOTREF(__pyx_v_self->current_event);
    __Pyx_DECREF(__pyx_v_self->current_event);
    __pyx_v_self->current_event = Py_None;
    goto __pyx_L3;
  }
   {

    






    __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parse(__pyx_v_self); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 666; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_v_value = __pyx_t_3;
    __pyx_t_3 = 0;
  }
  __pyx_L3:;

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(__pyx_v_value);
  __pyx_r = __pyx_v_value;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_AddTraceback("_yaml.CParser.get_event", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_value);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_7CParser_19peek_event(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused); 
static PyObject *__pyx_pw_5_yaml_7CParser_19peek_event(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("peek_event (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_7CParser_18peek_event(((struct __pyx_obj_5_yaml_CParser *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_7CParser_18peek_event(struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("peek_event", 0);

  






  __pyx_t_1 = (__pyx_v_self->current_event == Py_None);
  __pyx_t_2 = (__pyx_t_1 != 0);
  if (__pyx_t_2) {

    






    __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parse(__pyx_v_self); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 671; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_GIVEREF(__pyx_t_3);
    __Pyx_GOTREF(__pyx_v_self->current_event);
    __Pyx_DECREF(__pyx_v_self->current_event);
    __pyx_v_self->current_event = __pyx_t_3;
    __pyx_t_3 = 0;
    goto __pyx_L3;
  }
  __pyx_L3:;

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(__pyx_v_self->current_event);
  __pyx_r = __pyx_v_self->current_event;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_AddTraceback("_yaml.CParser.peek_event", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_7CParser_21check_event(PyObject *__pyx_v_self, PyObject *__pyx_args, PyObject *__pyx_kwds); 
static PyObject *__pyx_pw_5_yaml_7CParser_21check_event(PyObject *__pyx_v_self, PyObject *__pyx_args, PyObject *__pyx_kwds) {
  PyObject *__pyx_v_choices = 0;
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("check_event (wrapper)", 0);
  if (unlikely(__pyx_kwds) && unlikely(PyDict_Size(__pyx_kwds) > 0) && unlikely(!__Pyx_CheckKeywordStrings(__pyx_kwds, "check_event", 0))) return NULL;
  __Pyx_INCREF(__pyx_args);
  __pyx_v_choices = __pyx_args;
  __pyx_r = __pyx_pf_5_yaml_7CParser_20check_event(((struct __pyx_obj_5_yaml_CParser *)__pyx_v_self), __pyx_v_choices);

  
  __Pyx_XDECREF(__pyx_v_choices);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_7CParser_20check_event(struct __pyx_obj_5_yaml_CParser *__pyx_v_self, PyObject *__pyx_v_choices) {
  PyObject *__pyx_v_event_class = NULL;
  PyObject *__pyx_v_choice = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  Py_ssize_t __pyx_t_4;
  PyObject *__pyx_t_5 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("check_event", 0);

  






  __pyx_t_1 = (__pyx_v_self->current_event == Py_None);
  __pyx_t_2 = (__pyx_t_1 != 0);
  if (__pyx_t_2) {

    






    __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parse(__pyx_v_self); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 676; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_GIVEREF(__pyx_t_3);
    __Pyx_GOTREF(__pyx_v_self->current_event);
    __Pyx_DECREF(__pyx_v_self->current_event);
    __pyx_v_self->current_event = __pyx_t_3;
    __pyx_t_3 = 0;
    goto __pyx_L3;
  }
  __pyx_L3:;

  






  __pyx_t_2 = (__pyx_v_self->current_event == Py_None);
  __pyx_t_1 = (__pyx_t_2 != 0);
  if (__pyx_t_1) {

    






    __Pyx_XDECREF(__pyx_r);
    __Pyx_INCREF(Py_False);
    __pyx_r = Py_False;
    goto __pyx_L0;
  }

  






  __pyx_t_1 = (__pyx_v_choices != Py_None) && (PyTuple_GET_SIZE(__pyx_v_choices) != 0);
  __pyx_t_2 = ((!__pyx_t_1) != 0);
  if (__pyx_t_2) {

    






    __Pyx_XDECREF(__pyx_r);
    __Pyx_INCREF(Py_True);
    __pyx_r = Py_True;
    goto __pyx_L0;
  }

  






  __pyx_t_3 = __Pyx_PyObject_GetAttrStr(__pyx_v_self->current_event, __pyx_n_s_class); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 681; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __pyx_v_event_class = __pyx_t_3;
  __pyx_t_3 = 0;

  






  __pyx_t_3 = __pyx_v_choices; __Pyx_INCREF(__pyx_t_3); __pyx_t_4 = 0;
  for (;;) {
    if (__pyx_t_4 >= PyTuple_GET_SIZE(__pyx_t_3)) break;
    #if CYTHON_COMPILING_IN_CPYTHON
    __pyx_t_5 = PyTuple_GET_ITEM(__pyx_t_3, __pyx_t_4); __Pyx_INCREF(__pyx_t_5); __pyx_t_4++; if (unlikely(0 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 682; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    #else
    __pyx_t_5 = PySequence_ITEM(__pyx_t_3, __pyx_t_4); __pyx_t_4++; if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 682; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    #endif
    __Pyx_XDECREF_SET(__pyx_v_choice, __pyx_t_5);
    __pyx_t_5 = 0;

    






    __pyx_t_2 = (__pyx_v_event_class == __pyx_v_choice);
    __pyx_t_1 = (__pyx_t_2 != 0);
    if (__pyx_t_1) {

      






      __Pyx_XDECREF(__pyx_r);
      __Pyx_INCREF(Py_True);
      __pyx_r = Py_True;
      __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
      goto __pyx_L0;
    }
  }
  __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(Py_False);
  __pyx_r = Py_False;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_XDECREF(__pyx_t_5);
  __Pyx_AddTraceback("_yaml.CParser.check_event", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_event_class);
  __Pyx_XDECREF(__pyx_v_choice);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_7CParser_23check_node(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused); 
static PyObject *__pyx_pw_5_yaml_7CParser_23check_node(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("check_node (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_7CParser_22check_node(((struct __pyx_obj_5_yaml_CParser *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_7CParser_22check_node(struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("check_node", 0);

  






  __pyx_t_1 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parse_next_event(__pyx_v_self); if (unlikely(__pyx_t_1 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 688; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

  






  __pyx_t_2 = ((__pyx_v_self->parsed_event.type == YAML_STREAM_START_EVENT) != 0);
  if (__pyx_t_2) {

    






    yaml_event_delete((&__pyx_v_self->parsed_event));

    






    __pyx_t_1 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parse_next_event(__pyx_v_self); if (unlikely(__pyx_t_1 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 691; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    goto __pyx_L3;
  }
  __pyx_L3:;

  






  __pyx_t_2 = ((__pyx_v_self->parsed_event.type != YAML_STREAM_END_EVENT) != 0);
  if (__pyx_t_2) {

    






    __Pyx_XDECREF(__pyx_r);
    __Pyx_INCREF(Py_True);
    __pyx_r = Py_True;
    goto __pyx_L0;
  }

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(Py_False);
  __pyx_r = Py_False;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_AddTraceback("_yaml.CParser.check_node", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_7CParser_25get_node(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused); 
static PyObject *__pyx_pw_5_yaml_7CParser_25get_node(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("get_node (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_7CParser_24get_node(((struct __pyx_obj_5_yaml_CParser *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_7CParser_24get_node(struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("get_node", 0);

  






  __pyx_t_1 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parse_next_event(__pyx_v_self); if (unlikely(__pyx_t_1 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 697; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

  






  __pyx_t_2 = ((__pyx_v_self->parsed_event.type != YAML_STREAM_END_EVENT) != 0);
  if (__pyx_t_2) {

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_compose_document(__pyx_v_self); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 699; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_r = __pyx_t_3;
    __pyx_t_3 = 0;
    goto __pyx_L0;
  }

  







  
  __pyx_r = Py_None; __Pyx_INCREF(Py_None);
  goto __pyx_L0;
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_AddTraceback("_yaml.CParser.get_node", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_7CParser_27get_single_node(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused); 
static PyObject *__pyx_pw_5_yaml_7CParser_27get_single_node(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("get_single_node (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_7CParser_26get_single_node(((struct __pyx_obj_5_yaml_CParser *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_7CParser_26get_single_node(struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  PyObject *__pyx_v_document = NULL;
  struct __pyx_obj_5_yaml_Mark *__pyx_v_mark = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  PyObject *__pyx_t_4 = NULL;
  PyObject *__pyx_t_5 = NULL;
  PyObject *__pyx_t_6 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("get_single_node", 0);

  






  __pyx_t_1 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parse_next_event(__pyx_v_self); if (unlikely(__pyx_t_1 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 702; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

  






  yaml_event_delete((&__pyx_v_self->parsed_event));

  






  __pyx_t_1 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parse_next_event(__pyx_v_self); if (unlikely(__pyx_t_1 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 704; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

  






  __Pyx_INCREF(Py_None);
  __pyx_v_document = Py_None;

  






  __pyx_t_2 = ((__pyx_v_self->parsed_event.type != YAML_STREAM_END_EVENT) != 0);
  if (__pyx_t_2) {

    






    __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_compose_document(__pyx_v_self); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 707; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_DECREF_SET(__pyx_v_document, __pyx_t_3);
    __pyx_t_3 = 0;
    goto __pyx_L3;
  }
  __pyx_L3:;

  






  __pyx_t_1 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parse_next_event(__pyx_v_self); if (unlikely(__pyx_t_1 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 708; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

  






  __pyx_t_2 = ((__pyx_v_self->parsed_event.type != YAML_STREAM_END_EVENT) != 0);
  if (__pyx_t_2) {

    






    __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.index); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 711; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);

    






    __pyx_t_4 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.line); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 712; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);

    






    __pyx_t_5 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.column); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 713; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);

    






    __pyx_t_6 = PyTuple_New(6); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 710; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_6);
    __Pyx_INCREF(__pyx_v_self->stream_name);
    PyTuple_SET_ITEM(__pyx_t_6, 0, __pyx_v_self->stream_name);
    __Pyx_GIVEREF(__pyx_v_self->stream_name);
    PyTuple_SET_ITEM(__pyx_t_6, 1, __pyx_t_3);
    __Pyx_GIVEREF(__pyx_t_3);
    PyTuple_SET_ITEM(__pyx_t_6, 2, __pyx_t_4);
    __Pyx_GIVEREF(__pyx_t_4);
    PyTuple_SET_ITEM(__pyx_t_6, 3, __pyx_t_5);
    __Pyx_GIVEREF(__pyx_t_5);
    __Pyx_INCREF(Py_None);
    PyTuple_SET_ITEM(__pyx_t_6, 4, Py_None);
    __Pyx_GIVEREF(Py_None);
    __Pyx_INCREF(Py_None);
    PyTuple_SET_ITEM(__pyx_t_6, 5, Py_None);
    __Pyx_GIVEREF(Py_None);
    __pyx_t_3 = 0;
    __pyx_t_4 = 0;
    __pyx_t_5 = 0;
    __pyx_t_5 = __Pyx_PyObject_Call(((PyObject *)((PyObject*)__pyx_ptype_5_yaml_Mark)), __pyx_t_6, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 710; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
    __pyx_v_mark = ((struct __pyx_obj_5_yaml_Mark *)__pyx_t_5);
    __pyx_t_5 = 0;

    






    __pyx_t_2 = ((PY_MAJOR_VERSION < 3) != 0);
    if (__pyx_t_2) {

      






      __pyx_t_5 = __Pyx_GetModuleGlobalName(__pyx_n_s_ComposerError); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 716; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);

      






      __pyx_t_6 = __Pyx_PyObject_GetAttrStr(__pyx_v_document, __pyx_n_s_start_mark); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 717; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);

      






      __pyx_t_4 = PyTuple_New(4); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 716; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);
      __Pyx_INCREF(__pyx_kp_s_expected_a_single_document_in_th);
      PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_kp_s_expected_a_single_document_in_th);
      __Pyx_GIVEREF(__pyx_kp_s_expected_a_single_document_in_th);
      PyTuple_SET_ITEM(__pyx_t_4, 1, __pyx_t_6);
      __Pyx_GIVEREF(__pyx_t_6);
      __Pyx_INCREF(__pyx_kp_s_but_found_another_document);
      PyTuple_SET_ITEM(__pyx_t_4, 2, __pyx_kp_s_but_found_another_document);
      __Pyx_GIVEREF(__pyx_kp_s_but_found_another_document);
      __Pyx_INCREF(((PyObject *)__pyx_v_mark));
      PyTuple_SET_ITEM(__pyx_t_4, 3, ((PyObject *)__pyx_v_mark));
      __Pyx_GIVEREF(((PyObject *)__pyx_v_mark));
      __pyx_t_6 = 0;
      __pyx_t_6 = __Pyx_PyObject_Call(__pyx_t_5, __pyx_t_4, NULL); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 716; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
      __Pyx_Raise(__pyx_t_6, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 716; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
     {

      






      __pyx_t_6 = __Pyx_GetModuleGlobalName(__pyx_n_s_ComposerError); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 719; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);

      






      __pyx_t_4 = __Pyx_PyObject_GetAttrStr(__pyx_v_document, __pyx_n_s_start_mark); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 720; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);

      






      __pyx_t_5 = PyTuple_New(4); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 719; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_INCREF(__pyx_kp_u_expected_a_single_document_in_th);
      PyTuple_SET_ITEM(__pyx_t_5, 0, __pyx_kp_u_expected_a_single_document_in_th);
      __Pyx_GIVEREF(__pyx_kp_u_expected_a_single_document_in_th);
      PyTuple_SET_ITEM(__pyx_t_5, 1, __pyx_t_4);
      __Pyx_GIVEREF(__pyx_t_4);
      __Pyx_INCREF(__pyx_kp_u_but_found_another_document);
      PyTuple_SET_ITEM(__pyx_t_5, 2, __pyx_kp_u_but_found_another_document);
      __Pyx_GIVEREF(__pyx_kp_u_but_found_another_document);
      __Pyx_INCREF(((PyObject *)__pyx_v_mark));
      PyTuple_SET_ITEM(__pyx_t_5, 3, ((PyObject *)__pyx_v_mark));
      __Pyx_GIVEREF(((PyObject *)__pyx_v_mark));
      __pyx_t_4 = 0;
      __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_6, __pyx_t_5, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 719; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);
      __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      __Pyx_Raise(__pyx_t_4, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 719; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
  }

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(__pyx_v_document);
  __pyx_r = __pyx_v_document;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_XDECREF(__pyx_t_4);
  __Pyx_XDECREF(__pyx_t_5);
  __Pyx_XDECREF(__pyx_t_6);
  __Pyx_AddTraceback("_yaml.CParser.get_single_node", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_document);
  __Pyx_XDECREF((PyObject *)__pyx_v_mark);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static PyObject *__pyx_f_5_yaml_7CParser__compose_document(struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  PyObject *__pyx_v_node = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  PyObject *__pyx_t_1 = NULL;
  int __pyx_t_2;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("_compose_document", 0);

  






  yaml_event_delete((&__pyx_v_self->parsed_event));

  






  __pyx_t_1 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_compose_node(__pyx_v_self, Py_None, Py_None); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 725; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_v_node = __pyx_t_1;
  __pyx_t_1 = 0;

  






  __pyx_t_2 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parse_next_event(__pyx_v_self); if (unlikely(__pyx_t_2 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 726; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

  






  yaml_event_delete((&__pyx_v_self->parsed_event));

  






  __pyx_t_1 = PyDict_New(); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 728; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_GIVEREF(__pyx_t_1);
  __Pyx_GOTREF(__pyx_v_self->anchors);
  __Pyx_DECREF(__pyx_v_self->anchors);
  __pyx_v_self->anchors = __pyx_t_1;
  __pyx_t_1 = 0;

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(__pyx_v_node);
  __pyx_r = __pyx_v_node;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_AddTraceback("_yaml.CParser._compose_document", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_node);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static PyObject *__pyx_f_5_yaml_7CParser__compose_node(struct __pyx_obj_5_yaml_CParser *__pyx_v_self, PyObject *__pyx_v_parent, PyObject *__pyx_v_index) {
  PyObject *__pyx_v_anchor = NULL;
  struct __pyx_obj_5_yaml_Mark *__pyx_v_mark = NULL;
  PyObject *__pyx_v_node = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  int __pyx_t_4;
  PyObject *__pyx_t_5 = NULL;
  PyObject *__pyx_t_6 = NULL;
  PyObject *__pyx_t_7 = NULL;
  int __pyx_t_8;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("_compose_node", 0);

  






  __pyx_t_1 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parse_next_event(__pyx_v_self); if (unlikely(__pyx_t_1 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 732; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

  






  __pyx_t_2 = ((__pyx_v_self->parsed_event.type == YAML_ALIAS_EVENT) != 0);
  if (__pyx_t_2) {

    






    __pyx_t_3 = PyUnicode_FromString(__pyx_v_self->parsed_event.data.alias.anchor); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 734; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_v_anchor = __pyx_t_3;
    __pyx_t_3 = 0;

    






    __pyx_t_2 = (__Pyx_PySequence_Contains(__pyx_v_anchor, __pyx_v_self->anchors, Py_NE)); if (unlikely(__pyx_t_2 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 735; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __pyx_t_4 = (__pyx_t_2 != 0);
    if (__pyx_t_4) {

      






      __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.index); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 737; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);

      






      __pyx_t_5 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.line); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 738; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);

      






      __pyx_t_6 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.column); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 739; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);

      






      __pyx_t_7 = PyTuple_New(6); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 736; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_7);
      __Pyx_INCREF(__pyx_v_self->stream_name);
      PyTuple_SET_ITEM(__pyx_t_7, 0, __pyx_v_self->stream_name);
      __Pyx_GIVEREF(__pyx_v_self->stream_name);
      PyTuple_SET_ITEM(__pyx_t_7, 1, __pyx_t_3);
      __Pyx_GIVEREF(__pyx_t_3);
      PyTuple_SET_ITEM(__pyx_t_7, 2, __pyx_t_5);
      __Pyx_GIVEREF(__pyx_t_5);
      PyTuple_SET_ITEM(__pyx_t_7, 3, __pyx_t_6);
      __Pyx_GIVEREF(__pyx_t_6);
      __Pyx_INCREF(Py_None);
      PyTuple_SET_ITEM(__pyx_t_7, 4, Py_None);
      __Pyx_GIVEREF(Py_None);
      __Pyx_INCREF(Py_None);
      PyTuple_SET_ITEM(__pyx_t_7, 5, Py_None);
      __Pyx_GIVEREF(Py_None);
      __pyx_t_3 = 0;
      __pyx_t_5 = 0;
      __pyx_t_6 = 0;
      __pyx_t_6 = __Pyx_PyObject_Call(((PyObject *)((PyObject*)__pyx_ptype_5_yaml_Mark)), __pyx_t_7, NULL); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 736; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);
      __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;
      __pyx_v_mark = ((struct __pyx_obj_5_yaml_Mark *)__pyx_t_6);
      __pyx_t_6 = 0;

      






      __pyx_t_4 = ((PY_MAJOR_VERSION < 3) != 0);
      if (__pyx_t_4) {

        






        __pyx_t_6 = __Pyx_GetModuleGlobalName(__pyx_n_s_ComposerError); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 742; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_6);
        __pyx_t_7 = PyTuple_New(4); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 742; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_7);
        __Pyx_INCREF(Py_None);
        PyTuple_SET_ITEM(__pyx_t_7, 0, Py_None);
        __Pyx_GIVEREF(Py_None);
        __Pyx_INCREF(Py_None);
        PyTuple_SET_ITEM(__pyx_t_7, 1, Py_None);
        __Pyx_GIVEREF(Py_None);
        __Pyx_INCREF(__pyx_kp_s_found_undefined_alias);
        PyTuple_SET_ITEM(__pyx_t_7, 2, __pyx_kp_s_found_undefined_alias);
        __Pyx_GIVEREF(__pyx_kp_s_found_undefined_alias);
        __Pyx_INCREF(((PyObject *)__pyx_v_mark));
        PyTuple_SET_ITEM(__pyx_t_7, 3, ((PyObject *)__pyx_v_mark));
        __Pyx_GIVEREF(((PyObject *)__pyx_v_mark));
        __pyx_t_5 = __Pyx_PyObject_Call(__pyx_t_6, __pyx_t_7, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 742; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
        __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;
        __Pyx_Raise(__pyx_t_5, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 742; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
       {

        






        __pyx_t_5 = __Pyx_GetModuleGlobalName(__pyx_n_s_ComposerError); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 744; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __pyx_t_7 = PyTuple_New(4); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 744; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_7);
        __Pyx_INCREF(Py_None);
        PyTuple_SET_ITEM(__pyx_t_7, 0, Py_None);
        __Pyx_GIVEREF(Py_None);
        __Pyx_INCREF(Py_None);
        PyTuple_SET_ITEM(__pyx_t_7, 1, Py_None);
        __Pyx_GIVEREF(Py_None);
        __Pyx_INCREF(__pyx_kp_u_found_undefined_alias);
        PyTuple_SET_ITEM(__pyx_t_7, 2, __pyx_kp_u_found_undefined_alias);
        __Pyx_GIVEREF(__pyx_kp_u_found_undefined_alias);
        __Pyx_INCREF(((PyObject *)__pyx_v_mark));
        PyTuple_SET_ITEM(__pyx_t_7, 3, ((PyObject *)__pyx_v_mark));
        __Pyx_GIVEREF(((PyObject *)__pyx_v_mark));
        __pyx_t_6 = __Pyx_PyObject_Call(__pyx_t_5, __pyx_t_7, NULL); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 744; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_6);
        __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
        __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;
        __Pyx_Raise(__pyx_t_6, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 744; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
    }

    






    yaml_event_delete((&__pyx_v_self->parsed_event));

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_6 = PyObject_GetItem(__pyx_v_self->anchors, __pyx_v_anchor); if (unlikely(__pyx_t_6 == NULL)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 746; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
    __Pyx_GOTREF(__pyx_t_6);
    __pyx_r = __pyx_t_6;
    __pyx_t_6 = 0;
    goto __pyx_L0;
  }

  






  __Pyx_INCREF(Py_None);
  __pyx_v_anchor = Py_None;

  






  __pyx_t_4 = ((__pyx_v_self->parsed_event.type == YAML_SCALAR_EVENT) != 0);
  if (__pyx_t_4) {

    






    __pyx_t_2 = ((__pyx_v_self->parsed_event.data.scalar.anchor != NULL) != 0);
    __pyx_t_8 = __pyx_t_2;
  } else {
    __pyx_t_8 = __pyx_t_4;
  }
  if (__pyx_t_8) {

    






    __pyx_t_6 = PyUnicode_FromString(__pyx_v_self->parsed_event.data.scalar.anchor); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 750; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_6);
    __Pyx_DECREF_SET(__pyx_v_anchor, __pyx_t_6);
    __pyx_t_6 = 0;
    goto __pyx_L6;
  }

  






  __pyx_t_8 = ((__pyx_v_self->parsed_event.type == YAML_SEQUENCE_START_EVENT) != 0);
  if (__pyx_t_8) {

    






    __pyx_t_4 = ((__pyx_v_self->parsed_event.data.sequence_start.anchor != NULL) != 0);
    __pyx_t_2 = __pyx_t_4;
  } else {
    __pyx_t_2 = __pyx_t_8;
  }
  if (__pyx_t_2) {

    






    __pyx_t_6 = PyUnicode_FromString(__pyx_v_self->parsed_event.data.sequence_start.anchor); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 753; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_6);
    __Pyx_DECREF_SET(__pyx_v_anchor, __pyx_t_6);
    __pyx_t_6 = 0;
    goto __pyx_L6;
  }

  






  __pyx_t_2 = ((__pyx_v_self->parsed_event.type == YAML_MAPPING_START_EVENT) != 0);
  if (__pyx_t_2) {

    






    __pyx_t_8 = ((__pyx_v_self->parsed_event.data.mapping_start.anchor != NULL) != 0);
    __pyx_t_4 = __pyx_t_8;
  } else {
    __pyx_t_4 = __pyx_t_2;
  }
  if (__pyx_t_4) {

    






    __pyx_t_6 = PyUnicode_FromString(__pyx_v_self->parsed_event.data.mapping_start.anchor); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 756; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_6);
    __Pyx_DECREF_SET(__pyx_v_anchor, __pyx_t_6);
    __pyx_t_6 = 0;
    goto __pyx_L6;
  }
  __pyx_L6:;

  






  __pyx_t_4 = (__pyx_v_anchor != Py_None);
  __pyx_t_2 = (__pyx_t_4 != 0);
  if (__pyx_t_2) {

    






    __pyx_t_2 = (__Pyx_PySequence_Contains(__pyx_v_anchor, __pyx_v_self->anchors, Py_EQ)); if (unlikely(__pyx_t_2 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 758; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __pyx_t_4 = (__pyx_t_2 != 0);
    if (__pyx_t_4) {

      






      __pyx_t_6 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.index); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 760; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);

      






      __pyx_t_7 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.line); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 761; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_7);

      






      __pyx_t_5 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.column); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 762; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);

      






      __pyx_t_3 = PyTuple_New(6); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 759; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __Pyx_INCREF(__pyx_v_self->stream_name);
      PyTuple_SET_ITEM(__pyx_t_3, 0, __pyx_v_self->stream_name);
      __Pyx_GIVEREF(__pyx_v_self->stream_name);
      PyTuple_SET_ITEM(__pyx_t_3, 1, __pyx_t_6);
      __Pyx_GIVEREF(__pyx_t_6);
      PyTuple_SET_ITEM(__pyx_t_3, 2, __pyx_t_7);
      __Pyx_GIVEREF(__pyx_t_7);
      PyTuple_SET_ITEM(__pyx_t_3, 3, __pyx_t_5);
      __Pyx_GIVEREF(__pyx_t_5);
      __Pyx_INCREF(Py_None);
      PyTuple_SET_ITEM(__pyx_t_3, 4, Py_None);
      __Pyx_GIVEREF(Py_None);
      __Pyx_INCREF(Py_None);
      PyTuple_SET_ITEM(__pyx_t_3, 5, Py_None);
      __Pyx_GIVEREF(Py_None);
      __pyx_t_6 = 0;
      __pyx_t_7 = 0;
      __pyx_t_5 = 0;
      __pyx_t_5 = __Pyx_PyObject_Call(((PyObject *)((PyObject*)__pyx_ptype_5_yaml_Mark)), __pyx_t_3, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 759; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
      __pyx_v_mark = ((struct __pyx_obj_5_yaml_Mark *)__pyx_t_5);
      __pyx_t_5 = 0;

      






      __pyx_t_4 = ((PY_MAJOR_VERSION < 3) != 0);
      if (__pyx_t_4) {

        






        __pyx_t_5 = __Pyx_GetModuleGlobalName(__pyx_n_s_ComposerError); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 765; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);

        






        __pyx_t_3 = PyObject_GetItem(__pyx_v_self->anchors, __pyx_v_anchor); if (unlikely(__pyx_t_3 == NULL)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 766; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
        __Pyx_GOTREF(__pyx_t_3);
        __pyx_t_7 = __Pyx_PyObject_GetAttrStr(__pyx_t_3, __pyx_n_s_start_mark); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 766; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_7);
        __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;

        






        __pyx_t_3 = PyTuple_New(4); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 765; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_3);
        __Pyx_INCREF(__pyx_kp_s_found_duplicate_anchor_first_occ);
        PyTuple_SET_ITEM(__pyx_t_3, 0, __pyx_kp_s_found_duplicate_anchor_first_occ);
        __Pyx_GIVEREF(__pyx_kp_s_found_duplicate_anchor_first_occ);
        PyTuple_SET_ITEM(__pyx_t_3, 1, __pyx_t_7);
        __Pyx_GIVEREF(__pyx_t_7);
        __Pyx_INCREF(__pyx_kp_s_second_occurence);
        PyTuple_SET_ITEM(__pyx_t_3, 2, __pyx_kp_s_second_occurence);
        __Pyx_GIVEREF(__pyx_kp_s_second_occurence);
        __Pyx_INCREF(((PyObject *)__pyx_v_mark));
        PyTuple_SET_ITEM(__pyx_t_3, 3, ((PyObject *)__pyx_v_mark));
        __Pyx_GIVEREF(((PyObject *)__pyx_v_mark));
        __pyx_t_7 = 0;
        __pyx_t_7 = __Pyx_PyObject_Call(__pyx_t_5, __pyx_t_3, NULL); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 765; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_7);
        __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
        __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
        __Pyx_Raise(__pyx_t_7, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 765; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
       {

        






        __pyx_t_7 = __Pyx_GetModuleGlobalName(__pyx_n_s_ComposerError); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 768; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_7);

        






        __pyx_t_3 = PyObject_GetItem(__pyx_v_self->anchors, __pyx_v_anchor); if (unlikely(__pyx_t_3 == NULL)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 769; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
        __Pyx_GOTREF(__pyx_t_3);
        __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_t_3, __pyx_n_s_start_mark); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 769; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;

        






        __pyx_t_3 = PyTuple_New(4); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 768; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_3);
        __Pyx_INCREF(__pyx_kp_u_found_duplicate_anchor_first_occ);
        PyTuple_SET_ITEM(__pyx_t_3, 0, __pyx_kp_u_found_duplicate_anchor_first_occ);
        __Pyx_GIVEREF(__pyx_kp_u_found_duplicate_anchor_first_occ);
        PyTuple_SET_ITEM(__pyx_t_3, 1, __pyx_t_5);
        __Pyx_GIVEREF(__pyx_t_5);
        __Pyx_INCREF(__pyx_kp_u_second_occurence);
        PyTuple_SET_ITEM(__pyx_t_3, 2, __pyx_kp_u_second_occurence);
        __Pyx_GIVEREF(__pyx_kp_u_second_occurence);
        __Pyx_INCREF(((PyObject *)__pyx_v_mark));
        PyTuple_SET_ITEM(__pyx_t_3, 3, ((PyObject *)__pyx_v_mark));
        __Pyx_GIVEREF(((PyObject *)__pyx_v_mark));
        __pyx_t_5 = 0;
        __pyx_t_5 = __Pyx_PyObject_Call(__pyx_t_7, __pyx_t_3, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 768; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;
        __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
        __Pyx_Raise(__pyx_t_5, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 768; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
    }
    goto __pyx_L7;
  }
  __pyx_L7:;

  






  __pyx_t_5 = __Pyx_PyObject_GetAttrStr(((PyObject *)__pyx_v_self), __pyx_n_s_descend_resolver); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 770; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_5);
  __pyx_t_3 = PyTuple_New(2); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 770; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __Pyx_INCREF(__pyx_v_parent);
  PyTuple_SET_ITEM(__pyx_t_3, 0, __pyx_v_parent);
  __Pyx_GIVEREF(__pyx_v_parent);
  __Pyx_INCREF(__pyx_v_index);
  PyTuple_SET_ITEM(__pyx_t_3, 1, __pyx_v_index);
  __Pyx_GIVEREF(__pyx_v_index);
  __pyx_t_7 = __Pyx_PyObject_Call(__pyx_t_5, __pyx_t_3, NULL); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 770; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_7);
  __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
  __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
  __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;

  






  switch (__pyx_v_self->parsed_event.type) {

    






    case YAML_SCALAR_EVENT:

    






    __pyx_t_7 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_compose_scalar_node(__pyx_v_self, __pyx_v_anchor); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 772; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_7);
    __pyx_v_node = __pyx_t_7;
    __pyx_t_7 = 0;
    break;

    






    case YAML_SEQUENCE_START_EVENT:

    






    __pyx_t_7 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_compose_sequence_node(__pyx_v_self, __pyx_v_anchor); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 774; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_7);
    __pyx_v_node = __pyx_t_7;
    __pyx_t_7 = 0;
    break;

    






    case YAML_MAPPING_START_EVENT:

    






    __pyx_t_7 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_compose_mapping_node(__pyx_v_self, __pyx_v_anchor); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 776; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_7);
    __pyx_v_node = __pyx_t_7;
    __pyx_t_7 = 0;
    break;
    default: break;
  }

  






  __pyx_t_7 = __Pyx_PyObject_GetAttrStr(((PyObject *)__pyx_v_self), __pyx_n_s_ascend_resolver); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 777; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_7);
  __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_7, __pyx_empty_tuple, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 777; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;
  __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;

  






  __Pyx_XDECREF(__pyx_r);
  if (unlikely(!__pyx_v_node)) { __Pyx_RaiseUnboundLocalError("node"); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 778; __pyx_clineno = __LINE__; goto __pyx_L1_error;} }
  __Pyx_INCREF(__pyx_v_node);
  __pyx_r = __pyx_v_node;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_XDECREF(__pyx_t_5);
  __Pyx_XDECREF(__pyx_t_6);
  __Pyx_XDECREF(__pyx_t_7);
  __Pyx_AddTraceback("_yaml.CParser._compose_node", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_anchor);
  __Pyx_XDECREF((PyObject *)__pyx_v_mark);
  __Pyx_XDECREF(__pyx_v_node);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static PyObject *__pyx_f_5_yaml_7CParser__compose_scalar_node(struct __pyx_obj_5_yaml_CParser *__pyx_v_self, PyObject *__pyx_v_anchor) {
  struct __pyx_obj_5_yaml_Mark *__pyx_v_start_mark = NULL;
  struct __pyx_obj_5_yaml_Mark *__pyx_v_end_mark = NULL;
  PyObject *__pyx_v_value = NULL;
  int __pyx_v_plain_implicit;
  int __pyx_v_quoted_implicit;
  PyObject *__pyx_v_tag = NULL;
  PyObject *__pyx_v_style = NULL;
  PyObject *__pyx_v_node = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  PyObject *__pyx_t_1 = NULL;
  PyObject *__pyx_t_2 = NULL;
  PyObject *__pyx_t_3 = NULL;
  PyObject *__pyx_t_4 = NULL;
  int __pyx_t_5;
  int __pyx_t_6;
  int __pyx_t_7;
  int __pyx_t_8;
  PyObject *__pyx_t_9 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("_compose_scalar_node", 0);

  






  __pyx_t_1 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.index); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 782; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);

  






  __pyx_t_2 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.line); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 783; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);

  






  __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.column); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 784; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);

  






  __pyx_t_4 = PyTuple_New(6); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 781; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_4);
  __Pyx_INCREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_v_self->stream_name);
  __Pyx_GIVEREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_4, 1, __pyx_t_1);
  __Pyx_GIVEREF(__pyx_t_1);
  PyTuple_SET_ITEM(__pyx_t_4, 2, __pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  PyTuple_SET_ITEM(__pyx_t_4, 3, __pyx_t_3);
  __Pyx_GIVEREF(__pyx_t_3);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_4, 4, Py_None);
  __Pyx_GIVEREF(Py_None);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_4, 5, Py_None);
  __Pyx_GIVEREF(Py_None);
  __pyx_t_1 = 0;
  __pyx_t_2 = 0;
  __pyx_t_3 = 0;
  __pyx_t_3 = __Pyx_PyObject_Call(((PyObject *)((PyObject*)__pyx_ptype_5_yaml_Mark)), __pyx_t_4, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 781; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
  __pyx_v_start_mark = ((struct __pyx_obj_5_yaml_Mark *)__pyx_t_3);
  __pyx_t_3 = 0;

  






  __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.end_mark.index); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 787; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);

  






  __pyx_t_4 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.end_mark.line); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 788; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_4);

  






  __pyx_t_2 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.end_mark.column); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 789; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);

  






  __pyx_t_1 = PyTuple_New(6); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 786; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_INCREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_1, 0, __pyx_v_self->stream_name);
  __Pyx_GIVEREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_1, 1, __pyx_t_3);
  __Pyx_GIVEREF(__pyx_t_3);
  PyTuple_SET_ITEM(__pyx_t_1, 2, __pyx_t_4);
  __Pyx_GIVEREF(__pyx_t_4);
  PyTuple_SET_ITEM(__pyx_t_1, 3, __pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_1, 4, Py_None);
  __Pyx_GIVEREF(Py_None);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_1, 5, Py_None);
  __Pyx_GIVEREF(Py_None);
  __pyx_t_3 = 0;
  __pyx_t_4 = 0;
  __pyx_t_2 = 0;
  __pyx_t_2 = __Pyx_PyObject_Call(((PyObject *)((PyObject*)__pyx_ptype_5_yaml_Mark)), __pyx_t_1, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 786; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_v_end_mark = ((struct __pyx_obj_5_yaml_Mark *)__pyx_t_2);
  __pyx_t_2 = 0;

  






  __pyx_t_2 = PyUnicode_DecodeUTF8(__pyx_v_self->parsed_event.data.scalar.value, __pyx_v_self->parsed_event.data.scalar.length, __pyx_k_strict); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 791; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __pyx_v_value = __pyx_t_2;
  __pyx_t_2 = 0;

  






  __pyx_v_plain_implicit = 0;

  






  __pyx_t_5 = ((__pyx_v_self->parsed_event.data.scalar.plain_implicit == 1) != 0);
  if (__pyx_t_5) {

    






    __pyx_v_plain_implicit = 1;
    goto __pyx_L3;
  }
  __pyx_L3:;

  






  __pyx_v_quoted_implicit = 0;

  






  __pyx_t_5 = ((__pyx_v_self->parsed_event.data.scalar.quoted_implicit == 1) != 0);
  if (__pyx_t_5) {

    






    __pyx_v_quoted_implicit = 1;
    goto __pyx_L4;
  }
  __pyx_L4:;

  






  __pyx_t_5 = ((__pyx_v_self->parsed_event.data.scalar.tag == NULL) != 0);
  if (!__pyx_t_5) {

    






    __pyx_t_6 = (((__pyx_v_self->parsed_event.data.scalar.tag[0]) == '!') != 0);
    if (__pyx_t_6) {

      






      __pyx_t_7 = (((__pyx_v_self->parsed_event.data.scalar.tag[1]) == '\x00') != 0);
      __pyx_t_8 = __pyx_t_7;
    } else {
      __pyx_t_8 = __pyx_t_6;
    }
    __pyx_t_6 = __pyx_t_8;
  } else {
    __pyx_t_6 = __pyx_t_5;
  }
  if (__pyx_t_6) {

    






    __pyx_t_2 = __Pyx_PyObject_GetAttrStr(((PyObject *)__pyx_v_self), __pyx_n_s_resolve); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 802; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_ScalarNode); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 802; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __pyx_t_4 = __Pyx_PyBool_FromLong(__pyx_v_plain_implicit); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 802; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_3 = __Pyx_PyBool_FromLong(__pyx_v_quoted_implicit); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 802; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_9 = PyTuple_New(2); if (unlikely(!__pyx_t_9)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 802; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_9);
    PyTuple_SET_ITEM(__pyx_t_9, 0, __pyx_t_4);
    __Pyx_GIVEREF(__pyx_t_4);
    PyTuple_SET_ITEM(__pyx_t_9, 1, __pyx_t_3);
    __Pyx_GIVEREF(__pyx_t_3);
    __pyx_t_4 = 0;
    __pyx_t_3 = 0;
    __pyx_t_3 = PyTuple_New(3); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 802; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    PyTuple_SET_ITEM(__pyx_t_3, 0, __pyx_t_1);
    __Pyx_GIVEREF(__pyx_t_1);
    __Pyx_INCREF(__pyx_v_value);
    PyTuple_SET_ITEM(__pyx_t_3, 1, __pyx_v_value);
    __Pyx_GIVEREF(__pyx_v_value);
    PyTuple_SET_ITEM(__pyx_t_3, 2, __pyx_t_9);
    __Pyx_GIVEREF(__pyx_t_9);
    __pyx_t_1 = 0;
    __pyx_t_9 = 0;
    __pyx_t_9 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_t_3, NULL); if (unlikely(!__pyx_t_9)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 802; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_9);
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __pyx_v_tag = __pyx_t_9;
    __pyx_t_9 = 0;
    goto __pyx_L5;
  }
   {

    






    __pyx_t_9 = PyUnicode_FromString(__pyx_v_self->parsed_event.data.scalar.tag); if (unlikely(!__pyx_t_9)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 804; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_9);
    __pyx_v_tag = __pyx_t_9;
    __pyx_t_9 = 0;
  }
  __pyx_L5:;

  






  __Pyx_INCREF(Py_None);
  __pyx_v_style = Py_None;

  






  switch (__pyx_v_self->parsed_event.data.scalar.style) {

    






    case YAML_PLAIN_SCALAR_STYLE:

    






    __Pyx_INCREF(__pyx_kp_u__6);
    __Pyx_DECREF_SET(__pyx_v_style, __pyx_kp_u__6);
    break;

    






    case YAML_SINGLE_QUOTED_SCALAR_STYLE:

    






    __Pyx_INCREF(__pyx_kp_u__7);
    __Pyx_DECREF_SET(__pyx_v_style, __pyx_kp_u__7);
    break;

    






    case YAML_DOUBLE_QUOTED_SCALAR_STYLE:

    






    __Pyx_INCREF(__pyx_kp_u__8);
    __Pyx_DECREF_SET(__pyx_v_style, __pyx_kp_u__8);
    break;

    






    case YAML_LITERAL_SCALAR_STYLE:

    






    __Pyx_INCREF(__pyx_kp_u__9);
    __Pyx_DECREF_SET(__pyx_v_style, __pyx_kp_u__9);
    break;

    






    case YAML_FOLDED_SCALAR_STYLE:

    






    __Pyx_INCREF(__pyx_kp_u__10);
    __Pyx_DECREF_SET(__pyx_v_style, __pyx_kp_u__10);
    break;
    default: break;
  }

  






  __pyx_t_9 = __Pyx_GetModuleGlobalName(__pyx_n_s_ScalarNode); if (unlikely(!__pyx_t_9)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 816; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_9);
  __pyx_t_3 = PyTuple_New(5); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 816; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __Pyx_INCREF(__pyx_v_tag);
  PyTuple_SET_ITEM(__pyx_t_3, 0, __pyx_v_tag);
  __Pyx_GIVEREF(__pyx_v_tag);
  __Pyx_INCREF(__pyx_v_value);
  PyTuple_SET_ITEM(__pyx_t_3, 1, __pyx_v_value);
  __Pyx_GIVEREF(__pyx_v_value);
  __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
  PyTuple_SET_ITEM(__pyx_t_3, 2, ((PyObject *)__pyx_v_start_mark));
  __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
  __Pyx_INCREF(((PyObject *)__pyx_v_end_mark));
  PyTuple_SET_ITEM(__pyx_t_3, 3, ((PyObject *)__pyx_v_end_mark));
  __Pyx_GIVEREF(((PyObject *)__pyx_v_end_mark));
  __Pyx_INCREF(__pyx_v_style);
  PyTuple_SET_ITEM(__pyx_t_3, 4, __pyx_v_style);
  __Pyx_GIVEREF(__pyx_v_style);
  __pyx_t_2 = __Pyx_PyObject_Call(__pyx_t_9, __pyx_t_3, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 816; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_9); __pyx_t_9 = 0;
  __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
  __pyx_v_node = __pyx_t_2;
  __pyx_t_2 = 0;

  






  __pyx_t_6 = (__pyx_v_anchor != Py_None);
  __pyx_t_5 = (__pyx_t_6 != 0);
  if (__pyx_t_5) {

    






    if (unlikely(PyObject_SetItem(__pyx_v_self->anchors, __pyx_v_anchor, __pyx_v_node) < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 818; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    goto __pyx_L6;
  }
  __pyx_L6:;

  






  yaml_event_delete((&__pyx_v_self->parsed_event));

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(__pyx_v_node);
  __pyx_r = __pyx_v_node;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_XDECREF(__pyx_t_2);
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_XDECREF(__pyx_t_4);
  __Pyx_XDECREF(__pyx_t_9);
  __Pyx_AddTraceback("_yaml.CParser._compose_scalar_node", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF((PyObject *)__pyx_v_start_mark);
  __Pyx_XDECREF((PyObject *)__pyx_v_end_mark);
  __Pyx_XDECREF(__pyx_v_value);
  __Pyx_XDECREF(__pyx_v_tag);
  __Pyx_XDECREF(__pyx_v_style);
  __Pyx_XDECREF(__pyx_v_node);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static PyObject *__pyx_f_5_yaml_7CParser__compose_sequence_node(struct __pyx_obj_5_yaml_CParser *__pyx_v_self, PyObject *__pyx_v_anchor) {
  int __pyx_v_index;
  struct __pyx_obj_5_yaml_Mark *__pyx_v_start_mark = NULL;
  int __pyx_v_implicit;
  PyObject *__pyx_v_tag = NULL;
  PyObject *__pyx_v_flow_style = NULL;
  PyObject *__pyx_v_value = NULL;
  PyObject *__pyx_v_node = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  PyObject *__pyx_t_1 = NULL;
  PyObject *__pyx_t_2 = NULL;
  PyObject *__pyx_t_3 = NULL;
  PyObject *__pyx_t_4 = NULL;
  int __pyx_t_5;
  int __pyx_t_6;
  int __pyx_t_7;
  int __pyx_t_8;
  int __pyx_t_9;
  int __pyx_t_10;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("_compose_sequence_node", 0);

  






  __pyx_t_1 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.index); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 825; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);

  






  __pyx_t_2 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.line); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 826; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);

  






  __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.column); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 827; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);

  






  __pyx_t_4 = PyTuple_New(6); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 824; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_4);
  __Pyx_INCREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_v_self->stream_name);
  __Pyx_GIVEREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_4, 1, __pyx_t_1);
  __Pyx_GIVEREF(__pyx_t_1);
  PyTuple_SET_ITEM(__pyx_t_4, 2, __pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  PyTuple_SET_ITEM(__pyx_t_4, 3, __pyx_t_3);
  __Pyx_GIVEREF(__pyx_t_3);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_4, 4, Py_None);
  __Pyx_GIVEREF(Py_None);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_4, 5, Py_None);
  __Pyx_GIVEREF(Py_None);
  __pyx_t_1 = 0;
  __pyx_t_2 = 0;
  __pyx_t_3 = 0;
  __pyx_t_3 = __Pyx_PyObject_Call(((PyObject *)((PyObject*)__pyx_ptype_5_yaml_Mark)), __pyx_t_4, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 824; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
  __pyx_v_start_mark = ((struct __pyx_obj_5_yaml_Mark *)__pyx_t_3);
  __pyx_t_3 = 0;

  






  __pyx_v_implicit = 0;

  






  __pyx_t_5 = ((__pyx_v_self->parsed_event.data.sequence_start.implicit == 1) != 0);
  if (__pyx_t_5) {

    






    __pyx_v_implicit = 1;
    goto __pyx_L3;
  }
  __pyx_L3:;

  






  __pyx_t_5 = ((__pyx_v_self->parsed_event.data.sequence_start.tag == NULL) != 0);
  if (!__pyx_t_5) {

    






    __pyx_t_6 = (((__pyx_v_self->parsed_event.data.sequence_start.tag[0]) == '!') != 0);
    if (__pyx_t_6) {

      






      __pyx_t_7 = (((__pyx_v_self->parsed_event.data.sequence_start.tag[1]) == '\x00') != 0);
      __pyx_t_8 = __pyx_t_7;
    } else {
      __pyx_t_8 = __pyx_t_6;
    }
    __pyx_t_6 = __pyx_t_8;
  } else {
    __pyx_t_6 = __pyx_t_5;
  }
  if (__pyx_t_6) {

    






    __pyx_t_3 = __Pyx_PyObject_GetAttrStr(((PyObject *)__pyx_v_self), __pyx_n_s_resolve); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 835; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_4 = __Pyx_GetModuleGlobalName(__pyx_n_s_SequenceNode); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 835; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_2 = __Pyx_PyBool_FromLong(__pyx_v_implicit); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 835; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_t_1 = PyTuple_New(3); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 835; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    PyTuple_SET_ITEM(__pyx_t_1, 0, __pyx_t_4);
    __Pyx_GIVEREF(__pyx_t_4);
    __Pyx_INCREF(Py_None);
    PyTuple_SET_ITEM(__pyx_t_1, 1, Py_None);
    __Pyx_GIVEREF(Py_None);
    PyTuple_SET_ITEM(__pyx_t_1, 2, __pyx_t_2);
    __Pyx_GIVEREF(__pyx_t_2);
    __pyx_t_4 = 0;
    __pyx_t_2 = 0;
    __pyx_t_2 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_t_1, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 835; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    __pyx_v_tag = __pyx_t_2;
    __pyx_t_2 = 0;
    goto __pyx_L4;
  }
   {

    






    __pyx_t_2 = PyUnicode_FromString(__pyx_v_self->parsed_event.data.sequence_start.tag); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 837; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_v_tag = __pyx_t_2;
    __pyx_t_2 = 0;
  }
  __pyx_L4:;

  






  __Pyx_INCREF(Py_None);
  __pyx_v_flow_style = Py_None;

  






  switch (__pyx_v_self->parsed_event.data.sequence_start.style) {

    






    case YAML_FLOW_SEQUENCE_STYLE:

    






    __Pyx_INCREF(Py_True);
    __Pyx_DECREF_SET(__pyx_v_flow_style, Py_True);
    break;

    






    case YAML_BLOCK_SEQUENCE_STYLE:

    






    __Pyx_INCREF(Py_False);
    __Pyx_DECREF_SET(__pyx_v_flow_style, Py_False);
    break;
    default: break;
  }

  






  __pyx_t_2 = PyList_New(0); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 843; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __pyx_v_value = ((PyObject*)__pyx_t_2);
  __pyx_t_2 = 0;

  






  __pyx_t_2 = __Pyx_GetModuleGlobalName(__pyx_n_s_SequenceNode); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 844; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __pyx_t_1 = PyTuple_New(5); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 844; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_INCREF(__pyx_v_tag);
  PyTuple_SET_ITEM(__pyx_t_1, 0, __pyx_v_tag);
  __Pyx_GIVEREF(__pyx_v_tag);
  __Pyx_INCREF(__pyx_v_value);
  PyTuple_SET_ITEM(__pyx_t_1, 1, __pyx_v_value);
  __Pyx_GIVEREF(__pyx_v_value);
  __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
  PyTuple_SET_ITEM(__pyx_t_1, 2, ((PyObject *)__pyx_v_start_mark));
  __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_1, 3, Py_None);
  __Pyx_GIVEREF(Py_None);
  __Pyx_INCREF(__pyx_v_flow_style);
  PyTuple_SET_ITEM(__pyx_t_1, 4, __pyx_v_flow_style);
  __Pyx_GIVEREF(__pyx_v_flow_style);
  __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_t_1, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 844; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_v_node = __pyx_t_3;
  __pyx_t_3 = 0;

  






  __pyx_t_6 = (__pyx_v_anchor != Py_None);
  __pyx_t_5 = (__pyx_t_6 != 0);
  if (__pyx_t_5) {

    






    if (unlikely(PyObject_SetItem(__pyx_v_self->anchors, __pyx_v_anchor, __pyx_v_node) < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 846; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    goto __pyx_L5;
  }
  __pyx_L5:;

  






  yaml_event_delete((&__pyx_v_self->parsed_event));

  






  __pyx_v_index = 0;

  






  __pyx_t_9 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parse_next_event(__pyx_v_self); if (unlikely(__pyx_t_9 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 849; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

  






  while (1) {
    __pyx_t_5 = ((__pyx_v_self->parsed_event.type != YAML_SEQUENCE_END_EVENT) != 0);
    if (!__pyx_t_5) break;

    






    __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_index); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 851; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_1 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_compose_node(__pyx_v_self, __pyx_v_node, __pyx_t_3); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 851; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __pyx_t_10 = __Pyx_PyList_Append(__pyx_v_value, __pyx_t_1); if (unlikely(__pyx_t_10 == -1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 851; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

    






    __pyx_v_index = (__pyx_v_index + 1);

    






    __pyx_t_9 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parse_next_event(__pyx_v_self); if (unlikely(__pyx_t_9 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 853; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  }

  






  __pyx_t_1 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.end_mark.index); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 855; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);

  






  __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.end_mark.line); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 856; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);

  






  __pyx_t_2 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.end_mark.column); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 857; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);

  






  __pyx_t_4 = PyTuple_New(6); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 854; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_4);
  __Pyx_INCREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_v_self->stream_name);
  __Pyx_GIVEREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_4, 1, __pyx_t_1);
  __Pyx_GIVEREF(__pyx_t_1);
  PyTuple_SET_ITEM(__pyx_t_4, 2, __pyx_t_3);
  __Pyx_GIVEREF(__pyx_t_3);
  PyTuple_SET_ITEM(__pyx_t_4, 3, __pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_4, 4, Py_None);
  __Pyx_GIVEREF(Py_None);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_4, 5, Py_None);
  __Pyx_GIVEREF(Py_None);
  __pyx_t_1 = 0;
  __pyx_t_3 = 0;
  __pyx_t_2 = 0;
  __pyx_t_2 = __Pyx_PyObject_Call(((PyObject *)((PyObject*)__pyx_ptype_5_yaml_Mark)), __pyx_t_4, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 854; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
  if (__Pyx_PyObject_SetAttrStr(__pyx_v_node, __pyx_n_s_end_mark, __pyx_t_2) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 854; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;

  






  yaml_event_delete((&__pyx_v_self->parsed_event));

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(__pyx_v_node);
  __pyx_r = __pyx_v_node;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_XDECREF(__pyx_t_2);
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_XDECREF(__pyx_t_4);
  __Pyx_AddTraceback("_yaml.CParser._compose_sequence_node", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF((PyObject *)__pyx_v_start_mark);
  __Pyx_XDECREF(__pyx_v_tag);
  __Pyx_XDECREF(__pyx_v_flow_style);
  __Pyx_XDECREF(__pyx_v_value);
  __Pyx_XDECREF(__pyx_v_node);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static PyObject *__pyx_f_5_yaml_7CParser__compose_mapping_node(struct __pyx_obj_5_yaml_CParser *__pyx_v_self, PyObject *__pyx_v_anchor) {
  struct __pyx_obj_5_yaml_Mark *__pyx_v_start_mark = NULL;
  int __pyx_v_implicit;
  PyObject *__pyx_v_tag = NULL;
  PyObject *__pyx_v_flow_style = NULL;
  PyObject *__pyx_v_value = NULL;
  PyObject *__pyx_v_node = NULL;
  PyObject *__pyx_v_item_key = NULL;
  PyObject *__pyx_v_item_value = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  PyObject *__pyx_t_1 = NULL;
  PyObject *__pyx_t_2 = NULL;
  PyObject *__pyx_t_3 = NULL;
  PyObject *__pyx_t_4 = NULL;
  int __pyx_t_5;
  int __pyx_t_6;
  int __pyx_t_7;
  int __pyx_t_8;
  int __pyx_t_9;
  int __pyx_t_10;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("_compose_mapping_node", 0);

  






  __pyx_t_1 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.index); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 864; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);

  






  __pyx_t_2 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.line); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 865; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);

  






  __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.start_mark.column); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 866; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);

  






  __pyx_t_4 = PyTuple_New(6); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 863; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_4);
  __Pyx_INCREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_v_self->stream_name);
  __Pyx_GIVEREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_4, 1, __pyx_t_1);
  __Pyx_GIVEREF(__pyx_t_1);
  PyTuple_SET_ITEM(__pyx_t_4, 2, __pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  PyTuple_SET_ITEM(__pyx_t_4, 3, __pyx_t_3);
  __Pyx_GIVEREF(__pyx_t_3);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_4, 4, Py_None);
  __Pyx_GIVEREF(Py_None);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_4, 5, Py_None);
  __Pyx_GIVEREF(Py_None);
  __pyx_t_1 = 0;
  __pyx_t_2 = 0;
  __pyx_t_3 = 0;
  __pyx_t_3 = __Pyx_PyObject_Call(((PyObject *)((PyObject*)__pyx_ptype_5_yaml_Mark)), __pyx_t_4, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 863; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
  __pyx_v_start_mark = ((struct __pyx_obj_5_yaml_Mark *)__pyx_t_3);
  __pyx_t_3 = 0;

  






  __pyx_v_implicit = 0;

  






  __pyx_t_5 = ((__pyx_v_self->parsed_event.data.mapping_start.implicit == 1) != 0);
  if (__pyx_t_5) {

    






    __pyx_v_implicit = 1;
    goto __pyx_L3;
  }
  __pyx_L3:;

  






  __pyx_t_5 = ((__pyx_v_self->parsed_event.data.mapping_start.tag == NULL) != 0);
  if (!__pyx_t_5) {

    






    __pyx_t_6 = (((__pyx_v_self->parsed_event.data.mapping_start.tag[0]) == '!') != 0);
    if (__pyx_t_6) {

      






      __pyx_t_7 = (((__pyx_v_self->parsed_event.data.mapping_start.tag[1]) == '\x00') != 0);
      __pyx_t_8 = __pyx_t_7;
    } else {
      __pyx_t_8 = __pyx_t_6;
    }
    __pyx_t_6 = __pyx_t_8;
  } else {
    __pyx_t_6 = __pyx_t_5;
  }
  if (__pyx_t_6) {

    






    __pyx_t_3 = __Pyx_PyObject_GetAttrStr(((PyObject *)__pyx_v_self), __pyx_n_s_resolve); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 874; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_4 = __Pyx_GetModuleGlobalName(__pyx_n_s_MappingNode); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 874; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_2 = __Pyx_PyBool_FromLong(__pyx_v_implicit); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 874; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_t_1 = PyTuple_New(3); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 874; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    PyTuple_SET_ITEM(__pyx_t_1, 0, __pyx_t_4);
    __Pyx_GIVEREF(__pyx_t_4);
    __Pyx_INCREF(Py_None);
    PyTuple_SET_ITEM(__pyx_t_1, 1, Py_None);
    __Pyx_GIVEREF(Py_None);
    PyTuple_SET_ITEM(__pyx_t_1, 2, __pyx_t_2);
    __Pyx_GIVEREF(__pyx_t_2);
    __pyx_t_4 = 0;
    __pyx_t_2 = 0;
    __pyx_t_2 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_t_1, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 874; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    __pyx_v_tag = __pyx_t_2;
    __pyx_t_2 = 0;
    goto __pyx_L4;
  }
   {

    






    __pyx_t_2 = PyUnicode_FromString(__pyx_v_self->parsed_event.data.mapping_start.tag); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 876; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_v_tag = __pyx_t_2;
    __pyx_t_2 = 0;
  }
  __pyx_L4:;

  






  __Pyx_INCREF(Py_None);
  __pyx_v_flow_style = Py_None;

  






  switch (__pyx_v_self->parsed_event.data.mapping_start.style) {

    






    case YAML_FLOW_MAPPING_STYLE:

    






    __Pyx_INCREF(Py_True);
    __Pyx_DECREF_SET(__pyx_v_flow_style, Py_True);
    break;

    






    case YAML_BLOCK_MAPPING_STYLE:

    






    __Pyx_INCREF(Py_False);
    __Pyx_DECREF_SET(__pyx_v_flow_style, Py_False);
    break;
    default: break;
  }

  






  __pyx_t_2 = PyList_New(0); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 882; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __pyx_v_value = ((PyObject*)__pyx_t_2);
  __pyx_t_2 = 0;

  






  __pyx_t_2 = __Pyx_GetModuleGlobalName(__pyx_n_s_MappingNode); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 883; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __pyx_t_1 = PyTuple_New(5); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 883; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_INCREF(__pyx_v_tag);
  PyTuple_SET_ITEM(__pyx_t_1, 0, __pyx_v_tag);
  __Pyx_GIVEREF(__pyx_v_tag);
  __Pyx_INCREF(__pyx_v_value);
  PyTuple_SET_ITEM(__pyx_t_1, 1, __pyx_v_value);
  __Pyx_GIVEREF(__pyx_v_value);
  __Pyx_INCREF(((PyObject *)__pyx_v_start_mark));
  PyTuple_SET_ITEM(__pyx_t_1, 2, ((PyObject *)__pyx_v_start_mark));
  __Pyx_GIVEREF(((PyObject *)__pyx_v_start_mark));
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_1, 3, Py_None);
  __Pyx_GIVEREF(Py_None);
  __Pyx_INCREF(__pyx_v_flow_style);
  PyTuple_SET_ITEM(__pyx_t_1, 4, __pyx_v_flow_style);
  __Pyx_GIVEREF(__pyx_v_flow_style);
  __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_t_1, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 883; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_v_node = __pyx_t_3;
  __pyx_t_3 = 0;

  






  __pyx_t_6 = (__pyx_v_anchor != Py_None);
  __pyx_t_5 = (__pyx_t_6 != 0);
  if (__pyx_t_5) {

    






    if (unlikely(PyObject_SetItem(__pyx_v_self->anchors, __pyx_v_anchor, __pyx_v_node) < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 885; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    goto __pyx_L5;
  }
  __pyx_L5:;

  






  yaml_event_delete((&__pyx_v_self->parsed_event));

  






  __pyx_t_9 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parse_next_event(__pyx_v_self); if (unlikely(__pyx_t_9 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 887; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

  






  while (1) {
    __pyx_t_5 = ((__pyx_v_self->parsed_event.type != YAML_MAPPING_END_EVENT) != 0);
    if (!__pyx_t_5) break;

    






    __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_compose_node(__pyx_v_self, __pyx_v_node, Py_None); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 889; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_XDECREF_SET(__pyx_v_item_key, __pyx_t_3);
    __pyx_t_3 = 0;

    






    __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_compose_node(__pyx_v_self, __pyx_v_node, __pyx_v_item_key); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 890; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_XDECREF_SET(__pyx_v_item_value, __pyx_t_3);
    __pyx_t_3 = 0;

    






    __pyx_t_3 = PyTuple_New(2); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 891; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_INCREF(__pyx_v_item_key);
    PyTuple_SET_ITEM(__pyx_t_3, 0, __pyx_v_item_key);
    __Pyx_GIVEREF(__pyx_v_item_key);
    __Pyx_INCREF(__pyx_v_item_value);
    PyTuple_SET_ITEM(__pyx_t_3, 1, __pyx_v_item_value);
    __Pyx_GIVEREF(__pyx_v_item_value);
    __pyx_t_10 = __Pyx_PyList_Append(__pyx_v_value, __pyx_t_3); if (unlikely(__pyx_t_10 == -1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 891; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;

    






    __pyx_t_9 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parse_next_event(__pyx_v_self); if (unlikely(__pyx_t_9 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 892; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  }

  






  __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.end_mark.index); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 894; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);

  






  __pyx_t_1 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.end_mark.line); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 895; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);

  






  __pyx_t_2 = __Pyx_PyInt_From_int(__pyx_v_self->parsed_event.end_mark.column); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 896; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);

  






  __pyx_t_4 = PyTuple_New(6); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 893; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_4);
  __Pyx_INCREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_4, 0, __pyx_v_self->stream_name);
  __Pyx_GIVEREF(__pyx_v_self->stream_name);
  PyTuple_SET_ITEM(__pyx_t_4, 1, __pyx_t_3);
  __Pyx_GIVEREF(__pyx_t_3);
  PyTuple_SET_ITEM(__pyx_t_4, 2, __pyx_t_1);
  __Pyx_GIVEREF(__pyx_t_1);
  PyTuple_SET_ITEM(__pyx_t_4, 3, __pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_4, 4, Py_None);
  __Pyx_GIVEREF(Py_None);
  __Pyx_INCREF(Py_None);
  PyTuple_SET_ITEM(__pyx_t_4, 5, Py_None);
  __Pyx_GIVEREF(Py_None);
  __pyx_t_3 = 0;
  __pyx_t_1 = 0;
  __pyx_t_2 = 0;
  __pyx_t_2 = __Pyx_PyObject_Call(((PyObject *)((PyObject*)__pyx_ptype_5_yaml_Mark)), __pyx_t_4, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 893; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
  if (__Pyx_PyObject_SetAttrStr(__pyx_v_node, __pyx_n_s_end_mark, __pyx_t_2) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 893; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;

  






  yaml_event_delete((&__pyx_v_self->parsed_event));

  






  __Pyx_XDECREF(__pyx_r);
  __Pyx_INCREF(__pyx_v_node);
  __pyx_r = __pyx_v_node;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_XDECREF(__pyx_t_2);
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_XDECREF(__pyx_t_4);
  __Pyx_AddTraceback("_yaml.CParser._compose_mapping_node", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF((PyObject *)__pyx_v_start_mark);
  __Pyx_XDECREF(__pyx_v_tag);
  __Pyx_XDECREF(__pyx_v_flow_style);
  __Pyx_XDECREF(__pyx_v_value);
  __Pyx_XDECREF(__pyx_v_node);
  __Pyx_XDECREF(__pyx_v_item_key);
  __Pyx_XDECREF(__pyx_v_item_value);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static int __pyx_f_5_yaml_7CParser__parse_next_event(struct __pyx_obj_5_yaml_CParser *__pyx_v_self) {
  PyObject *__pyx_v_error = NULL;
  int __pyx_r;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("_parse_next_event", 0);

  






  __pyx_t_1 = ((__pyx_v_self->parsed_event.type == YAML_NO_EVENT) != 0);
  if (__pyx_t_1) {

    






    __pyx_t_2 = yaml_parser_parse((&__pyx_v_self->parser), (&__pyx_v_self->parsed_event)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 903; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __pyx_t_1 = ((__pyx_t_2 == 0) != 0);
    if (__pyx_t_1) {

      






      __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CParser *)__pyx_v_self->__pyx_vtab)->_parser_error(__pyx_v_self); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 904; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __pyx_v_error = __pyx_t_3;
      __pyx_t_3 = 0;

      






      __Pyx_Raise(__pyx_v_error, 0, 0, 0);
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 905; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
    goto __pyx_L3;
  }
  __pyx_L3:;

  






  __pyx_r = 1;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_AddTraceback("_yaml.CParser._parse_next_event", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_error);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static int __pyx_f_5_yaml_input_handler(void *__pyx_v_data, char *__pyx_v_buffer, int __pyx_v_size, int *__pyx_v_read) {
  struct __pyx_obj_5_yaml_CParser *__pyx_v_parser = 0;
  PyObject *__pyx_v_value = NULL;
  int __pyx_r;
  __Pyx_RefNannyDeclarations
  PyObject *__pyx_t_1 = NULL;
  int __pyx_t_2;
  int __pyx_t_3;
  PyObject *__pyx_t_4 = NULL;
  PyObject *__pyx_t_5 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("input_handler", 0);

  






  __pyx_t_1 = ((PyObject *)__pyx_v_data);
  __Pyx_INCREF(__pyx_t_1);
  __pyx_v_parser = ((struct __pyx_obj_5_yaml_CParser *)__pyx_t_1);
  __pyx_t_1 = 0;

  






  __pyx_t_2 = (__pyx_v_parser->stream_cache == Py_None);
  __pyx_t_3 = (__pyx_t_2 != 0);
  if (__pyx_t_3) {

    






    __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_parser->stream, __pyx_n_s_read); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 912; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __pyx_t_4 = __Pyx_PyInt_From_int(__pyx_v_size); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 912; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_5 = PyTuple_New(1); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 912; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    PyTuple_SET_ITEM(__pyx_t_5, 0, __pyx_t_4);
    __Pyx_GIVEREF(__pyx_t_4);
    __pyx_t_4 = 0;
    __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_1, __pyx_t_5, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 912; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
    __pyx_v_value = __pyx_t_4;
    __pyx_t_4 = 0;

    






    __pyx_t_3 = ((PyUnicode_CheckExact(__pyx_v_value) != 0) != 0);
    if (__pyx_t_3) {

      






      __pyx_t_4 = PyUnicode_AsUTF8String(__pyx_v_value); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 914; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);
      __Pyx_DECREF_SET(__pyx_v_value, __pyx_t_4);
      __pyx_t_4 = 0;

      






      __pyx_v_parser->unicode_source = 1;
      goto __pyx_L4;
    }
    __pyx_L4:;

    






    __pyx_t_3 = ((PyString_CheckExact(__pyx_v_value) == 0) != 0);
    if (__pyx_t_3) {

      






      __pyx_t_3 = ((PY_MAJOR_VERSION < 3) != 0);
      if (__pyx_t_3) {

        






        __pyx_t_4 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__15, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 918; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_4);
        __Pyx_Raise(__pyx_t_4, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 918; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
       {

        






        __pyx_t_4 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__16, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 920; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_4);
        __Pyx_Raise(__pyx_t_4, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 920; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
    }

    






    __Pyx_INCREF(__pyx_v_value);
    __Pyx_GIVEREF(__pyx_v_value);
    __Pyx_GOTREF(__pyx_v_parser->stream_cache);
    __Pyx_DECREF(__pyx_v_parser->stream_cache);
    __pyx_v_parser->stream_cache = __pyx_v_value;

    






    __pyx_v_parser->stream_cache_pos = 0;

    






    __pyx_v_parser->stream_cache_len = PyString_GET_SIZE(__pyx_v_value);
    goto __pyx_L3;
  }
  __pyx_L3:;

  






  __pyx_t_3 = (((__pyx_v_parser->stream_cache_len - __pyx_v_parser->stream_cache_pos) < __pyx_v_size) != 0);
  if (__pyx_t_3) {

    






    __pyx_v_size = (__pyx_v_parser->stream_cache_len - __pyx_v_parser->stream_cache_pos);
    goto __pyx_L7;
  }
  __pyx_L7:;

  






  __pyx_t_3 = ((__pyx_v_size > 0) != 0);
  if (__pyx_t_3) {

    






    __pyx_t_4 = __pyx_v_parser->stream_cache;
    __Pyx_INCREF(__pyx_t_4);

    






    memcpy(__pyx_v_buffer, (PyString_AS_STRING(__pyx_t_4) + __pyx_v_parser->stream_cache_pos), __pyx_v_size);
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    goto __pyx_L8;
  }
  __pyx_L8:;

  






  (__pyx_v_read[0]) = __pyx_v_size;

  






  __pyx_v_parser->stream_cache_pos = (__pyx_v_parser->stream_cache_pos + __pyx_v_size);

  






  __pyx_t_3 = ((__pyx_v_parser->stream_cache_pos == __pyx_v_parser->stream_cache_len) != 0);
  if (__pyx_t_3) {

    






    __Pyx_INCREF(Py_None);
    __Pyx_GIVEREF(Py_None);
    __Pyx_GOTREF(__pyx_v_parser->stream_cache);
    __Pyx_DECREF(__pyx_v_parser->stream_cache);
    __pyx_v_parser->stream_cache = Py_None;
    goto __pyx_L9;
  }
  __pyx_L9:;

  






  __pyx_r = 1;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_XDECREF(__pyx_t_4);
  __Pyx_XDECREF(__pyx_t_5);
  __Pyx_AddTraceback("_yaml.input_handler", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF((PyObject *)__pyx_v_parser);
  __Pyx_XDECREF(__pyx_v_value);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static int __pyx_pw_5_yaml_8CEmitter_1__init__(PyObject *__pyx_v_self, PyObject *__pyx_args, PyObject *__pyx_kwds); 
static int __pyx_pw_5_yaml_8CEmitter_1__init__(PyObject *__pyx_v_self, PyObject *__pyx_args, PyObject *__pyx_kwds) {
  PyObject *__pyx_v_stream = 0;
  PyObject *__pyx_v_canonical = 0;
  PyObject *__pyx_v_indent = 0;
  PyObject *__pyx_v_width = 0;
  PyObject *__pyx_v_allow_unicode = 0;
  PyObject *__pyx_v_line_break = 0;
  PyObject *__pyx_v_encoding = 0;
  PyObject *__pyx_v_explicit_start = 0;
  PyObject *__pyx_v_explicit_end = 0;
  PyObject *__pyx_v_version = 0;
  PyObject *__pyx_v_tags = 0;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  int __pyx_r;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__init__ (wrapper)", 0);
  {
    static PyObject **__pyx_pyargnames[] = {&__pyx_n_s_stream,&__pyx_n_s_canonical,&__pyx_n_s_indent,&__pyx_n_s_width,&__pyx_n_s_allow_unicode,&__pyx_n_s_line_break,&__pyx_n_s_encoding,&__pyx_n_s_explicit_start,&__pyx_n_s_explicit_end,&__pyx_n_s_version,&__pyx_n_s_tags,0};
    PyObject* values[11] = {0,0,0,0,0,0,0,0,0,0,0};
    values[1] = ((PyObject *)Py_None);
    values[2] = ((PyObject *)Py_None);
    values[3] = ((PyObject *)Py_None);

    






    values[4] = ((PyObject *)Py_None);
    values[5] = ((PyObject *)Py_None);
    values[6] = ((PyObject *)Py_None);

    






    values[7] = ((PyObject *)Py_None);
    values[8] = ((PyObject *)Py_None);
    values[9] = ((PyObject *)Py_None);
    values[10] = ((PyObject *)Py_None);
    if (unlikely(__pyx_kwds)) {
      Py_ssize_t kw_args;
      const Py_ssize_t pos_args = PyTuple_GET_SIZE(__pyx_args);
      switch (pos_args) {
        case 11: values[10] = PyTuple_GET_ITEM(__pyx_args, 10);
        case 10: values[9] = PyTuple_GET_ITEM(__pyx_args, 9);
        case  9: values[8] = PyTuple_GET_ITEM(__pyx_args, 8);
        case  8: values[7] = PyTuple_GET_ITEM(__pyx_args, 7);
        case  7: values[6] = PyTuple_GET_ITEM(__pyx_args, 6);
        case  6: values[5] = PyTuple_GET_ITEM(__pyx_args, 5);
        case  5: values[4] = PyTuple_GET_ITEM(__pyx_args, 4);
        case  4: values[3] = PyTuple_GET_ITEM(__pyx_args, 3);
        case  3: values[2] = PyTuple_GET_ITEM(__pyx_args, 2);
        case  2: values[1] = PyTuple_GET_ITEM(__pyx_args, 1);
        case  1: values[0] = PyTuple_GET_ITEM(__pyx_args, 0);
        case  0: break;
        default: goto __pyx_L5_argtuple_error;
      }
      kw_args = PyDict_Size(__pyx_kwds);
      switch (pos_args) {
        case  0:
        if (likely((values[0] = PyDict_GetItem(__pyx_kwds, __pyx_n_s_stream)) != 0)) kw_args--;
        else goto __pyx_L5_argtuple_error;
        case  1:
        if (kw_args > 0) {
          PyObject* value = PyDict_GetItem(__pyx_kwds, __pyx_n_s_canonical);
          if (value) { values[1] = value; kw_args--; }
        }
        case  2:
        if (kw_args > 0) {
          PyObject* value = PyDict_GetItem(__pyx_kwds, __pyx_n_s_indent);
          if (value) { values[2] = value; kw_args--; }
        }
        case  3:
        if (kw_args > 0) {
          PyObject* value = PyDict_GetItem(__pyx_kwds, __pyx_n_s_width);
          if (value) { values[3] = value; kw_args--; }
        }
        case  4:
        if (kw_args > 0) {
          PyObject* value = PyDict_GetItem(__pyx_kwds, __pyx_n_s_allow_unicode);
          if (value) { values[4] = value; kw_args--; }
        }
        case  5:
        if (kw_args > 0) {
          PyObject* value = PyDict_GetItem(__pyx_kwds, __pyx_n_s_line_break);
          if (value) { values[5] = value; kw_args--; }
        }
        case  6:
        if (kw_args > 0) {
          PyObject* value = PyDict_GetItem(__pyx_kwds, __pyx_n_s_encoding);
          if (value) { values[6] = value; kw_args--; }
        }
        case  7:
        if (kw_args > 0) {
          PyObject* value = PyDict_GetItem(__pyx_kwds, __pyx_n_s_explicit_start);
          if (value) { values[7] = value; kw_args--; }
        }
        case  8:
        if (kw_args > 0) {
          PyObject* value = PyDict_GetItem(__pyx_kwds, __pyx_n_s_explicit_end);
          if (value) { values[8] = value; kw_args--; }
        }
        case  9:
        if (kw_args > 0) {
          PyObject* value = PyDict_GetItem(__pyx_kwds, __pyx_n_s_version);
          if (value) { values[9] = value; kw_args--; }
        }
        case 10:
        if (kw_args > 0) {
          PyObject* value = PyDict_GetItem(__pyx_kwds, __pyx_n_s_tags);
          if (value) { values[10] = value; kw_args--; }
        }
      }
      if (unlikely(kw_args > 0)) {
        if (unlikely(__Pyx_ParseOptionalKeywords(__pyx_kwds, __pyx_pyargnames, 0, values, pos_args, "__init__") < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 953; __pyx_clineno = __LINE__; goto __pyx_L3_error;}
      }
    } else {
      switch (PyTuple_GET_SIZE(__pyx_args)) {
        case 11: values[10] = PyTuple_GET_ITEM(__pyx_args, 10);
        case 10: values[9] = PyTuple_GET_ITEM(__pyx_args, 9);
        case  9: values[8] = PyTuple_GET_ITEM(__pyx_args, 8);
        case  8: values[7] = PyTuple_GET_ITEM(__pyx_args, 7);
        case  7: values[6] = PyTuple_GET_ITEM(__pyx_args, 6);
        case  6: values[5] = PyTuple_GET_ITEM(__pyx_args, 5);
        case  5: values[4] = PyTuple_GET_ITEM(__pyx_args, 4);
        case  4: values[3] = PyTuple_GET_ITEM(__pyx_args, 3);
        case  3: values[2] = PyTuple_GET_ITEM(__pyx_args, 2);
        case  2: values[1] = PyTuple_GET_ITEM(__pyx_args, 1);
        case  1: values[0] = PyTuple_GET_ITEM(__pyx_args, 0);
        break;
        default: goto __pyx_L5_argtuple_error;
      }
    }
    __pyx_v_stream = values[0];
    __pyx_v_canonical = values[1];
    __pyx_v_indent = values[2];
    __pyx_v_width = values[3];
    __pyx_v_allow_unicode = values[4];
    __pyx_v_line_break = values[5];
    __pyx_v_encoding = values[6];
    __pyx_v_explicit_start = values[7];
    __pyx_v_explicit_end = values[8];
    __pyx_v_version = values[9];
    __pyx_v_tags = values[10];
  }
  goto __pyx_L4_argument_unpacking_done;
  __pyx_L5_argtuple_error:;
  __Pyx_RaiseArgtupleInvalid("__init__", 0, 1, 11, PyTuple_GET_SIZE(__pyx_args)); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 953; __pyx_clineno = __LINE__; goto __pyx_L3_error;}
  __pyx_L3_error:;
  __Pyx_AddTraceback("_yaml.CEmitter.__init__", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __Pyx_RefNannyFinishContext();
  return -1;
  __pyx_L4_argument_unpacking_done:;
  __pyx_r = __pyx_pf_5_yaml_8CEmitter___init__(((struct __pyx_obj_5_yaml_CEmitter *)__pyx_v_self), __pyx_v_stream, __pyx_v_canonical, __pyx_v_indent, __pyx_v_width, __pyx_v_allow_unicode, __pyx_v_line_break, __pyx_v_encoding, __pyx_v_explicit_start, __pyx_v_explicit_end, __pyx_v_version, __pyx_v_tags);

  







  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static int __pyx_pf_5_yaml_8CEmitter___init__(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self, PyObject *__pyx_v_stream, PyObject *__pyx_v_canonical, PyObject *__pyx_v_indent, PyObject *__pyx_v_width, PyObject *__pyx_v_allow_unicode, PyObject *__pyx_v_line_break, PyObject *__pyx_v_encoding, PyObject *__pyx_v_explicit_start, PyObject *__pyx_v_explicit_end, PyObject *__pyx_v_version, PyObject *__pyx_v_tags) {
  int __pyx_r;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  PyObject *__pyx_t_2 = NULL;
  int __pyx_t_3;
  int __pyx_t_4;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("__init__", 0);

  






  __pyx_t_1 = ((yaml_emitter_initialize((&__pyx_v_self->emitter)) == 0) != 0);
  if (__pyx_t_1) {

    






    PyErr_NoMemory(); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 957; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  }

  






  __Pyx_INCREF(__pyx_v_stream);
  __Pyx_GIVEREF(__pyx_v_stream);
  __Pyx_GOTREF(__pyx_v_self->stream);
  __Pyx_DECREF(__pyx_v_self->stream);
  __pyx_v_self->stream = __pyx_v_stream;

  






  __pyx_v_self->dump_unicode = 0;

  






  __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
  if (__pyx_t_1) {

    






    __pyx_t_2 = __Pyx_GetAttr3(__pyx_v_stream, __pyx_n_s_encoding, Py_None); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 961; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_t_1 = __Pyx_PyObject_IsTrue(__pyx_t_2); if (unlikely(__pyx_t_1 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 961; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    if (__pyx_t_1) {

      






      __pyx_v_self->dump_unicode = 1;
      goto __pyx_L5;
    }
    __pyx_L5:;
    goto __pyx_L4;
  }
   {

    






    __pyx_t_1 = PyObject_HasAttr(__pyx_v_stream, __pyx_n_u_encoding); if (unlikely(__pyx_t_1 == -1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 964; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __pyx_t_3 = (__pyx_t_1 != 0);
    if (__pyx_t_3) {

      






      __pyx_v_self->dump_unicode = 1;
      goto __pyx_L6;
    }
    __pyx_L6:;
  }
  __pyx_L4:;

  






  __Pyx_INCREF(__pyx_v_encoding);
  __Pyx_GIVEREF(__pyx_v_encoding);
  __Pyx_GOTREF(__pyx_v_self->use_encoding);
  __Pyx_DECREF(__pyx_v_self->use_encoding);
  __pyx_v_self->use_encoding = __pyx_v_encoding;

  






  yaml_emitter_set_output((&__pyx_v_self->emitter), __pyx_f_5_yaml_output_handler, ((void *)__pyx_v_self));

  






  __pyx_t_3 = __Pyx_PyObject_IsTrue(__pyx_v_canonical); if (unlikely(__pyx_t_3 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 968; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  if (__pyx_t_3) {

    






    yaml_emitter_set_canonical((&__pyx_v_self->emitter), 1);
    goto __pyx_L7;
  }
  __pyx_L7:;

  






  __pyx_t_3 = (__pyx_v_indent != Py_None);
  __pyx_t_1 = (__pyx_t_3 != 0);
  if (__pyx_t_1) {

    






    __pyx_t_4 = __Pyx_PyInt_As_int(__pyx_v_indent); if (unlikely((__pyx_t_4 == (int)-1) && PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 971; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    yaml_emitter_set_indent((&__pyx_v_self->emitter), __pyx_t_4);
    goto __pyx_L8;
  }
  __pyx_L8:;

  






  __pyx_t_1 = (__pyx_v_width != Py_None);
  __pyx_t_3 = (__pyx_t_1 != 0);
  if (__pyx_t_3) {

    






    __pyx_t_4 = __Pyx_PyInt_As_int(__pyx_v_width); if (unlikely((__pyx_t_4 == (int)-1) && PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 973; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    yaml_emitter_set_width((&__pyx_v_self->emitter), __pyx_t_4);
    goto __pyx_L9;
  }
  __pyx_L9:;

  






  __pyx_t_3 = __Pyx_PyObject_IsTrue(__pyx_v_allow_unicode); if (unlikely(__pyx_t_3 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 974; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  if (__pyx_t_3) {

    






    yaml_emitter_set_unicode((&__pyx_v_self->emitter), 1);
    goto __pyx_L10;
  }
  __pyx_L10:;

  






  __pyx_t_3 = (__pyx_v_line_break != Py_None);
  __pyx_t_1 = (__pyx_t_3 != 0);
  if (__pyx_t_1) {

    






    __pyx_t_1 = (__Pyx_PyString_Equals(__pyx_v_line_break, __pyx_kp_s__17, Py_EQ)); if (unlikely(__pyx_t_1 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 977; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    if (__pyx_t_1) {

      






      yaml_emitter_set_break((&__pyx_v_self->emitter), YAML_CR_BREAK);
      goto __pyx_L12;
    }

    






    __pyx_t_1 = (__Pyx_PyString_Equals(__pyx_v_line_break, __pyx_kp_s__18, Py_EQ)); if (unlikely(__pyx_t_1 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 979; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    if (__pyx_t_1) {

      






      yaml_emitter_set_break((&__pyx_v_self->emitter), YAML_LN_BREAK);
      goto __pyx_L12;
    }

    






    __pyx_t_1 = (__Pyx_PyString_Equals(__pyx_v_line_break, __pyx_kp_s__19, Py_EQ)); if (unlikely(__pyx_t_1 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 981; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    if (__pyx_t_1) {

      






      yaml_emitter_set_break((&__pyx_v_self->emitter), YAML_CRLN_BREAK);
      goto __pyx_L12;
    }
    __pyx_L12:;
    goto __pyx_L11;
  }
  __pyx_L11:;

  






  __pyx_v_self->document_start_implicit = 1;

  






  __pyx_t_1 = __Pyx_PyObject_IsTrue(__pyx_v_explicit_start); if (unlikely(__pyx_t_1 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 984; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  if (__pyx_t_1) {

    






    __pyx_v_self->document_start_implicit = 0;
    goto __pyx_L13;
  }
  __pyx_L13:;

  






  __pyx_v_self->document_end_implicit = 1;

  






  __pyx_t_1 = __Pyx_PyObject_IsTrue(__pyx_v_explicit_end); if (unlikely(__pyx_t_1 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 987; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  if (__pyx_t_1) {

    






    __pyx_v_self->document_end_implicit = 0;
    goto __pyx_L14;
  }
  __pyx_L14:;

  






  __Pyx_INCREF(__pyx_v_version);
  __Pyx_GIVEREF(__pyx_v_version);
  __Pyx_GOTREF(__pyx_v_self->use_version);
  __Pyx_DECREF(__pyx_v_self->use_version);
  __pyx_v_self->use_version = __pyx_v_version;

  






  __Pyx_INCREF(__pyx_v_tags);
  __Pyx_GIVEREF(__pyx_v_tags);
  __Pyx_GOTREF(__pyx_v_self->use_tags);
  __Pyx_DECREF(__pyx_v_self->use_tags);
  __pyx_v_self->use_tags = __pyx_v_tags;

  






  __pyx_t_2 = PyDict_New(); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 991; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  __Pyx_GOTREF(__pyx_v_self->serialized_nodes);
  __Pyx_DECREF(__pyx_v_self->serialized_nodes);
  __pyx_v_self->serialized_nodes = __pyx_t_2;
  __pyx_t_2 = 0;

  






  __pyx_t_2 = PyDict_New(); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 992; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  __Pyx_GOTREF(__pyx_v_self->anchors);
  __Pyx_DECREF(__pyx_v_self->anchors);
  __pyx_v_self->anchors = __pyx_t_2;
  __pyx_t_2 = 0;

  






  __pyx_v_self->last_alias_id = 0;

  






  __pyx_v_self->closed = -1;

  







  
  __pyx_r = 0;
  goto __pyx_L0;
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_2);
  __Pyx_AddTraceback("_yaml.CEmitter.__init__", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = -1;
  __pyx_L0:;
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static void __pyx_pw_5_yaml_8CEmitter_3__dealloc__(PyObject *__pyx_v_self); 
static void __pyx_pw_5_yaml_8CEmitter_3__dealloc__(PyObject *__pyx_v_self) {
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__dealloc__ (wrapper)", 0);
  __pyx_pf_5_yaml_8CEmitter_2__dealloc__(((struct __pyx_obj_5_yaml_CEmitter *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
}

static void __pyx_pf_5_yaml_8CEmitter_2__dealloc__(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self) {
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__dealloc__", 0);

  






  yaml_emitter_delete((&__pyx_v_self->emitter));

  







  
  __Pyx_RefNannyFinishContext();
}










static PyObject *__pyx_pw_5_yaml_8CEmitter_5dispose(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused); 
static PyObject *__pyx_pw_5_yaml_8CEmitter_5dispose(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("dispose (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_8CEmitter_4dispose(((struct __pyx_obj_5_yaml_CEmitter *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_8CEmitter_4dispose(CYTHON_UNUSED struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self) {
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("dispose", 0);

  
  __pyx_r = Py_None; __Pyx_INCREF(Py_None);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static PyObject *__pyx_f_5_yaml_8CEmitter__emitter_error(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self) {
  PyObject *__pyx_v_problem = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  PyObject *__pyx_t_2 = NULL;
  PyObject *__pyx_t_3 = NULL;
  PyObject *__pyx_t_4 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("_emitter_error", 0);

  






  switch (__pyx_v_self->emitter.error) {

    






    case YAML_MEMORY_ERROR:

    






    __Pyx_XDECREF(__pyx_r);
    __Pyx_INCREF(__pyx_builtin_MemoryError);
    __pyx_r = __pyx_builtin_MemoryError;
    goto __pyx_L0;
    break;

    






    case YAML_EMITTER_ERROR:

    






    __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
    if (__pyx_t_1) {

      






      __pyx_t_2 = __Pyx_PyBytes_FromString(__pyx_v_self->emitter.problem); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1007; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __pyx_v_problem = __pyx_t_2;
      __pyx_t_2 = 0;
      goto __pyx_L3;
    }
     {

      






      __pyx_t_2 = PyUnicode_FromString(__pyx_v_self->emitter.problem); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1009; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __pyx_v_problem = __pyx_t_2;
      __pyx_t_2 = 0;
    }
    __pyx_L3:;

    






    __Pyx_XDECREF(__pyx_r);
    __pyx_t_2 = __Pyx_GetModuleGlobalName(__pyx_n_s_EmitterError); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1010; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_t_3 = PyTuple_New(1); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1010; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __Pyx_INCREF(__pyx_v_problem);
    PyTuple_SET_ITEM(__pyx_t_3, 0, __pyx_v_problem);
    __Pyx_GIVEREF(__pyx_v_problem);
    __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_t_3, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1010; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __pyx_r = __pyx_t_4;
    __pyx_t_4 = 0;
    goto __pyx_L0;
    break;
    default: break;
  }

  






  __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
  if (__pyx_t_1) {

    






    __pyx_t_4 = __Pyx_PyObject_Call(__pyx_builtin_ValueError, __pyx_tuple__20, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1012; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_Raise(__pyx_t_4, 0, 0, 0);
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1012; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  }
   {

    






    __pyx_t_4 = __Pyx_PyObject_Call(__pyx_builtin_ValueError, __pyx_tuple__21, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1014; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __Pyx_Raise(__pyx_t_4, 0, 0, 0);
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1014; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  }

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_2);
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_XDECREF(__pyx_t_4);
  __Pyx_AddTraceback("_yaml.CEmitter._emitter_error", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_problem);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static int __pyx_f_5_yaml_8CEmitter__object_to_event(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self, PyObject *__pyx_v_event_object, yaml_event_t *__pyx_v_event) {
  yaml_encoding_t __pyx_v_encoding;
  yaml_version_directive_t __pyx_v_version_directive_value;
  yaml_version_directive_t *__pyx_v_version_directive;
  yaml_tag_directive_t __pyx_v_tag_directives_value[128];
  yaml_tag_directive_t *__pyx_v_tag_directives_start;
  yaml_tag_directive_t *__pyx_v_tag_directives_end;
  int __pyx_v_implicit;
  int __pyx_v_plain_implicit;
  int __pyx_v_quoted_implicit;
  char *__pyx_v_anchor;
  char *__pyx_v_tag;
  char *__pyx_v_value;
  int __pyx_v_length;
  yaml_scalar_style_t __pyx_v_scalar_style;
  yaml_sequence_style_t __pyx_v_sequence_style;
  yaml_mapping_style_t __pyx_v_mapping_style;
  PyObject *__pyx_v_event_class = NULL;
  PyObject *__pyx_v_cache = NULL;
  PyObject *__pyx_v_handle = NULL;
  PyObject *__pyx_v_prefix = NULL;
  PyObject *__pyx_v_anchor_object = NULL;
  PyObject *__pyx_v_tag_object = NULL;
  PyObject *__pyx_v_value_object = NULL;
  PyObject *__pyx_v_style_object = NULL;
  int __pyx_r;
  __Pyx_RefNannyDeclarations
  PyObject *__pyx_t_1 = NULL;
  int __pyx_t_2;
  int __pyx_t_3;
  int __pyx_t_4;
  PyObject *__pyx_t_5 = NULL;
  int __pyx_t_6;
  Py_ssize_t __pyx_t_7;
  PyObject *(*__pyx_t_8)(PyObject *);
  PyObject *__pyx_t_9 = NULL;
  int __pyx_t_10;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("_object_to_event", 0);

  






  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_class); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1033; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_v_event_class = __pyx_t_1;
  __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_StreamStartEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1034; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = (__pyx_v_event_class == __pyx_t_1);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_3 = (__pyx_t_2 != 0);
  if (__pyx_t_3) {

    






    __pyx_v_encoding = YAML_UTF8_ENCODING;

    






    __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_encoding); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1036; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __pyx_t_3 = (__Pyx_PyUnicode_Equals(__pyx_t_1, __pyx_kp_u_utf_16_le, Py_EQ)); if (unlikely(__pyx_t_3 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1036; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    if (!__pyx_t_3) {
      __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_encoding); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1036; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __pyx_t_2 = (__Pyx_PyString_Equals(__pyx_t_1, __pyx_kp_s_utf_16_le, Py_EQ)); if (unlikely(__pyx_t_2 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1036; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
      __pyx_t_4 = __pyx_t_2;
    } else {
      __pyx_t_4 = __pyx_t_3;
    }
    if (__pyx_t_4) {

      






      __pyx_v_encoding = YAML_UTF16LE_ENCODING;
      goto __pyx_L4;
    }

    






    __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_encoding); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1038; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __pyx_t_4 = (__Pyx_PyUnicode_Equals(__pyx_t_1, __pyx_kp_u_utf_16_be, Py_EQ)); if (unlikely(__pyx_t_4 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1038; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    if (!__pyx_t_4) {
      __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_encoding); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1038; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __pyx_t_3 = (__Pyx_PyString_Equals(__pyx_t_1, __pyx_kp_s_utf_16_be, Py_EQ)); if (unlikely(__pyx_t_3 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1038; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
      __pyx_t_2 = __pyx_t_3;
    } else {
      __pyx_t_2 = __pyx_t_4;
    }
    if (__pyx_t_2) {

      






      __pyx_v_encoding = YAML_UTF16BE_ENCODING;
      goto __pyx_L4;
    }
    __pyx_L4:;

    






    __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_encoding); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1040; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __pyx_t_2 = (__pyx_t_1 == Py_None);
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    __pyx_t_4 = (__pyx_t_2 != 0);
    if (__pyx_t_4) {

      






      __pyx_v_self->dump_unicode = 1;
      goto __pyx_L5;
    }
    __pyx_L5:;

    






    __pyx_t_4 = ((__pyx_v_self->dump_unicode == 1) != 0);
    if (__pyx_t_4) {

      






      __pyx_v_encoding = YAML_UTF8_ENCODING;
      goto __pyx_L6;
    }
    __pyx_L6:;

    






    yaml_stream_start_event_initialize(__pyx_v_event, __pyx_v_encoding);
    goto __pyx_L3;
  }

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_StreamEndEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1045; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_4 = (__pyx_v_event_class == __pyx_t_1);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_2 = (__pyx_t_4 != 0);
  if (__pyx_t_2) {

    






    yaml_stream_end_event_initialize(__pyx_v_event);
    goto __pyx_L3;
  }

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_DocumentStartEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1047; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = (__pyx_v_event_class == __pyx_t_1);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_4 = (__pyx_t_2 != 0);
  if (__pyx_t_4) {

    






    __pyx_v_version_directive = NULL;

    






    __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_version); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1049; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __pyx_t_4 = __Pyx_PyObject_IsTrue(__pyx_t_1); if (unlikely(__pyx_t_4 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1049; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    if (__pyx_t_4) {

      






      __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_version); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1050; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __pyx_t_5 = __Pyx_GetItemInt(__pyx_t_1, 0, long, 1, __Pyx_PyInt_From_long, 0, 0, 1); if (unlikely(__pyx_t_5 == NULL)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1050; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
      __pyx_t_6 = __Pyx_PyInt_As_int(__pyx_t_5); if (unlikely((__pyx_t_6 == (int)-1) && PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1050; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      __pyx_v_version_directive_value.major = __pyx_t_6;

      






      __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_version); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1051; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __pyx_t_1 = __Pyx_GetItemInt(__pyx_t_5, 1, long, 1, __Pyx_PyInt_From_long, 0, 0, 1); if (unlikely(__pyx_t_1 == NULL)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1051; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
      __Pyx_GOTREF(__pyx_t_1);
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      __pyx_t_6 = __Pyx_PyInt_As_int(__pyx_t_1); if (unlikely((__pyx_t_6 == (int)-1) && PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1051; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
      __pyx_v_version_directive_value.minor = __pyx_t_6;

      






      __pyx_v_version_directive = (&__pyx_v_version_directive_value);
      goto __pyx_L7;
    }
    __pyx_L7:;

    






    __pyx_v_tag_directives_start = NULL;

    






    __pyx_v_tag_directives_end = NULL;

    






    __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_tags); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1055; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __pyx_t_4 = __Pyx_PyObject_IsTrue(__pyx_t_1); if (unlikely(__pyx_t_4 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1055; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    if (__pyx_t_4) {

      






      __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_tags); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1056; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __pyx_t_7 = PyObject_Length(__pyx_t_1); if (unlikely(__pyx_t_7 == -1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1056; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
      __pyx_t_4 = ((__pyx_t_7 > 128) != 0);
      if (__pyx_t_4) {

        






        __pyx_t_4 = ((PY_MAJOR_VERSION < 3) != 0);
        if (__pyx_t_4) {

          






          __pyx_t_1 = __Pyx_PyObject_Call(__pyx_builtin_ValueError, __pyx_tuple__22, NULL); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1058; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_1);
          __Pyx_Raise(__pyx_t_1, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1058; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
         {

          






          __pyx_t_1 = __Pyx_PyObject_Call(__pyx_builtin_ValueError, __pyx_tuple__23, NULL); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1060; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_1);
          __Pyx_Raise(__pyx_t_1, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1060; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
      }

      






      __pyx_v_tag_directives_start = __pyx_v_tag_directives_value;

      






      __pyx_v_tag_directives_end = __pyx_v_tag_directives_value;

      






      __pyx_t_1 = PyList_New(0); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1063; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __pyx_v_cache = ((PyObject*)__pyx_t_1);
      __pyx_t_1 = 0;

      






      __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_tags); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1064; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      if (PyList_CheckExact(__pyx_t_1) || PyTuple_CheckExact(__pyx_t_1)) {
        __pyx_t_5 = __pyx_t_1; __Pyx_INCREF(__pyx_t_5); __pyx_t_7 = 0;
        __pyx_t_8 = NULL;
      } else {
        __pyx_t_7 = -1; __pyx_t_5 = PyObject_GetIter(__pyx_t_1); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1064; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __pyx_t_8 = Py_TYPE(__pyx_t_5)->tp_iternext;
      }
      __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
      for (;;) {
        if (!__pyx_t_8 && PyList_CheckExact(__pyx_t_5)) {
          if (__pyx_t_7 >= PyList_GET_SIZE(__pyx_t_5)) break;
          #if CYTHON_COMPILING_IN_CPYTHON
          __pyx_t_1 = PyList_GET_ITEM(__pyx_t_5, __pyx_t_7); __Pyx_INCREF(__pyx_t_1); __pyx_t_7++; if (unlikely(0 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1064; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #else
          __pyx_t_1 = PySequence_ITEM(__pyx_t_5, __pyx_t_7); __pyx_t_7++; if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1064; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #endif
        } else if (!__pyx_t_8 && PyTuple_CheckExact(__pyx_t_5)) {
          if (__pyx_t_7 >= PyTuple_GET_SIZE(__pyx_t_5)) break;
          #if CYTHON_COMPILING_IN_CPYTHON
          __pyx_t_1 = PyTuple_GET_ITEM(__pyx_t_5, __pyx_t_7); __Pyx_INCREF(__pyx_t_1); __pyx_t_7++; if (unlikely(0 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1064; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #else
          __pyx_t_1 = PySequence_ITEM(__pyx_t_5, __pyx_t_7); __pyx_t_7++; if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1064; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #endif
        } else {
          __pyx_t_1 = __pyx_t_8(__pyx_t_5);
          if (unlikely(!__pyx_t_1)) {
            PyObject* exc_type = PyErr_Occurred();
            if (exc_type) {
              if (likely(exc_type == PyExc_StopIteration || PyErr_GivenExceptionMatches(exc_type, PyExc_StopIteration))) PyErr_Clear();
              else {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1064; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
            }
            break;
          }
          __Pyx_GOTREF(__pyx_t_1);
        }
        __Pyx_XDECREF_SET(__pyx_v_handle, __pyx_t_1);
        __pyx_t_1 = 0;

        






        __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_tags); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1065; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_1);
        __pyx_t_9 = PyObject_GetItem(__pyx_t_1, __pyx_v_handle); if (unlikely(__pyx_t_9 == NULL)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1065; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
        __Pyx_GOTREF(__pyx_t_9);
        __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
        __Pyx_XDECREF_SET(__pyx_v_prefix, __pyx_t_9);
        __pyx_t_9 = 0;

        






        __pyx_t_4 = (PyUnicode_CheckExact(__pyx_v_handle) != 0);
        if (__pyx_t_4) {

          






          __pyx_t_9 = PyUnicode_AsUTF8String(__pyx_v_handle); if (unlikely(!__pyx_t_9)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1067; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_9);
          __Pyx_DECREF_SET(__pyx_v_handle, __pyx_t_9);
          __pyx_t_9 = 0;

          






          __pyx_t_10 = __Pyx_PyList_Append(__pyx_v_cache, __pyx_v_handle); if (unlikely(__pyx_t_10 == -1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1068; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          goto __pyx_L13;
        }
        __pyx_L13:;

        






        __pyx_t_4 = ((!(PyString_CheckExact(__pyx_v_handle) != 0)) != 0);
        if (__pyx_t_4) {

          






          __pyx_t_4 = ((PY_MAJOR_VERSION < 3) != 0);
          if (__pyx_t_4) {

            






            __pyx_t_9 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__24, NULL); if (unlikely(!__pyx_t_9)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1071; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
            __Pyx_GOTREF(__pyx_t_9);
            __Pyx_Raise(__pyx_t_9, 0, 0, 0);
            __Pyx_DECREF(__pyx_t_9); __pyx_t_9 = 0;
            {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1071; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          }
           {

            






            __pyx_t_9 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__25, NULL); if (unlikely(!__pyx_t_9)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1073; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
            __Pyx_GOTREF(__pyx_t_9);
            __Pyx_Raise(__pyx_t_9, 0, 0, 0);
            __Pyx_DECREF(__pyx_t_9); __pyx_t_9 = 0;
            {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1073; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          }
        }

        






        __pyx_v_tag_directives_end->handle = PyString_AS_STRING(__pyx_v_handle);

        






        __pyx_t_4 = (PyUnicode_CheckExact(__pyx_v_prefix) != 0);
        if (__pyx_t_4) {

          






          __pyx_t_9 = PyUnicode_AsUTF8String(__pyx_v_prefix); if (unlikely(!__pyx_t_9)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1076; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_9);
          __Pyx_DECREF_SET(__pyx_v_prefix, __pyx_t_9);
          __pyx_t_9 = 0;

          






          __pyx_t_10 = __Pyx_PyList_Append(__pyx_v_cache, __pyx_v_prefix); if (unlikely(__pyx_t_10 == -1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1077; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          goto __pyx_L16;
        }
        __pyx_L16:;

        






        __pyx_t_4 = ((!(PyString_CheckExact(__pyx_v_prefix) != 0)) != 0);
        if (__pyx_t_4) {

          






          __pyx_t_4 = ((PY_MAJOR_VERSION < 3) != 0);
          if (__pyx_t_4) {

            






            __pyx_t_9 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__26, NULL); if (unlikely(!__pyx_t_9)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1080; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
            __Pyx_GOTREF(__pyx_t_9);
            __Pyx_Raise(__pyx_t_9, 0, 0, 0);
            __Pyx_DECREF(__pyx_t_9); __pyx_t_9 = 0;
            {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1080; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          }
           {

            






            __pyx_t_9 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__27, NULL); if (unlikely(!__pyx_t_9)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1082; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
            __Pyx_GOTREF(__pyx_t_9);
            __Pyx_Raise(__pyx_t_9, 0, 0, 0);
            __Pyx_DECREF(__pyx_t_9); __pyx_t_9 = 0;
            {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1082; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          }
        }

        






        __pyx_v_tag_directives_end->prefix = PyString_AS_STRING(__pyx_v_prefix);

        






        __pyx_v_tag_directives_end = (__pyx_v_tag_directives_end + 1);
      }
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      goto __pyx_L8;
    }
    __pyx_L8:;

    






    __pyx_v_implicit = 1;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_explicit); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1086; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_t_4 = __Pyx_PyObject_IsTrue(__pyx_t_5); if (unlikely(__pyx_t_4 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1086; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
    if (__pyx_t_4) {

      






      __pyx_v_implicit = 0;
      goto __pyx_L19;
    }
    __pyx_L19:;

    






    __pyx_t_4 = ((yaml_document_start_event_initialize(__pyx_v_event, __pyx_v_version_directive, __pyx_v_tag_directives_start, __pyx_v_tag_directives_end, __pyx_v_implicit) == 0) != 0);
    if (__pyx_t_4) {

      






      PyErr_NoMemory(); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1090; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
    goto __pyx_L3;
  }

  






  __pyx_t_5 = __Pyx_GetModuleGlobalName(__pyx_n_s_DocumentEndEvent); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1091; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_5);
  __pyx_t_4 = (__pyx_v_event_class == __pyx_t_5);
  __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
  __pyx_t_2 = (__pyx_t_4 != 0);
  if (__pyx_t_2) {

    






    __pyx_v_implicit = 1;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_explicit); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1093; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_t_2 = __Pyx_PyObject_IsTrue(__pyx_t_5); if (unlikely(__pyx_t_2 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1093; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
    if (__pyx_t_2) {

      






      __pyx_v_implicit = 0;
      goto __pyx_L21;
    }
    __pyx_L21:;

    






    yaml_document_end_event_initialize(__pyx_v_event, __pyx_v_implicit);
    goto __pyx_L3;
  }

  






  __pyx_t_5 = __Pyx_GetModuleGlobalName(__pyx_n_s_AliasEvent); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1096; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_5);
  __pyx_t_2 = (__pyx_v_event_class == __pyx_t_5);
  __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
  __pyx_t_4 = (__pyx_t_2 != 0);
  if (__pyx_t_4) {

    






    __pyx_v_anchor = NULL;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_anchor); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1098; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_v_anchor_object = __pyx_t_5;
    __pyx_t_5 = 0;

    






    __pyx_t_4 = (PyUnicode_CheckExact(__pyx_v_anchor_object) != 0);
    if (__pyx_t_4) {

      






      __pyx_t_5 = PyUnicode_AsUTF8String(__pyx_v_anchor_object); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1100; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_DECREF_SET(__pyx_v_anchor_object, __pyx_t_5);
      __pyx_t_5 = 0;
      goto __pyx_L22;
    }
    __pyx_L22:;

    






    __pyx_t_4 = ((!(PyString_CheckExact(__pyx_v_anchor_object) != 0)) != 0);
    if (__pyx_t_4) {

      






      __pyx_t_4 = ((PY_MAJOR_VERSION < 3) != 0);
      if (__pyx_t_4) {

        






        __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__28, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1103; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_Raise(__pyx_t_5, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1103; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
       {

        






        __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__29, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1105; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_Raise(__pyx_t_5, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1105; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
    }

    






    __pyx_v_anchor = PyString_AS_STRING(__pyx_v_anchor_object);

    






    __pyx_t_4 = ((yaml_alias_event_initialize(__pyx_v_event, __pyx_v_anchor) == 0) != 0);
    if (__pyx_t_4) {

      






      PyErr_NoMemory(); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1108; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
    goto __pyx_L3;
  }

  






  __pyx_t_5 = __Pyx_GetModuleGlobalName(__pyx_n_s_ScalarEvent); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1109; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_5);
  __pyx_t_4 = (__pyx_v_event_class == __pyx_t_5);
  __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
  __pyx_t_2 = (__pyx_t_4 != 0);
  if (__pyx_t_2) {

    






    __pyx_v_anchor = NULL;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_anchor); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1111; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_v_anchor_object = __pyx_t_5;
    __pyx_t_5 = 0;

    






    __pyx_t_2 = (__pyx_v_anchor_object != Py_None);
    __pyx_t_4 = (__pyx_t_2 != 0);
    if (__pyx_t_4) {

      






      __pyx_t_4 = (PyUnicode_CheckExact(__pyx_v_anchor_object) != 0);
      if (__pyx_t_4) {

        






        __pyx_t_5 = PyUnicode_AsUTF8String(__pyx_v_anchor_object); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1114; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_DECREF_SET(__pyx_v_anchor_object, __pyx_t_5);
        __pyx_t_5 = 0;
        goto __pyx_L27;
      }
      __pyx_L27:;

      






      __pyx_t_4 = ((!(PyString_CheckExact(__pyx_v_anchor_object) != 0)) != 0);
      if (__pyx_t_4) {

        






        __pyx_t_4 = ((PY_MAJOR_VERSION < 3) != 0);
        if (__pyx_t_4) {

          






          __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__30, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1117; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_5);
          __Pyx_Raise(__pyx_t_5, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1117; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
         {

          






          __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__31, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1119; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_5);
          __Pyx_Raise(__pyx_t_5, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1119; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
      }

      






      __pyx_v_anchor = PyString_AS_STRING(__pyx_v_anchor_object);
      goto __pyx_L26;
    }
    __pyx_L26:;

    






    __pyx_v_tag = NULL;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_tag); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1122; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_v_tag_object = __pyx_t_5;
    __pyx_t_5 = 0;

    






    __pyx_t_4 = (__pyx_v_tag_object != Py_None);
    __pyx_t_2 = (__pyx_t_4 != 0);
    if (__pyx_t_2) {

      






      __pyx_t_2 = (PyUnicode_CheckExact(__pyx_v_tag_object) != 0);
      if (__pyx_t_2) {

        






        __pyx_t_5 = PyUnicode_AsUTF8String(__pyx_v_tag_object); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1125; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_DECREF_SET(__pyx_v_tag_object, __pyx_t_5);
        __pyx_t_5 = 0;
        goto __pyx_L31;
      }
      __pyx_L31:;

      






      __pyx_t_2 = ((!(PyString_CheckExact(__pyx_v_tag_object) != 0)) != 0);
      if (__pyx_t_2) {

        






        __pyx_t_2 = ((PY_MAJOR_VERSION < 3) != 0);
        if (__pyx_t_2) {

          






          __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__32, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1128; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_5);
          __Pyx_Raise(__pyx_t_5, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1128; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
         {

          






          __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__33, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1130; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_5);
          __Pyx_Raise(__pyx_t_5, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1130; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
      }

      






      __pyx_v_tag = PyString_AS_STRING(__pyx_v_tag_object);
      goto __pyx_L30;
    }
    __pyx_L30:;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_value); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1132; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_v_value_object = __pyx_t_5;
    __pyx_t_5 = 0;

    






    __pyx_t_2 = (PyUnicode_CheckExact(__pyx_v_value_object) != 0);
    if (__pyx_t_2) {

      






      __pyx_t_5 = PyUnicode_AsUTF8String(__pyx_v_value_object); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1134; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_DECREF_SET(__pyx_v_value_object, __pyx_t_5);
      __pyx_t_5 = 0;
      goto __pyx_L34;
    }
    __pyx_L34:;

    






    __pyx_t_2 = ((!(PyString_CheckExact(__pyx_v_value_object) != 0)) != 0);
    if (__pyx_t_2) {

      






      __pyx_t_2 = ((PY_MAJOR_VERSION < 3) != 0);
      if (__pyx_t_2) {

        






        __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__34, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1137; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_Raise(__pyx_t_5, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1137; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
       {

        






        __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__35, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1139; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_Raise(__pyx_t_5, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1139; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
    }

    






    __pyx_v_value = PyString_AS_STRING(__pyx_v_value_object);

    






    __pyx_v_length = PyString_GET_SIZE(__pyx_v_value_object);

    






    __pyx_v_plain_implicit = 0;

    






    __pyx_v_quoted_implicit = 0;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_implicit); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1144; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_t_2 = (__pyx_t_5 != Py_None);
    __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
    __pyx_t_4 = (__pyx_t_2 != 0);
    if (__pyx_t_4) {

      






      __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_implicit); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1145; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __pyx_t_9 = __Pyx_GetItemInt(__pyx_t_5, 0, long, 1, __Pyx_PyInt_From_long, 0, 0, 1); if (unlikely(__pyx_t_9 == NULL)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1145; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
      __Pyx_GOTREF(__pyx_t_9);
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      __pyx_t_6 = __Pyx_PyInt_As_int(__pyx_t_9); if (unlikely((__pyx_t_6 == (int)-1) && PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1145; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_9); __pyx_t_9 = 0;
      __pyx_v_plain_implicit = __pyx_t_6;

      






      __pyx_t_9 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_implicit); if (unlikely(!__pyx_t_9)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1146; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_9);
      __pyx_t_5 = __Pyx_GetItemInt(__pyx_t_9, 1, long, 1, __Pyx_PyInt_From_long, 0, 0, 1); if (unlikely(__pyx_t_5 == NULL)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1146; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_DECREF(__pyx_t_9); __pyx_t_9 = 0;
      __pyx_t_6 = __Pyx_PyInt_As_int(__pyx_t_5); if (unlikely((__pyx_t_6 == (int)-1) && PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1146; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      __pyx_v_quoted_implicit = __pyx_t_6;
      goto __pyx_L37;
    }
    __pyx_L37:;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_style); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1147; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_v_style_object = __pyx_t_5;
    __pyx_t_5 = 0;

    






    __pyx_v_scalar_style = YAML_PLAIN_SCALAR_STYLE;

    






    __pyx_t_4 = (__Pyx_PyString_Equals(__pyx_v_style_object, __pyx_kp_s__7, Py_EQ)); if (unlikely(__pyx_t_4 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1149; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    if (!__pyx_t_4) {
      __pyx_t_2 = (__Pyx_PyUnicode_Equals(__pyx_v_style_object, __pyx_kp_u__7, Py_EQ)); if (unlikely(__pyx_t_2 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1149; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __pyx_t_3 = __pyx_t_2;
    } else {
      __pyx_t_3 = __pyx_t_4;
    }
    if (__pyx_t_3) {

      






      __pyx_v_scalar_style = YAML_SINGLE_QUOTED_SCALAR_STYLE;
      goto __pyx_L38;
    }

    






    __pyx_t_3 = (__Pyx_PyString_Equals(__pyx_v_style_object, __pyx_kp_s__8, Py_EQ)); if (unlikely(__pyx_t_3 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1151; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    if (!__pyx_t_3) {
      __pyx_t_4 = (__Pyx_PyUnicode_Equals(__pyx_v_style_object, __pyx_kp_u__8, Py_EQ)); if (unlikely(__pyx_t_4 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1151; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __pyx_t_2 = __pyx_t_4;
    } else {
      __pyx_t_2 = __pyx_t_3;
    }
    if (__pyx_t_2) {

      






      __pyx_v_scalar_style = YAML_DOUBLE_QUOTED_SCALAR_STYLE;
      goto __pyx_L38;
    }

    






    __pyx_t_2 = (__Pyx_PyString_Equals(__pyx_v_style_object, __pyx_kp_s__9, Py_EQ)); if (unlikely(__pyx_t_2 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1153; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    if (!__pyx_t_2) {
      __pyx_t_3 = (__Pyx_PyUnicode_Equals(__pyx_v_style_object, __pyx_kp_u__9, Py_EQ)); if (unlikely(__pyx_t_3 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1153; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __pyx_t_4 = __pyx_t_3;
    } else {
      __pyx_t_4 = __pyx_t_2;
    }
    if (__pyx_t_4) {

      






      __pyx_v_scalar_style = YAML_LITERAL_SCALAR_STYLE;
      goto __pyx_L38;
    }

    






    __pyx_t_4 = (__Pyx_PyString_Equals(__pyx_v_style_object, __pyx_kp_s__10, Py_EQ)); if (unlikely(__pyx_t_4 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1155; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    if (!__pyx_t_4) {
      __pyx_t_2 = (__Pyx_PyUnicode_Equals(__pyx_v_style_object, __pyx_kp_u__10, Py_EQ)); if (unlikely(__pyx_t_2 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1155; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __pyx_t_3 = __pyx_t_2;
    } else {
      __pyx_t_3 = __pyx_t_4;
    }
    if (__pyx_t_3) {

      






      __pyx_v_scalar_style = YAML_FOLDED_SCALAR_STYLE;
      goto __pyx_L38;
    }
    __pyx_L38:;

    






    __pyx_t_3 = ((yaml_scalar_event_initialize(__pyx_v_event, __pyx_v_anchor, __pyx_v_tag, __pyx_v_value, __pyx_v_length, __pyx_v_plain_implicit, __pyx_v_quoted_implicit, __pyx_v_scalar_style) == 0) != 0);
    if (__pyx_t_3) {

      






      PyErr_NoMemory(); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1159; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
    goto __pyx_L3;
  }

  






  __pyx_t_5 = __Pyx_GetModuleGlobalName(__pyx_n_s_SequenceStartEvent); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1160; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_5);
  __pyx_t_3 = (__pyx_v_event_class == __pyx_t_5);
  __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
  __pyx_t_4 = (__pyx_t_3 != 0);
  if (__pyx_t_4) {

    






    __pyx_v_anchor = NULL;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_anchor); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1162; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_v_anchor_object = __pyx_t_5;
    __pyx_t_5 = 0;

    






    __pyx_t_4 = (__pyx_v_anchor_object != Py_None);
    __pyx_t_3 = (__pyx_t_4 != 0);
    if (__pyx_t_3) {

      






      __pyx_t_3 = (PyUnicode_CheckExact(__pyx_v_anchor_object) != 0);
      if (__pyx_t_3) {

        






        __pyx_t_5 = PyUnicode_AsUTF8String(__pyx_v_anchor_object); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1165; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_DECREF_SET(__pyx_v_anchor_object, __pyx_t_5);
        __pyx_t_5 = 0;
        goto __pyx_L41;
      }
      __pyx_L41:;

      






      __pyx_t_3 = ((!(PyString_CheckExact(__pyx_v_anchor_object) != 0)) != 0);
      if (__pyx_t_3) {

        






        __pyx_t_3 = ((PY_MAJOR_VERSION < 3) != 0);
        if (__pyx_t_3) {

          






          __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__36, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1168; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_5);
          __Pyx_Raise(__pyx_t_5, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1168; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
         {

          






          __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__37, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1170; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_5);
          __Pyx_Raise(__pyx_t_5, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1170; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
      }

      






      __pyx_v_anchor = PyString_AS_STRING(__pyx_v_anchor_object);
      goto __pyx_L40;
    }
    __pyx_L40:;

    






    __pyx_v_tag = NULL;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_tag); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1173; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_v_tag_object = __pyx_t_5;
    __pyx_t_5 = 0;

    






    __pyx_t_3 = (__pyx_v_tag_object != Py_None);
    __pyx_t_4 = (__pyx_t_3 != 0);
    if (__pyx_t_4) {

      






      __pyx_t_4 = (PyUnicode_CheckExact(__pyx_v_tag_object) != 0);
      if (__pyx_t_4) {

        






        __pyx_t_5 = PyUnicode_AsUTF8String(__pyx_v_tag_object); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1176; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_DECREF_SET(__pyx_v_tag_object, __pyx_t_5);
        __pyx_t_5 = 0;
        goto __pyx_L45;
      }
      __pyx_L45:;

      






      __pyx_t_4 = ((!(PyString_CheckExact(__pyx_v_tag_object) != 0)) != 0);
      if (__pyx_t_4) {

        






        __pyx_t_4 = ((PY_MAJOR_VERSION < 3) != 0);
        if (__pyx_t_4) {

          






          __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__38, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1179; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_5);
          __Pyx_Raise(__pyx_t_5, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1179; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
         {

          






          __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__39, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1181; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_5);
          __Pyx_Raise(__pyx_t_5, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1181; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
      }

      






      __pyx_v_tag = PyString_AS_STRING(__pyx_v_tag_object);
      goto __pyx_L44;
    }
    __pyx_L44:;

    






    __pyx_v_implicit = 0;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_implicit); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1184; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_t_4 = __Pyx_PyObject_IsTrue(__pyx_t_5); if (unlikely(__pyx_t_4 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1184; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
    if (__pyx_t_4) {

      






      __pyx_v_implicit = 1;
      goto __pyx_L48;
    }
    __pyx_L48:;

    






    __pyx_v_sequence_style = YAML_BLOCK_SEQUENCE_STYLE;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_flow_style); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1187; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_t_4 = __Pyx_PyObject_IsTrue(__pyx_t_5); if (unlikely(__pyx_t_4 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1187; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
    if (__pyx_t_4) {

      






      __pyx_v_sequence_style = YAML_FLOW_SEQUENCE_STYLE;
      goto __pyx_L49;
    }
    __pyx_L49:;

    






    __pyx_t_4 = ((yaml_sequence_start_event_initialize(__pyx_v_event, __pyx_v_anchor, __pyx_v_tag, __pyx_v_implicit, __pyx_v_sequence_style) == 0) != 0);
    if (__pyx_t_4) {

      






      PyErr_NoMemory(); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1191; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
    goto __pyx_L3;
  }

  






  __pyx_t_5 = __Pyx_GetModuleGlobalName(__pyx_n_s_MappingStartEvent); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1192; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_5);
  __pyx_t_4 = (__pyx_v_event_class == __pyx_t_5);
  __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
  __pyx_t_3 = (__pyx_t_4 != 0);
  if (__pyx_t_3) {

    






    __pyx_v_anchor = NULL;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_anchor); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1194; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_v_anchor_object = __pyx_t_5;
    __pyx_t_5 = 0;

    






    __pyx_t_3 = (__pyx_v_anchor_object != Py_None);
    __pyx_t_4 = (__pyx_t_3 != 0);
    if (__pyx_t_4) {

      






      __pyx_t_4 = (PyUnicode_CheckExact(__pyx_v_anchor_object) != 0);
      if (__pyx_t_4) {

        






        __pyx_t_5 = PyUnicode_AsUTF8String(__pyx_v_anchor_object); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1197; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_DECREF_SET(__pyx_v_anchor_object, __pyx_t_5);
        __pyx_t_5 = 0;
        goto __pyx_L52;
      }
      __pyx_L52:;

      






      __pyx_t_4 = ((!(PyString_CheckExact(__pyx_v_anchor_object) != 0)) != 0);
      if (__pyx_t_4) {

        






        __pyx_t_4 = ((PY_MAJOR_VERSION < 3) != 0);
        if (__pyx_t_4) {

          






          __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__40, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1200; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_5);
          __Pyx_Raise(__pyx_t_5, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1200; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
         {

          






          __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__41, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1202; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_5);
          __Pyx_Raise(__pyx_t_5, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1202; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
      }

      






      __pyx_v_anchor = PyString_AS_STRING(__pyx_v_anchor_object);
      goto __pyx_L51;
    }
    __pyx_L51:;

    






    __pyx_v_tag = NULL;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_tag); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1205; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_v_tag_object = __pyx_t_5;
    __pyx_t_5 = 0;

    






    __pyx_t_4 = (__pyx_v_tag_object != Py_None);
    __pyx_t_3 = (__pyx_t_4 != 0);
    if (__pyx_t_3) {

      






      __pyx_t_3 = (PyUnicode_CheckExact(__pyx_v_tag_object) != 0);
      if (__pyx_t_3) {

        






        __pyx_t_5 = PyUnicode_AsUTF8String(__pyx_v_tag_object); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1208; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_DECREF_SET(__pyx_v_tag_object, __pyx_t_5);
        __pyx_t_5 = 0;
        goto __pyx_L56;
      }
      __pyx_L56:;

      






      __pyx_t_3 = ((!(PyString_CheckExact(__pyx_v_tag_object) != 0)) != 0);
      if (__pyx_t_3) {

        






        __pyx_t_3 = ((PY_MAJOR_VERSION < 3) != 0);
        if (__pyx_t_3) {

          






          __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__42, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1211; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_5);
          __Pyx_Raise(__pyx_t_5, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1211; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
         {

          






          __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__43, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1213; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_5);
          __Pyx_Raise(__pyx_t_5, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1213; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
      }

      






      __pyx_v_tag = PyString_AS_STRING(__pyx_v_tag_object);
      goto __pyx_L55;
    }
    __pyx_L55:;

    






    __pyx_v_implicit = 0;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_implicit); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1216; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_t_3 = __Pyx_PyObject_IsTrue(__pyx_t_5); if (unlikely(__pyx_t_3 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1216; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
    if (__pyx_t_3) {

      






      __pyx_v_implicit = 1;
      goto __pyx_L59;
    }
    __pyx_L59:;

    






    __pyx_v_mapping_style = YAML_BLOCK_MAPPING_STYLE;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_event_object, __pyx_n_s_flow_style); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1219; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_t_3 = __Pyx_PyObject_IsTrue(__pyx_t_5); if (unlikely(__pyx_t_3 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1219; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
    if (__pyx_t_3) {

      






      __pyx_v_mapping_style = YAML_FLOW_MAPPING_STYLE;
      goto __pyx_L60;
    }
    __pyx_L60:;

    






    __pyx_t_3 = ((yaml_mapping_start_event_initialize(__pyx_v_event, __pyx_v_anchor, __pyx_v_tag, __pyx_v_implicit, __pyx_v_mapping_style) == 0) != 0);
    if (__pyx_t_3) {

      






      PyErr_NoMemory(); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1223; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
    goto __pyx_L3;
  }

  






  __pyx_t_5 = __Pyx_GetModuleGlobalName(__pyx_n_s_SequenceEndEvent); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1224; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_5);
  __pyx_t_3 = (__pyx_v_event_class == __pyx_t_5);
  __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
  __pyx_t_4 = (__pyx_t_3 != 0);
  if (__pyx_t_4) {

    






    yaml_sequence_end_event_initialize(__pyx_v_event);
    goto __pyx_L3;
  }

  






  __pyx_t_5 = __Pyx_GetModuleGlobalName(__pyx_n_s_MappingEndEvent); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1226; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_5);
  __pyx_t_4 = (__pyx_v_event_class == __pyx_t_5);
  __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
  __pyx_t_3 = (__pyx_t_4 != 0);
  if (__pyx_t_3) {

    






    yaml_mapping_end_event_initialize(__pyx_v_event);
    goto __pyx_L3;
  }
   {

    






    __pyx_t_3 = ((PY_MAJOR_VERSION < 3) != 0);
    if (__pyx_t_3) {

      






      __pyx_t_5 = __Pyx_PyString_Format(__pyx_kp_s_invalid_event_s, __pyx_v_event_object); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1230; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __pyx_t_9 = PyTuple_New(1); if (unlikely(!__pyx_t_9)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1230; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_9);
      PyTuple_SET_ITEM(__pyx_t_9, 0, __pyx_t_5);
      __Pyx_GIVEREF(__pyx_t_5);
      __pyx_t_5 = 0;
      __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_t_9, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1230; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_DECREF(__pyx_t_9); __pyx_t_9 = 0;
      __Pyx_Raise(__pyx_t_5, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1230; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
     {

      






      __pyx_t_5 = PyUnicode_Format(__pyx_kp_u_invalid_event_s, __pyx_v_event_object); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1232; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __pyx_t_9 = PyTuple_New(1); if (unlikely(!__pyx_t_9)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1232; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_9);
      PyTuple_SET_ITEM(__pyx_t_9, 0, __pyx_t_5);
      __Pyx_GIVEREF(__pyx_t_5);
      __pyx_t_5 = 0;
      __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_t_9, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1232; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_DECREF(__pyx_t_9); __pyx_t_9 = 0;
      __Pyx_Raise(__pyx_t_5, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1232; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
  }
  __pyx_L3:;

  






  __pyx_r = 1;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_XDECREF(__pyx_t_5);
  __Pyx_XDECREF(__pyx_t_9);
  __Pyx_AddTraceback("_yaml.CEmitter._object_to_event", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_event_class);
  __Pyx_XDECREF(__pyx_v_cache);
  __Pyx_XDECREF(__pyx_v_handle);
  __Pyx_XDECREF(__pyx_v_prefix);
  __Pyx_XDECREF(__pyx_v_anchor_object);
  __Pyx_XDECREF(__pyx_v_tag_object);
  __Pyx_XDECREF(__pyx_v_value_object);
  __Pyx_XDECREF(__pyx_v_style_object);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_8CEmitter_7emit(PyObject *__pyx_v_self, PyObject *__pyx_v_event_object); 
static PyObject *__pyx_pw_5_yaml_8CEmitter_7emit(PyObject *__pyx_v_self, PyObject *__pyx_v_event_object) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("emit (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_8CEmitter_6emit(((struct __pyx_obj_5_yaml_CEmitter *)__pyx_v_self), ((PyObject *)__pyx_v_event_object));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_8CEmitter_6emit(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self, PyObject *__pyx_v_event_object) {
  yaml_event_t __pyx_v_event;
  PyObject *__pyx_v_error = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("emit", 0);

  






  __pyx_t_1 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_object_to_event(__pyx_v_self, __pyx_v_event_object, (&__pyx_v_event)); if (unlikely(__pyx_t_1 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1237; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

  






  __pyx_t_1 = yaml_emitter_emit((&__pyx_v_self->emitter), (&__pyx_v_event)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1238; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_t_2 = ((__pyx_t_1 == 0) != 0);
  if (__pyx_t_2) {

    






    __pyx_t_3 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_emitter_error(__pyx_v_self); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1239; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_v_error = __pyx_t_3;
    __pyx_t_3 = 0;

    






    __Pyx_Raise(__pyx_v_error, 0, 0, 0);
    {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1240; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  }

  







  
  __pyx_r = Py_None; __Pyx_INCREF(Py_None);
  goto __pyx_L0;
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_AddTraceback("_yaml.CEmitter.emit", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_error);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_8CEmitter_9open(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused); 
static PyObject *__pyx_pw_5_yaml_8CEmitter_9open(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("open (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_8CEmitter_8open(((struct __pyx_obj_5_yaml_CEmitter *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_8CEmitter_8open(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self) {
  yaml_event_t __pyx_v_event;
  yaml_encoding_t __pyx_v_encoding;
  PyObject *__pyx_v_error = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  int __pyx_t_3;
  int __pyx_t_4;
  PyObject *__pyx_t_5 = NULL;
  PyObject *__pyx_t_6 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("open", 0);

  






  switch (__pyx_v_self->closed) {

    






    case -1:

    






    __pyx_t_1 = (__Pyx_PyUnicode_Equals(__pyx_v_self->use_encoding, __pyx_kp_u_utf_16_le, Py_EQ)); if (unlikely(__pyx_t_1 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1246; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    if (!__pyx_t_1) {
      __pyx_t_2 = (__Pyx_PyString_Equals(__pyx_v_self->use_encoding, __pyx_kp_s_utf_16_le, Py_EQ)); if (unlikely(__pyx_t_2 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1246; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __pyx_t_3 = __pyx_t_2;
    } else {
      __pyx_t_3 = __pyx_t_1;
    }
    if (__pyx_t_3) {

      






      __pyx_v_encoding = YAML_UTF16LE_ENCODING;
      goto __pyx_L3;
    }

    






    __pyx_t_3 = (__Pyx_PyUnicode_Equals(__pyx_v_self->use_encoding, __pyx_kp_u_utf_16_be, Py_EQ)); if (unlikely(__pyx_t_3 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1248; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    if (!__pyx_t_3) {
      __pyx_t_1 = (__Pyx_PyString_Equals(__pyx_v_self->use_encoding, __pyx_kp_s_utf_16_be, Py_EQ)); if (unlikely(__pyx_t_1 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1248; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __pyx_t_2 = __pyx_t_1;
    } else {
      __pyx_t_2 = __pyx_t_3;
    }
    if (__pyx_t_2) {

      






      __pyx_v_encoding = YAML_UTF16BE_ENCODING;
      goto __pyx_L3;
    }
     {

      






      __pyx_v_encoding = YAML_UTF8_ENCODING;
    }
    __pyx_L3:;

    






    __pyx_t_2 = (__pyx_v_self->use_encoding == Py_None);
    __pyx_t_3 = (__pyx_t_2 != 0);
    if (__pyx_t_3) {

      






      __pyx_v_self->dump_unicode = 1;
      goto __pyx_L4;
    }
    __pyx_L4:;

    






    __pyx_t_3 = ((__pyx_v_self->dump_unicode == 1) != 0);
    if (__pyx_t_3) {

      






      __pyx_v_encoding = YAML_UTF8_ENCODING;
      goto __pyx_L5;
    }
    __pyx_L5:;

    






    yaml_stream_start_event_initialize((&__pyx_v_event), __pyx_v_encoding);

    






    __pyx_t_4 = yaml_emitter_emit((&__pyx_v_self->emitter), (&__pyx_v_event)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1257; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __pyx_t_3 = ((__pyx_t_4 == 0) != 0);
    if (__pyx_t_3) {

      






      __pyx_t_5 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_emitter_error(__pyx_v_self); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1258; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __pyx_v_error = __pyx_t_5;
      __pyx_t_5 = 0;

      






      __Pyx_Raise(__pyx_v_error, 0, 0, 0);
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1259; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }

    






    __pyx_v_self->closed = 0;
    break;

    






    case 1:

    






    __pyx_t_3 = ((PY_MAJOR_VERSION < 3) != 0);
    if (__pyx_t_3) {

      






      __pyx_t_5 = __Pyx_GetModuleGlobalName(__pyx_n_s_SerializerError); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1263; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __pyx_t_6 = __Pyx_PyObject_Call(__pyx_t_5, __pyx_tuple__44, NULL); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1263; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      __Pyx_Raise(__pyx_t_6, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1263; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
     {

      






      __pyx_t_6 = __Pyx_GetModuleGlobalName(__pyx_n_s_SerializerError); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1265; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);
      __pyx_t_5 = __Pyx_PyObject_Call(__pyx_t_6, __pyx_tuple__45, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1265; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
      __Pyx_Raise(__pyx_t_5, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1265; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
    break;
    default:

    






    __pyx_t_3 = ((PY_MAJOR_VERSION < 3) != 0);
    if (__pyx_t_3) {

      






      __pyx_t_5 = __Pyx_GetModuleGlobalName(__pyx_n_s_SerializerError); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1268; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __pyx_t_6 = __Pyx_PyObject_Call(__pyx_t_5, __pyx_tuple__46, NULL); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1268; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      __Pyx_Raise(__pyx_t_6, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1268; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
     {

      






      __pyx_t_6 = __Pyx_GetModuleGlobalName(__pyx_n_s_SerializerError); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1270; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);
      __pyx_t_5 = __Pyx_PyObject_Call(__pyx_t_6, __pyx_tuple__47, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1270; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
      __Pyx_Raise(__pyx_t_5, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1270; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
    break;
  }

  







  
  __pyx_r = Py_None; __Pyx_INCREF(Py_None);
  goto __pyx_L0;
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_5);
  __Pyx_XDECREF(__pyx_t_6);
  __Pyx_AddTraceback("_yaml.CEmitter.open", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_error);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_8CEmitter_11close(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused); 
static PyObject *__pyx_pw_5_yaml_8CEmitter_11close(PyObject *__pyx_v_self, CYTHON_UNUSED PyObject *unused) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("close (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_8CEmitter_10close(((struct __pyx_obj_5_yaml_CEmitter *)__pyx_v_self));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_8CEmitter_10close(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self) {
  yaml_event_t __pyx_v_event;
  PyObject *__pyx_v_error = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  PyObject *__pyx_t_2 = NULL;
  PyObject *__pyx_t_3 = NULL;
  int __pyx_t_4;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("close", 0);

  






  switch (__pyx_v_self->closed) {

    






    case -1:

    






    __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
    if (__pyx_t_1) {

      






      __pyx_t_2 = __Pyx_GetModuleGlobalName(__pyx_n_s_SerializerError); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1276; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_tuple__48, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1276; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
      __Pyx_Raise(__pyx_t_3, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1276; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
     {

      






      __pyx_t_3 = __Pyx_GetModuleGlobalName(__pyx_n_s_SerializerError); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1278; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __pyx_t_2 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_tuple__49, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1278; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
      __Pyx_Raise(__pyx_t_2, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1278; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
    break;

    






    case 0:

    






    yaml_stream_end_event_initialize((&__pyx_v_event));

    






    __pyx_t_4 = yaml_emitter_emit((&__pyx_v_self->emitter), (&__pyx_v_event)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1281; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __pyx_t_1 = ((__pyx_t_4 == 0) != 0);
    if (__pyx_t_1) {

      






      __pyx_t_2 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_emitter_error(__pyx_v_self); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1282; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __pyx_v_error = __pyx_t_2;
      __pyx_t_2 = 0;

      






      __Pyx_Raise(__pyx_v_error, 0, 0, 0);
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1283; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }

    






    __pyx_v_self->closed = 1;
    break;
    default: break;
  }

  







  
  __pyx_r = Py_None; __Pyx_INCREF(Py_None);
  goto __pyx_L0;
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_2);
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_AddTraceback("_yaml.CEmitter.close", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_error);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}










static PyObject *__pyx_pw_5_yaml_8CEmitter_13serialize(PyObject *__pyx_v_self, PyObject *__pyx_v_node); 
static PyObject *__pyx_pw_5_yaml_8CEmitter_13serialize(PyObject *__pyx_v_self, PyObject *__pyx_v_node) {
  PyObject *__pyx_r = 0;
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("serialize (wrapper)", 0);
  __pyx_r = __pyx_pf_5_yaml_8CEmitter_12serialize(((struct __pyx_obj_5_yaml_CEmitter *)__pyx_v_self), ((PyObject *)__pyx_v_node));

  
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_pf_5_yaml_8CEmitter_12serialize(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self, PyObject *__pyx_v_node) {
  yaml_event_t __pyx_v_event;
  yaml_version_directive_t __pyx_v_version_directive_value;
  yaml_version_directive_t *__pyx_v_version_directive;
  yaml_tag_directive_t __pyx_v_tag_directives_value[128];
  yaml_tag_directive_t *__pyx_v_tag_directives_start;
  yaml_tag_directive_t *__pyx_v_tag_directives_end;
  PyObject *__pyx_v_cache = NULL;
  PyObject *__pyx_v_handle = NULL;
  PyObject *__pyx_v_prefix = NULL;
  PyObject *__pyx_v_error = NULL;
  PyObject *__pyx_r = NULL;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  PyObject *__pyx_t_2 = NULL;
  PyObject *__pyx_t_3 = NULL;
  int __pyx_t_4;
  Py_ssize_t __pyx_t_5;
  PyObject *(*__pyx_t_6)(PyObject *);
  int __pyx_t_7;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("serialize", 0);

  






  switch (__pyx_v_self->closed) {

    






    case -1:

    






    __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
    if (__pyx_t_1) {

      






      __pyx_t_2 = __Pyx_GetModuleGlobalName(__pyx_n_s_SerializerError); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1295; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_tuple__50, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1295; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
      __Pyx_Raise(__pyx_t_3, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1295; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
     {

      






      __pyx_t_3 = __Pyx_GetModuleGlobalName(__pyx_n_s_SerializerError); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1297; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __pyx_t_2 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_tuple__51, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1297; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
      __Pyx_Raise(__pyx_t_2, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1297; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
    break;

    






    case 1:

    






    __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
    if (__pyx_t_1) {

      






      __pyx_t_2 = __Pyx_GetModuleGlobalName(__pyx_n_s_SerializerError); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1300; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __pyx_t_3 = __Pyx_PyObject_Call(__pyx_t_2, __pyx_tuple__52, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1300; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
      __Pyx_Raise(__pyx_t_3, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1300; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
     {

      






      __pyx_t_3 = __Pyx_GetModuleGlobalName(__pyx_n_s_SerializerError); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1302; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __pyx_t_2 = __Pyx_PyObject_Call(__pyx_t_3, __pyx_tuple__53, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1302; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
      __Pyx_Raise(__pyx_t_2, 0, 0, 0);
      __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1302; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
    break;
    default: break;
  }

  






  __pyx_t_2 = PyList_New(0); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1303; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __pyx_v_cache = ((PyObject*)__pyx_t_2);
  __pyx_t_2 = 0;

  






  __pyx_v_version_directive = NULL;

  






  __pyx_t_1 = __Pyx_PyObject_IsTrue(__pyx_v_self->use_version); if (unlikely(__pyx_t_1 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1305; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  if (__pyx_t_1) {

    






    __pyx_t_2 = __Pyx_GetItemInt(__pyx_v_self->use_version, 0, long, 1, __Pyx_PyInt_From_long, 0, 0, 1); if (unlikely(__pyx_t_2 == NULL)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1306; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_t_4 = __Pyx_PyInt_As_int(__pyx_t_2); if (unlikely((__pyx_t_4 == (int)-1) && PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1306; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_v_version_directive_value.major = __pyx_t_4;

    






    __pyx_t_2 = __Pyx_GetItemInt(__pyx_v_self->use_version, 1, long, 1, __Pyx_PyInt_From_long, 0, 0, 1); if (unlikely(__pyx_t_2 == NULL)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1307; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_t_4 = __Pyx_PyInt_As_int(__pyx_t_2); if (unlikely((__pyx_t_4 == (int)-1) && PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1307; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_v_version_directive_value.minor = __pyx_t_4;

    






    __pyx_v_version_directive = (&__pyx_v_version_directive_value);
    goto __pyx_L5;
  }
  __pyx_L5:;

  






  __pyx_v_tag_directives_start = NULL;

  






  __pyx_v_tag_directives_end = NULL;

  






  __pyx_t_1 = __Pyx_PyObject_IsTrue(__pyx_v_self->use_tags); if (unlikely(__pyx_t_1 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1311; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  if (__pyx_t_1) {

    






    __pyx_t_2 = __pyx_v_self->use_tags;
    __Pyx_INCREF(__pyx_t_2);
    __pyx_t_5 = PyObject_Length(__pyx_t_2); if (unlikely(__pyx_t_5 == -1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1312; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    __pyx_t_1 = ((__pyx_t_5 > 128) != 0);
    if (__pyx_t_1) {

      






      __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
      if (__pyx_t_1) {

        






        __pyx_t_2 = __Pyx_PyObject_Call(__pyx_builtin_ValueError, __pyx_tuple__54, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1314; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_2);
        __Pyx_Raise(__pyx_t_2, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1314; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
       {

        






        __pyx_t_2 = __Pyx_PyObject_Call(__pyx_builtin_ValueError, __pyx_tuple__55, NULL); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1316; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_2);
        __Pyx_Raise(__pyx_t_2, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1316; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
    }

    






    __pyx_v_tag_directives_start = __pyx_v_tag_directives_value;

    






    __pyx_v_tag_directives_end = __pyx_v_tag_directives_value;

    






    if (PyList_CheckExact(__pyx_v_self->use_tags) || PyTuple_CheckExact(__pyx_v_self->use_tags)) {
      __pyx_t_2 = __pyx_v_self->use_tags; __Pyx_INCREF(__pyx_t_2); __pyx_t_5 = 0;
      __pyx_t_6 = NULL;
    } else {
      __pyx_t_5 = -1; __pyx_t_2 = PyObject_GetIter(__pyx_v_self->use_tags); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1319; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_2);
      __pyx_t_6 = Py_TYPE(__pyx_t_2)->tp_iternext;
    }
    for (;;) {
      if (!__pyx_t_6 && PyList_CheckExact(__pyx_t_2)) {
        if (__pyx_t_5 >= PyList_GET_SIZE(__pyx_t_2)) break;
        #if CYTHON_COMPILING_IN_CPYTHON
        __pyx_t_3 = PyList_GET_ITEM(__pyx_t_2, __pyx_t_5); __Pyx_INCREF(__pyx_t_3); __pyx_t_5++; if (unlikely(0 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1319; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        #else
        __pyx_t_3 = PySequence_ITEM(__pyx_t_2, __pyx_t_5); __pyx_t_5++; if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1319; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        #endif
      } else if (!__pyx_t_6 && PyTuple_CheckExact(__pyx_t_2)) {
        if (__pyx_t_5 >= PyTuple_GET_SIZE(__pyx_t_2)) break;
        #if CYTHON_COMPILING_IN_CPYTHON
        __pyx_t_3 = PyTuple_GET_ITEM(__pyx_t_2, __pyx_t_5); __Pyx_INCREF(__pyx_t_3); __pyx_t_5++; if (unlikely(0 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1319; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        #else
        __pyx_t_3 = PySequence_ITEM(__pyx_t_2, __pyx_t_5); __pyx_t_5++; if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1319; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        #endif
      } else {
        __pyx_t_3 = __pyx_t_6(__pyx_t_2);
        if (unlikely(!__pyx_t_3)) {
          PyObject* exc_type = PyErr_Occurred();
          if (exc_type) {
            if (likely(exc_type == PyExc_StopIteration || PyErr_GivenExceptionMatches(exc_type, PyExc_StopIteration))) PyErr_Clear();
            else {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1319; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          }
          break;
        }
        __Pyx_GOTREF(__pyx_t_3);
      }
      __Pyx_XDECREF_SET(__pyx_v_handle, __pyx_t_3);
      __pyx_t_3 = 0;

      






      __pyx_t_3 = PyObject_GetItem(__pyx_v_self->use_tags, __pyx_v_handle); if (unlikely(__pyx_t_3 == NULL)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1320; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
      __Pyx_GOTREF(__pyx_t_3);
      __Pyx_XDECREF_SET(__pyx_v_prefix, __pyx_t_3);
      __pyx_t_3 = 0;

      






      __pyx_t_1 = (PyUnicode_CheckExact(__pyx_v_handle) != 0);
      if (__pyx_t_1) {

        






        __pyx_t_3 = PyUnicode_AsUTF8String(__pyx_v_handle); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1322; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_3);
        __Pyx_DECREF_SET(__pyx_v_handle, __pyx_t_3);
        __pyx_t_3 = 0;

        






        __pyx_t_7 = __Pyx_PyList_Append(__pyx_v_cache, __pyx_v_handle); if (unlikely(__pyx_t_7 == -1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1323; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        goto __pyx_L11;
      }
      __pyx_L11:;

      






      __pyx_t_1 = ((!(PyString_CheckExact(__pyx_v_handle) != 0)) != 0);
      if (__pyx_t_1) {

        






        __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
        if (__pyx_t_1) {

          






          __pyx_t_3 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__56, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1326; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_3);
          __Pyx_Raise(__pyx_t_3, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1326; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
         {

          






          __pyx_t_3 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__57, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1328; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_3);
          __Pyx_Raise(__pyx_t_3, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1328; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
      }

      






      __pyx_v_tag_directives_end->handle = PyString_AS_STRING(__pyx_v_handle);

      






      __pyx_t_1 = (PyUnicode_CheckExact(__pyx_v_prefix) != 0);
      if (__pyx_t_1) {

        






        __pyx_t_3 = PyUnicode_AsUTF8String(__pyx_v_prefix); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1331; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_3);
        __Pyx_DECREF_SET(__pyx_v_prefix, __pyx_t_3);
        __pyx_t_3 = 0;

        






        __pyx_t_7 = __Pyx_PyList_Append(__pyx_v_cache, __pyx_v_prefix); if (unlikely(__pyx_t_7 == -1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1332; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        goto __pyx_L14;
      }
      __pyx_L14:;

      






      __pyx_t_1 = ((!(PyString_CheckExact(__pyx_v_prefix) != 0)) != 0);
      if (__pyx_t_1) {

        






        __pyx_t_1 = ((PY_MAJOR_VERSION < 3) != 0);
        if (__pyx_t_1) {

          






          __pyx_t_3 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__58, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1335; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_3);
          __Pyx_Raise(__pyx_t_3, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1335; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
         {

          






          __pyx_t_3 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__59, NULL); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1337; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_3);
          __Pyx_Raise(__pyx_t_3, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1337; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
      }

      






      __pyx_v_tag_directives_end->prefix = PyString_AS_STRING(__pyx_v_prefix);

      






      __pyx_v_tag_directives_end = (__pyx_v_tag_directives_end + 1);
    }
    __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
    goto __pyx_L6;
  }
  __pyx_L6:;

  






  __pyx_t_1 = ((yaml_document_start_event_initialize((&__pyx_v_event), __pyx_v_version_directive, __pyx_v_tag_directives_start, __pyx_v_tag_directives_end, __pyx_v_self->document_start_implicit) == 0) != 0);
  if (__pyx_t_1) {

    






    PyErr_NoMemory(); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1343; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  }

  






  __pyx_t_4 = yaml_emitter_emit((&__pyx_v_self->emitter), (&__pyx_v_event)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1344; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_t_1 = ((__pyx_t_4 == 0) != 0);
  if (__pyx_t_1) {

    






    __pyx_t_2 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_emitter_error(__pyx_v_self); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1345; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_v_error = __pyx_t_2;
    __pyx_t_2 = 0;

    






    __Pyx_Raise(__pyx_v_error, 0, 0, 0);
    {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1346; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  }

  






  __pyx_t_4 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_anchor_node(__pyx_v_self, __pyx_v_node); if (unlikely(__pyx_t_4 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1347; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

  






  __pyx_t_4 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_serialize_node(__pyx_v_self, __pyx_v_node, Py_None, Py_None); if (unlikely(__pyx_t_4 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1348; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

  






  yaml_document_end_event_initialize((&__pyx_v_event), __pyx_v_self->document_end_implicit);

  






  __pyx_t_4 = yaml_emitter_emit((&__pyx_v_self->emitter), (&__pyx_v_event)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1350; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_t_1 = ((__pyx_t_4 == 0) != 0);
  if (__pyx_t_1) {

    






    __pyx_t_2 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_emitter_error(__pyx_v_self); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1351; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_2);
    __pyx_v_error = __pyx_t_2;
    __pyx_t_2 = 0;

    






    __Pyx_Raise(__pyx_v_error, 0, 0, 0);
    {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1352; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  }

  






  __pyx_t_2 = PyDict_New(); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1353; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  __Pyx_GOTREF(__pyx_v_self->serialized_nodes);
  __Pyx_DECREF(__pyx_v_self->serialized_nodes);
  __pyx_v_self->serialized_nodes = __pyx_t_2;
  __pyx_t_2 = 0;

  






  __pyx_t_2 = PyDict_New(); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1354; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_GIVEREF(__pyx_t_2);
  __Pyx_GOTREF(__pyx_v_self->anchors);
  __Pyx_DECREF(__pyx_v_self->anchors);
  __pyx_v_self->anchors = __pyx_t_2;
  __pyx_t_2 = 0;

  






  __pyx_v_self->last_alias_id = 0;

  







  
  __pyx_r = Py_None; __Pyx_INCREF(Py_None);
  goto __pyx_L0;
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_2);
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_AddTraceback("_yaml.CEmitter.serialize", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = NULL;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_cache);
  __Pyx_XDECREF(__pyx_v_handle);
  __Pyx_XDECREF(__pyx_v_prefix);
  __Pyx_XDECREF(__pyx_v_error);
  __Pyx_XGIVEREF(__pyx_r);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static int __pyx_f_5_yaml_8CEmitter__anchor_node(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self, PyObject *__pyx_v_node) {
  PyObject *__pyx_v_node_class = NULL;
  PyObject *__pyx_v_item = NULL;
  PyObject *__pyx_v_key = NULL;
  PyObject *__pyx_v_value = NULL;
  int __pyx_r;
  __Pyx_RefNannyDeclarations
  int __pyx_t_1;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  PyObject *__pyx_t_4 = NULL;
  Py_ssize_t __pyx_t_5;
  PyObject *(*__pyx_t_6)(PyObject *);
  int __pyx_t_7;
  PyObject *__pyx_t_8 = NULL;
  PyObject *__pyx_t_9 = NULL;
  PyObject *__pyx_t_10 = NULL;
  PyObject *(*__pyx_t_11)(PyObject *);
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("_anchor_node", 0);

  






  __pyx_t_1 = (__Pyx_PySequence_Contains(__pyx_v_node, __pyx_v_self->anchors, Py_EQ)); if (unlikely(__pyx_t_1 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1358; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_t_2 = (__pyx_t_1 != 0);
  if (__pyx_t_2) {

    






    __pyx_t_3 = PyObject_GetItem(__pyx_v_self->anchors, __pyx_v_node); if (unlikely(__pyx_t_3 == NULL)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1359; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_2 = (__pyx_t_3 == Py_None);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __pyx_t_1 = (__pyx_t_2 != 0);
    if (__pyx_t_1) {

      






      __pyx_v_self->last_alias_id = (__pyx_v_self->last_alias_id + 1);

      






      __pyx_t_3 = __Pyx_PyInt_From_int(__pyx_v_self->last_alias_id); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1361; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      __pyx_t_4 = PyUnicode_Format(__pyx_kp_u_id_03d, __pyx_t_3); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1361; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);
      __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
      if (unlikely(PyObject_SetItem(__pyx_v_self->anchors, __pyx_v_node, __pyx_t_4) < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1361; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
      goto __pyx_L4;
    }
    __pyx_L4:;
    goto __pyx_L3;
  }
   {

    






    if (unlikely(PyObject_SetItem(__pyx_v_self->anchors, __pyx_v_node, Py_None) < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1363; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

    






    __pyx_t_4 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_class); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1364; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_v_node_class = __pyx_t_4;
    __pyx_t_4 = 0;

    






    __pyx_t_4 = __Pyx_GetModuleGlobalName(__pyx_n_s_SequenceNode); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1365; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_4);
    __pyx_t_1 = (__pyx_v_node_class == __pyx_t_4);
    __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
    __pyx_t_2 = (__pyx_t_1 != 0);
    if (__pyx_t_2) {

      






      __pyx_t_4 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_value); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1366; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_4);
      if (PyList_CheckExact(__pyx_t_4) || PyTuple_CheckExact(__pyx_t_4)) {
        __pyx_t_3 = __pyx_t_4; __Pyx_INCREF(__pyx_t_3); __pyx_t_5 = 0;
        __pyx_t_6 = NULL;
      } else {
        __pyx_t_5 = -1; __pyx_t_3 = PyObject_GetIter(__pyx_t_4); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1366; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_3);
        __pyx_t_6 = Py_TYPE(__pyx_t_3)->tp_iternext;
      }
      __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
      for (;;) {
        if (!__pyx_t_6 && PyList_CheckExact(__pyx_t_3)) {
          if (__pyx_t_5 >= PyList_GET_SIZE(__pyx_t_3)) break;
          #if CYTHON_COMPILING_IN_CPYTHON
          __pyx_t_4 = PyList_GET_ITEM(__pyx_t_3, __pyx_t_5); __Pyx_INCREF(__pyx_t_4); __pyx_t_5++; if (unlikely(0 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1366; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #else
          __pyx_t_4 = PySequence_ITEM(__pyx_t_3, __pyx_t_5); __pyx_t_5++; if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1366; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #endif
        } else if (!__pyx_t_6 && PyTuple_CheckExact(__pyx_t_3)) {
          if (__pyx_t_5 >= PyTuple_GET_SIZE(__pyx_t_3)) break;
          #if CYTHON_COMPILING_IN_CPYTHON
          __pyx_t_4 = PyTuple_GET_ITEM(__pyx_t_3, __pyx_t_5); __Pyx_INCREF(__pyx_t_4); __pyx_t_5++; if (unlikely(0 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1366; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #else
          __pyx_t_4 = PySequence_ITEM(__pyx_t_3, __pyx_t_5); __pyx_t_5++; if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1366; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #endif
        } else {
          __pyx_t_4 = __pyx_t_6(__pyx_t_3);
          if (unlikely(!__pyx_t_4)) {
            PyObject* exc_type = PyErr_Occurred();
            if (exc_type) {
              if (likely(exc_type == PyExc_StopIteration || PyErr_GivenExceptionMatches(exc_type, PyExc_StopIteration))) PyErr_Clear();
              else {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1366; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
            }
            break;
          }
          __Pyx_GOTREF(__pyx_t_4);
        }
        __Pyx_XDECREF_SET(__pyx_v_item, __pyx_t_4);
        __pyx_t_4 = 0;

        






        __pyx_t_7 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_anchor_node(__pyx_v_self, __pyx_v_item); if (unlikely(__pyx_t_7 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1367; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
      __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
      goto __pyx_L5;
    }

    






    __pyx_t_3 = __Pyx_GetModuleGlobalName(__pyx_n_s_MappingNode); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1368; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_3);
    __pyx_t_2 = (__pyx_v_node_class == __pyx_t_3);
    __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
    __pyx_t_1 = (__pyx_t_2 != 0);
    if (__pyx_t_1) {

      






      __pyx_t_3 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_value); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1369; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_3);
      if (PyList_CheckExact(__pyx_t_3) || PyTuple_CheckExact(__pyx_t_3)) {
        __pyx_t_4 = __pyx_t_3; __Pyx_INCREF(__pyx_t_4); __pyx_t_5 = 0;
        __pyx_t_6 = NULL;
      } else {
        __pyx_t_5 = -1; __pyx_t_4 = PyObject_GetIter(__pyx_t_3); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1369; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_4);
        __pyx_t_6 = Py_TYPE(__pyx_t_4)->tp_iternext;
      }
      __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
      for (;;) {
        if (!__pyx_t_6 && PyList_CheckExact(__pyx_t_4)) {
          if (__pyx_t_5 >= PyList_GET_SIZE(__pyx_t_4)) break;
          #if CYTHON_COMPILING_IN_CPYTHON
          __pyx_t_3 = PyList_GET_ITEM(__pyx_t_4, __pyx_t_5); __Pyx_INCREF(__pyx_t_3); __pyx_t_5++; if (unlikely(0 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1369; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #else
          __pyx_t_3 = PySequence_ITEM(__pyx_t_4, __pyx_t_5); __pyx_t_5++; if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1369; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #endif
        } else if (!__pyx_t_6 && PyTuple_CheckExact(__pyx_t_4)) {
          if (__pyx_t_5 >= PyTuple_GET_SIZE(__pyx_t_4)) break;
          #if CYTHON_COMPILING_IN_CPYTHON
          __pyx_t_3 = PyTuple_GET_ITEM(__pyx_t_4, __pyx_t_5); __Pyx_INCREF(__pyx_t_3); __pyx_t_5++; if (unlikely(0 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1369; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #else
          __pyx_t_3 = PySequence_ITEM(__pyx_t_4, __pyx_t_5); __pyx_t_5++; if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1369; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #endif
        } else {
          __pyx_t_3 = __pyx_t_6(__pyx_t_4);
          if (unlikely(!__pyx_t_3)) {
            PyObject* exc_type = PyErr_Occurred();
            if (exc_type) {
              if (likely(exc_type == PyExc_StopIteration || PyErr_GivenExceptionMatches(exc_type, PyExc_StopIteration))) PyErr_Clear();
              else {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1369; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
            }
            break;
          }
          __Pyx_GOTREF(__pyx_t_3);
        }
        if ((likely(PyTuple_CheckExact(__pyx_t_3))) || (PyList_CheckExact(__pyx_t_3))) {
          PyObject* sequence = __pyx_t_3;
          #if CYTHON_COMPILING_IN_CPYTHON
          Py_ssize_t size = Py_SIZE(sequence);
          #else
          Py_ssize_t size = PySequence_Size(sequence);
          #endif
          if (unlikely(size != 2)) {
            if (size > 2) __Pyx_RaiseTooManyValuesError(2);
            else if (size >= 0) __Pyx_RaiseNeedMoreValuesError(size);
            {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1369; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          }
          #if CYTHON_COMPILING_IN_CPYTHON
          if (likely(PyTuple_CheckExact(sequence))) {
            __pyx_t_8 = PyTuple_GET_ITEM(sequence, 0); 
            __pyx_t_9 = PyTuple_GET_ITEM(sequence, 1); 
          } else {
            __pyx_t_8 = PyList_GET_ITEM(sequence, 0); 
            __pyx_t_9 = PyList_GET_ITEM(sequence, 1); 
          }
          __Pyx_INCREF(__pyx_t_8);
          __Pyx_INCREF(__pyx_t_9);
          #else
          __pyx_t_8 = PySequence_ITEM(sequence, 0); if (unlikely(!__pyx_t_8)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1369; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_8);
          __pyx_t_9 = PySequence_ITEM(sequence, 1); if (unlikely(!__pyx_t_9)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1369; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_9);
          #endif
          __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
        } else {
          Py_ssize_t index = -1;
          __pyx_t_10 = PyObject_GetIter(__pyx_t_3); if (unlikely(!__pyx_t_10)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1369; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_10);
          __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
          __pyx_t_11 = Py_TYPE(__pyx_t_10)->tp_iternext;
          index = 0; __pyx_t_8 = __pyx_t_11(__pyx_t_10); if (unlikely(!__pyx_t_8)) goto __pyx_L10_unpacking_failed;
          __Pyx_GOTREF(__pyx_t_8);
          index = 1; __pyx_t_9 = __pyx_t_11(__pyx_t_10); if (unlikely(!__pyx_t_9)) goto __pyx_L10_unpacking_failed;
          __Pyx_GOTREF(__pyx_t_9);
          if (__Pyx_IternextUnpackEndCheck(__pyx_t_11(__pyx_t_10), 2) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1369; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __pyx_t_11 = NULL;
          __Pyx_DECREF(__pyx_t_10); __pyx_t_10 = 0;
          goto __pyx_L11_unpacking_done;
          __pyx_L10_unpacking_failed:;
          __Pyx_DECREF(__pyx_t_10); __pyx_t_10 = 0;
          __pyx_t_11 = NULL;
          if (__Pyx_IterFinish() == 0) __Pyx_RaiseNeedMoreValuesError(index);
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1369; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __pyx_L11_unpacking_done:;
        }
        __Pyx_XDECREF_SET(__pyx_v_key, __pyx_t_8);
        __pyx_t_8 = 0;
        __Pyx_XDECREF_SET(__pyx_v_value, __pyx_t_9);
        __pyx_t_9 = 0;

        






        __pyx_t_7 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_anchor_node(__pyx_v_self, __pyx_v_key); if (unlikely(__pyx_t_7 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1370; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

        






        __pyx_t_7 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_anchor_node(__pyx_v_self, __pyx_v_value); if (unlikely(__pyx_t_7 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1371; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
      __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;
      goto __pyx_L5;
    }
    __pyx_L5:;
  }
  __pyx_L3:;

  






  __pyx_r = 1;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_XDECREF(__pyx_t_4);
  __Pyx_XDECREF(__pyx_t_8);
  __Pyx_XDECREF(__pyx_t_9);
  __Pyx_XDECREF(__pyx_t_10);
  __Pyx_AddTraceback("_yaml.CEmitter._anchor_node", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_node_class);
  __Pyx_XDECREF(__pyx_v_item);
  __Pyx_XDECREF(__pyx_v_key);
  __Pyx_XDECREF(__pyx_v_value);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static int __pyx_f_5_yaml_8CEmitter__serialize_node(struct __pyx_obj_5_yaml_CEmitter *__pyx_v_self, PyObject *__pyx_v_node, PyObject *__pyx_v_parent, PyObject *__pyx_v_index) {
  yaml_event_t __pyx_v_event;
  int __pyx_v_implicit;
  int __pyx_v_plain_implicit;
  int __pyx_v_quoted_implicit;
  char *__pyx_v_anchor;
  char *__pyx_v_tag;
  char *__pyx_v_value;
  int __pyx_v_length;
  int __pyx_v_item_index;
  yaml_scalar_style_t __pyx_v_scalar_style;
  yaml_sequence_style_t __pyx_v_sequence_style;
  yaml_mapping_style_t __pyx_v_mapping_style;
  PyObject *__pyx_v_anchor_object = NULL;
  PyObject *__pyx_v_error = NULL;
  PyObject *__pyx_v_node_class = NULL;
  PyObject *__pyx_v_tag_object = NULL;
  PyObject *__pyx_v_value_object = NULL;
  PyObject *__pyx_v_style_object = NULL;
  PyObject *__pyx_v_item = NULL;
  PyObject *__pyx_v_item_key = NULL;
  PyObject *__pyx_v_item_value = NULL;
  int __pyx_r;
  __Pyx_RefNannyDeclarations
  PyObject *__pyx_t_1 = NULL;
  int __pyx_t_2;
  int __pyx_t_3;
  int __pyx_t_4;
  PyObject *__pyx_t_5 = NULL;
  PyObject *__pyx_t_6 = NULL;
  PyObject *__pyx_t_7 = NULL;
  int __pyx_t_8;
  Py_ssize_t __pyx_t_9;
  PyObject *(*__pyx_t_10)(PyObject *);
  PyObject *__pyx_t_11 = NULL;
  PyObject *(*__pyx_t_12)(PyObject *);
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("_serialize_node", 0);

  






  __pyx_t_1 = PyObject_GetItem(__pyx_v_self->anchors, __pyx_v_node); if (unlikely(__pyx_t_1 == NULL)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1387; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_v_anchor_object = __pyx_t_1;
  __pyx_t_1 = 0;

  






  __pyx_v_anchor = NULL;

  






  __pyx_t_2 = (__pyx_v_anchor_object != Py_None);
  __pyx_t_3 = (__pyx_t_2 != 0);
  if (__pyx_t_3) {

    






    __pyx_t_3 = (PyUnicode_CheckExact(__pyx_v_anchor_object) != 0);
    if (__pyx_t_3) {

      






      __pyx_t_1 = PyUnicode_AsUTF8String(__pyx_v_anchor_object); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1391; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __Pyx_DECREF_SET(__pyx_v_anchor_object, __pyx_t_1);
      __pyx_t_1 = 0;
      goto __pyx_L4;
    }
    __pyx_L4:;

    






    __pyx_t_3 = ((!(PyString_CheckExact(__pyx_v_anchor_object) != 0)) != 0);
    if (__pyx_t_3) {

      






      __pyx_t_3 = ((PY_MAJOR_VERSION < 3) != 0);
      if (__pyx_t_3) {

        






        __pyx_t_1 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__60, NULL); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1394; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_1);
        __Pyx_Raise(__pyx_t_1, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1394; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
       {

        






        __pyx_t_1 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__61, NULL); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1396; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_1);
        __Pyx_Raise(__pyx_t_1, 0, 0, 0);
        __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1396; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
    }

    






    __pyx_v_anchor = PyString_AS_STRING(__pyx_v_anchor_object);
    goto __pyx_L3;
  }
  __pyx_L3:;

  






  __pyx_t_3 = (__Pyx_PySequence_Contains(__pyx_v_node, __pyx_v_self->serialized_nodes, Py_EQ)); if (unlikely(__pyx_t_3 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1398; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_t_2 = (__pyx_t_3 != 0);
  if (__pyx_t_2) {

    






    __pyx_t_2 = ((yaml_alias_event_initialize((&__pyx_v_event), __pyx_v_anchor) == 0) != 0);
    if (__pyx_t_2) {

      






      PyErr_NoMemory(); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1400; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }

    






    __pyx_t_4 = yaml_emitter_emit((&__pyx_v_self->emitter), (&__pyx_v_event)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1401; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __pyx_t_2 = ((__pyx_t_4 == 0) != 0);
    if (__pyx_t_2) {

      






      __pyx_t_1 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_emitter_error(__pyx_v_self); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1402; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __pyx_v_error = __pyx_t_1;
      __pyx_t_1 = 0;

      






      __Pyx_Raise(__pyx_v_error, 0, 0, 0);
      {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1403; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
    goto __pyx_L7;
  }
   {

    






    __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_class); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1405; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __pyx_v_node_class = __pyx_t_1;
    __pyx_t_1 = 0;

    






    if (unlikely(PyObject_SetItem(__pyx_v_self->serialized_nodes, __pyx_v_node, Py_True) < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1406; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

    






    __pyx_t_1 = __Pyx_PyObject_GetAttrStr(((PyObject *)__pyx_v_self), __pyx_n_s_descend_resolver); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1407; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __pyx_t_5 = PyTuple_New(2); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1407; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __Pyx_INCREF(__pyx_v_parent);
    PyTuple_SET_ITEM(__pyx_t_5, 0, __pyx_v_parent);
    __Pyx_GIVEREF(__pyx_v_parent);
    __Pyx_INCREF(__pyx_v_index);
    PyTuple_SET_ITEM(__pyx_t_5, 1, __pyx_v_index);
    __Pyx_GIVEREF(__pyx_v_index);
    __pyx_t_6 = __Pyx_PyObject_Call(__pyx_t_1, __pyx_t_5, NULL); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1407; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_6);
    __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
    __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
    __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;

    






    __pyx_t_6 = __Pyx_GetModuleGlobalName(__pyx_n_s_ScalarNode); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1408; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_6);
    __pyx_t_2 = (__pyx_v_node_class == __pyx_t_6);
    __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
    __pyx_t_3 = (__pyx_t_2 != 0);
    if (__pyx_t_3) {

      






      __pyx_v_plain_implicit = 0;

      






      __pyx_v_quoted_implicit = 0;

      






      __pyx_t_6 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_tag); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1411; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);
      __pyx_v_tag_object = __pyx_t_6;
      __pyx_t_6 = 0;

      






      __pyx_t_6 = __Pyx_PyObject_GetAttrStr(((PyObject *)__pyx_v_self), __pyx_n_s_resolve); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1412; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);
      __pyx_t_5 = __Pyx_GetModuleGlobalName(__pyx_n_s_ScalarNode); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1412; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_value); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1412; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __pyx_t_7 = PyTuple_New(3); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1412; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_7);
      PyTuple_SET_ITEM(__pyx_t_7, 0, __pyx_t_5);
      __Pyx_GIVEREF(__pyx_t_5);
      PyTuple_SET_ITEM(__pyx_t_7, 1, __pyx_t_1);
      __Pyx_GIVEREF(__pyx_t_1);
      __Pyx_INCREF(__pyx_tuple__62);
      PyTuple_SET_ITEM(__pyx_t_7, 2, __pyx_tuple__62);
      __Pyx_GIVEREF(__pyx_tuple__62);
      __pyx_t_5 = 0;
      __pyx_t_1 = 0;
      __pyx_t_1 = __Pyx_PyObject_Call(__pyx_t_6, __pyx_t_7, NULL); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1412; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
      __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;
      __pyx_t_7 = PyObject_RichCompare(__pyx_t_1, __pyx_v_tag_object, Py_EQ); __Pyx_XGOTREF(__pyx_t_7); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1412; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
      __pyx_t_3 = __Pyx_PyObject_IsTrue(__pyx_t_7); if (unlikely(__pyx_t_3 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1412; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;
      if (__pyx_t_3) {

        






        __pyx_v_plain_implicit = 1;
        goto __pyx_L11;
      }
      __pyx_L11:;

      






      __pyx_t_7 = __Pyx_PyObject_GetAttrStr(((PyObject *)__pyx_v_self), __pyx_n_s_resolve); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1414; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_7);
      __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_ScalarNode); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1414; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __pyx_t_6 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_value); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1414; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);
      __pyx_t_5 = PyTuple_New(3); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1414; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      PyTuple_SET_ITEM(__pyx_t_5, 0, __pyx_t_1);
      __Pyx_GIVEREF(__pyx_t_1);
      PyTuple_SET_ITEM(__pyx_t_5, 1, __pyx_t_6);
      __Pyx_GIVEREF(__pyx_t_6);
      __Pyx_INCREF(__pyx_tuple__63);
      PyTuple_SET_ITEM(__pyx_t_5, 2, __pyx_tuple__63);
      __Pyx_GIVEREF(__pyx_tuple__63);
      __pyx_t_1 = 0;
      __pyx_t_6 = 0;
      __pyx_t_6 = __Pyx_PyObject_Call(__pyx_t_7, __pyx_t_5, NULL); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1414; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);
      __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      __pyx_t_5 = PyObject_RichCompare(__pyx_t_6, __pyx_v_tag_object, Py_EQ); __Pyx_XGOTREF(__pyx_t_5); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1414; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
      __pyx_t_3 = __Pyx_PyObject_IsTrue(__pyx_t_5); if (unlikely(__pyx_t_3 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1414; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      if (__pyx_t_3) {

        






        __pyx_v_quoted_implicit = 1;
        goto __pyx_L12;
      }
      __pyx_L12:;

      






      __pyx_v_tag = NULL;

      






      __pyx_t_3 = (__pyx_v_tag_object != Py_None);
      __pyx_t_2 = (__pyx_t_3 != 0);
      if (__pyx_t_2) {

        






        __pyx_t_2 = (PyUnicode_CheckExact(__pyx_v_tag_object) != 0);
        if (__pyx_t_2) {

          






          __pyx_t_5 = PyUnicode_AsUTF8String(__pyx_v_tag_object); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1419; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_5);
          __Pyx_DECREF_SET(__pyx_v_tag_object, __pyx_t_5);
          __pyx_t_5 = 0;
          goto __pyx_L14;
        }
        __pyx_L14:;

        






        __pyx_t_2 = ((!(PyString_CheckExact(__pyx_v_tag_object) != 0)) != 0);
        if (__pyx_t_2) {

          






          __pyx_t_2 = ((PY_MAJOR_VERSION < 3) != 0);
          if (__pyx_t_2) {

            






            __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__64, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1422; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
            __Pyx_GOTREF(__pyx_t_5);
            __Pyx_Raise(__pyx_t_5, 0, 0, 0);
            __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
            {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1422; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          }
           {

            






            __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__65, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1424; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
            __Pyx_GOTREF(__pyx_t_5);
            __Pyx_Raise(__pyx_t_5, 0, 0, 0);
            __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
            {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1424; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          }
        }

        






        __pyx_v_tag = PyString_AS_STRING(__pyx_v_tag_object);
        goto __pyx_L13;
      }
      __pyx_L13:;

      






      __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_value); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1426; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __pyx_v_value_object = __pyx_t_5;
      __pyx_t_5 = 0;

      






      __pyx_t_2 = (PyUnicode_CheckExact(__pyx_v_value_object) != 0);
      if (__pyx_t_2) {

        






        __pyx_t_5 = PyUnicode_AsUTF8String(__pyx_v_value_object); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1428; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __Pyx_DECREF_SET(__pyx_v_value_object, __pyx_t_5);
        __pyx_t_5 = 0;
        goto __pyx_L17;
      }
      __pyx_L17:;

      






      __pyx_t_2 = ((!(PyString_CheckExact(__pyx_v_value_object) != 0)) != 0);
      if (__pyx_t_2) {

        






        __pyx_t_2 = ((PY_MAJOR_VERSION < 3) != 0);
        if (__pyx_t_2) {

          






          __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__66, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1431; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_5);
          __Pyx_Raise(__pyx_t_5, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1431; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
         {

          






          __pyx_t_5 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__67, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1433; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_5);
          __Pyx_Raise(__pyx_t_5, 0, 0, 0);
          __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1433; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        }
      }

      






      __pyx_v_value = PyString_AS_STRING(__pyx_v_value_object);

      






      __pyx_v_length = PyString_GET_SIZE(__pyx_v_value_object);

      






      __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_style); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1436; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __pyx_v_style_object = __pyx_t_5;
      __pyx_t_5 = 0;

      






      __pyx_v_scalar_style = YAML_PLAIN_SCALAR_STYLE;

      






      __pyx_t_2 = (__Pyx_PyString_Equals(__pyx_v_style_object, __pyx_kp_s__7, Py_EQ)); if (unlikely(__pyx_t_2 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1438; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      if (!__pyx_t_2) {
        __pyx_t_3 = (__Pyx_PyUnicode_Equals(__pyx_v_style_object, __pyx_kp_u__7, Py_EQ)); if (unlikely(__pyx_t_3 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1438; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __pyx_t_8 = __pyx_t_3;
      } else {
        __pyx_t_8 = __pyx_t_2;
      }
      if (__pyx_t_8) {

        






        __pyx_v_scalar_style = YAML_SINGLE_QUOTED_SCALAR_STYLE;
        goto __pyx_L20;
      }

      






      __pyx_t_8 = (__Pyx_PyString_Equals(__pyx_v_style_object, __pyx_kp_s__8, Py_EQ)); if (unlikely(__pyx_t_8 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1440; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      if (!__pyx_t_8) {
        __pyx_t_2 = (__Pyx_PyUnicode_Equals(__pyx_v_style_object, __pyx_kp_u__8, Py_EQ)); if (unlikely(__pyx_t_2 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1440; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __pyx_t_3 = __pyx_t_2;
      } else {
        __pyx_t_3 = __pyx_t_8;
      }
      if (__pyx_t_3) {

        






        __pyx_v_scalar_style = YAML_DOUBLE_QUOTED_SCALAR_STYLE;
        goto __pyx_L20;
      }

      






      __pyx_t_3 = (__Pyx_PyString_Equals(__pyx_v_style_object, __pyx_kp_s__9, Py_EQ)); if (unlikely(__pyx_t_3 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1442; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      if (!__pyx_t_3) {
        __pyx_t_8 = (__Pyx_PyUnicode_Equals(__pyx_v_style_object, __pyx_kp_u__9, Py_EQ)); if (unlikely(__pyx_t_8 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1442; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __pyx_t_2 = __pyx_t_8;
      } else {
        __pyx_t_2 = __pyx_t_3;
      }
      if (__pyx_t_2) {

        






        __pyx_v_scalar_style = YAML_LITERAL_SCALAR_STYLE;
        goto __pyx_L20;
      }

      






      __pyx_t_2 = (__Pyx_PyString_Equals(__pyx_v_style_object, __pyx_kp_s__10, Py_EQ)); if (unlikely(__pyx_t_2 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1444; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      if (!__pyx_t_2) {
        __pyx_t_3 = (__Pyx_PyUnicode_Equals(__pyx_v_style_object, __pyx_kp_u__10, Py_EQ)); if (unlikely(__pyx_t_3 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1444; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __pyx_t_8 = __pyx_t_3;
      } else {
        __pyx_t_8 = __pyx_t_2;
      }
      if (__pyx_t_8) {

        






        __pyx_v_scalar_style = YAML_FOLDED_SCALAR_STYLE;
        goto __pyx_L20;
      }
      __pyx_L20:;

      






      __pyx_t_8 = ((yaml_scalar_event_initialize((&__pyx_v_event), __pyx_v_anchor, __pyx_v_tag, __pyx_v_value, __pyx_v_length, __pyx_v_plain_implicit, __pyx_v_quoted_implicit, __pyx_v_scalar_style) == 0) != 0);
      if (__pyx_t_8) {

        






        PyErr_NoMemory(); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1448; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }

      






      __pyx_t_4 = yaml_emitter_emit((&__pyx_v_self->emitter), (&__pyx_v_event)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1449; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __pyx_t_8 = ((__pyx_t_4 == 0) != 0);
      if (__pyx_t_8) {

        






        __pyx_t_5 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_emitter_error(__pyx_v_self); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1450; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __pyx_v_error = __pyx_t_5;
        __pyx_t_5 = 0;

        






        __Pyx_Raise(__pyx_v_error, 0, 0, 0);
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1451; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
      goto __pyx_L10;
    }

    






    __pyx_t_5 = __Pyx_GetModuleGlobalName(__pyx_n_s_SequenceNode); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1452; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_t_8 = (__pyx_v_node_class == __pyx_t_5);
    __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
    __pyx_t_2 = (__pyx_t_8 != 0);
    if (__pyx_t_2) {

      






      __pyx_v_implicit = 0;

      






      __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_tag); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1454; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __pyx_v_tag_object = __pyx_t_5;
      __pyx_t_5 = 0;

      






      __pyx_t_5 = __Pyx_PyObject_GetAttrStr(((PyObject *)__pyx_v_self), __pyx_n_s_resolve); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1455; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __pyx_t_6 = __Pyx_GetModuleGlobalName(__pyx_n_s_SequenceNode); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1455; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);
      __pyx_t_7 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_value); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1455; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_7);
      __pyx_t_1 = PyTuple_New(3); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1455; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      PyTuple_SET_ITEM(__pyx_t_1, 0, __pyx_t_6);
      __Pyx_GIVEREF(__pyx_t_6);
      PyTuple_SET_ITEM(__pyx_t_1, 1, __pyx_t_7);
      __Pyx_GIVEREF(__pyx_t_7);
      __Pyx_INCREF(Py_True);
      PyTuple_SET_ITEM(__pyx_t_1, 2, Py_True);
      __Pyx_GIVEREF(Py_True);
      __pyx_t_6 = 0;
      __pyx_t_7 = 0;
      __pyx_t_7 = __Pyx_PyObject_Call(__pyx_t_5, __pyx_t_1, NULL); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1455; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_7);
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
      __pyx_t_1 = PyObject_RichCompare(__pyx_t_7, __pyx_v_tag_object, Py_EQ); __Pyx_XGOTREF(__pyx_t_1); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1455; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;
      __pyx_t_2 = __Pyx_PyObject_IsTrue(__pyx_t_1); if (unlikely(__pyx_t_2 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1455; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
      if (__pyx_t_2) {

        






        __pyx_v_implicit = 1;
        goto __pyx_L23;
      }
      __pyx_L23:;

      






      __pyx_v_tag = NULL;

      






      __pyx_t_2 = (__pyx_v_tag_object != Py_None);
      __pyx_t_8 = (__pyx_t_2 != 0);
      if (__pyx_t_8) {

        






        __pyx_t_8 = (PyUnicode_CheckExact(__pyx_v_tag_object) != 0);
        if (__pyx_t_8) {

          






          __pyx_t_1 = PyUnicode_AsUTF8String(__pyx_v_tag_object); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1460; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_1);
          __Pyx_DECREF_SET(__pyx_v_tag_object, __pyx_t_1);
          __pyx_t_1 = 0;
          goto __pyx_L25;
        }
        __pyx_L25:;

        






        __pyx_t_8 = ((!(PyString_CheckExact(__pyx_v_tag_object) != 0)) != 0);
        if (__pyx_t_8) {

          






          __pyx_t_8 = ((PY_MAJOR_VERSION < 3) != 0);
          if (__pyx_t_8) {

            






            __pyx_t_1 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__68, NULL); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1463; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
            __Pyx_GOTREF(__pyx_t_1);
            __Pyx_Raise(__pyx_t_1, 0, 0, 0);
            __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
            {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1463; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          }
           {

            






            __pyx_t_1 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__69, NULL); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1465; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
            __Pyx_GOTREF(__pyx_t_1);
            __Pyx_Raise(__pyx_t_1, 0, 0, 0);
            __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
            {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1465; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          }
        }

        






        __pyx_v_tag = PyString_AS_STRING(__pyx_v_tag_object);
        goto __pyx_L24;
      }
      __pyx_L24:;

      






      __pyx_v_sequence_style = YAML_BLOCK_SEQUENCE_STYLE;

      






      __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_flow_style); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1468; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __pyx_t_8 = __Pyx_PyObject_IsTrue(__pyx_t_1); if (unlikely(__pyx_t_8 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1468; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
      if (__pyx_t_8) {

        






        __pyx_v_sequence_style = YAML_FLOW_SEQUENCE_STYLE;
        goto __pyx_L28;
      }
      __pyx_L28:;

      






      __pyx_t_8 = ((yaml_sequence_start_event_initialize((&__pyx_v_event), __pyx_v_anchor, __pyx_v_tag, __pyx_v_implicit, __pyx_v_sequence_style) == 0) != 0);
      if (__pyx_t_8) {

        






        PyErr_NoMemory(); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1472; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }

      






      __pyx_t_4 = yaml_emitter_emit((&__pyx_v_self->emitter), (&__pyx_v_event)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1473; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __pyx_t_8 = ((__pyx_t_4 == 0) != 0);
      if (__pyx_t_8) {

        






        __pyx_t_1 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_emitter_error(__pyx_v_self); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1474; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_1);
        __pyx_v_error = __pyx_t_1;
        __pyx_t_1 = 0;

        






        __Pyx_Raise(__pyx_v_error, 0, 0, 0);
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1475; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }

      






      __pyx_v_item_index = 0;

      






      __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_value); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1477; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      if (PyList_CheckExact(__pyx_t_1) || PyTuple_CheckExact(__pyx_t_1)) {
        __pyx_t_7 = __pyx_t_1; __Pyx_INCREF(__pyx_t_7); __pyx_t_9 = 0;
        __pyx_t_10 = NULL;
      } else {
        __pyx_t_9 = -1; __pyx_t_7 = PyObject_GetIter(__pyx_t_1); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1477; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_7);
        __pyx_t_10 = Py_TYPE(__pyx_t_7)->tp_iternext;
      }
      __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
      for (;;) {
        if (!__pyx_t_10 && PyList_CheckExact(__pyx_t_7)) {
          if (__pyx_t_9 >= PyList_GET_SIZE(__pyx_t_7)) break;
          #if CYTHON_COMPILING_IN_CPYTHON
          __pyx_t_1 = PyList_GET_ITEM(__pyx_t_7, __pyx_t_9); __Pyx_INCREF(__pyx_t_1); __pyx_t_9++; if (unlikely(0 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1477; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #else
          __pyx_t_1 = PySequence_ITEM(__pyx_t_7, __pyx_t_9); __pyx_t_9++; if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1477; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #endif
        } else if (!__pyx_t_10 && PyTuple_CheckExact(__pyx_t_7)) {
          if (__pyx_t_9 >= PyTuple_GET_SIZE(__pyx_t_7)) break;
          #if CYTHON_COMPILING_IN_CPYTHON
          __pyx_t_1 = PyTuple_GET_ITEM(__pyx_t_7, __pyx_t_9); __Pyx_INCREF(__pyx_t_1); __pyx_t_9++; if (unlikely(0 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1477; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #else
          __pyx_t_1 = PySequence_ITEM(__pyx_t_7, __pyx_t_9); __pyx_t_9++; if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1477; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #endif
        } else {
          __pyx_t_1 = __pyx_t_10(__pyx_t_7);
          if (unlikely(!__pyx_t_1)) {
            PyObject* exc_type = PyErr_Occurred();
            if (exc_type) {
              if (likely(exc_type == PyExc_StopIteration || PyErr_GivenExceptionMatches(exc_type, PyExc_StopIteration))) PyErr_Clear();
              else {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1477; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
            }
            break;
          }
          __Pyx_GOTREF(__pyx_t_1);
        }
        __Pyx_XDECREF_SET(__pyx_v_item, __pyx_t_1);
        __pyx_t_1 = 0;

        






        __pyx_t_1 = __Pyx_PyInt_From_int(__pyx_v_item_index); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1478; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_1);
        __pyx_t_4 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_serialize_node(__pyx_v_self, __pyx_v_item, __pyx_v_node, __pyx_t_1); if (unlikely(__pyx_t_4 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1478; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

        






        __pyx_v_item_index = (__pyx_v_item_index + 1);
      }
      __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;

      






      yaml_sequence_end_event_initialize((&__pyx_v_event));

      






      __pyx_t_4 = yaml_emitter_emit((&__pyx_v_self->emitter), (&__pyx_v_event)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1481; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __pyx_t_8 = ((__pyx_t_4 == 0) != 0);
      if (__pyx_t_8) {

        






        __pyx_t_7 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_emitter_error(__pyx_v_self); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1482; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_7);
        __pyx_v_error = __pyx_t_7;
        __pyx_t_7 = 0;

        






        __Pyx_Raise(__pyx_v_error, 0, 0, 0);
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1483; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
      goto __pyx_L10;
    }

    






    __pyx_t_7 = __Pyx_GetModuleGlobalName(__pyx_n_s_MappingNode); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1484; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_7);
    __pyx_t_8 = (__pyx_v_node_class == __pyx_t_7);
    __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;
    __pyx_t_2 = (__pyx_t_8 != 0);
    if (__pyx_t_2) {

      






      __pyx_v_implicit = 0;

      






      __pyx_t_7 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_tag); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1486; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_7);
      __pyx_v_tag_object = __pyx_t_7;
      __pyx_t_7 = 0;

      






      __pyx_t_7 = __Pyx_PyObject_GetAttrStr(((PyObject *)__pyx_v_self), __pyx_n_s_resolve); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1487; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_7);
      __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_MappingNode); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1487; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_1);
      __pyx_t_5 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_value); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1487; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __pyx_t_6 = PyTuple_New(3); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1487; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);
      PyTuple_SET_ITEM(__pyx_t_6, 0, __pyx_t_1);
      __Pyx_GIVEREF(__pyx_t_1);
      PyTuple_SET_ITEM(__pyx_t_6, 1, __pyx_t_5);
      __Pyx_GIVEREF(__pyx_t_5);
      __Pyx_INCREF(Py_True);
      PyTuple_SET_ITEM(__pyx_t_6, 2, Py_True);
      __Pyx_GIVEREF(Py_True);
      __pyx_t_1 = 0;
      __pyx_t_5 = 0;
      __pyx_t_5 = __Pyx_PyObject_Call(__pyx_t_7, __pyx_t_6, NULL); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1487; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_5);
      __Pyx_DECREF(__pyx_t_7); __pyx_t_7 = 0;
      __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
      __pyx_t_6 = PyObject_RichCompare(__pyx_t_5, __pyx_v_tag_object, Py_EQ); __Pyx_XGOTREF(__pyx_t_6); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1487; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
      __pyx_t_2 = __Pyx_PyObject_IsTrue(__pyx_t_6); if (unlikely(__pyx_t_2 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1487; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
      if (__pyx_t_2) {

        






        __pyx_v_implicit = 1;
        goto __pyx_L34;
      }
      __pyx_L34:;

      






      __pyx_v_tag = NULL;

      






      __pyx_t_2 = (__pyx_v_tag_object != Py_None);
      __pyx_t_8 = (__pyx_t_2 != 0);
      if (__pyx_t_8) {

        






        __pyx_t_8 = (PyUnicode_CheckExact(__pyx_v_tag_object) != 0);
        if (__pyx_t_8) {

          






          __pyx_t_6 = PyUnicode_AsUTF8String(__pyx_v_tag_object); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1492; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_6);
          __Pyx_DECREF_SET(__pyx_v_tag_object, __pyx_t_6);
          __pyx_t_6 = 0;
          goto __pyx_L36;
        }
        __pyx_L36:;

        






        __pyx_t_8 = ((!(PyString_CheckExact(__pyx_v_tag_object) != 0)) != 0);
        if (__pyx_t_8) {

          






          __pyx_t_8 = ((PY_MAJOR_VERSION < 3) != 0);
          if (__pyx_t_8) {

            






            __pyx_t_6 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__70, NULL); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1495; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
            __Pyx_GOTREF(__pyx_t_6);
            __Pyx_Raise(__pyx_t_6, 0, 0, 0);
            __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
            {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1495; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          }
           {

            






            __pyx_t_6 = __Pyx_PyObject_Call(__pyx_builtin_TypeError, __pyx_tuple__71, NULL); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1497; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
            __Pyx_GOTREF(__pyx_t_6);
            __Pyx_Raise(__pyx_t_6, 0, 0, 0);
            __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
            {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1497; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          }
        }

        






        __pyx_v_tag = PyString_AS_STRING(__pyx_v_tag_object);
        goto __pyx_L35;
      }
      __pyx_L35:;

      






      __pyx_v_mapping_style = YAML_BLOCK_MAPPING_STYLE;

      






      __pyx_t_6 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_flow_style); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1500; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);
      __pyx_t_8 = __Pyx_PyObject_IsTrue(__pyx_t_6); if (unlikely(__pyx_t_8 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1500; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
      if (__pyx_t_8) {

        






        __pyx_v_mapping_style = YAML_FLOW_MAPPING_STYLE;
        goto __pyx_L39;
      }
      __pyx_L39:;

      






      __pyx_t_8 = ((yaml_mapping_start_event_initialize((&__pyx_v_event), __pyx_v_anchor, __pyx_v_tag, __pyx_v_implicit, __pyx_v_mapping_style) == 0) != 0);
      if (__pyx_t_8) {

        






        PyErr_NoMemory(); {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1504; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }

      






      __pyx_t_4 = yaml_emitter_emit((&__pyx_v_self->emitter), (&__pyx_v_event)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1505; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __pyx_t_8 = ((__pyx_t_4 == 0) != 0);
      if (__pyx_t_8) {

        






        __pyx_t_6 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_emitter_error(__pyx_v_self); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1506; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_6);
        __pyx_v_error = __pyx_t_6;
        __pyx_t_6 = 0;

        






        __Pyx_Raise(__pyx_v_error, 0, 0, 0);
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1507; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }

      






      __pyx_t_6 = __Pyx_PyObject_GetAttrStr(__pyx_v_node, __pyx_n_s_value); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1508; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __Pyx_GOTREF(__pyx_t_6);
      if (PyList_CheckExact(__pyx_t_6) || PyTuple_CheckExact(__pyx_t_6)) {
        __pyx_t_5 = __pyx_t_6; __Pyx_INCREF(__pyx_t_5); __pyx_t_9 = 0;
        __pyx_t_10 = NULL;
      } else {
        __pyx_t_9 = -1; __pyx_t_5 = PyObject_GetIter(__pyx_t_6); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1508; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __pyx_t_10 = Py_TYPE(__pyx_t_5)->tp_iternext;
      }
      __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
      for (;;) {
        if (!__pyx_t_10 && PyList_CheckExact(__pyx_t_5)) {
          if (__pyx_t_9 >= PyList_GET_SIZE(__pyx_t_5)) break;
          #if CYTHON_COMPILING_IN_CPYTHON
          __pyx_t_6 = PyList_GET_ITEM(__pyx_t_5, __pyx_t_9); __Pyx_INCREF(__pyx_t_6); __pyx_t_9++; if (unlikely(0 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1508; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #else
          __pyx_t_6 = PySequence_ITEM(__pyx_t_5, __pyx_t_9); __pyx_t_9++; if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1508; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #endif
        } else if (!__pyx_t_10 && PyTuple_CheckExact(__pyx_t_5)) {
          if (__pyx_t_9 >= PyTuple_GET_SIZE(__pyx_t_5)) break;
          #if CYTHON_COMPILING_IN_CPYTHON
          __pyx_t_6 = PyTuple_GET_ITEM(__pyx_t_5, __pyx_t_9); __Pyx_INCREF(__pyx_t_6); __pyx_t_9++; if (unlikely(0 < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1508; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #else
          __pyx_t_6 = PySequence_ITEM(__pyx_t_5, __pyx_t_9); __pyx_t_9++; if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1508; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          #endif
        } else {
          __pyx_t_6 = __pyx_t_10(__pyx_t_5);
          if (unlikely(!__pyx_t_6)) {
            PyObject* exc_type = PyErr_Occurred();
            if (exc_type) {
              if (likely(exc_type == PyExc_StopIteration || PyErr_GivenExceptionMatches(exc_type, PyExc_StopIteration))) PyErr_Clear();
              else {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1508; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
            }
            break;
          }
          __Pyx_GOTREF(__pyx_t_6);
        }
        if ((likely(PyTuple_CheckExact(__pyx_t_6))) || (PyList_CheckExact(__pyx_t_6))) {
          PyObject* sequence = __pyx_t_6;
          #if CYTHON_COMPILING_IN_CPYTHON
          Py_ssize_t size = Py_SIZE(sequence);
          #else
          Py_ssize_t size = PySequence_Size(sequence);
          #endif
          if (unlikely(size != 2)) {
            if (size > 2) __Pyx_RaiseTooManyValuesError(2);
            else if (size >= 0) __Pyx_RaiseNeedMoreValuesError(size);
            {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1508; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          }
          #if CYTHON_COMPILING_IN_CPYTHON
          if (likely(PyTuple_CheckExact(sequence))) {
            __pyx_t_7 = PyTuple_GET_ITEM(sequence, 0); 
            __pyx_t_1 = PyTuple_GET_ITEM(sequence, 1); 
          } else {
            __pyx_t_7 = PyList_GET_ITEM(sequence, 0); 
            __pyx_t_1 = PyList_GET_ITEM(sequence, 1); 
          }
          __Pyx_INCREF(__pyx_t_7);
          __Pyx_INCREF(__pyx_t_1);
          #else
          __pyx_t_7 = PySequence_ITEM(sequence, 0); if (unlikely(!__pyx_t_7)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1508; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_7);
          __pyx_t_1 = PySequence_ITEM(sequence, 1); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1508; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_1);
          #endif
          __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
        } else {
          Py_ssize_t index = -1;
          __pyx_t_11 = PyObject_GetIter(__pyx_t_6); if (unlikely(!__pyx_t_11)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1508; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __Pyx_GOTREF(__pyx_t_11);
          __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
          __pyx_t_12 = Py_TYPE(__pyx_t_11)->tp_iternext;
          index = 0; __pyx_t_7 = __pyx_t_12(__pyx_t_11); if (unlikely(!__pyx_t_7)) goto __pyx_L44_unpacking_failed;
          __Pyx_GOTREF(__pyx_t_7);
          index = 1; __pyx_t_1 = __pyx_t_12(__pyx_t_11); if (unlikely(!__pyx_t_1)) goto __pyx_L44_unpacking_failed;
          __Pyx_GOTREF(__pyx_t_1);
          if (__Pyx_IternextUnpackEndCheck(__pyx_t_12(__pyx_t_11), 2) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1508; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __pyx_t_12 = NULL;
          __Pyx_DECREF(__pyx_t_11); __pyx_t_11 = 0;
          goto __pyx_L45_unpacking_done;
          __pyx_L44_unpacking_failed:;
          __Pyx_DECREF(__pyx_t_11); __pyx_t_11 = 0;
          __pyx_t_12 = NULL;
          if (__Pyx_IterFinish() == 0) __Pyx_RaiseNeedMoreValuesError(index);
          {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1508; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
          __pyx_L45_unpacking_done:;
        }
        __Pyx_XDECREF_SET(__pyx_v_item_key, __pyx_t_7);
        __pyx_t_7 = 0;
        __Pyx_XDECREF_SET(__pyx_v_item_value, __pyx_t_1);
        __pyx_t_1 = 0;

        






        __pyx_t_4 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_serialize_node(__pyx_v_self, __pyx_v_item_key, __pyx_v_node, Py_None); if (unlikely(__pyx_t_4 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1509; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

        






        __pyx_t_4 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_serialize_node(__pyx_v_self, __pyx_v_item_value, __pyx_v_node, __pyx_v_item_key); if (unlikely(__pyx_t_4 == 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1510; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
      __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;

      






      yaml_mapping_end_event_initialize((&__pyx_v_event));

      






      __pyx_t_4 = yaml_emitter_emit((&__pyx_v_self->emitter), (&__pyx_v_event)); if (unlikely(PyErr_Occurred())) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1512; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      __pyx_t_8 = ((__pyx_t_4 == 0) != 0);
      if (__pyx_t_8) {

        






        __pyx_t_5 = ((struct __pyx_vtabstruct_5_yaml_CEmitter *)__pyx_v_self->__pyx_vtab)->_emitter_error(__pyx_v_self); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1513; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
        __Pyx_GOTREF(__pyx_t_5);
        __pyx_v_error = __pyx_t_5;
        __pyx_t_5 = 0;

        






        __Pyx_Raise(__pyx_v_error, 0, 0, 0);
        {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1514; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
      }
      goto __pyx_L10;
    }
    __pyx_L10:;

    






    __pyx_t_5 = __Pyx_PyObject_GetAttrStr(((PyObject *)__pyx_v_self), __pyx_n_s_ascend_resolver); if (unlikely(!__pyx_t_5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1515; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_5);
    __pyx_t_6 = __Pyx_PyObject_Call(__pyx_t_5, __pyx_empty_tuple, NULL); if (unlikely(!__pyx_t_6)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1515; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_6);
    __Pyx_DECREF(__pyx_t_5); __pyx_t_5 = 0;
    __Pyx_DECREF(__pyx_t_6); __pyx_t_6 = 0;
  }
  __pyx_L7:;

  






  __pyx_r = 1;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_XDECREF(__pyx_t_5);
  __Pyx_XDECREF(__pyx_t_6);
  __Pyx_XDECREF(__pyx_t_7);
  __Pyx_XDECREF(__pyx_t_11);
  __Pyx_AddTraceback("_yaml.CEmitter._serialize_node", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF(__pyx_v_anchor_object);
  __Pyx_XDECREF(__pyx_v_error);
  __Pyx_XDECREF(__pyx_v_node_class);
  __Pyx_XDECREF(__pyx_v_tag_object);
  __Pyx_XDECREF(__pyx_v_value_object);
  __Pyx_XDECREF(__pyx_v_style_object);
  __Pyx_XDECREF(__pyx_v_item);
  __Pyx_XDECREF(__pyx_v_item_key);
  __Pyx_XDECREF(__pyx_v_item_value);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}









static int __pyx_f_5_yaml_output_handler(void *__pyx_v_data, char *__pyx_v_buffer, int __pyx_v_size) {
  struct __pyx_obj_5_yaml_CEmitter *__pyx_v_emitter = 0;
  PyObject *__pyx_v_value = NULL;
  int __pyx_r;
  __Pyx_RefNannyDeclarations
  PyObject *__pyx_t_1 = NULL;
  int __pyx_t_2;
  PyObject *__pyx_t_3 = NULL;
  PyObject *__pyx_t_4 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannySetupContext("output_handler", 0);

  






  __pyx_t_1 = ((PyObject *)__pyx_v_data);
  __Pyx_INCREF(__pyx_t_1);
  __pyx_v_emitter = ((struct __pyx_obj_5_yaml_CEmitter *)__pyx_t_1);
  __pyx_t_1 = 0;

  






  __pyx_t_2 = ((__pyx_v_emitter->dump_unicode == 0) != 0);
  if (__pyx_t_2) {

    






    __pyx_t_1 = PyString_FromStringAndSize(__pyx_v_buffer, __pyx_v_size); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1522; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __pyx_v_value = __pyx_t_1;
    __pyx_t_1 = 0;
    goto __pyx_L3;
  }
   {

    






    __pyx_t_1 = PyUnicode_DecodeUTF8(__pyx_v_buffer, __pyx_v_size, __pyx_k_strict); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1524; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    __Pyx_GOTREF(__pyx_t_1);
    __pyx_v_value = __pyx_t_1;
    __pyx_t_1 = 0;
  }
  __pyx_L3:;

  






  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_v_emitter->stream, __pyx_n_s_write); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1525; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_3 = PyTuple_New(1); if (unlikely(!__pyx_t_3)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1525; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_3);
  __Pyx_INCREF(__pyx_v_value);
  PyTuple_SET_ITEM(__pyx_t_3, 0, __pyx_v_value);
  __Pyx_GIVEREF(__pyx_v_value);
  __pyx_t_4 = __Pyx_PyObject_Call(__pyx_t_1, __pyx_t_3, NULL); if (unlikely(!__pyx_t_4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1525; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_4);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __Pyx_DECREF(__pyx_t_3); __pyx_t_3 = 0;
  __Pyx_DECREF(__pyx_t_4); __pyx_t_4 = 0;

  





  __pyx_r = 1;
  goto __pyx_L0;

  







  
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_XDECREF(__pyx_t_3);
  __Pyx_XDECREF(__pyx_t_4);
  __Pyx_AddTraceback("_yaml.output_handler", __pyx_clineno, __pyx_lineno, __pyx_filename);
  __pyx_r = 0;
  __pyx_L0:;
  __Pyx_XDECREF((PyObject *)__pyx_v_emitter);
  __Pyx_XDECREF(__pyx_v_value);
  __Pyx_RefNannyFinishContext();
  return __pyx_r;
}

static PyObject *__pyx_tp_new_5_yaml_Mark(PyTypeObject *t, CYTHON_UNUSED PyObject *a, CYTHON_UNUSED PyObject *k) {
  struct __pyx_obj_5_yaml_Mark *p;
  PyObject *o;
  if (likely((t->tp_flags & Py_TPFLAGS_IS_ABSTRACT) == 0)) {
    o = (*t->tp_alloc)(t, 0);
  } else {
    o = (PyObject *) PyBaseObject_Type.tp_new(t, __pyx_empty_tuple, 0);
  }
  if (unlikely(!o)) return 0;
  p = ((struct __pyx_obj_5_yaml_Mark *)o);
  p->name = Py_None; Py_INCREF(Py_None);
  p->buffer = Py_None; Py_INCREF(Py_None);
  p->pointer = Py_None; Py_INCREF(Py_None);
  return o;
}

static void __pyx_tp_dealloc_5_yaml_Mark(PyObject *o) {
  struct __pyx_obj_5_yaml_Mark *p = (struct __pyx_obj_5_yaml_Mark *)o;
  #if PY_VERSION_HEX >= 0x030400a1
  if (unlikely(Py_TYPE(o)->tp_finalize) && !_PyGC_FINALIZED(o)) {
    if (PyObject_CallFinalizerFromDealloc(o)) return;
  }
  #endif
  PyObject_GC_UnTrack(o);
  Py_CLEAR(p->name);
  Py_CLEAR(p->buffer);
  Py_CLEAR(p->pointer);
  (*Py_TYPE(o)->tp_free)(o);
}

static int __pyx_tp_traverse_5_yaml_Mark(PyObject *o, visitproc v, void *a) {
  int e;
  struct __pyx_obj_5_yaml_Mark *p = (struct __pyx_obj_5_yaml_Mark *)o;
  if (p->name) {
    e = (*v)(p->name, a); if (e) return e;
  }
  if (p->buffer) {
    e = (*v)(p->buffer, a); if (e) return e;
  }
  if (p->pointer) {
    e = (*v)(p->pointer, a); if (e) return e;
  }
  return 0;
}

static int __pyx_tp_clear_5_yaml_Mark(PyObject *o) {
  PyObject* tmp;
  struct __pyx_obj_5_yaml_Mark *p = (struct __pyx_obj_5_yaml_Mark *)o;
  tmp = ((PyObject*)p->name);
  p->name = Py_None; Py_INCREF(Py_None);
  Py_XDECREF(tmp);
  tmp = ((PyObject*)p->buffer);
  p->buffer = Py_None; Py_INCREF(Py_None);
  Py_XDECREF(tmp);
  tmp = ((PyObject*)p->pointer);
  p->pointer = Py_None; Py_INCREF(Py_None);
  Py_XDECREF(tmp);
  return 0;
}

static PyObject *__pyx_getprop_5_yaml_4Mark_name(PyObject *o, CYTHON_UNUSED void *x) {
  return __pyx_pw_5_yaml_4Mark_4name_1__get__(o);
}

static PyObject *__pyx_getprop_5_yaml_4Mark_index(PyObject *o, CYTHON_UNUSED void *x) {
  return __pyx_pw_5_yaml_4Mark_5index_1__get__(o);
}

static PyObject *__pyx_getprop_5_yaml_4Mark_line(PyObject *o, CYTHON_UNUSED void *x) {
  return __pyx_pw_5_yaml_4Mark_4line_1__get__(o);
}

static PyObject *__pyx_getprop_5_yaml_4Mark_column(PyObject *o, CYTHON_UNUSED void *x) {
  return __pyx_pw_5_yaml_4Mark_6column_1__get__(o);
}

static PyObject *__pyx_getprop_5_yaml_4Mark_buffer(PyObject *o, CYTHON_UNUSED void *x) {
  return __pyx_pw_5_yaml_4Mark_6buffer_1__get__(o);
}

static PyObject *__pyx_getprop_5_yaml_4Mark_pointer(PyObject *o, CYTHON_UNUSED void *x) {
  return __pyx_pw_5_yaml_4Mark_7pointer_1__get__(o);
}

static PyMethodDef __pyx_methods_5_yaml_Mark[] = {
  {__Pyx_NAMESTR("get_snippet"), (PyCFunction)__pyx_pw_5_yaml_4Mark_3get_snippet, METH_NOARGS, __Pyx_DOCSTR(0)},
  {0, 0, 0, 0}
};

static struct PyGetSetDef __pyx_getsets_5_yaml_Mark[] = {
  {(char *)"name", __pyx_getprop_5_yaml_4Mark_name, 0, 0, 0},
  {(char *)"index", __pyx_getprop_5_yaml_4Mark_index, 0, 0, 0},
  {(char *)"line", __pyx_getprop_5_yaml_4Mark_line, 0, 0, 0},
  {(char *)"column", __pyx_getprop_5_yaml_4Mark_column, 0, 0, 0},
  {(char *)"buffer", __pyx_getprop_5_yaml_4Mark_buffer, 0, 0, 0},
  {(char *)"pointer", __pyx_getprop_5_yaml_4Mark_pointer, 0, 0, 0},
  {0, 0, 0, 0, 0}
};

static PyTypeObject __pyx_type_5_yaml_Mark = {
  PyVarObject_HEAD_INIT(0, 0)
  __Pyx_NAMESTR("_yaml.Mark"), 
  sizeof(struct __pyx_obj_5_yaml_Mark), 
  0, 
  __pyx_tp_dealloc_5_yaml_Mark, 
  0, 
  0, 
  0, 
  #if PY_MAJOR_VERSION < 3
  0, 
  #else
  0, 
  #endif
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  __pyx_pw_5_yaml_4Mark_5__str__, 
  0, 
  0, 
  0, 
  Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_VERSION_TAG|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_NEWBUFFER|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_HAVE_GC, 
  0, 
  __pyx_tp_traverse_5_yaml_Mark, 
  __pyx_tp_clear_5_yaml_Mark, 
  0, 
  0, 
  0, 
  0, 
  __pyx_methods_5_yaml_Mark, 
  0, 
  __pyx_getsets_5_yaml_Mark, 
  0, 
  0, 
  0, 
  0, 
  0, 
  __pyx_pw_5_yaml_4Mark_1__init__, 
  0, 
  __pyx_tp_new_5_yaml_Mark, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  #if PY_VERSION_HEX >= 0x02060000
  0, 
  #endif
  #if PY_VERSION_HEX >= 0x030400a1
  0, 
  #endif
};
static struct __pyx_vtabstruct_5_yaml_CParser __pyx_vtable_5_yaml_CParser;

static PyObject *__pyx_tp_new_5_yaml_CParser(PyTypeObject *t, CYTHON_UNUSED PyObject *a, CYTHON_UNUSED PyObject *k) {
  struct __pyx_obj_5_yaml_CParser *p;
  PyObject *o;
  if (likely((t->tp_flags & Py_TPFLAGS_IS_ABSTRACT) == 0)) {
    o = (*t->tp_alloc)(t, 0);
  } else {
    o = (PyObject *) PyBaseObject_Type.tp_new(t, __pyx_empty_tuple, 0);
  }
  if (unlikely(!o)) return 0;
  p = ((struct __pyx_obj_5_yaml_CParser *)o);
  p->__pyx_vtab = __pyx_vtabptr_5_yaml_CParser;
  p->stream = Py_None; Py_INCREF(Py_None);
  p->stream_name = Py_None; Py_INCREF(Py_None);
  p->current_token = Py_None; Py_INCREF(Py_None);
  p->current_event = Py_None; Py_INCREF(Py_None);
  p->anchors = Py_None; Py_INCREF(Py_None);
  p->stream_cache = Py_None; Py_INCREF(Py_None);
  return o;
}

static void __pyx_tp_dealloc_5_yaml_CParser(PyObject *o) {
  struct __pyx_obj_5_yaml_CParser *p = (struct __pyx_obj_5_yaml_CParser *)o;
  #if PY_VERSION_HEX >= 0x030400a1
  if (unlikely(Py_TYPE(o)->tp_finalize) && !_PyGC_FINALIZED(o)) {
    if (PyObject_CallFinalizerFromDealloc(o)) return;
  }
  #endif
  PyObject_GC_UnTrack(o);
  {
    PyObject *etype, *eval, *etb;
    PyErr_Fetch(&etype, &eval, &etb);
    ++Py_REFCNT(o);
    __pyx_pw_5_yaml_7CParser_3__dealloc__(o);
    --Py_REFCNT(o);
    PyErr_Restore(etype, eval, etb);
  }
  Py_CLEAR(p->stream);
  Py_CLEAR(p->stream_name);
  Py_CLEAR(p->current_token);
  Py_CLEAR(p->current_event);
  Py_CLEAR(p->anchors);
  Py_CLEAR(p->stream_cache);
  (*Py_TYPE(o)->tp_free)(o);
}

static int __pyx_tp_traverse_5_yaml_CParser(PyObject *o, visitproc v, void *a) {
  int e;
  struct __pyx_obj_5_yaml_CParser *p = (struct __pyx_obj_5_yaml_CParser *)o;
  if (p->stream) {
    e = (*v)(p->stream, a); if (e) return e;
  }
  if (p->stream_name) {
    e = (*v)(p->stream_name, a); if (e) return e;
  }
  if (p->current_token) {
    e = (*v)(p->current_token, a); if (e) return e;
  }
  if (p->current_event) {
    e = (*v)(p->current_event, a); if (e) return e;
  }
  if (p->anchors) {
    e = (*v)(p->anchors, a); if (e) return e;
  }
  if (p->stream_cache) {
    e = (*v)(p->stream_cache, a); if (e) return e;
  }
  return 0;
}

static int __pyx_tp_clear_5_yaml_CParser(PyObject *o) {
  PyObject* tmp;
  struct __pyx_obj_5_yaml_CParser *p = (struct __pyx_obj_5_yaml_CParser *)o;
  tmp = ((PyObject*)p->stream);
  p->stream = Py_None; Py_INCREF(Py_None);
  Py_XDECREF(tmp);
  tmp = ((PyObject*)p->stream_name);
  p->stream_name = Py_None; Py_INCREF(Py_None);
  Py_XDECREF(tmp);
  tmp = ((PyObject*)p->current_token);
  p->current_token = Py_None; Py_INCREF(Py_None);
  Py_XDECREF(tmp);
  tmp = ((PyObject*)p->current_event);
  p->current_event = Py_None; Py_INCREF(Py_None);
  Py_XDECREF(tmp);
  tmp = ((PyObject*)p->anchors);
  p->anchors = Py_None; Py_INCREF(Py_None);
  Py_XDECREF(tmp);
  tmp = ((PyObject*)p->stream_cache);
  p->stream_cache = Py_None; Py_INCREF(Py_None);
  Py_XDECREF(tmp);
  return 0;
}

static PyMethodDef __pyx_methods_5_yaml_CParser[] = {
  {__Pyx_NAMESTR("dispose"), (PyCFunction)__pyx_pw_5_yaml_7CParser_5dispose, METH_NOARGS, __Pyx_DOCSTR(0)},
  {__Pyx_NAMESTR("raw_scan"), (PyCFunction)__pyx_pw_5_yaml_7CParser_7raw_scan, METH_NOARGS, __Pyx_DOCSTR(0)},
  {__Pyx_NAMESTR("get_token"), (PyCFunction)__pyx_pw_5_yaml_7CParser_9get_token, METH_NOARGS, __Pyx_DOCSTR(0)},
  {__Pyx_NAMESTR("peek_token"), (PyCFunction)__pyx_pw_5_yaml_7CParser_11peek_token, METH_NOARGS, __Pyx_DOCSTR(0)},
  {__Pyx_NAMESTR("check_token"), (PyCFunction)__pyx_pw_5_yaml_7CParser_13check_token, METH_VARARGS|METH_KEYWORDS, __Pyx_DOCSTR(0)},
  {__Pyx_NAMESTR("raw_parse"), (PyCFunction)__pyx_pw_5_yaml_7CParser_15raw_parse, METH_NOARGS, __Pyx_DOCSTR(0)},
  {__Pyx_NAMESTR("get_event"), (PyCFunction)__pyx_pw_5_yaml_7CParser_17get_event, METH_NOARGS, __Pyx_DOCSTR(0)},
  {__Pyx_NAMESTR("peek_event"), (PyCFunction)__pyx_pw_5_yaml_7CParser_19peek_event, METH_NOARGS, __Pyx_DOCSTR(0)},
  {__Pyx_NAMESTR("check_event"), (PyCFunction)__pyx_pw_5_yaml_7CParser_21check_event, METH_VARARGS|METH_KEYWORDS, __Pyx_DOCSTR(0)},
  {__Pyx_NAMESTR("check_node"), (PyCFunction)__pyx_pw_5_yaml_7CParser_23check_node, METH_NOARGS, __Pyx_DOCSTR(0)},
  {__Pyx_NAMESTR("get_node"), (PyCFunction)__pyx_pw_5_yaml_7CParser_25get_node, METH_NOARGS, __Pyx_DOCSTR(0)},
  {__Pyx_NAMESTR("get_single_node"), (PyCFunction)__pyx_pw_5_yaml_7CParser_27get_single_node, METH_NOARGS, __Pyx_DOCSTR(0)},
  {0, 0, 0, 0}
};

static PyTypeObject __pyx_type_5_yaml_CParser = {
  PyVarObject_HEAD_INIT(0, 0)
  __Pyx_NAMESTR("_yaml.CParser"), 
  sizeof(struct __pyx_obj_5_yaml_CParser), 
  0, 
  __pyx_tp_dealloc_5_yaml_CParser, 
  0, 
  0, 
  0, 
  #if PY_MAJOR_VERSION < 3
  0, 
  #else
  0, 
  #endif
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_VERSION_TAG|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_NEWBUFFER|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_HAVE_GC, 
  0, 
  __pyx_tp_traverse_5_yaml_CParser, 
  __pyx_tp_clear_5_yaml_CParser, 
  0, 
  0, 
  0, 
  0, 
  __pyx_methods_5_yaml_CParser, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  __pyx_pw_5_yaml_7CParser_1__init__, 
  0, 
  __pyx_tp_new_5_yaml_CParser, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  #if PY_VERSION_HEX >= 0x02060000
  0, 
  #endif
  #if PY_VERSION_HEX >= 0x030400a1
  0, 
  #endif
};
static struct __pyx_vtabstruct_5_yaml_CEmitter __pyx_vtable_5_yaml_CEmitter;

static PyObject *__pyx_tp_new_5_yaml_CEmitter(PyTypeObject *t, CYTHON_UNUSED PyObject *a, CYTHON_UNUSED PyObject *k) {
  struct __pyx_obj_5_yaml_CEmitter *p;
  PyObject *o;
  if (likely((t->tp_flags & Py_TPFLAGS_IS_ABSTRACT) == 0)) {
    o = (*t->tp_alloc)(t, 0);
  } else {
    o = (PyObject *) PyBaseObject_Type.tp_new(t, __pyx_empty_tuple, 0);
  }
  if (unlikely(!o)) return 0;
  p = ((struct __pyx_obj_5_yaml_CEmitter *)o);
  p->__pyx_vtab = __pyx_vtabptr_5_yaml_CEmitter;
  p->stream = Py_None; Py_INCREF(Py_None);
  p->use_version = Py_None; Py_INCREF(Py_None);
  p->use_tags = Py_None; Py_INCREF(Py_None);
  p->serialized_nodes = Py_None; Py_INCREF(Py_None);
  p->anchors = Py_None; Py_INCREF(Py_None);
  p->use_encoding = Py_None; Py_INCREF(Py_None);
  return o;
}

static void __pyx_tp_dealloc_5_yaml_CEmitter(PyObject *o) {
  struct __pyx_obj_5_yaml_CEmitter *p = (struct __pyx_obj_5_yaml_CEmitter *)o;
  #if PY_VERSION_HEX >= 0x030400a1
  if (unlikely(Py_TYPE(o)->tp_finalize) && !_PyGC_FINALIZED(o)) {
    if (PyObject_CallFinalizerFromDealloc(o)) return;
  }
  #endif
  PyObject_GC_UnTrack(o);
  {
    PyObject *etype, *eval, *etb;
    PyErr_Fetch(&etype, &eval, &etb);
    ++Py_REFCNT(o);
    __pyx_pw_5_yaml_8CEmitter_3__dealloc__(o);
    --Py_REFCNT(o);
    PyErr_Restore(etype, eval, etb);
  }
  Py_CLEAR(p->stream);
  Py_CLEAR(p->use_version);
  Py_CLEAR(p->use_tags);
  Py_CLEAR(p->serialized_nodes);
  Py_CLEAR(p->anchors);
  Py_CLEAR(p->use_encoding);
  (*Py_TYPE(o)->tp_free)(o);
}

static int __pyx_tp_traverse_5_yaml_CEmitter(PyObject *o, visitproc v, void *a) {
  int e;
  struct __pyx_obj_5_yaml_CEmitter *p = (struct __pyx_obj_5_yaml_CEmitter *)o;
  if (p->stream) {
    e = (*v)(p->stream, a); if (e) return e;
  }
  if (p->use_version) {
    e = (*v)(p->use_version, a); if (e) return e;
  }
  if (p->use_tags) {
    e = (*v)(p->use_tags, a); if (e) return e;
  }
  if (p->serialized_nodes) {
    e = (*v)(p->serialized_nodes, a); if (e) return e;
  }
  if (p->anchors) {
    e = (*v)(p->anchors, a); if (e) return e;
  }
  if (p->use_encoding) {
    e = (*v)(p->use_encoding, a); if (e) return e;
  }
  return 0;
}

static int __pyx_tp_clear_5_yaml_CEmitter(PyObject *o) {
  PyObject* tmp;
  struct __pyx_obj_5_yaml_CEmitter *p = (struct __pyx_obj_5_yaml_CEmitter *)o;
  tmp = ((PyObject*)p->stream);
  p->stream = Py_None; Py_INCREF(Py_None);
  Py_XDECREF(tmp);
  tmp = ((PyObject*)p->use_version);
  p->use_version = Py_None; Py_INCREF(Py_None);
  Py_XDECREF(tmp);
  tmp = ((PyObject*)p->use_tags);
  p->use_tags = Py_None; Py_INCREF(Py_None);
  Py_XDECREF(tmp);
  tmp = ((PyObject*)p->serialized_nodes);
  p->serialized_nodes = Py_None; Py_INCREF(Py_None);
  Py_XDECREF(tmp);
  tmp = ((PyObject*)p->anchors);
  p->anchors = Py_None; Py_INCREF(Py_None);
  Py_XDECREF(tmp);
  tmp = ((PyObject*)p->use_encoding);
  p->use_encoding = Py_None; Py_INCREF(Py_None);
  Py_XDECREF(tmp);
  return 0;
}

static PyMethodDef __pyx_methods_5_yaml_CEmitter[] = {
  {__Pyx_NAMESTR("dispose"), (PyCFunction)__pyx_pw_5_yaml_8CEmitter_5dispose, METH_NOARGS, __Pyx_DOCSTR(0)},
  {__Pyx_NAMESTR("emit"), (PyCFunction)__pyx_pw_5_yaml_8CEmitter_7emit, METH_O, __Pyx_DOCSTR(0)},
  {__Pyx_NAMESTR("open"), (PyCFunction)__pyx_pw_5_yaml_8CEmitter_9open, METH_NOARGS, __Pyx_DOCSTR(0)},
  {__Pyx_NAMESTR("close"), (PyCFunction)__pyx_pw_5_yaml_8CEmitter_11close, METH_NOARGS, __Pyx_DOCSTR(0)},
  {__Pyx_NAMESTR("serialize"), (PyCFunction)__pyx_pw_5_yaml_8CEmitter_13serialize, METH_O, __Pyx_DOCSTR(0)},
  {0, 0, 0, 0}
};

static PyTypeObject __pyx_type_5_yaml_CEmitter = {
  PyVarObject_HEAD_INIT(0, 0)
  __Pyx_NAMESTR("_yaml.CEmitter"), 
  sizeof(struct __pyx_obj_5_yaml_CEmitter), 
  0, 
  __pyx_tp_dealloc_5_yaml_CEmitter, 
  0, 
  0, 
  0, 
  #if PY_MAJOR_VERSION < 3
  0, 
  #else
  0, 
  #endif
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_VERSION_TAG|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_NEWBUFFER|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_HAVE_GC, 
  0, 
  __pyx_tp_traverse_5_yaml_CEmitter, 
  __pyx_tp_clear_5_yaml_CEmitter, 
  0, 
  0, 
  0, 
  0, 
  __pyx_methods_5_yaml_CEmitter, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  __pyx_pw_5_yaml_8CEmitter_1__init__, 
  0, 
  __pyx_tp_new_5_yaml_CEmitter, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  0, 
  #if PY_VERSION_HEX >= 0x02060000
  0, 
  #endif
  #if PY_VERSION_HEX >= 0x030400a1
  0, 
  #endif
};

static PyMethodDef __pyx_methods[] = {
  {0, 0, 0, 0}
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef __pyx_moduledef = {
  #if PY_VERSION_HEX < 0x03020000
    { PyObject_HEAD_INIT(NULL) NULL, 0, NULL },
  #else
    PyModuleDef_HEAD_INIT,
  #endif
    __Pyx_NAMESTR("_yaml"),
    0, 
    -1, 
    __pyx_methods ,
    NULL, 
    NULL, 
    NULL, 
    NULL 
};
#endif

static __Pyx_StringTabEntry __pyx_string_tab[] = {
  {&__pyx_n_s_AliasEvent, __pyx_k_AliasEvent, sizeof(__pyx_k_AliasEvent), 0, 0, 1, 1},
  {&__pyx_n_s_AliasToken, __pyx_k_AliasToken, sizeof(__pyx_k_AliasToken), 0, 0, 1, 1},
  {&__pyx_n_s_AnchorToken, __pyx_k_AnchorToken, sizeof(__pyx_k_AnchorToken), 0, 0, 1, 1},
  {&__pyx_n_s_AttributeError, __pyx_k_AttributeError, sizeof(__pyx_k_AttributeError), 0, 0, 1, 1},
  {&__pyx_n_s_BlockEndToken, __pyx_k_BlockEndToken, sizeof(__pyx_k_BlockEndToken), 0, 0, 1, 1},
  {&__pyx_n_s_BlockEntryToken, __pyx_k_BlockEntryToken, sizeof(__pyx_k_BlockEntryToken), 0, 0, 1, 1},
  {&__pyx_n_s_BlockMappingStartToken, __pyx_k_BlockMappingStartToken, sizeof(__pyx_k_BlockMappingStartToken), 0, 0, 1, 1},
  {&__pyx_n_s_BlockSequenceStartToken, __pyx_k_BlockSequenceStartToken, sizeof(__pyx_k_BlockSequenceStartToken), 0, 0, 1, 1},
  {&__pyx_n_s_ComposerError, __pyx_k_ComposerError, sizeof(__pyx_k_ComposerError), 0, 0, 1, 1},
  {&__pyx_n_s_ConstructorError, __pyx_k_ConstructorError, sizeof(__pyx_k_ConstructorError), 0, 0, 1, 1},
  {&__pyx_n_s_DirectiveToken, __pyx_k_DirectiveToken, sizeof(__pyx_k_DirectiveToken), 0, 0, 1, 1},
  {&__pyx_n_s_DocumentEndEvent, __pyx_k_DocumentEndEvent, sizeof(__pyx_k_DocumentEndEvent), 0, 0, 1, 1},
  {&__pyx_n_s_DocumentEndToken, __pyx_k_DocumentEndToken, sizeof(__pyx_k_DocumentEndToken), 0, 0, 1, 1},
  {&__pyx_n_s_DocumentStartEvent, __pyx_k_DocumentStartEvent, sizeof(__pyx_k_DocumentStartEvent), 0, 0, 1, 1},
  {&__pyx_n_s_DocumentStartToken, __pyx_k_DocumentStartToken, sizeof(__pyx_k_DocumentStartToken), 0, 0, 1, 1},
  {&__pyx_n_s_EmitterError, __pyx_k_EmitterError, sizeof(__pyx_k_EmitterError), 0, 0, 1, 1},
  {&__pyx_n_s_FlowEntryToken, __pyx_k_FlowEntryToken, sizeof(__pyx_k_FlowEntryToken), 0, 0, 1, 1},
  {&__pyx_n_s_FlowMappingEndToken, __pyx_k_FlowMappingEndToken, sizeof(__pyx_k_FlowMappingEndToken), 0, 0, 1, 1},
  {&__pyx_n_s_FlowMappingStartToken, __pyx_k_FlowMappingStartToken, sizeof(__pyx_k_FlowMappingStartToken), 0, 0, 1, 1},
  {&__pyx_n_s_FlowSequenceEndToken, __pyx_k_FlowSequenceEndToken, sizeof(__pyx_k_FlowSequenceEndToken), 0, 0, 1, 1},
  {&__pyx_n_s_FlowSequenceStartToken, __pyx_k_FlowSequenceStartToken, sizeof(__pyx_k_FlowSequenceStartToken), 0, 0, 1, 1},
  {&__pyx_n_s_KeyToken, __pyx_k_KeyToken, sizeof(__pyx_k_KeyToken), 0, 0, 1, 1},
  {&__pyx_n_s_MappingEndEvent, __pyx_k_MappingEndEvent, sizeof(__pyx_k_MappingEndEvent), 0, 0, 1, 1},
  {&__pyx_n_s_MappingNode, __pyx_k_MappingNode, sizeof(__pyx_k_MappingNode), 0, 0, 1, 1},
  {&__pyx_n_s_MappingStartEvent, __pyx_k_MappingStartEvent, sizeof(__pyx_k_MappingStartEvent), 0, 0, 1, 1},
  {&__pyx_n_s_MemoryError, __pyx_k_MemoryError, sizeof(__pyx_k_MemoryError), 0, 0, 1, 1},
  {&__pyx_n_s_ParserError, __pyx_k_ParserError, sizeof(__pyx_k_ParserError), 0, 0, 1, 1},
  {&__pyx_n_s_ReaderError, __pyx_k_ReaderError, sizeof(__pyx_k_ReaderError), 0, 0, 1, 1},
  {&__pyx_n_s_RepresenterError, __pyx_k_RepresenterError, sizeof(__pyx_k_RepresenterError), 0, 0, 1, 1},
  {&__pyx_n_s_ScalarEvent, __pyx_k_ScalarEvent, sizeof(__pyx_k_ScalarEvent), 0, 0, 1, 1},
  {&__pyx_n_s_ScalarNode, __pyx_k_ScalarNode, sizeof(__pyx_k_ScalarNode), 0, 0, 1, 1},
  {&__pyx_n_s_ScalarToken, __pyx_k_ScalarToken, sizeof(__pyx_k_ScalarToken), 0, 0, 1, 1},
  {&__pyx_n_s_ScannerError, __pyx_k_ScannerError, sizeof(__pyx_k_ScannerError), 0, 0, 1, 1},
  {&__pyx_n_s_SequenceEndEvent, __pyx_k_SequenceEndEvent, sizeof(__pyx_k_SequenceEndEvent), 0, 0, 1, 1},
  {&__pyx_n_s_SequenceNode, __pyx_k_SequenceNode, sizeof(__pyx_k_SequenceNode), 0, 0, 1, 1},
  {&__pyx_n_s_SequenceStartEvent, __pyx_k_SequenceStartEvent, sizeof(__pyx_k_SequenceStartEvent), 0, 0, 1, 1},
  {&__pyx_n_s_SerializerError, __pyx_k_SerializerError, sizeof(__pyx_k_SerializerError), 0, 0, 1, 1},
  {&__pyx_n_s_StreamEndEvent, __pyx_k_StreamEndEvent, sizeof(__pyx_k_StreamEndEvent), 0, 0, 1, 1},
  {&__pyx_n_s_StreamEndToken, __pyx_k_StreamEndToken, sizeof(__pyx_k_StreamEndToken), 0, 0, 1, 1},
  {&__pyx_n_s_StreamStartEvent, __pyx_k_StreamStartEvent, sizeof(__pyx_k_StreamStartEvent), 0, 0, 1, 1},
  {&__pyx_n_s_StreamStartToken, __pyx_k_StreamStartToken, sizeof(__pyx_k_StreamStartToken), 0, 0, 1, 1},
  {&__pyx_n_u_TAG, __pyx_k_TAG, sizeof(__pyx_k_TAG), 0, 1, 0, 1},
  {&__pyx_n_s_TagToken, __pyx_k_TagToken, sizeof(__pyx_k_TagToken), 0, 0, 1, 1},
  {&__pyx_n_s_TypeError, __pyx_k_TypeError, sizeof(__pyx_k_TypeError), 0, 0, 1, 1},
  {&__pyx_n_s_ValueError, __pyx_k_ValueError, sizeof(__pyx_k_ValueError), 0, 0, 1, 1},
  {&__pyx_n_s_ValueToken, __pyx_k_ValueToken, sizeof(__pyx_k_ValueToken), 0, 0, 1, 1},
  {&__pyx_n_u_YAML, __pyx_k_YAML, sizeof(__pyx_k_YAML), 0, 1, 0, 1},
  {&__pyx_n_s_YAMLError, __pyx_k_YAMLError, sizeof(__pyx_k_YAMLError), 0, 0, 1, 1},
  {&__pyx_kp_s__10, __pyx_k__10, sizeof(__pyx_k__10), 0, 0, 1, 0},
  {&__pyx_kp_u__10, __pyx_k__10, sizeof(__pyx_k__10), 0, 1, 0, 0},
  {&__pyx_kp_s__17, __pyx_k__17, sizeof(__pyx_k__17), 0, 0, 1, 0},
  {&__pyx_kp_s__18, __pyx_k__18, sizeof(__pyx_k__18), 0, 0, 1, 0},
  {&__pyx_kp_s__19, __pyx_k__19, sizeof(__pyx_k__19), 0, 0, 1, 0},
  {&__pyx_kp_s__3, __pyx_k__3, sizeof(__pyx_k__3), 0, 0, 1, 0},
  {&__pyx_kp_u__3, __pyx_k__3, sizeof(__pyx_k__3), 0, 1, 0, 0},
  {&__pyx_kp_u__6, __pyx_k__6, sizeof(__pyx_k__6), 0, 1, 0, 0},
  {&__pyx_kp_s__7, __pyx_k__7, sizeof(__pyx_k__7), 0, 0, 1, 0},
  {&__pyx_kp_u__7, __pyx_k__7, sizeof(__pyx_k__7), 0, 1, 0, 0},
  {&__pyx_kp_s__8, __pyx_k__8, sizeof(__pyx_k__8), 0, 0, 1, 0},
  {&__pyx_kp_u__8, __pyx_k__8, sizeof(__pyx_k__8), 0, 1, 0, 0},
  {&__pyx_kp_s__9, __pyx_k__9, sizeof(__pyx_k__9), 0, 0, 1, 0},
  {&__pyx_kp_u__9, __pyx_k__9, sizeof(__pyx_k__9), 0, 1, 0, 0},
  {&__pyx_kp_s_a_string_or_stream_input_is_requ, __pyx_k_a_string_or_stream_input_is_requ, sizeof(__pyx_k_a_string_or_stream_input_is_requ), 0, 0, 1, 0},
  {&__pyx_kp_u_a_string_or_stream_input_is_requ, __pyx_k_a_string_or_stream_input_is_requ, sizeof(__pyx_k_a_string_or_stream_input_is_requ), 0, 1, 0, 0},
  {&__pyx_kp_s_a_string_value_is_expected, __pyx_k_a_string_value_is_expected, sizeof(__pyx_k_a_string_value_is_expected), 0, 0, 1, 0},
  {&__pyx_kp_u_a_string_value_is_expected, __pyx_k_a_string_value_is_expected, sizeof(__pyx_k_a_string_value_is_expected), 0, 1, 0, 0},
  {&__pyx_n_s_allow_unicode, __pyx_k_allow_unicode, sizeof(__pyx_k_allow_unicode), 0, 0, 1, 1},
  {&__pyx_n_s_anchor, __pyx_k_anchor, sizeof(__pyx_k_anchor), 0, 0, 1, 1},
  {&__pyx_kp_s_anchor_must_be_a_string, __pyx_k_anchor_must_be_a_string, sizeof(__pyx_k_anchor_must_be_a_string), 0, 0, 1, 0},
  {&__pyx_kp_u_anchor_must_be_a_string, __pyx_k_anchor_must_be_a_string, sizeof(__pyx_k_anchor_must_be_a_string), 0, 1, 0, 0},
  {&__pyx_n_s_ascend_resolver, __pyx_k_ascend_resolver, sizeof(__pyx_k_ascend_resolver), 0, 0, 1, 1},
  {&__pyx_n_s_buffer, __pyx_k_buffer, sizeof(__pyx_k_buffer), 0, 0, 1, 1},
  {&__pyx_kp_s_but_found_another_document, __pyx_k_but_found_another_document, sizeof(__pyx_k_but_found_another_document), 0, 0, 1, 0},
  {&__pyx_kp_u_but_found_another_document, __pyx_k_but_found_another_document, sizeof(__pyx_k_but_found_another_document), 0, 1, 0, 0},
  {&__pyx_kp_s_byte_string, __pyx_k_byte_string, sizeof(__pyx_k_byte_string), 0, 0, 1, 0},
  {&__pyx_kp_u_byte_string, __pyx_k_byte_string, sizeof(__pyx_k_byte_string), 0, 1, 0, 0},
  {&__pyx_n_s_canonical, __pyx_k_canonical, sizeof(__pyx_k_canonical), 0, 0, 1, 1},
  {&__pyx_n_s_class, __pyx_k_class, sizeof(__pyx_k_class), 0, 0, 1, 1},
  {&__pyx_n_s_column, __pyx_k_column, sizeof(__pyx_k_column), 0, 0, 1, 1},
  {&__pyx_n_s_composer, __pyx_k_composer, sizeof(__pyx_k_composer), 0, 0, 1, 1},
  {&__pyx_n_s_constructor, __pyx_k_constructor, sizeof(__pyx_k_constructor), 0, 0, 1, 1},
  {&__pyx_n_s_descend_resolver, __pyx_k_descend_resolver, sizeof(__pyx_k_descend_resolver), 0, 0, 1, 1},
  {&__pyx_n_s_emitter, __pyx_k_emitter, sizeof(__pyx_k_emitter), 0, 0, 1, 1},
  {&__pyx_n_s_encoding, __pyx_k_encoding, sizeof(__pyx_k_encoding), 0, 0, 1, 1},
  {&__pyx_n_u_encoding, __pyx_k_encoding, sizeof(__pyx_k_encoding), 0, 1, 0, 1},
  {&__pyx_n_s_end_mark, __pyx_k_end_mark, sizeof(__pyx_k_end_mark), 0, 0, 1, 1},
  {&__pyx_n_s_error, __pyx_k_error, sizeof(__pyx_k_error), 0, 0, 1, 1},
  {&__pyx_n_s_events, __pyx_k_events, sizeof(__pyx_k_events), 0, 0, 1, 1},
  {&__pyx_kp_s_expected_a_single_document_in_th, __pyx_k_expected_a_single_document_in_th, sizeof(__pyx_k_expected_a_single_document_in_th), 0, 0, 1, 0},
  {&__pyx_kp_u_expected_a_single_document_in_th, __pyx_k_expected_a_single_document_in_th, sizeof(__pyx_k_expected_a_single_document_in_th), 0, 1, 0, 0},
  {&__pyx_n_s_explicit, __pyx_k_explicit, sizeof(__pyx_k_explicit), 0, 0, 1, 1},
  {&__pyx_n_s_explicit_end, __pyx_k_explicit_end, sizeof(__pyx_k_explicit_end), 0, 0, 1, 1},
  {&__pyx_n_s_explicit_start, __pyx_k_explicit_start, sizeof(__pyx_k_explicit_start), 0, 0, 1, 1},
  {&__pyx_kp_s_file, __pyx_k_file, sizeof(__pyx_k_file), 0, 0, 1, 0},
  {&__pyx_kp_u_file, __pyx_k_file, sizeof(__pyx_k_file), 0, 1, 0, 0},
  {&__pyx_n_s_flow_style, __pyx_k_flow_style, sizeof(__pyx_k_flow_style), 0, 0, 1, 1},
  {&__pyx_kp_s_found_duplicate_anchor_first_occ, __pyx_k_found_duplicate_anchor_first_occ, sizeof(__pyx_k_found_duplicate_anchor_first_occ), 0, 0, 1, 0},
  {&__pyx_kp_u_found_duplicate_anchor_first_occ, __pyx_k_found_duplicate_anchor_first_occ, sizeof(__pyx_k_found_duplicate_anchor_first_occ), 0, 1, 0, 0},
  {&__pyx_kp_s_found_undefined_alias, __pyx_k_found_undefined_alias, sizeof(__pyx_k_found_undefined_alias), 0, 0, 1, 0},
  {&__pyx_kp_u_found_undefined_alias, __pyx_k_found_undefined_alias, sizeof(__pyx_k_found_undefined_alias), 0, 1, 0, 0},
  {&__pyx_n_s_get_version, __pyx_k_get_version, sizeof(__pyx_k_get_version), 0, 0, 1, 1},
  {&__pyx_n_s_get_version_string, __pyx_k_get_version_string, sizeof(__pyx_k_get_version_string), 0, 0, 1, 1},
  {&__pyx_kp_u_id_03d, __pyx_k_id_03d, sizeof(__pyx_k_id_03d), 0, 1, 0, 0},
  {&__pyx_n_s_implicit, __pyx_k_implicit, sizeof(__pyx_k_implicit), 0, 0, 1, 1},
  {&__pyx_n_s_import, __pyx_k_import, sizeof(__pyx_k_import), 0, 0, 1, 1},
  {&__pyx_kp_s_in_s_line_d_column_d, __pyx_k_in_s_line_d_column_d, sizeof(__pyx_k_in_s_line_d_column_d), 0, 0, 1, 0},
  {&__pyx_n_s_indent, __pyx_k_indent, sizeof(__pyx_k_indent), 0, 0, 1, 1},
  {&__pyx_n_s_index, __pyx_k_index, sizeof(__pyx_k_index), 0, 0, 1, 1},
  {&__pyx_kp_s_invalid_event_s, __pyx_k_invalid_event_s, sizeof(__pyx_k_invalid_event_s), 0, 0, 1, 0},
  {&__pyx_kp_u_invalid_event_s, __pyx_k_invalid_event_s, sizeof(__pyx_k_invalid_event_s), 0, 1, 0, 0},
  {&__pyx_n_s_line, __pyx_k_line, sizeof(__pyx_k_line), 0, 0, 1, 1},
  {&__pyx_n_s_line_break, __pyx_k_line_break, sizeof(__pyx_k_line_break), 0, 0, 1, 1},
  {&__pyx_n_s_main, __pyx_k_main, sizeof(__pyx_k_main), 0, 0, 1, 1},
  {&__pyx_n_s_major, __pyx_k_major, sizeof(__pyx_k_major), 0, 0, 1, 1},
  {&__pyx_n_s_minor, __pyx_k_minor, sizeof(__pyx_k_minor), 0, 0, 1, 1},
  {&__pyx_n_s_name, __pyx_k_name, sizeof(__pyx_k_name), 0, 0, 1, 1},
  {&__pyx_kp_s_no_emitter_error, __pyx_k_no_emitter_error, sizeof(__pyx_k_no_emitter_error), 0, 0, 1, 0},
  {&__pyx_kp_u_no_emitter_error, __pyx_k_no_emitter_error, sizeof(__pyx_k_no_emitter_error), 0, 1, 0, 0},
  {&__pyx_kp_s_no_parser_error, __pyx_k_no_parser_error, sizeof(__pyx_k_no_parser_error), 0, 0, 1, 0},
  {&__pyx_kp_u_no_parser_error, __pyx_k_no_parser_error, sizeof(__pyx_k_no_parser_error), 0, 1, 0, 0},
  {&__pyx_n_s_nodes, __pyx_k_nodes, sizeof(__pyx_k_nodes), 0, 0, 1, 1},
  {&__pyx_n_s_parser, __pyx_k_parser, sizeof(__pyx_k_parser), 0, 0, 1, 1},
  {&__pyx_n_s_patch, __pyx_k_patch, sizeof(__pyx_k_patch), 0, 0, 1, 1},
  {&__pyx_n_s_pointer, __pyx_k_pointer, sizeof(__pyx_k_pointer), 0, 0, 1, 1},
  {&__pyx_n_s_pyx_vtable, __pyx_k_pyx_vtable, sizeof(__pyx_k_pyx_vtable), 0, 0, 1, 1},
  {&__pyx_n_s_read, __pyx_k_read, sizeof(__pyx_k_read), 0, 0, 1, 1},
  {&__pyx_n_s_reader, __pyx_k_reader, sizeof(__pyx_k_reader), 0, 0, 1, 1},
  {&__pyx_n_s_representer, __pyx_k_representer, sizeof(__pyx_k_representer), 0, 0, 1, 1},
  {&__pyx_n_s_resolve, __pyx_k_resolve, sizeof(__pyx_k_resolve), 0, 0, 1, 1},
  {&__pyx_kp_s_root_src_pyyaml_ext__yaml_pyx, __pyx_k_root_src_pyyaml_ext__yaml_pyx, sizeof(__pyx_k_root_src_pyyaml_ext__yaml_pyx), 0, 0, 1, 0},
  {&__pyx_n_s_scanner, __pyx_k_scanner, sizeof(__pyx_k_scanner), 0, 0, 1, 1},
  {&__pyx_kp_s_second_occurence, __pyx_k_second_occurence, sizeof(__pyx_k_second_occurence), 0, 0, 1, 0},
  {&__pyx_kp_u_second_occurence, __pyx_k_second_occurence, sizeof(__pyx_k_second_occurence), 0, 1, 0, 0},
  {&__pyx_n_s_serializer, __pyx_k_serializer, sizeof(__pyx_k_serializer), 0, 0, 1, 1},
  {&__pyx_kp_s_serializer_is_already_opened, __pyx_k_serializer_is_already_opened, sizeof(__pyx_k_serializer_is_already_opened), 0, 0, 1, 0},
  {&__pyx_kp_u_serializer_is_already_opened, __pyx_k_serializer_is_already_opened, sizeof(__pyx_k_serializer_is_already_opened), 0, 1, 0, 0},
  {&__pyx_kp_s_serializer_is_closed, __pyx_k_serializer_is_closed, sizeof(__pyx_k_serializer_is_closed), 0, 0, 1, 0},
  {&__pyx_kp_u_serializer_is_closed, __pyx_k_serializer_is_closed, sizeof(__pyx_k_serializer_is_closed), 0, 1, 0, 0},
  {&__pyx_kp_s_serializer_is_not_opened, __pyx_k_serializer_is_not_opened, sizeof(__pyx_k_serializer_is_not_opened), 0, 0, 1, 0},
  {&__pyx_kp_u_serializer_is_not_opened, __pyx_k_serializer_is_not_opened, sizeof(__pyx_k_serializer_is_not_opened), 0, 1, 0, 0},
  {&__pyx_n_s_start_mark, __pyx_k_start_mark, sizeof(__pyx_k_start_mark), 0, 0, 1, 1},
  {&__pyx_n_s_stream, __pyx_k_stream, sizeof(__pyx_k_stream), 0, 0, 1, 1},
  {&__pyx_n_s_style, __pyx_k_style, sizeof(__pyx_k_style), 0, 0, 1, 1},
  {&__pyx_n_s_tag, __pyx_k_tag, sizeof(__pyx_k_tag), 0, 0, 1, 1},
  {&__pyx_kp_s_tag_handle_must_be_a_string, __pyx_k_tag_handle_must_be_a_string, sizeof(__pyx_k_tag_handle_must_be_a_string), 0, 0, 1, 0},
  {&__pyx_kp_u_tag_handle_must_be_a_string, __pyx_k_tag_handle_must_be_a_string, sizeof(__pyx_k_tag_handle_must_be_a_string), 0, 1, 0, 0},
  {&__pyx_kp_s_tag_must_be_a_string, __pyx_k_tag_must_be_a_string, sizeof(__pyx_k_tag_must_be_a_string), 0, 0, 1, 0},
  {&__pyx_kp_u_tag_must_be_a_string, __pyx_k_tag_must_be_a_string, sizeof(__pyx_k_tag_must_be_a_string), 0, 1, 0, 0},
  {&__pyx_kp_s_tag_prefix_must_be_a_string, __pyx_k_tag_prefix_must_be_a_string, sizeof(__pyx_k_tag_prefix_must_be_a_string), 0, 0, 1, 0},
  {&__pyx_kp_u_tag_prefix_must_be_a_string, __pyx_k_tag_prefix_must_be_a_string, sizeof(__pyx_k_tag_prefix_must_be_a_string), 0, 1, 0, 0},
  {&__pyx_n_s_tags, __pyx_k_tags, sizeof(__pyx_k_tags), 0, 0, 1, 1},
  {&__pyx_n_s_test, __pyx_k_test, sizeof(__pyx_k_test), 0, 0, 1, 1},
  {&__pyx_n_s_tokens, __pyx_k_tokens, sizeof(__pyx_k_tokens), 0, 0, 1, 1},
  {&__pyx_kp_s_too_many_tags, __pyx_k_too_many_tags, sizeof(__pyx_k_too_many_tags), 0, 0, 1, 0},
  {&__pyx_kp_u_too_many_tags, __pyx_k_too_many_tags, sizeof(__pyx_k_too_many_tags), 0, 1, 0, 0},
  {&__pyx_kp_s_unicode_string, __pyx_k_unicode_string, sizeof(__pyx_k_unicode_string), 0, 0, 1, 0},
  {&__pyx_kp_u_unicode_string, __pyx_k_unicode_string, sizeof(__pyx_k_unicode_string), 0, 1, 0, 0},
  {&__pyx_kp_s_unknown_event_type, __pyx_k_unknown_event_type, sizeof(__pyx_k_unknown_event_type), 0, 0, 1, 0},
  {&__pyx_kp_u_unknown_event_type, __pyx_k_unknown_event_type, sizeof(__pyx_k_unknown_event_type), 0, 1, 0, 0},
  {&__pyx_kp_s_unknown_token_type, __pyx_k_unknown_token_type, sizeof(__pyx_k_unknown_token_type), 0, 0, 1, 0},
  {&__pyx_kp_u_unknown_token_type, __pyx_k_unknown_token_type, sizeof(__pyx_k_unknown_token_type), 0, 1, 0, 0},
  {&__pyx_kp_s_utf_16_be, __pyx_k_utf_16_be, sizeof(__pyx_k_utf_16_be), 0, 0, 1, 0},
  {&__pyx_kp_u_utf_16_be, __pyx_k_utf_16_be, sizeof(__pyx_k_utf_16_be), 0, 1, 0, 0},
  {&__pyx_kp_s_utf_16_le, __pyx_k_utf_16_le, sizeof(__pyx_k_utf_16_le), 0, 0, 1, 0},
  {&__pyx_kp_u_utf_16_le, __pyx_k_utf_16_le, sizeof(__pyx_k_utf_16_le), 0, 1, 0, 0},
  {&__pyx_kp_u_utf_8, __pyx_k_utf_8, sizeof(__pyx_k_utf_8), 0, 1, 0, 0},
  {&__pyx_n_s_value, __pyx_k_value, sizeof(__pyx_k_value), 0, 0, 1, 1},
  {&__pyx_kp_s_value_must_be_a_string, __pyx_k_value_must_be_a_string, sizeof(__pyx_k_value_must_be_a_string), 0, 0, 1, 0},
  {&__pyx_kp_u_value_must_be_a_string, __pyx_k_value_must_be_a_string, sizeof(__pyx_k_value_must_be_a_string), 0, 1, 0, 0},
  {&__pyx_n_s_version, __pyx_k_version, sizeof(__pyx_k_version), 0, 0, 1, 1},
  {&__pyx_n_s_width, __pyx_k_width, sizeof(__pyx_k_width), 0, 0, 1, 1},
  {&__pyx_n_s_write, __pyx_k_write, sizeof(__pyx_k_write), 0, 0, 1, 1},
  {&__pyx_n_s_yaml, __pyx_k_yaml, sizeof(__pyx_k_yaml), 0, 0, 1, 1},
  {&__pyx_n_s_yaml_2, __pyx_k_yaml_2, sizeof(__pyx_k_yaml_2), 0, 0, 1, 1},
  {0, 0, 0, 0, 0, 0, 0}
};
static int __Pyx_InitCachedBuiltins(void) {
  __pyx_builtin_MemoryError = __Pyx_GetBuiltinName(__pyx_n_s_MemoryError); if (!__pyx_builtin_MemoryError) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 265; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_builtin_AttributeError = __Pyx_GetBuiltinName(__pyx_n_s_AttributeError); if (!__pyx_builtin_AttributeError) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 270; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_builtin_TypeError = __Pyx_GetBuiltinName(__pyx_n_s_TypeError); if (!__pyx_builtin_TypeError) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 301; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_builtin_ValueError = __Pyx_GetBuiltinName(__pyx_n_s_ValueError); if (!__pyx_builtin_ValueError) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 356; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  return 0;
  __pyx_L1_error:;
  return -1;
}

static int __Pyx_InitCachedConstants(void) {
  __Pyx_RefNannyDeclarations
  __Pyx_RefNannySetupContext("__Pyx_InitCachedConstants", 0);

  






  __pyx_tuple_ = PyTuple_Pack(1, __pyx_kp_s_a_string_or_stream_input_is_requ); if (unlikely(!__pyx_tuple_)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 301; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple_);
  __Pyx_GIVEREF(__pyx_tuple_);

  






  __pyx_tuple__2 = PyTuple_Pack(1, __pyx_kp_u_a_string_or_stream_input_is_requ); if (unlikely(!__pyx_tuple__2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 303; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__2);
  __Pyx_GIVEREF(__pyx_tuple__2);

  






  __pyx_tuple__4 = PyTuple_Pack(1, __pyx_kp_s_no_parser_error); if (unlikely(!__pyx_tuple__4)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 356; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__4);
  __Pyx_GIVEREF(__pyx_tuple__4);

  






  __pyx_tuple__5 = PyTuple_Pack(1, __pyx_kp_u_no_parser_error); if (unlikely(!__pyx_tuple__5)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 358; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__5);
  __Pyx_GIVEREF(__pyx_tuple__5);

  






  __pyx_tuple__11 = PyTuple_Pack(1, __pyx_kp_s_unknown_token_type); if (unlikely(!__pyx_tuple__11)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 479; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__11);
  __Pyx_GIVEREF(__pyx_tuple__11);

  






  __pyx_tuple__12 = PyTuple_Pack(1, __pyx_kp_u_unknown_token_type); if (unlikely(!__pyx_tuple__12)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 481; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__12);
  __Pyx_GIVEREF(__pyx_tuple__12);

  






  __pyx_tuple__13 = PyTuple_Pack(1, __pyx_kp_s_unknown_event_type); if (unlikely(!__pyx_tuple__13)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 657; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__13);
  __Pyx_GIVEREF(__pyx_tuple__13);

  






  __pyx_tuple__14 = PyTuple_Pack(1, __pyx_kp_u_unknown_event_type); if (unlikely(!__pyx_tuple__14)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 659; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__14);
  __Pyx_GIVEREF(__pyx_tuple__14);

  






  __pyx_tuple__15 = PyTuple_Pack(1, __pyx_kp_s_a_string_value_is_expected); if (unlikely(!__pyx_tuple__15)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 918; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__15);
  __Pyx_GIVEREF(__pyx_tuple__15);

  






  __pyx_tuple__16 = PyTuple_Pack(1, __pyx_kp_u_a_string_value_is_expected); if (unlikely(!__pyx_tuple__16)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 920; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__16);
  __Pyx_GIVEREF(__pyx_tuple__16);

  






  __pyx_tuple__20 = PyTuple_Pack(1, __pyx_kp_s_no_emitter_error); if (unlikely(!__pyx_tuple__20)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1012; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__20);
  __Pyx_GIVEREF(__pyx_tuple__20);

  






  __pyx_tuple__21 = PyTuple_Pack(1, __pyx_kp_u_no_emitter_error); if (unlikely(!__pyx_tuple__21)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1014; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__21);
  __Pyx_GIVEREF(__pyx_tuple__21);

  






  __pyx_tuple__22 = PyTuple_Pack(1, __pyx_kp_s_too_many_tags); if (unlikely(!__pyx_tuple__22)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1058; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__22);
  __Pyx_GIVEREF(__pyx_tuple__22);

  






  __pyx_tuple__23 = PyTuple_Pack(1, __pyx_kp_u_too_many_tags); if (unlikely(!__pyx_tuple__23)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1060; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__23);
  __Pyx_GIVEREF(__pyx_tuple__23);

  






  __pyx_tuple__24 = PyTuple_Pack(1, __pyx_kp_s_tag_handle_must_be_a_string); if (unlikely(!__pyx_tuple__24)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1071; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__24);
  __Pyx_GIVEREF(__pyx_tuple__24);

  






  __pyx_tuple__25 = PyTuple_Pack(1, __pyx_kp_u_tag_handle_must_be_a_string); if (unlikely(!__pyx_tuple__25)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1073; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__25);
  __Pyx_GIVEREF(__pyx_tuple__25);

  






  __pyx_tuple__26 = PyTuple_Pack(1, __pyx_kp_s_tag_prefix_must_be_a_string); if (unlikely(!__pyx_tuple__26)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1080; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__26);
  __Pyx_GIVEREF(__pyx_tuple__26);

  






  __pyx_tuple__27 = PyTuple_Pack(1, __pyx_kp_u_tag_prefix_must_be_a_string); if (unlikely(!__pyx_tuple__27)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1082; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__27);
  __Pyx_GIVEREF(__pyx_tuple__27);

  






  __pyx_tuple__28 = PyTuple_Pack(1, __pyx_kp_s_anchor_must_be_a_string); if (unlikely(!__pyx_tuple__28)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1103; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__28);
  __Pyx_GIVEREF(__pyx_tuple__28);

  






  __pyx_tuple__29 = PyTuple_Pack(1, __pyx_kp_u_anchor_must_be_a_string); if (unlikely(!__pyx_tuple__29)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1105; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__29);
  __Pyx_GIVEREF(__pyx_tuple__29);

  






  __pyx_tuple__30 = PyTuple_Pack(1, __pyx_kp_s_anchor_must_be_a_string); if (unlikely(!__pyx_tuple__30)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1117; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__30);
  __Pyx_GIVEREF(__pyx_tuple__30);

  






  __pyx_tuple__31 = PyTuple_Pack(1, __pyx_kp_u_anchor_must_be_a_string); if (unlikely(!__pyx_tuple__31)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1119; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__31);
  __Pyx_GIVEREF(__pyx_tuple__31);

  






  __pyx_tuple__32 = PyTuple_Pack(1, __pyx_kp_s_tag_must_be_a_string); if (unlikely(!__pyx_tuple__32)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1128; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__32);
  __Pyx_GIVEREF(__pyx_tuple__32);

  






  __pyx_tuple__33 = PyTuple_Pack(1, __pyx_kp_u_tag_must_be_a_string); if (unlikely(!__pyx_tuple__33)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1130; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__33);
  __Pyx_GIVEREF(__pyx_tuple__33);

  






  __pyx_tuple__34 = PyTuple_Pack(1, __pyx_kp_s_value_must_be_a_string); if (unlikely(!__pyx_tuple__34)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1137; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__34);
  __Pyx_GIVEREF(__pyx_tuple__34);

  






  __pyx_tuple__35 = PyTuple_Pack(1, __pyx_kp_u_value_must_be_a_string); if (unlikely(!__pyx_tuple__35)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1139; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__35);
  __Pyx_GIVEREF(__pyx_tuple__35);

  






  __pyx_tuple__36 = PyTuple_Pack(1, __pyx_kp_s_anchor_must_be_a_string); if (unlikely(!__pyx_tuple__36)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1168; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__36);
  __Pyx_GIVEREF(__pyx_tuple__36);

  






  __pyx_tuple__37 = PyTuple_Pack(1, __pyx_kp_u_anchor_must_be_a_string); if (unlikely(!__pyx_tuple__37)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1170; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__37);
  __Pyx_GIVEREF(__pyx_tuple__37);

  






  __pyx_tuple__38 = PyTuple_Pack(1, __pyx_kp_s_tag_must_be_a_string); if (unlikely(!__pyx_tuple__38)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1179; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__38);
  __Pyx_GIVEREF(__pyx_tuple__38);

  






  __pyx_tuple__39 = PyTuple_Pack(1, __pyx_kp_u_tag_must_be_a_string); if (unlikely(!__pyx_tuple__39)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1181; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__39);
  __Pyx_GIVEREF(__pyx_tuple__39);

  






  __pyx_tuple__40 = PyTuple_Pack(1, __pyx_kp_s_anchor_must_be_a_string); if (unlikely(!__pyx_tuple__40)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1200; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__40);
  __Pyx_GIVEREF(__pyx_tuple__40);

  






  __pyx_tuple__41 = PyTuple_Pack(1, __pyx_kp_u_anchor_must_be_a_string); if (unlikely(!__pyx_tuple__41)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1202; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__41);
  __Pyx_GIVEREF(__pyx_tuple__41);

  






  __pyx_tuple__42 = PyTuple_Pack(1, __pyx_kp_s_tag_must_be_a_string); if (unlikely(!__pyx_tuple__42)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1211; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__42);
  __Pyx_GIVEREF(__pyx_tuple__42);

  






  __pyx_tuple__43 = PyTuple_Pack(1, __pyx_kp_u_tag_must_be_a_string); if (unlikely(!__pyx_tuple__43)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1213; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__43);
  __Pyx_GIVEREF(__pyx_tuple__43);

  






  __pyx_tuple__44 = PyTuple_Pack(1, __pyx_kp_s_serializer_is_closed); if (unlikely(!__pyx_tuple__44)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1263; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__44);
  __Pyx_GIVEREF(__pyx_tuple__44);

  






  __pyx_tuple__45 = PyTuple_Pack(1, __pyx_kp_u_serializer_is_closed); if (unlikely(!__pyx_tuple__45)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1265; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__45);
  __Pyx_GIVEREF(__pyx_tuple__45);

  






  __pyx_tuple__46 = PyTuple_Pack(1, __pyx_kp_s_serializer_is_already_opened); if (unlikely(!__pyx_tuple__46)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1268; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__46);
  __Pyx_GIVEREF(__pyx_tuple__46);

  






  __pyx_tuple__47 = PyTuple_Pack(1, __pyx_kp_u_serializer_is_already_opened); if (unlikely(!__pyx_tuple__47)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1270; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__47);
  __Pyx_GIVEREF(__pyx_tuple__47);

  






  __pyx_tuple__48 = PyTuple_Pack(1, __pyx_kp_s_serializer_is_not_opened); if (unlikely(!__pyx_tuple__48)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1276; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__48);
  __Pyx_GIVEREF(__pyx_tuple__48);

  






  __pyx_tuple__49 = PyTuple_Pack(1, __pyx_kp_u_serializer_is_not_opened); if (unlikely(!__pyx_tuple__49)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1278; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__49);
  __Pyx_GIVEREF(__pyx_tuple__49);

  






  __pyx_tuple__50 = PyTuple_Pack(1, __pyx_kp_s_serializer_is_not_opened); if (unlikely(!__pyx_tuple__50)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1295; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__50);
  __Pyx_GIVEREF(__pyx_tuple__50);

  






  __pyx_tuple__51 = PyTuple_Pack(1, __pyx_kp_u_serializer_is_not_opened); if (unlikely(!__pyx_tuple__51)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1297; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__51);
  __Pyx_GIVEREF(__pyx_tuple__51);

  






  __pyx_tuple__52 = PyTuple_Pack(1, __pyx_kp_s_serializer_is_closed); if (unlikely(!__pyx_tuple__52)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1300; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__52);
  __Pyx_GIVEREF(__pyx_tuple__52);

  






  __pyx_tuple__53 = PyTuple_Pack(1, __pyx_kp_u_serializer_is_closed); if (unlikely(!__pyx_tuple__53)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1302; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__53);
  __Pyx_GIVEREF(__pyx_tuple__53);

  






  __pyx_tuple__54 = PyTuple_Pack(1, __pyx_kp_s_too_many_tags); if (unlikely(!__pyx_tuple__54)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1314; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__54);
  __Pyx_GIVEREF(__pyx_tuple__54);

  






  __pyx_tuple__55 = PyTuple_Pack(1, __pyx_kp_u_too_many_tags); if (unlikely(!__pyx_tuple__55)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1316; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__55);
  __Pyx_GIVEREF(__pyx_tuple__55);

  






  __pyx_tuple__56 = PyTuple_Pack(1, __pyx_kp_s_tag_handle_must_be_a_string); if (unlikely(!__pyx_tuple__56)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1326; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__56);
  __Pyx_GIVEREF(__pyx_tuple__56);

  






  __pyx_tuple__57 = PyTuple_Pack(1, __pyx_kp_u_tag_handle_must_be_a_string); if (unlikely(!__pyx_tuple__57)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1328; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__57);
  __Pyx_GIVEREF(__pyx_tuple__57);

  






  __pyx_tuple__58 = PyTuple_Pack(1, __pyx_kp_s_tag_prefix_must_be_a_string); if (unlikely(!__pyx_tuple__58)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1335; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__58);
  __Pyx_GIVEREF(__pyx_tuple__58);

  






  __pyx_tuple__59 = PyTuple_Pack(1, __pyx_kp_u_tag_prefix_must_be_a_string); if (unlikely(!__pyx_tuple__59)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1337; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__59);
  __Pyx_GIVEREF(__pyx_tuple__59);

  






  __pyx_tuple__60 = PyTuple_Pack(1, __pyx_kp_s_anchor_must_be_a_string); if (unlikely(!__pyx_tuple__60)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1394; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__60);
  __Pyx_GIVEREF(__pyx_tuple__60);

  






  __pyx_tuple__61 = PyTuple_Pack(1, __pyx_kp_u_anchor_must_be_a_string); if (unlikely(!__pyx_tuple__61)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1396; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__61);
  __Pyx_GIVEREF(__pyx_tuple__61);

  






  __pyx_tuple__62 = PyTuple_Pack(2, Py_True, Py_False); if (unlikely(!__pyx_tuple__62)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1412; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__62);
  __Pyx_GIVEREF(__pyx_tuple__62);

  






  __pyx_tuple__63 = PyTuple_Pack(2, Py_False, Py_True); if (unlikely(!__pyx_tuple__63)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1414; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__63);
  __Pyx_GIVEREF(__pyx_tuple__63);

  






  __pyx_tuple__64 = PyTuple_Pack(1, __pyx_kp_s_tag_must_be_a_string); if (unlikely(!__pyx_tuple__64)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1422; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__64);
  __Pyx_GIVEREF(__pyx_tuple__64);

  






  __pyx_tuple__65 = PyTuple_Pack(1, __pyx_kp_u_tag_must_be_a_string); if (unlikely(!__pyx_tuple__65)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1424; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__65);
  __Pyx_GIVEREF(__pyx_tuple__65);

  






  __pyx_tuple__66 = PyTuple_Pack(1, __pyx_kp_s_value_must_be_a_string); if (unlikely(!__pyx_tuple__66)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1431; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__66);
  __Pyx_GIVEREF(__pyx_tuple__66);

  






  __pyx_tuple__67 = PyTuple_Pack(1, __pyx_kp_u_value_must_be_a_string); if (unlikely(!__pyx_tuple__67)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1433; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__67);
  __Pyx_GIVEREF(__pyx_tuple__67);

  






  __pyx_tuple__68 = PyTuple_Pack(1, __pyx_kp_s_tag_must_be_a_string); if (unlikely(!__pyx_tuple__68)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1463; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__68);
  __Pyx_GIVEREF(__pyx_tuple__68);

  






  __pyx_tuple__69 = PyTuple_Pack(1, __pyx_kp_u_tag_must_be_a_string); if (unlikely(!__pyx_tuple__69)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1465; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__69);
  __Pyx_GIVEREF(__pyx_tuple__69);

  






  __pyx_tuple__70 = PyTuple_Pack(1, __pyx_kp_s_tag_must_be_a_string); if (unlikely(!__pyx_tuple__70)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1495; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__70);
  __Pyx_GIVEREF(__pyx_tuple__70);

  






  __pyx_tuple__71 = PyTuple_Pack(1, __pyx_kp_u_tag_must_be_a_string); if (unlikely(!__pyx_tuple__71)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 1497; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__71);
  __Pyx_GIVEREF(__pyx_tuple__71);

  






  __pyx_tuple__72 = PyTuple_Pack(1, __pyx_n_s_value); if (unlikely(!__pyx_tuple__72)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 4; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__72);
  __Pyx_GIVEREF(__pyx_tuple__72);
  __pyx_codeobj__73 = (PyObject*)__Pyx_PyCode_New(0, 0, 1, 0, 0, __pyx_empty_bytes, __pyx_empty_tuple, __pyx_empty_tuple, __pyx_tuple__72, __pyx_empty_tuple, __pyx_empty_tuple, __pyx_kp_s_root_src_pyyaml_ext__yaml_pyx, __pyx_n_s_get_version_string, 4, __pyx_empty_bytes); if (unlikely(!__pyx_codeobj__73)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 4; __pyx_clineno = __LINE__; goto __pyx_L1_error;}

  






  __pyx_tuple__74 = PyTuple_Pack(3, __pyx_n_s_major, __pyx_n_s_minor, __pyx_n_s_patch); if (unlikely(!__pyx_tuple__74)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 12; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_tuple__74);
  __Pyx_GIVEREF(__pyx_tuple__74);
  __pyx_codeobj__75 = (PyObject*)__Pyx_PyCode_New(0, 0, 3, 0, 0, __pyx_empty_bytes, __pyx_empty_tuple, __pyx_empty_tuple, __pyx_tuple__74, __pyx_empty_tuple, __pyx_empty_tuple, __pyx_kp_s_root_src_pyyaml_ext__yaml_pyx, __pyx_n_s_get_version, 12, __pyx_empty_bytes); if (unlikely(!__pyx_codeobj__75)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 12; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_RefNannyFinishContext();
  return 0;
  __pyx_L1_error:;
  __Pyx_RefNannyFinishContext();
  return -1;
}

static int __Pyx_InitGlobals(void) {
  if (__Pyx_InitStrings(__pyx_string_tab) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
  __pyx_int_0 = PyInt_FromLong(0); if (unlikely(!__pyx_int_0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_int_1 = PyInt_FromLong(1); if (unlikely(!__pyx_int_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  return 0;
  __pyx_L1_error:;
  return -1;
}

#if PY_MAJOR_VERSION < 3
PyMODINIT_FUNC init_yaml(void); 
PyMODINIT_FUNC init_yaml(void)
#else
PyMODINIT_FUNC PyInit__yaml(void); 
PyMODINIT_FUNC PyInit__yaml(void)
#endif
{
  PyObject *__pyx_t_1 = NULL;
  PyObject *__pyx_t_2 = NULL;
  int __pyx_lineno = 0;
  const char *__pyx_filename = NULL;
  int __pyx_clineno = 0;
  __Pyx_RefNannyDeclarations
  #if CYTHON_REFNANNY
  __Pyx_RefNanny = __Pyx_RefNannyImportAPI("refnanny");
  if (!__Pyx_RefNanny) {
      PyErr_Clear();
      __Pyx_RefNanny = __Pyx_RefNannyImportAPI("Cython.Runtime.refnanny");
      if (!__Pyx_RefNanny)
          Py_FatalError("failed to import 'refnanny' module");
  }
  #endif
  __Pyx_RefNannySetupContext("PyMODINIT_FUNC PyInit__yaml(void)", 0);
  if ( __Pyx_check_binary_version() < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_empty_tuple = PyTuple_New(0); if (unlikely(!__pyx_empty_tuple)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_empty_bytes = PyBytes_FromStringAndSize("", 0); if (unlikely(!__pyx_empty_bytes)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  #ifdef __Pyx_CyFunction_USED
  if (__Pyx_CyFunction_init() < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  #endif
  #ifdef __Pyx_FusedFunction_USED
  if (__pyx_FusedFunction_init() < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  #endif
  #ifdef __Pyx_Generator_USED
  if (__pyx_Generator_init() < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  #endif
  
  
  #if defined(__PYX_FORCE_INIT_THREADS) && __PYX_FORCE_INIT_THREADS
  #ifdef WITH_THREAD 
  PyEval_InitThreads();
  #endif
  #endif
  
  #if PY_MAJOR_VERSION < 3
  __pyx_m = Py_InitModule4(__Pyx_NAMESTR("_yaml"), __pyx_methods, 0, 0, PYTHON_API_VERSION); Py_XINCREF(__pyx_m);
  #else
  __pyx_m = PyModule_Create(&__pyx_moduledef);
  #endif
  if (unlikely(!__pyx_m)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_d = PyModule_GetDict(__pyx_m); if (unlikely(!__pyx_d)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  Py_INCREF(__pyx_d);
  __pyx_b = PyImport_AddModule(__Pyx_NAMESTR(__Pyx_BUILTIN_MODULE_NAME)); if (unlikely(!__pyx_b)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  #if CYTHON_COMPILING_IN_PYPY
  Py_INCREF(__pyx_b);
  #endif
  if (__Pyx_SetAttrString(__pyx_m, "__builtins__", __pyx_b) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
  
  if (unlikely(__Pyx_InitGlobals() < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  #if PY_MAJOR_VERSION < 3 && (__PYX_DEFAULT_STRING_ENCODING_IS_ASCII || __PYX_DEFAULT_STRING_ENCODING_IS_DEFAULT)
  if (__Pyx_init_sys_getdefaultencoding_params() < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  #endif
  if (__pyx_module_is_main__yaml) {
    if (__Pyx_SetAttrString(__pyx_m, "__name__", __pyx_n_s_main) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;};
  }
  #if PY_MAJOR_VERSION >= 3
  {
    PyObject *modules = PyImport_GetModuleDict(); if (unlikely(!modules)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    if (!PyDict_GetItemString(modules, "_yaml")) {
      if (unlikely(PyDict_SetItemString(modules, "_yaml", __pyx_m) < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
    }
  }
  #endif
  
  if (unlikely(__Pyx_InitCachedBuiltins() < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  
  if (unlikely(__Pyx_InitCachedConstants() < 0)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  
  
  
  
  if (PyType_Ready(&__pyx_type_5_yaml_Mark) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 64; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_type_5_yaml_Mark.tp_print = 0;
  if (__Pyx_SetAttrString(__pyx_m, "Mark", (PyObject *)&__pyx_type_5_yaml_Mark) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 64; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_ptype_5_yaml_Mark = &__pyx_type_5_yaml_Mark;
  __pyx_vtabptr_5_yaml_CParser = &__pyx_vtable_5_yaml_CParser;
  __pyx_vtable_5_yaml_CParser._parser_error = (PyObject *(*)(struct __pyx_obj_5_yaml_CParser *))__pyx_f_5_yaml_7CParser__parser_error;
  __pyx_vtable_5_yaml_CParser._scan = (PyObject *(*)(struct __pyx_obj_5_yaml_CParser *))__pyx_f_5_yaml_7CParser__scan;
  __pyx_vtable_5_yaml_CParser._token_to_object = (PyObject *(*)(struct __pyx_obj_5_yaml_CParser *, yaml_token_t *))__pyx_f_5_yaml_7CParser__token_to_object;
  __pyx_vtable_5_yaml_CParser._parse = (PyObject *(*)(struct __pyx_obj_5_yaml_CParser *))__pyx_f_5_yaml_7CParser__parse;
  __pyx_vtable_5_yaml_CParser._event_to_object = (PyObject *(*)(struct __pyx_obj_5_yaml_CParser *, yaml_event_t *))__pyx_f_5_yaml_7CParser__event_to_object;
  __pyx_vtable_5_yaml_CParser._compose_document = (PyObject *(*)(struct __pyx_obj_5_yaml_CParser *))__pyx_f_5_yaml_7CParser__compose_document;
  __pyx_vtable_5_yaml_CParser._compose_node = (PyObject *(*)(struct __pyx_obj_5_yaml_CParser *, PyObject *, PyObject *))__pyx_f_5_yaml_7CParser__compose_node;
  __pyx_vtable_5_yaml_CParser._compose_scalar_node = (PyObject *(*)(struct __pyx_obj_5_yaml_CParser *, PyObject *))__pyx_f_5_yaml_7CParser__compose_scalar_node;
  __pyx_vtable_5_yaml_CParser._compose_sequence_node = (PyObject *(*)(struct __pyx_obj_5_yaml_CParser *, PyObject *))__pyx_f_5_yaml_7CParser__compose_sequence_node;
  __pyx_vtable_5_yaml_CParser._compose_mapping_node = (PyObject *(*)(struct __pyx_obj_5_yaml_CParser *, PyObject *))__pyx_f_5_yaml_7CParser__compose_mapping_node;
  __pyx_vtable_5_yaml_CParser._parse_next_event = (int (*)(struct __pyx_obj_5_yaml_CParser *))__pyx_f_5_yaml_7CParser__parse_next_event;
  if (PyType_Ready(&__pyx_type_5_yaml_CParser) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 247; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_type_5_yaml_CParser.tp_print = 0;
  if (__Pyx_SetVtable(__pyx_type_5_yaml_CParser.tp_dict, __pyx_vtabptr_5_yaml_CParser) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 247; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  if (__Pyx_SetAttrString(__pyx_m, "CParser", (PyObject *)&__pyx_type_5_yaml_CParser) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 247; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_ptype_5_yaml_CParser = &__pyx_type_5_yaml_CParser;
  __pyx_vtabptr_5_yaml_CEmitter = &__pyx_vtable_5_yaml_CEmitter;
  __pyx_vtable_5_yaml_CEmitter._emitter_error = (PyObject *(*)(struct __pyx_obj_5_yaml_CEmitter *))__pyx_f_5_yaml_8CEmitter__emitter_error;
  __pyx_vtable_5_yaml_CEmitter._object_to_event = (int (*)(struct __pyx_obj_5_yaml_CEmitter *, PyObject *, yaml_event_t *))__pyx_f_5_yaml_8CEmitter__object_to_event;
  __pyx_vtable_5_yaml_CEmitter._anchor_node = (int (*)(struct __pyx_obj_5_yaml_CEmitter *, PyObject *))__pyx_f_5_yaml_8CEmitter__anchor_node;
  __pyx_vtable_5_yaml_CEmitter._serialize_node = (int (*)(struct __pyx_obj_5_yaml_CEmitter *, PyObject *, PyObject *, PyObject *))__pyx_f_5_yaml_8CEmitter__serialize_node;
  if (PyType_Ready(&__pyx_type_5_yaml_CEmitter) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 935; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_type_5_yaml_CEmitter.tp_print = 0;
  if (__Pyx_SetVtable(__pyx_type_5_yaml_CEmitter.tp_dict, __pyx_vtabptr_5_yaml_CEmitter) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 935; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  if (__Pyx_SetAttrString(__pyx_m, "CEmitter", (PyObject *)&__pyx_type_5_yaml_CEmitter) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 935; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __pyx_ptype_5_yaml_CEmitter = &__pyx_type_5_yaml_CEmitter;
  
  
  
  

  





  __pyx_t_1 = __Pyx_Import(__pyx_n_s_yaml, 0, -1); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_yaml, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = PyCFunction_NewEx(&__pyx_mdef_5_yaml_1get_version_string, NULL, __pyx_n_s_yaml_2); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 4; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_get_version_string, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 4; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = PyCFunction_NewEx(&__pyx_mdef_5_yaml_3get_version, NULL, __pyx_n_s_yaml_2); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 12; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_get_version, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 12; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 18; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_error); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 18; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_YAMLError); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 18; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_YAMLError, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 18; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 19; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_reader); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 19; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_ReaderError); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 19; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_ReaderError, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 19; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 20; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_scanner); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 20; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_ScannerError); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 20; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_ScannerError, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 20; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 21; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_parser); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 21; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_ParserError); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 21; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_ParserError, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 21; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 22; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_composer); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 22; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_ComposerError); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 22; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_ComposerError, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 22; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 23; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_constructor); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 23; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_ConstructorError); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 23; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_ConstructorError, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 23; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 24; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_emitter); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 24; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_EmitterError); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 24; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_EmitterError, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 24; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 25; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_serializer); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 25; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_SerializerError); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 25; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_SerializerError, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 25; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 26; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_representer); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 26; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_RepresenterError); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 26; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_RepresenterError, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 26; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 28; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 28; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_StreamStartToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 28; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_StreamStartToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 28; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 29; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 29; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_StreamEndToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 29; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_StreamEndToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 29; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 30; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 30; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_DirectiveToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 30; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_DirectiveToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 30; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 31; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 31; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_DocumentStartToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 31; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_DocumentStartToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 31; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 32; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 32; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_DocumentEndToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 32; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_DocumentEndToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 32; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 33; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 33; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_BlockSequenceStartToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 33; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_BlockSequenceStartToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 33; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 34; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 34; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_BlockMappingStartToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 34; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_BlockMappingStartToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 34; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 35; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 35; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_BlockEndToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 35; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_BlockEndToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 35; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 36; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 36; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_FlowSequenceStartToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 36; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_FlowSequenceStartToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 36; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 37; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 37; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_FlowMappingStartToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 37; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_FlowMappingStartToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 37; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 38; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 38; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_FlowSequenceEndToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 38; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_FlowSequenceEndToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 38; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 39; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 39; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_FlowMappingEndToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 39; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_FlowMappingEndToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 39; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 40; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 40; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_KeyToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 40; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_KeyToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 40; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 41; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 41; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_ValueToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 41; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_ValueToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 41; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 42; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 42; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_BlockEntryToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 42; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_BlockEntryToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 42; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 43; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 43; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_FlowEntryToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 43; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_FlowEntryToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 43; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 44; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 44; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_AliasToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 44; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_AliasToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 44; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 45; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 45; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_AnchorToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 45; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_AnchorToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 45; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 46; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 46; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_TagToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 46; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_TagToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 46; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 47; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_tokens); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 47; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_ScalarToken); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 47; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_ScalarToken, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 47; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 49; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_events); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 49; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_StreamStartEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 49; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_StreamStartEvent, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 49; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 50; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_events); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 50; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_StreamEndEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 50; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_StreamEndEvent, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 50; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 51; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_events); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 51; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_DocumentStartEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 51; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_DocumentStartEvent, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 51; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 52; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_events); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 52; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_DocumentEndEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 52; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_DocumentEndEvent, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 52; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 53; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_events); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 53; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_AliasEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 53; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_AliasEvent, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 53; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 54; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_events); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 54; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_ScalarEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 54; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_ScalarEvent, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 54; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 55; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_events); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 55; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_SequenceStartEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 55; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_SequenceStartEvent, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 55; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 56; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_events); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 56; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_SequenceEndEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 56; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_SequenceEndEvent, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 56; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 57; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_events); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 57; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_MappingStartEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 57; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_MappingStartEvent, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 57; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 58; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_events); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 58; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_MappingEndEvent); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 58; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_MappingEndEvent, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 58; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 60; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_nodes); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 60; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_ScalarNode); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 60; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_ScalarNode, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 60; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 61; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_nodes); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 61; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_SequenceNode); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 61; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_SequenceNode, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 61; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  






  __pyx_t_1 = __Pyx_GetModuleGlobalName(__pyx_n_s_yaml); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 62; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __pyx_t_2 = __Pyx_PyObject_GetAttrStr(__pyx_t_1, __pyx_n_s_nodes); if (unlikely(!__pyx_t_2)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 62; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_2);
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  __pyx_t_1 = __Pyx_PyObject_GetAttrStr(__pyx_t_2, __pyx_n_s_MappingNode); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 62; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  __Pyx_DECREF(__pyx_t_2); __pyx_t_2 = 0;
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_MappingNode, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 62; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;

  





  __pyx_t_1 = PyDict_New(); if (unlikely(!__pyx_t_1)) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_GOTREF(__pyx_t_1);
  if (PyDict_SetItem(__pyx_d, __pyx_n_s_test, __pyx_t_1) < 0) {__pyx_filename = __pyx_f[0]; __pyx_lineno = 2; __pyx_clineno = __LINE__; goto __pyx_L1_error;}
  __Pyx_DECREF(__pyx_t_1); __pyx_t_1 = 0;
  goto __pyx_L0;
  __pyx_L1_error:;
  __Pyx_XDECREF(__pyx_t_1);
  __Pyx_XDECREF(__pyx_t_2);
  if (__pyx_m) {
    __Pyx_AddTraceback("init _yaml", __pyx_clineno, __pyx_lineno, __pyx_filename);
    Py_DECREF(__pyx_m); __pyx_m = 0;
  } else if (!PyErr_Occurred()) {
    PyErr_SetString(PyExc_ImportError, "init _yaml");
  }
  __pyx_L0:;
  __Pyx_RefNannyFinishContext();
  #if PY_MAJOR_VERSION < 3
  return;
  #else
  return __pyx_m;
  #endif
}


#if CYTHON_REFNANNY
static __Pyx_RefNannyAPIStruct *__Pyx_RefNannyImportAPI(const char *modname) {
    PyObject *m = NULL, *p = NULL;
    void *r = NULL;
    m = PyImport_ImportModule((char *)modname);
    if (!m) goto end;
    p = PyObject_GetAttrString(m, (char *)"RefNannyAPI");
    if (!p) goto end;
    r = PyLong_AsVoidPtr(p);
end:
    Py_XDECREF(p);
    Py_XDECREF(m);
    return (__Pyx_RefNannyAPIStruct *)r;
}
#endif 

static PyObject *__Pyx_GetBuiltinName(PyObject *name) {
    PyObject* result = __Pyx_PyObject_GetAttrStr(__pyx_b, name);
    if (unlikely(!result)) {
        PyErr_Format(PyExc_NameError,
#if PY_MAJOR_VERSION >= 3
            "name '%U' is not defined", name);
#else
            "name '%.200s' is not defined", PyString_AS_STRING(name));
#endif
    }
    return result;
}

static void __Pyx_RaiseArgtupleInvalid(
    const char* func_name,
    int exact,
    Py_ssize_t num_min,
    Py_ssize_t num_max,
    Py_ssize_t num_found)
{
    Py_ssize_t num_expected;
    const char *more_or_less;
    if (num_found < num_min) {
        num_expected = num_min;
        more_or_less = "at least";
    } else {
        num_expected = num_max;
        more_or_less = "at most";
    }
    if (exact) {
        more_or_less = "exactly";
    }
    PyErr_Format(PyExc_TypeError,
                 "%.200s() takes %.8s %" CYTHON_FORMAT_SSIZE_T "d positional argument%.1s (%" CYTHON_FORMAT_SSIZE_T "d given)",
                 func_name, more_or_less, num_expected,
                 (num_expected == 1) ? "" : "s", num_found);
}

static void __Pyx_RaiseDoubleKeywordsError(
    const char* func_name,
    PyObject* kw_name)
{
    PyErr_Format(PyExc_TypeError,
        #if PY_MAJOR_VERSION >= 3
        "%s() got multiple values for keyword argument '%U'", func_name, kw_name);
        #else
        "%s() got multiple values for keyword argument '%s'", func_name,
        PyString_AsString(kw_name));
        #endif
}

static int __Pyx_ParseOptionalKeywords(
    PyObject *kwds,
    PyObject **argnames[],
    PyObject *kwds2,
    PyObject *values[],
    Py_ssize_t num_pos_args,
    const char* function_name)
{
    PyObject *key = 0, *value = 0;
    Py_ssize_t pos = 0;
    PyObject*** name;
    PyObject*** first_kw_arg = argnames + num_pos_args;
    while (PyDict_Next(kwds, &pos, &key, &value)) {
        name = first_kw_arg;
        while (*name && (**name != key)) name++;
        if (*name) {
            values[name-argnames] = value;
            continue;
        }
        name = first_kw_arg;
        #if PY_MAJOR_VERSION < 3
        if (likely(PyString_CheckExact(key)) || likely(PyString_Check(key))) {
            while (*name) {
                if ((CYTHON_COMPILING_IN_PYPY || PyString_GET_SIZE(**name) == PyString_GET_SIZE(key))
                        && _PyString_Eq(**name, key)) {
                    values[name-argnames] = value;
                    break;
                }
                name++;
            }
            if (*name) continue;
            else {
                PyObject*** argname = argnames;
                while (argname != first_kw_arg) {
                    if ((**argname == key) || (
                            (CYTHON_COMPILING_IN_PYPY || PyString_GET_SIZE(**argname) == PyString_GET_SIZE(key))
                             && _PyString_Eq(**argname, key))) {
                        goto arg_passed_twice;
                    }
                    argname++;
                }
            }
        } else
        #endif
        if (likely(PyUnicode_Check(key))) {
            while (*name) {
                int cmp = (**name == key) ? 0 :
                #if !CYTHON_COMPILING_IN_PYPY && PY_MAJOR_VERSION >= 3
                    (PyUnicode_GET_SIZE(**name) != PyUnicode_GET_SIZE(key)) ? 1 :
                #endif
                    PyUnicode_Compare(**name, key);
                if (cmp < 0 && unlikely(PyErr_Occurred())) goto bad;
                if (cmp == 0) {
                    values[name-argnames] = value;
                    break;
                }
                name++;
            }
            if (*name) continue;
            else {
                PyObject*** argname = argnames;
                while (argname != first_kw_arg) {
                    int cmp = (**argname == key) ? 0 :
                    #if !CYTHON_COMPILING_IN_PYPY && PY_MAJOR_VERSION >= 3
                        (PyUnicode_GET_SIZE(**argname) != PyUnicode_GET_SIZE(key)) ? 1 :
                    #endif
                        PyUnicode_Compare(**argname, key);
                    if (cmp < 0 && unlikely(PyErr_Occurred())) goto bad;
                    if (cmp == 0) goto arg_passed_twice;
                    argname++;
                }
            }
        } else
            goto invalid_keyword_type;
        if (kwds2) {
            if (unlikely(PyDict_SetItem(kwds2, key, value))) goto bad;
        } else {
            goto invalid_keyword;
        }
    }
    return 0;
arg_passed_twice:
    __Pyx_RaiseDoubleKeywordsError(function_name, key);
    goto bad;
invalid_keyword_type:
    PyErr_Format(PyExc_TypeError,
        "%.200s() keywords must be strings", function_name);
    goto bad;
invalid_keyword:
    PyErr_Format(PyExc_TypeError,
    #if PY_MAJOR_VERSION < 3
        "%.200s() got an unexpected keyword argument '%.200s'",
        function_name, PyString_AsString(key));
    #else
        "%s() got an unexpected keyword argument '%U'",
        function_name, key);
    #endif
bad:
    return -1;
}

static CYTHON_INLINE void __Pyx_ExceptionSave(PyObject **type, PyObject **value, PyObject **tb) {
#if CYTHON_COMPILING_IN_CPYTHON
    PyThreadState *tstate = PyThreadState_GET();
    *type = tstate->exc_type;
    *value = tstate->exc_value;
    *tb = tstate->exc_traceback;
    Py_XINCREF(*type);
    Py_XINCREF(*value);
    Py_XINCREF(*tb);
#else
    PyErr_GetExcInfo(type, value, tb);
#endif
}
static void __Pyx_ExceptionReset(PyObject *type, PyObject *value, PyObject *tb) {
#if CYTHON_COMPILING_IN_CPYTHON
    PyObject *tmp_type, *tmp_value, *tmp_tb;
    PyThreadState *tstate = PyThreadState_GET();
    tmp_type = tstate->exc_type;
    tmp_value = tstate->exc_value;
    tmp_tb = tstate->exc_traceback;
    tstate->exc_type = type;
    tstate->exc_value = value;
    tstate->exc_traceback = tb;
    Py_XDECREF(tmp_type);
    Py_XDECREF(tmp_value);
    Py_XDECREF(tmp_tb);
#else
    PyErr_SetExcInfo(type, value, tb);
#endif
}

static int __Pyx_GetException(PyObject **type, PyObject **value, PyObject **tb) {
    PyObject *local_type, *local_value, *local_tb;
#if CYTHON_COMPILING_IN_CPYTHON
    PyObject *tmp_type, *tmp_value, *tmp_tb;
    PyThreadState *tstate = PyThreadState_GET();
    local_type = tstate->curexc_type;
    local_value = tstate->curexc_value;
    local_tb = tstate->curexc_traceback;
    tstate->curexc_type = 0;
    tstate->curexc_value = 0;
    tstate->curexc_traceback = 0;
#else
    PyErr_Fetch(&local_type, &local_value, &local_tb);
#endif
    PyErr_NormalizeException(&local_type, &local_value, &local_tb);
#if CYTHON_COMPILING_IN_CPYTHON
    if (unlikely(tstate->curexc_type))
#else
    if (unlikely(PyErr_Occurred()))
#endif
        goto bad;
    #if PY_MAJOR_VERSION >= 3
    if (local_tb) {
        if (unlikely(PyException_SetTraceback(local_value, local_tb) < 0))
            goto bad;
    }
    #endif
    Py_XINCREF(local_tb);
    Py_XINCREF(local_type);
    Py_XINCREF(local_value);
    *type = local_type;
    *value = local_value;
    *tb = local_tb;
#if CYTHON_COMPILING_IN_CPYTHON
    tmp_type = tstate->exc_type;
    tmp_value = tstate->exc_value;
    tmp_tb = tstate->exc_traceback;
    tstate->exc_type = local_type;
    tstate->exc_value = local_value;
    tstate->exc_traceback = local_tb;
    Py_XDECREF(tmp_type);
    Py_XDECREF(tmp_value);
    Py_XDECREF(tmp_tb);
#else
    PyErr_SetExcInfo(local_type, local_value, local_tb);
#endif
    return 0;
bad:
    *type = 0;
    *value = 0;
    *tb = 0;
    Py_XDECREF(local_type);
    Py_XDECREF(local_value);
    Py_XDECREF(local_tb);
    return -1;
}

#if CYTHON_COMPILING_IN_CPYTHON
static CYTHON_INLINE PyObject* __Pyx_PyObject_Call(PyObject *func, PyObject *arg, PyObject *kw) {
    PyObject *result;
    ternaryfunc call = func->ob_type->tp_call;
    if (unlikely(!call))
        return PyObject_Call(func, arg, kw);
#if PY_VERSION_HEX >= 0x02060000
    if (unlikely(Py_EnterRecursiveCall((char*)" while calling a Python object")))
        return NULL;
#endif
    result = (*call)(func, arg, kw);
#if PY_VERSION_HEX >= 0x02060000
    Py_LeaveRecursiveCall();
#endif
    if (unlikely(!result) && unlikely(!PyErr_Occurred())) {
        PyErr_SetString(
            PyExc_SystemError,
            "NULL result without error in PyObject_Call");
    }
    return result;
}
#endif

static CYTHON_INLINE void __Pyx_ErrRestore(PyObject *type, PyObject *value, PyObject *tb) {
#if CYTHON_COMPILING_IN_CPYTHON
    PyObject *tmp_type, *tmp_value, *tmp_tb;
    PyThreadState *tstate = PyThreadState_GET();
    tmp_type = tstate->curexc_type;
    tmp_value = tstate->curexc_value;
    tmp_tb = tstate->curexc_traceback;
    tstate->curexc_type = type;
    tstate->curexc_value = value;
    tstate->curexc_traceback = tb;
    Py_XDECREF(tmp_type);
    Py_XDECREF(tmp_value);
    Py_XDECREF(tmp_tb);
#else
    PyErr_Restore(type, value, tb);
#endif
}
static CYTHON_INLINE void __Pyx_ErrFetch(PyObject **type, PyObject **value, PyObject **tb) {
#if CYTHON_COMPILING_IN_CPYTHON
    PyThreadState *tstate = PyThreadState_GET();
    *type = tstate->curexc_type;
    *value = tstate->curexc_value;
    *tb = tstate->curexc_traceback;
    tstate->curexc_type = 0;
    tstate->curexc_value = 0;
    tstate->curexc_traceback = 0;
#else
    PyErr_Fetch(type, value, tb);
#endif
}

#if PY_MAJOR_VERSION < 3
static void __Pyx_Raise(PyObject *type, PyObject *value, PyObject *tb,
                        CYTHON_UNUSED PyObject *cause) {
    Py_XINCREF(type);
    if (!value || value == Py_None)
        value = NULL;
    else
        Py_INCREF(value);
    if (!tb || tb == Py_None)
        tb = NULL;
    else {
        Py_INCREF(tb);
        if (!PyTraceBack_Check(tb)) {
            PyErr_SetString(PyExc_TypeError,
                "raise: arg 3 must be a traceback or None");
            goto raise_error;
        }
    }
    #if PY_VERSION_HEX < 0x02050000
    if (PyClass_Check(type)) {
    #else
    if (PyType_Check(type)) {
    #endif
#if CYTHON_COMPILING_IN_PYPY
        if (!value) {
            Py_INCREF(Py_None);
            value = Py_None;
        }
#endif
        PyErr_NormalizeException(&type, &value, &tb);
    } else {
        if (value) {
            PyErr_SetString(PyExc_TypeError,
                "instance exception may not have a separate value");
            goto raise_error;
        }
        value = type;
        #if PY_VERSION_HEX < 0x02050000
        if (PyInstance_Check(type)) {
            type = (PyObject*) ((PyInstanceObject*)type)->in_class;
            Py_INCREF(type);
        } else {
            type = 0;
            PyErr_SetString(PyExc_TypeError,
                "raise: exception must be an old-style class or instance");
            goto raise_error;
        }
        #else
        type = (PyObject*) Py_TYPE(type);
        Py_INCREF(type);
        if (!PyType_IsSubtype((PyTypeObject *)type, (PyTypeObject *)PyExc_BaseException)) {
            PyErr_SetString(PyExc_TypeError,
                "raise: exception class must be a subclass of BaseException");
            goto raise_error;
        }
        #endif
    }
    __Pyx_ErrRestore(type, value, tb);
    return;
raise_error:
    Py_XDECREF(value);
    Py_XDECREF(type);
    Py_XDECREF(tb);
    return;
}
#else 
static void __Pyx_Raise(PyObject *type, PyObject *value, PyObject *tb, PyObject *cause) {
    PyObject* owned_instance = NULL;
    if (tb == Py_None) {
        tb = 0;
    } else if (tb && !PyTraceBack_Check(tb)) {
        PyErr_SetString(PyExc_TypeError,
            "raise: arg 3 must be a traceback or None");
        goto bad;
    }
    if (value == Py_None)
        value = 0;
    if (PyExceptionInstance_Check(type)) {
        if (value) {
            PyErr_SetString(PyExc_TypeError,
                "instance exception may not have a separate value");
            goto bad;
        }
        value = type;
        type = (PyObject*) Py_TYPE(value);
    } else if (PyExceptionClass_Check(type)) {
        PyObject *instance_class = NULL;
        if (value && PyExceptionInstance_Check(value)) {
            instance_class = (PyObject*) Py_TYPE(value);
            if (instance_class != type) {
                if (PyObject_IsSubclass(instance_class, type)) {
                    type = instance_class;
                } else {
                    instance_class = NULL;
                }
            }
        }
        if (!instance_class) {
            PyObject *args;
            if (!value)
                args = PyTuple_New(0);
            else if (PyTuple_Check(value)) {
                Py_INCREF(value);
                args = value;
            } else
                args = PyTuple_Pack(1, value);
            if (!args)
                goto bad;
            owned_instance = PyObject_Call(type, args, NULL);
            Py_DECREF(args);
            if (!owned_instance)
                goto bad;
            value = owned_instance;
            if (!PyExceptionInstance_Check(value)) {
                PyErr_Format(PyExc_TypeError,
                             "calling %R should have returned an instance of "
                             "BaseException, not %R",
                             type, Py_TYPE(value));
                goto bad;
            }
        }
    } else {
        PyErr_SetString(PyExc_TypeError,
            "raise: exception class must be a subclass of BaseException");
        goto bad;
    }
#if PY_VERSION_HEX >= 0x03030000
    if (cause) {
#else
    if (cause && cause != Py_None) {
#endif
        PyObject *fixed_cause;
        if (cause == Py_None) {
            fixed_cause = NULL;
        } else if (PyExceptionClass_Check(cause)) {
            fixed_cause = PyObject_CallObject(cause, NULL);
            if (fixed_cause == NULL)
                goto bad;
        } else if (PyExceptionInstance_Check(cause)) {
            fixed_cause = cause;
            Py_INCREF(fixed_cause);
        } else {
            PyErr_SetString(PyExc_TypeError,
                            "exception causes must derive from "
                            "BaseException");
            goto bad;
        }
        PyException_SetCause(value, fixed_cause);
    }
    PyErr_SetObject(type, value);
    if (tb) {
        PyThreadState *tstate = PyThreadState_GET();
        PyObject* tmp_tb = tstate->curexc_traceback;
        if (tb != tmp_tb) {
            Py_INCREF(tb);
            tstate->curexc_traceback = tb;
            Py_XDECREF(tmp_tb);
        }
    }
bad:
    Py_XDECREF(owned_instance);
    return;
}
#endif

static CYTHON_INLINE PyObject *__Pyx_GetModuleGlobalName(PyObject *name) {
    PyObject *result;
#if CYTHON_COMPILING_IN_CPYTHON
    result = PyDict_GetItem(__pyx_d, name);
    if (result) {
        Py_INCREF(result);
    } else {
#else
    result = PyObject_GetItem(__pyx_d, name);
    if (!result) {
        PyErr_Clear();
#endif
        result = __Pyx_GetBuiltinName(name);
    }
    return result;
}

static CYTHON_INLINE int __Pyx_CheckKeywordStrings(
    PyObject *kwdict,
    const char* function_name,
    int kw_allowed)
{
    PyObject* key = 0;
    Py_ssize_t pos = 0;
#if CPYTHON_COMPILING_IN_PYPY
    if (!kw_allowed && PyDict_Next(kwdict, &pos, &key, 0))
        goto invalid_keyword;
    return 1;
#else
    while (PyDict_Next(kwdict, &pos, &key, 0)) {
        #if PY_MAJOR_VERSION < 3
        if (unlikely(!PyString_CheckExact(key)) && unlikely(!PyString_Check(key)))
        #endif
            if (unlikely(!PyUnicode_Check(key)))
                goto invalid_keyword_type;
    }
    if ((!kw_allowed) && unlikely(key))
        goto invalid_keyword;
    return 1;
invalid_keyword_type:
    PyErr_Format(PyExc_TypeError,
        "%.200s() keywords must be strings", function_name);
    return 0;
#endif
invalid_keyword:
    PyErr_Format(PyExc_TypeError,
    #if PY_MAJOR_VERSION < 3
        "%.200s() got an unexpected keyword argument '%.200s'",
        function_name, PyString_AsString(key));
    #else
        "%s() got an unexpected keyword argument '%U'",
        function_name, key);
    #endif
    return 0;
}

static CYTHON_INLINE void __Pyx_RaiseUnboundLocalError(const char *varname) {
    PyErr_Format(PyExc_UnboundLocalError, "local variable '%s' referenced before assignment", varname);
}

static CYTHON_INLINE PyObject *__Pyx_GetAttr(PyObject *o, PyObject *n) {
#if CYTHON_COMPILING_IN_CPYTHON
#if PY_MAJOR_VERSION >= 3
    if (likely(PyUnicode_Check(n)))
#else
    if (likely(PyString_Check(n)))
#endif
        return __Pyx_PyObject_GetAttrStr(o, n);
#endif
    return PyObject_GetAttr(o, n);
}

static CYTHON_INLINE PyObject *__Pyx_GetAttr3(PyObject *o, PyObject *n, PyObject *d) {
    PyObject *r = __Pyx_GetAttr(o, n);
    if (unlikely(!r)) {
        if (!PyErr_ExceptionMatches(PyExc_AttributeError))
            goto bad;
        PyErr_Clear();
        r = d;
        Py_INCREF(d);
    }
    return r;
bad:
    return NULL;
}

static CYTHON_INLINE int __Pyx_PyBytes_Equals(PyObject* s1, PyObject* s2, int equals) {
#if CYTHON_COMPILING_IN_PYPY
    return PyObject_RichCompareBool(s1, s2, equals);
#else
    if (s1 == s2) {
        return (equals == Py_EQ);
    } else if (PyBytes_CheckExact(s1) & PyBytes_CheckExact(s2)) {
        const char *ps1, *ps2;
        Py_ssize_t length = PyBytes_GET_SIZE(s1);
        if (length != PyBytes_GET_SIZE(s2))
            return (equals == Py_NE);
        ps1 = PyBytes_AS_STRING(s1);
        ps2 = PyBytes_AS_STRING(s2);
        if (ps1[0] != ps2[0]) {
            return (equals == Py_NE);
        } else if (length == 1) {
            return (equals == Py_EQ);
        } else {
            int result = memcmp(ps1, ps2, (size_t)length);
            return (equals == Py_EQ) ? (result == 0) : (result != 0);
        }
    } else if ((s1 == Py_None) & PyBytes_CheckExact(s2)) {
        return (equals == Py_NE);
    } else if ((s2 == Py_None) & PyBytes_CheckExact(s1)) {
        return (equals == Py_NE);
    } else {
        int result;
        PyObject* py_result = PyObject_RichCompare(s1, s2, equals);
        if (!py_result)
            return -1;
        result = __Pyx_PyObject_IsTrue(py_result);
        Py_DECREF(py_result);
        return result;
    }
#endif
}

static CYTHON_INLINE int __Pyx_PyUnicode_Equals(PyObject* s1, PyObject* s2, int equals) {
#if CYTHON_COMPILING_IN_PYPY
    return PyObject_RichCompareBool(s1, s2, equals);
#else
#if PY_MAJOR_VERSION < 3
    PyObject* owned_ref = NULL;
#endif
    int s1_is_unicode, s2_is_unicode;
    if (s1 == s2) {
        goto return_eq;
    }
    s1_is_unicode = PyUnicode_CheckExact(s1);
    s2_is_unicode = PyUnicode_CheckExact(s2);
#if PY_MAJOR_VERSION < 3
    if ((s1_is_unicode & (!s2_is_unicode)) && PyString_CheckExact(s2)) {
        owned_ref = PyUnicode_FromObject(s2);
        if (unlikely(!owned_ref))
            return -1;
        s2 = owned_ref;
        s2_is_unicode = 1;
    } else if ((s2_is_unicode & (!s1_is_unicode)) && PyString_CheckExact(s1)) {
        owned_ref = PyUnicode_FromObject(s1);
        if (unlikely(!owned_ref))
            return -1;
        s1 = owned_ref;
        s1_is_unicode = 1;
    } else if (((!s2_is_unicode) & (!s1_is_unicode))) {
        return __Pyx_PyBytes_Equals(s1, s2, equals);
    }
#endif
    if (s1_is_unicode & s2_is_unicode) {
        Py_ssize_t length;
        int kind;
        void *data1, *data2;
        #if CYTHON_PEP393_ENABLED
        if (unlikely(PyUnicode_READY(s1) < 0) || unlikely(PyUnicode_READY(s2) < 0))
            return -1;
        #endif
        length = __Pyx_PyUnicode_GET_LENGTH(s1);
        if (length != __Pyx_PyUnicode_GET_LENGTH(s2)) {
            goto return_ne;
        }
        kind = __Pyx_PyUnicode_KIND(s1);
        if (kind != __Pyx_PyUnicode_KIND(s2)) {
            goto return_ne;
        }
        data1 = __Pyx_PyUnicode_DATA(s1);
        data2 = __Pyx_PyUnicode_DATA(s2);
        if (__Pyx_PyUnicode_READ(kind, data1, 0) != __Pyx_PyUnicode_READ(kind, data2, 0)) {
            goto return_ne;
        } else if (length == 1) {
            goto return_eq;
        } else {
            int result = memcmp(data1, data2, length * kind);
            #if PY_MAJOR_VERSION < 3
            Py_XDECREF(owned_ref);
            #endif
            return (equals == Py_EQ) ? (result == 0) : (result != 0);
        }
    } else if ((s1 == Py_None) & s2_is_unicode) {
        goto return_ne;
    } else if ((s2 == Py_None) & s1_is_unicode) {
        goto return_ne;
    } else {
        int result;
        PyObject* py_result = PyObject_RichCompare(s1, s2, equals);
        if (!py_result)
            return -1;
        result = __Pyx_PyObject_IsTrue(py_result);
        Py_DECREF(py_result);
        return result;
    }
return_eq:
    #if PY_MAJOR_VERSION < 3
    Py_XDECREF(owned_ref);
    #endif
    return (equals == Py_EQ);
return_ne:
    #if PY_MAJOR_VERSION < 3
    Py_XDECREF(owned_ref);
    #endif
    return (equals == Py_NE);
#endif
}

static CYTHON_INLINE PyObject *__Pyx_GetItemInt_Generic(PyObject *o, PyObject* j) {
    PyObject *r;
    if (!j) return NULL;
    r = PyObject_GetItem(o, j);
    Py_DECREF(j);
    return r;
}
static CYTHON_INLINE PyObject *__Pyx_GetItemInt_List_Fast(PyObject *o, Py_ssize_t i,
                                                              int wraparound, int boundscheck) {
#if CYTHON_COMPILING_IN_CPYTHON
    if (wraparound & unlikely(i < 0)) i += PyList_GET_SIZE(o);
    if ((!boundscheck) || likely((0 <= i) & (i < PyList_GET_SIZE(o)))) {
        PyObject *r = PyList_GET_ITEM(o, i);
        Py_INCREF(r);
        return r;
    }
    return __Pyx_GetItemInt_Generic(o, PyInt_FromSsize_t(i));
#else
    return PySequence_GetItem(o, i);
#endif
}
static CYTHON_INLINE PyObject *__Pyx_GetItemInt_Tuple_Fast(PyObject *o, Py_ssize_t i,
                                                              int wraparound, int boundscheck) {
#if CYTHON_COMPILING_IN_CPYTHON
    if (wraparound & unlikely(i < 0)) i += PyTuple_GET_SIZE(o);
    if ((!boundscheck) || likely((0 <= i) & (i < PyTuple_GET_SIZE(o)))) {
        PyObject *r = PyTuple_GET_ITEM(o, i);
        Py_INCREF(r);
        return r;
    }
    return __Pyx_GetItemInt_Generic(o, PyInt_FromSsize_t(i));
#else
    return PySequence_GetItem(o, i);
#endif
}
static CYTHON_INLINE PyObject *__Pyx_GetItemInt_Fast(PyObject *o, Py_ssize_t i,
                                                     int is_list, int wraparound, int boundscheck) {
#if CYTHON_COMPILING_IN_CPYTHON
    if (is_list || PyList_CheckExact(o)) {
        Py_ssize_t n = ((!wraparound) | likely(i >= 0)) ? i : i + PyList_GET_SIZE(o);
        if ((!boundscheck) || (likely((n >= 0) & (n < PyList_GET_SIZE(o))))) {
            PyObject *r = PyList_GET_ITEM(o, n);
            Py_INCREF(r);
            return r;
        }
    }
    else if (PyTuple_CheckExact(o)) {
        Py_ssize_t n = ((!wraparound) | likely(i >= 0)) ? i : i + PyTuple_GET_SIZE(o);
        if ((!boundscheck) || likely((n >= 0) & (n < PyTuple_GET_SIZE(o)))) {
            PyObject *r = PyTuple_GET_ITEM(o, n);
            Py_INCREF(r);
            return r;
        }
    } else {
        PySequenceMethods *m = Py_TYPE(o)->tp_as_sequence;
        if (likely(m && m->sq_item)) {
            if (wraparound && unlikely(i < 0) && likely(m->sq_length)) {
                Py_ssize_t l = m->sq_length(o);
                if (likely(l >= 0)) {
                    i += l;
                } else {
                    if (PyErr_ExceptionMatches(PyExc_OverflowError))
                        PyErr_Clear();
                    else
                        return NULL;
                }
            }
            return m->sq_item(o, i);
        }
    }
#else
    if (is_list || PySequence_Check(o)) {
        return PySequence_GetItem(o, i);
    }
#endif
    return __Pyx_GetItemInt_Generic(o, PyInt_FromSsize_t(i));
}

static CYTHON_INLINE void __Pyx_RaiseTooManyValuesError(Py_ssize_t expected) {
    PyErr_Format(PyExc_ValueError,
                 "too many values to unpack (expected %" CYTHON_FORMAT_SSIZE_T "d)", expected);
}

static CYTHON_INLINE void __Pyx_RaiseNeedMoreValuesError(Py_ssize_t index) {
    PyErr_Format(PyExc_ValueError,
                 "need more than %" CYTHON_FORMAT_SSIZE_T "d value%.1s to unpack",
                 index, (index == 1) ? "" : "s");
}

static CYTHON_INLINE int __Pyx_IterFinish(void) {
#if CYTHON_COMPILING_IN_CPYTHON
    PyThreadState *tstate = PyThreadState_GET();
    PyObject* exc_type = tstate->curexc_type;
    if (unlikely(exc_type)) {
        if (likely(exc_type == PyExc_StopIteration) || PyErr_GivenExceptionMatches(exc_type, PyExc_StopIteration)) {
            PyObject *exc_value, *exc_tb;
            exc_value = tstate->curexc_value;
            exc_tb = tstate->curexc_traceback;
            tstate->curexc_type = 0;
            tstate->curexc_value = 0;
            tstate->curexc_traceback = 0;
            Py_DECREF(exc_type);
            Py_XDECREF(exc_value);
            Py_XDECREF(exc_tb);
            return 0;
        } else {
            return -1;
        }
    }
    return 0;
#else
    if (unlikely(PyErr_Occurred())) {
        if (likely(PyErr_ExceptionMatches(PyExc_StopIteration))) {
            PyErr_Clear();
            return 0;
        } else {
            return -1;
        }
    }
    return 0;
#endif
}

static int __Pyx_IternextUnpackEndCheck(PyObject *retval, Py_ssize_t expected) {
    if (unlikely(retval)) {
        Py_DECREF(retval);
        __Pyx_RaiseTooManyValuesError(expected);
        return -1;
    } else {
        return __Pyx_IterFinish();
    }
    return 0;
}

static int __Pyx_SetVtable(PyObject *dict, void *vtable) {
#if PY_VERSION_HEX >= 0x02070000 && !(PY_MAJOR_VERSION==3&&PY_MINOR_VERSION==0)
    PyObject *ob = PyCapsule_New(vtable, 0, 0);
#else
    PyObject *ob = PyCObject_FromVoidPtr(vtable, 0);
#endif
    if (!ob)
        goto bad;
    if (PyDict_SetItem(dict, __pyx_n_s_pyx_vtable, ob) < 0)
        goto bad;
    Py_DECREF(ob);
    return 0;
bad:
    Py_XDECREF(ob);
    return -1;
}

static CYTHON_INLINE PyObject* __Pyx_PyInt_From_int(int value) {
    const int neg_one = (int) -1, const_zero = 0;
    const int is_unsigned = neg_one > const_zero;
    if (is_unsigned) {
        if (sizeof(int) < sizeof(long)) {
            return PyInt_FromLong((long) value);
        } else if (sizeof(int) <= sizeof(unsigned long)) {
            return PyLong_FromUnsignedLong((unsigned long) value);
        } else if (sizeof(int) <= sizeof(unsigned long long)) {
            return PyLong_FromUnsignedLongLong((unsigned long long) value);
        }
    } else {
        if (sizeof(int) <= sizeof(long)) {
            return PyInt_FromLong((long) value);
        } else if (sizeof(int) <= sizeof(long long)) {
            return PyLong_FromLongLong((long long) value);
        }
    }
    {
        int one = 1; int little = (int)*(unsigned char *)&one;
        unsigned char *bytes = (unsigned char *)&value;
        return _PyLong_FromByteArray(bytes, sizeof(int),
                                     little, !is_unsigned);
    }
}

static PyObject *__Pyx_Import(PyObject *name, PyObject *from_list, int level) {
    PyObject *empty_list = 0;
    PyObject *module = 0;
    PyObject *global_dict = 0;
    PyObject *empty_dict = 0;
    PyObject *list;
    #if PY_VERSION_HEX < 0x03030000
    PyObject *py_import;
    py_import = __Pyx_PyObject_GetAttrStr(__pyx_b, __pyx_n_s_import);
    if (!py_import)
        goto bad;
    #endif
    if (from_list)
        list = from_list;
    else {
        empty_list = PyList_New(0);
        if (!empty_list)
            goto bad;
        list = empty_list;
    }
    global_dict = PyModule_GetDict(__pyx_m);
    if (!global_dict)
        goto bad;
    empty_dict = PyDict_New();
    if (!empty_dict)
        goto bad;
    #if PY_VERSION_HEX >= 0x02050000
    {
        #if PY_MAJOR_VERSION >= 3
        if (level == -1) {
            if (strchr(__Pyx_MODULE_NAME, '.')) {
                #if PY_VERSION_HEX < 0x03030000
                PyObject *py_level = PyInt_FromLong(1);
                if (!py_level)
                    goto bad;
                module = PyObject_CallFunctionObjArgs(py_import,
                    name, global_dict, empty_dict, list, py_level, NULL);
                Py_DECREF(py_level);
                #else
                module = PyImport_ImportModuleLevelObject(
                    name, global_dict, empty_dict, list, 1);
                #endif
                if (!module) {
                    if (!PyErr_ExceptionMatches(PyExc_ImportError))
                        goto bad;
                    PyErr_Clear();
                }
            }
            level = 0; 
        }
        #endif
        if (!module) {
            #if PY_VERSION_HEX < 0x03030000
            PyObject *py_level = PyInt_FromLong(level);
            if (!py_level)
                goto bad;
            module = PyObject_CallFunctionObjArgs(py_import,
                name, global_dict, empty_dict, list, py_level, NULL);
            Py_DECREF(py_level);
            #else
            module = PyImport_ImportModuleLevelObject(
                name, global_dict, empty_dict, list, level);
            #endif
        }
    }
    #else
    if (level>0) {
        PyErr_SetString(PyExc_RuntimeError, "Relative import is not supported for Python <=2.4.");
        goto bad;
    }
    module = PyObject_CallFunctionObjArgs(py_import,
        name, global_dict, empty_dict, list, NULL);
    #endif
bad:
    #if PY_VERSION_HEX < 0x03030000
    Py_XDECREF(py_import);
    #endif
    Py_XDECREF(empty_list);
    Py_XDECREF(empty_dict);
    return module;
}

#define __PYX_VERIFY_RETURN_INT(target_type, func_type, func)             \
    {                                                                     \
        func_type value = func(x);                                        \
        if (sizeof(target_type) < sizeof(func_type)) {                    \
            if (unlikely(value != (func_type) (target_type) value)) {     \
                func_type zero = 0;                                       \
                PyErr_SetString(PyExc_OverflowError,                      \
                    (is_unsigned && unlikely(value < zero)) ?             \
                    "can't convert negative value to " #target_type :     \
                    "value too large to convert to " #target_type);       \
                return (target_type) -1;                                  \
            }                                                             \
        }                                                                 \
        return (target_type) value;                                       \
    }

#if CYTHON_COMPILING_IN_CPYTHON && PY_MAJOR_VERSION >= 3
 #if CYTHON_USE_PYLONG_INTERNALS
  #include "longintrepr.h"
 #endif
#endif
static CYTHON_INLINE int __Pyx_PyInt_As_int(PyObject *x) {
    const int neg_one = (int) -1, const_zero = 0;
    const int is_unsigned = neg_one > const_zero;
#if PY_MAJOR_VERSION < 3
    if (likely(PyInt_Check(x))) {
        if (sizeof(int) < sizeof(long)) {
            __PYX_VERIFY_RETURN_INT(int, long, PyInt_AS_LONG)
        } else {
            long val = PyInt_AS_LONG(x);
            if (is_unsigned && unlikely(val < 0)) {
                PyErr_SetString(PyExc_OverflowError,
                                "can't convert negative value to int");
                return (int) -1;
            }
            return (int) val;
        }
    } else
#endif
    if (likely(PyLong_Check(x))) {
        if (is_unsigned) {
#if CYTHON_COMPILING_IN_CPYTHON && PY_MAJOR_VERSION >= 3
 #if CYTHON_USE_PYLONG_INTERNALS
            if (sizeof(digit) <= sizeof(int)) {
                switch (Py_SIZE(x)) {
                    case  0: return 0;
                    case  1: return (int) ((PyLongObject*)x)->ob_digit[0];
                }
            }
 #endif
#endif
            if (unlikely(Py_SIZE(x) < 0)) {
                PyErr_SetString(PyExc_OverflowError,
                                "can't convert negative value to int");
                return (int) -1;
            }
            if (sizeof(int) <= sizeof(unsigned long)) {
                __PYX_VERIFY_RETURN_INT(int, unsigned long, PyLong_AsUnsignedLong)
            } else if (sizeof(int) <= sizeof(unsigned long long)) {
                __PYX_VERIFY_RETURN_INT(int, unsigned long long, PyLong_AsUnsignedLongLong)
            }
        } else {
#if CYTHON_COMPILING_IN_CPYTHON && PY_MAJOR_VERSION >= 3
 #if CYTHON_USE_PYLONG_INTERNALS
            if (sizeof(digit) <= sizeof(int)) {
                switch (Py_SIZE(x)) {
                    case  0: return 0;
                    case  1: return +(int) ((PyLongObject*)x)->ob_digit[0];
                    case -1: return -(int) ((PyLongObject*)x)->ob_digit[0];
                }
            }
 #endif
#endif
            if (sizeof(int) <= sizeof(long)) {
                __PYX_VERIFY_RETURN_INT(int, long, PyLong_AsLong)
            } else if (sizeof(int) <= sizeof(long long)) {
                __PYX_VERIFY_RETURN_INT(int, long long, PyLong_AsLongLong)
            }
        }
        {
#if CYTHON_COMPILING_IN_PYPY && !defined(_PyLong_AsByteArray)
            PyErr_SetString(PyExc_RuntimeError,
                            "_PyLong_AsByteArray() not available in PyPy, cannot convert large numbers");
#else
            int val;
            PyObject *v = __Pyx_PyNumber_Int(x);
 #if PY_MAJOR_VERSION < 3
            if (likely(v) && !PyLong_Check(v)) {
                PyObject *tmp = v;
                v = PyNumber_Long(tmp);
                Py_DECREF(tmp);
            }
 #endif
            if (likely(v)) {
                int one = 1; int is_little = (int)*(unsigned char *)&one;
                unsigned char *bytes = (unsigned char *)&val;
                int ret = _PyLong_AsByteArray((PyLongObject *)v,
                                              bytes, sizeof(val),
                                              is_little, !is_unsigned);
                Py_DECREF(v);
                if (likely(!ret))
                    return val;
            }
#endif
            return (int) -1;
        }
    } else {
        int val;
        PyObject *tmp = __Pyx_PyNumber_Int(x);
        if (!tmp) return (int) -1;
        val = __Pyx_PyInt_As_int(tmp);
        Py_DECREF(tmp);
        return val;
    }
}

static CYTHON_INLINE PyObject* __Pyx_PyInt_From_long(long value) {
    const long neg_one = (long) -1, const_zero = 0;
    const int is_unsigned = neg_one > const_zero;
    if (is_unsigned) {
        if (sizeof(long) < sizeof(long)) {
            return PyInt_FromLong((long) value);
        } else if (sizeof(long) <= sizeof(unsigned long)) {
            return PyLong_FromUnsignedLong((unsigned long) value);
        } else if (sizeof(long) <= sizeof(unsigned long long)) {
            return PyLong_FromUnsignedLongLong((unsigned long long) value);
        }
    } else {
        if (sizeof(long) <= sizeof(long)) {
            return PyInt_FromLong((long) value);
        } else if (sizeof(long) <= sizeof(long long)) {
            return PyLong_FromLongLong((long long) value);
        }
    }
    {
        int one = 1; int little = (int)*(unsigned char *)&one;
        unsigned char *bytes = (unsigned char *)&value;
        return _PyLong_FromByteArray(bytes, sizeof(long),
                                     little, !is_unsigned);
    }
}

#if CYTHON_COMPILING_IN_CPYTHON && PY_MAJOR_VERSION >= 3
 #if CYTHON_USE_PYLONG_INTERNALS
  #include "longintrepr.h"
 #endif
#endif
static CYTHON_INLINE long __Pyx_PyInt_As_long(PyObject *x) {
    const long neg_one = (long) -1, const_zero = 0;
    const int is_unsigned = neg_one > const_zero;
#if PY_MAJOR_VERSION < 3
    if (likely(PyInt_Check(x))) {
        if (sizeof(long) < sizeof(long)) {
            __PYX_VERIFY_RETURN_INT(long, long, PyInt_AS_LONG)
        } else {
            long val = PyInt_AS_LONG(x);
            if (is_unsigned && unlikely(val < 0)) {
                PyErr_SetString(PyExc_OverflowError,
                                "can't convert negative value to long");
                return (long) -1;
            }
            return (long) val;
        }
    } else
#endif
    if (likely(PyLong_Check(x))) {
        if (is_unsigned) {
#if CYTHON_COMPILING_IN_CPYTHON && PY_MAJOR_VERSION >= 3
 #if CYTHON_USE_PYLONG_INTERNALS
            if (sizeof(digit) <= sizeof(long)) {
                switch (Py_SIZE(x)) {
                    case  0: return 0;
                    case  1: return (long) ((PyLongObject*)x)->ob_digit[0];
                }
            }
 #endif
#endif
            if (unlikely(Py_SIZE(x) < 0)) {
                PyErr_SetString(PyExc_OverflowError,
                                "can't convert negative value to long");
                return (long) -1;
            }
            if (sizeof(long) <= sizeof(unsigned long)) {
                __PYX_VERIFY_RETURN_INT(long, unsigned long, PyLong_AsUnsignedLong)
            } else if (sizeof(long) <= sizeof(unsigned long long)) {
                __PYX_VERIFY_RETURN_INT(long, unsigned long long, PyLong_AsUnsignedLongLong)
            }
        } else {
#if CYTHON_COMPILING_IN_CPYTHON && PY_MAJOR_VERSION >= 3
 #if CYTHON_USE_PYLONG_INTERNALS
            if (sizeof(digit) <= sizeof(long)) {
                switch (Py_SIZE(x)) {
                    case  0: return 0;
                    case  1: return +(long) ((PyLongObject*)x)->ob_digit[0];
                    case -1: return -(long) ((PyLongObject*)x)->ob_digit[0];
                }
            }
 #endif
#endif
            if (sizeof(long) <= sizeof(long)) {
                __PYX_VERIFY_RETURN_INT(long, long, PyLong_AsLong)
            } else if (sizeof(long) <= sizeof(long long)) {
                __PYX_VERIFY_RETURN_INT(long, long long, PyLong_AsLongLong)
            }
        }
        {
#if CYTHON_COMPILING_IN_PYPY && !defined(_PyLong_AsByteArray)
            PyErr_SetString(PyExc_RuntimeError,
                            "_PyLong_AsByteArray() not available in PyPy, cannot convert large numbers");
#else
            long val;
            PyObject *v = __Pyx_PyNumber_Int(x);
 #if PY_MAJOR_VERSION < 3
            if (likely(v) && !PyLong_Check(v)) {
                PyObject *tmp = v;
                v = PyNumber_Long(tmp);
                Py_DECREF(tmp);
            }
 #endif
            if (likely(v)) {
                int one = 1; int is_little = (int)*(unsigned char *)&one;
                unsigned char *bytes = (unsigned char *)&val;
                int ret = _PyLong_AsByteArray((PyLongObject *)v,
                                              bytes, sizeof(val),
                                              is_little, !is_unsigned);
                Py_DECREF(v);
                if (likely(!ret))
                    return val;
            }
#endif
            return (long) -1;
        }
    } else {
        long val;
        PyObject *tmp = __Pyx_PyNumber_Int(x);
        if (!tmp) return (long) -1;
        val = __Pyx_PyInt_As_long(tmp);
        Py_DECREF(tmp);
        return val;
    }
}

static int __Pyx_check_binary_version(void) {
    char ctversion[4], rtversion[4];
    PyOS_snprintf(ctversion, 4, "%d.%d", PY_MAJOR_VERSION, PY_MINOR_VERSION);
    PyOS_snprintf(rtversion, 4, "%s", Py_GetVersion());
    if (ctversion[0] != rtversion[0] || ctversion[2] != rtversion[2]) {
        char message[200];
        PyOS_snprintf(message, sizeof(message),
                      "compiletime version %s of module '%.100s' "
                      "does not match runtime version %s",
                      ctversion, __Pyx_MODULE_NAME, rtversion);
        #if PY_VERSION_HEX < 0x02050000
        return PyErr_Warn(NULL, message);
        #else
        return PyErr_WarnEx(NULL, message, 1);
        #endif
    }
    return 0;
}

static int __pyx_bisect_code_objects(__Pyx_CodeObjectCacheEntry* entries, int count, int code_line) {
    int start = 0, mid = 0, end = count - 1;
    if (end >= 0 && code_line > entries[end].code_line) {
        return count;
    }
    while (start < end) {
        mid = (start + end) / 2;
        if (code_line < entries[mid].code_line) {
            end = mid;
        } else if (code_line > entries[mid].code_line) {
             start = mid + 1;
        } else {
            return mid;
        }
    }
    if (code_line <= entries[mid].code_line) {
        return mid;
    } else {
        return mid + 1;
    }
}
static PyCodeObject *__pyx_find_code_object(int code_line) {
    PyCodeObject* code_object;
    int pos;
    if (unlikely(!code_line) || unlikely(!__pyx_code_cache.entries)) {
        return NULL;
    }
    pos = __pyx_bisect_code_objects(__pyx_code_cache.entries, __pyx_code_cache.count, code_line);
    if (unlikely(pos >= __pyx_code_cache.count) || unlikely(__pyx_code_cache.entries[pos].code_line != code_line)) {
        return NULL;
    }
    code_object = __pyx_code_cache.entries[pos].code_object;
    Py_INCREF(code_object);
    return code_object;
}
static void __pyx_insert_code_object(int code_line, PyCodeObject* code_object) {
    int pos, i;
    __Pyx_CodeObjectCacheEntry* entries = __pyx_code_cache.entries;
    if (unlikely(!code_line)) {
        return;
    }
    if (unlikely(!entries)) {
        entries = (__Pyx_CodeObjectCacheEntry*)PyMem_Malloc(64*sizeof(__Pyx_CodeObjectCacheEntry));
        if (likely(entries)) {
            __pyx_code_cache.entries = entries;
            __pyx_code_cache.max_count = 64;
            __pyx_code_cache.count = 1;
            entries[0].code_line = code_line;
            entries[0].code_object = code_object;
            Py_INCREF(code_object);
        }
        return;
    }
    pos = __pyx_bisect_code_objects(__pyx_code_cache.entries, __pyx_code_cache.count, code_line);
    if ((pos < __pyx_code_cache.count) && unlikely(__pyx_code_cache.entries[pos].code_line == code_line)) {
        PyCodeObject* tmp = entries[pos].code_object;
        entries[pos].code_object = code_object;
        Py_DECREF(tmp);
        return;
    }
    if (__pyx_code_cache.count == __pyx_code_cache.max_count) {
        int new_max = __pyx_code_cache.max_count + 64;
        entries = (__Pyx_CodeObjectCacheEntry*)PyMem_Realloc(
            __pyx_code_cache.entries, new_max*sizeof(__Pyx_CodeObjectCacheEntry));
        if (unlikely(!entries)) {
            return;
        }
        __pyx_code_cache.entries = entries;
        __pyx_code_cache.max_count = new_max;
    }
    for (i=__pyx_code_cache.count; i>pos; i--) {
        entries[i] = entries[i-1];
    }
    entries[pos].code_line = code_line;
    entries[pos].code_object = code_object;
    __pyx_code_cache.count++;
    Py_INCREF(code_object);
}

#include "compile.h"
#include "frameobject.h"
#include "traceback.h"
static PyCodeObject* __Pyx_CreateCodeObjectForTraceback(
            const char *funcname, int c_line,
            int py_line, const char *filename) {
    PyCodeObject *py_code = 0;
    PyObject *py_srcfile = 0;
    PyObject *py_funcname = 0;
    #if PY_MAJOR_VERSION < 3
    py_srcfile = PyString_FromString(filename);
    #else
    py_srcfile = PyUnicode_FromString(filename);
    #endif
    if (!py_srcfile) goto bad;
    if (c_line) {
        #if PY_MAJOR_VERSION < 3
        py_funcname = PyString_FromFormat( "%s (%s:%d)", funcname, __pyx_cfilenm, c_line);
        #else
        py_funcname = PyUnicode_FromFormat( "%s (%s:%d)", funcname, __pyx_cfilenm, c_line);
        #endif
    }
    else {
        #if PY_MAJOR_VERSION < 3
        py_funcname = PyString_FromString(funcname);
        #else
        py_funcname = PyUnicode_FromString(funcname);
        #endif
    }
    if (!py_funcname) goto bad;
    py_code = __Pyx_PyCode_New(
        0,            
        0,            
        0,            
        0,            
        0,            
        __pyx_empty_bytes, 
        __pyx_empty_tuple, 
        __pyx_empty_tuple, 
        __pyx_empty_tuple, 
        __pyx_empty_tuple, 
        __pyx_empty_tuple, 
        py_srcfile,   
        py_funcname,  
        py_line,      
        __pyx_empty_bytes  
    );
    Py_DECREF(py_srcfile);
    Py_DECREF(py_funcname);
    return py_code;
bad:
    Py_XDECREF(py_srcfile);
    Py_XDECREF(py_funcname);
    return NULL;
}
static void __Pyx_AddTraceback(const char *funcname, int c_line,
                               int py_line, const char *filename) {
    PyCodeObject *py_code = 0;
    PyObject *py_globals = 0;
    PyFrameObject *py_frame = 0;
    py_code = __pyx_find_code_object(c_line ? c_line : py_line);
    if (!py_code) {
        py_code = __Pyx_CreateCodeObjectForTraceback(
            funcname, c_line, py_line, filename);
        if (!py_code) goto bad;
        __pyx_insert_code_object(c_line ? c_line : py_line, py_code);
    }
    py_globals = PyModule_GetDict(__pyx_m);
    if (!py_globals) goto bad;
    py_frame = PyFrame_New(
        PyThreadState_GET(), 
        py_code,             
        py_globals,          
        0                    
    );
    if (!py_frame) goto bad;
    py_frame->f_lineno = py_line;
    PyTraceBack_Here(py_frame);
bad:
    Py_XDECREF(py_code);
    Py_XDECREF(py_frame);
}

static int __Pyx_InitStrings(__Pyx_StringTabEntry *t) {
    while (t->p) {
        #if PY_MAJOR_VERSION < 3
        if (t->is_unicode) {
            *t->p = PyUnicode_DecodeUTF8(t->s, t->n - 1, NULL);
        } else if (t->intern) {
            *t->p = PyString_InternFromString(t->s);
        } else {
            *t->p = PyString_FromStringAndSize(t->s, t->n - 1);
        }
        #else  
        if (t->is_unicode | t->is_str) {
            if (t->intern) {
                *t->p = PyUnicode_InternFromString(t->s);
            } else if (t->encoding) {
                *t->p = PyUnicode_Decode(t->s, t->n - 1, t->encoding, NULL);
            } else {
                *t->p = PyUnicode_FromStringAndSize(t->s, t->n - 1);
            }
        } else {
            *t->p = PyBytes_FromStringAndSize(t->s, t->n - 1);
        }
        #endif
        if (!*t->p)
            return -1;
        ++t;
    }
    return 0;
}

static CYTHON_INLINE PyObject* __Pyx_PyUnicode_FromString(char* c_str) {
    return __Pyx_PyUnicode_FromStringAndSize(c_str, strlen(c_str));
}
static CYTHON_INLINE char* __Pyx_PyObject_AsString(PyObject* o) {
    Py_ssize_t ignore;
    return __Pyx_PyObject_AsStringAndSize(o, &ignore);
}
static CYTHON_INLINE char* __Pyx_PyObject_AsStringAndSize(PyObject* o, Py_ssize_t *length) {
#if __PYX_DEFAULT_STRING_ENCODING_IS_ASCII || __PYX_DEFAULT_STRING_ENCODING_IS_DEFAULT
    if (
#if PY_MAJOR_VERSION < 3 && __PYX_DEFAULT_STRING_ENCODING_IS_ASCII
            __Pyx_sys_getdefaultencoding_not_ascii &&
#endif
            PyUnicode_Check(o)) {
#if PY_VERSION_HEX < 0x03030000
        char* defenc_c;
        PyObject* defenc = _PyUnicode_AsDefaultEncodedString(o, NULL);
        if (!defenc) return NULL;
        defenc_c = PyBytes_AS_STRING(defenc);
#if __PYX_DEFAULT_STRING_ENCODING_IS_ASCII
        {
            char* end = defenc_c + PyBytes_GET_SIZE(defenc);
            char* c;
            for (c = defenc_c; c < end; c++) {
                if ((unsigned char) (*c) >= 128) {
                    PyUnicode_AsASCIIString(o);
                    return NULL;
                }
            }
        }
#endif 
        *length = PyBytes_GET_SIZE(defenc);
        return defenc_c;
#else 
        if (PyUnicode_READY(o) == -1) return NULL;
#if __PYX_DEFAULT_STRING_ENCODING_IS_ASCII
        if (PyUnicode_IS_ASCII(o)) {
            *length = PyUnicode_GET_DATA_SIZE(o);
            return PyUnicode_AsUTF8(o);
        } else {
            PyUnicode_AsASCIIString(o);
            return NULL;
        }
#else 
        return PyUnicode_AsUTF8AndSize(o, length);
#endif 
#endif 
    } else
#endif 
#if !CYTHON_COMPILING_IN_PYPY
#if PY_VERSION_HEX >= 0x02060000
    if (PyByteArray_Check(o)) {
        *length = PyByteArray_GET_SIZE(o);
        return PyByteArray_AS_STRING(o);
    } else
#endif
#endif
    {
        char* result;
        int r = PyBytes_AsStringAndSize(o, &result, length);
        if (unlikely(r < 0)) {
            return NULL;
        } else {
            return result;
        }
    }
}
static CYTHON_INLINE int __Pyx_PyObject_IsTrue(PyObject* x) {
   int is_true = x == Py_True;
   if (is_true | (x == Py_False) | (x == Py_None)) return is_true;
   else return PyObject_IsTrue(x);
}
static CYTHON_INLINE PyObject* __Pyx_PyNumber_Int(PyObject* x) {
  PyNumberMethods *m;
  const char *name = NULL;
  PyObject *res = NULL;
#if PY_MAJOR_VERSION < 3
  if (PyInt_Check(x) || PyLong_Check(x))
#else
  if (PyLong_Check(x))
#endif
    return Py_INCREF(x), x;
  m = Py_TYPE(x)->tp_as_number;
#if PY_MAJOR_VERSION < 3
  if (m && m->nb_int) {
    name = "int";
    res = PyNumber_Int(x);
  }
  else if (m && m->nb_long) {
    name = "long";
    res = PyNumber_Long(x);
  }
#else
  if (m && m->nb_int) {
    name = "int";
    res = PyNumber_Long(x);
  }
#endif
  if (res) {
#if PY_MAJOR_VERSION < 3
    if (!PyInt_Check(res) && !PyLong_Check(res)) {
#else
    if (!PyLong_Check(res)) {
#endif
      PyErr_Format(PyExc_TypeError,
                   "__%.4s__ returned non-%.4s (type %.200s)",
                   name, name, Py_TYPE(res)->tp_name);
      Py_DECREF(res);
      return NULL;
    }
  }
  else if (!PyErr_Occurred()) {
    PyErr_SetString(PyExc_TypeError,
                    "an integer is required");
  }
  return res;
}
#if CYTHON_COMPILING_IN_CPYTHON && PY_MAJOR_VERSION >= 3
 #if CYTHON_USE_PYLONG_INTERNALS
  #include "longintrepr.h"
 #endif
#endif
static CYTHON_INLINE Py_ssize_t __Pyx_PyIndex_AsSsize_t(PyObject* b) {
  Py_ssize_t ival;
  PyObject *x;
#if PY_MAJOR_VERSION < 3
  if (likely(PyInt_CheckExact(b)))
      return PyInt_AS_LONG(b);
#endif
  if (likely(PyLong_CheckExact(b))) {
    #if CYTHON_COMPILING_IN_CPYTHON && PY_MAJOR_VERSION >= 3
     #if CYTHON_USE_PYLONG_INTERNALS
       switch (Py_SIZE(b)) {
       case -1: return -(sdigit)((PyLongObject*)b)->ob_digit[0];
       case  0: return 0;
       case  1: return ((PyLongObject*)b)->ob_digit[0];
       }
     #endif
    #endif
  #if PY_VERSION_HEX < 0x02060000
    return PyInt_AsSsize_t(b);
  #else
    return PyLong_AsSsize_t(b);
  #endif
  }
  x = PyNumber_Index(b);
  if (!x) return -1;
  ival = PyInt_AsSsize_t(x);
  Py_DECREF(x);
  return ival;
}
static CYTHON_INLINE PyObject * __Pyx_PyInt_FromSize_t(size_t ival) {
#if PY_VERSION_HEX < 0x02050000
   if (ival <= LONG_MAX)
       return PyInt_FromLong((long)ival);
   else {
       unsigned char *bytes = (unsigned char *) &ival;
       int one = 1; int little = (int)*(unsigned char*)&one;
       return _PyLong_FromByteArray(bytes, sizeof(size_t), little, 0);
   }
#else
   return PyInt_FromSize_t(ival);
#endif
}


#endif 
