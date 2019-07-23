








































#ifndef JRITYPES_H
#define JRITYPES_H

#include "jri_md.h"
#include "jni.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif





struct JRIEnvInterface;

typedef void*		JRIRef;
typedef void*		JRIGlobalRef;

typedef jint		JRIFieldID;
typedef jint		JRIMethodID;


typedef JRIGlobalRef	jglobal;

typedef union JRIValue {
	jbool			z;
	jbyte			b;
	jchar			c;
	jshort			s;
	jint			i;
	jlong			l;
	jfloat			f;
	jdouble			d;
	jref			r;
} JRIValue;

typedef enum JRIBoolean {
    JRIFalse		= 0,
    JRITrue			= 1
} JRIBoolean;

typedef enum JRIConstant {
	JRIUninitialized	= -1
} JRIConstant;


#if 0	
typedef struct jbooleanArrayStruct*		jbooleanArray;
typedef struct jbyteArrayStruct*		jbyteArray;
typedef struct jcharArrayStruct*		jcharArray;
typedef struct jshortArrayStruct*		jshortArray;
typedef struct jintArrayStruct*			jintArray;
typedef struct jlongArrayStruct*		jlongArray;
typedef struct jfloatArrayStruct*		jfloatArray;
typedef struct jdoubleArrayStruct*		jdoubleArray;
typedef struct jobjectArrayStruct*		jobjectArray;
#endif
typedef struct jstringArrayStruct*		jstringArray;
typedef struct jarrayArrayStruct*		jarrayArray;

#define JRIConstructorMethodName	"<init>"




















#define JRISigArray(T)		"[" T
#define JRISigByte			"B"
#define JRISigChar			"C"
#define JRISigClass(name)	"L" name ";"
#define JRISigFloat			"F"
#define JRISigDouble		"D"
#define JRISigMethod(args)	"(" args ")"
#define JRISigNoArgs		""
#define JRISigInt			"I"
#define JRISigLong			"J"
#define JRISigShort			"S"
#define JRISigVoid			"V"
#define JRISigBoolean		"Z"





extern JRI_PUBLIC_API(const struct JRIEnvInterface**)
JRI_GetCurrentEnv(void);













#define JRI_NewByteArray(env, length, initialValues)	\
	JRI_NewScalarArray(env, length, JRISigByte, (jbyte*)(initialValues))
#define JRI_GetByteArrayLength(env, array)	\
	JRI_GetScalarArrayLength(env, array)
#define JRI_GetByteArrayElements(env, array)	\
	JRI_GetScalarArrayElements(env, array)

#define JRI_NewCharArray(env, length, initialValues)	\
	JRI_NewScalarArray(env, ((length) * sizeof(jchar)), JRISigChar, (jbyte*)(initialValues))
#define JRI_GetCharArrayLength(env, array)	\
	JRI_GetScalarArrayLength(env, array)
#define JRI_GetCharArrayElements(env, array)		   \
	((jchar*)JRI_GetScalarArrayElements(env, array))

#define JRI_NewShortArray(env, length, initialValues)	\
	JRI_NewScalarArray(env, ((length) * sizeof(jshort)), JRISigShort, (jbyte*)(initialValues))
#define JRI_GetShortArrayLength(env, array)	\
	JRI_GetScalarArrayLength(env, array)
#define JRI_GetShortArrayElements(env, array)		   \
	((jshort*)JRI_GetScalarArrayElements(env, array))

#define JRI_NewIntArray(env, length, initialValues)	\
	JRI_NewScalarArray(env, ((length) * sizeof(jint)), JRISigInt, (jbyte*)(initialValues))
#define JRI_GetIntArrayLength(env, array)	\
	JRI_GetScalarArrayLength(env, array)
#define JRI_GetIntArrayElements(env, array)		   \
	((jint*)JRI_GetScalarArrayElements(env, array))

#define JRI_NewLongArray(env, length, initialValues)	\
	JRI_NewScalarArray(env, ((length) * sizeof(jlong)), JRISigLong, (jbyte*)(initialValues))
#define JRI_GetLongArrayLength(env, array)	\
	JRI_GetScalarArrayLength(env, array)
#define JRI_GetLongArrayElements(env, array)		   \
	((jlong*)JRI_GetScalarArrayElements(env, array))

#define JRI_NewFloatArray(env, length, initialValues)	\
	JRI_NewScalarArray(env, ((length) * sizeof(jfloat)), JRISigFloat, (jbyte*)(initialValues))
#define JRI_GetFloatArrayLength(env, array)	\
	JRI_GetScalarArrayLength(env, array)
#define JRI_GetFloatArrayElements(env, array)		   \
	((jfloat*)JRI_GetScalarArrayElements(env, array))

#define JRI_NewDoubleArray(env, length, initialValues)	\
	JRI_NewScalarArray(env, ((length) * sizeof(jdouble)), JRISigDouble, (jbyte*)(initialValues))
#define JRI_GetDoubleArrayLength(env, array)	\
	JRI_GetScalarArrayLength(env, array)
#define JRI_GetDoubleArrayElements(env, array)		   \
	((jdouble*)JRI_GetScalarArrayElements(env, array))







typedef union JRI_JDK_stack_item {
    
    jint           i;
    jfloat         f;
    jint           o;
    
    void          *h;
    void          *p;
    unsigned char *addr;
#ifdef IS_64
    double         d;
    long           l;		
#endif
} JRI_JDK_stack_item;

typedef union JRI_JDK_Java8Str {
    jint x[2];
    jdouble d;
    jlong l;
    void *p;
    float f;
} JRI_JDK_Java8;


#ifdef __cplusplus
}
#endif
#endif

