








































#include "jni.h"
#include "nsISecureJNI2.h"
#include "nsHashtable.h"
#include "nsVoidArray.h"
#include "plstr.h"



static jni_type get_jni_type(char sig)
{
	switch (sig) {
	case 'L':
	case '[':
		return jobject_type;
	case 'Z':
		return jboolean_type;
	case 'B':
		return jbyte_type;
	case 'C':
		return jchar_type;
	case 'S':
		return jshort_type;
	case 'I':
		return jint_type;
	case 'J':
		return jlong_type;
	case 'F':
		return jfloat_type;
	case 'D':
		return jdouble_type;
	case 'V':
		return jvoid_type;
	}
	return jvoid_type;
}

static PRBool get_method_type(const char* sig, PRUint32& arg_count, jni_type*& arg_types, jni_type& return_type)
{
	arg_count = 0;
	if (sig[0] == '(') {
		nsVoidArray vec;
		++sig;
		while (*sig != ')' && *sig) {
			char arg_sig = *sig++;
			jni_type arg_type = get_jni_type(arg_sig);
			if (arg_type == jobject_type) {
				
				while (*sig == '[') ++sig;
				if (*sig == 'L') {
					
					++sig;
					while (*sig != ';') ++sig;
				}
				
				++sig;
			}
			vec.AppendElement((void*)arg_type);
		}
		arg_count = vec.Count();
		arg_types = new jni_type[arg_count];
		for (int index = arg_count - 1; index >= 0; --index)
			arg_types[index] = jni_type(vec.ElementAt(index));
		if (*sig == ')') {
			char return_sig = *++sig;
			return_type = get_jni_type(return_sig);
		}
	}
	return PR_FALSE;
}

class JNIHashKey : public nsHashKey {
public:
	JNIHashKey(void* key) : mKey(key) {}

	virtual PRUint32 HashValue(void) const { return PRUint32(mKey); }
	virtual PRBool Equals(const nsHashKey *aKey) const { return mKey == ((JNIHashKey*)aKey)->mKey; }
	virtual nsHashKey *Clone(void) const { return new JNIHashKey(mKey); }

private:
	void* mKey;
};

struct JNIMember {
	char* mName;
	char* mSignature;
	
	JNIMember(const char* name, const char* sig);
	~JNIMember();
};

JNIMember::JNIMember(const char* name, const char* sig)
	: mName(NULL), mSignature(NULL)
{
	mName = PL_strdup(name);
	mSignature = PL_strdup(sig);
}

JNIMember::~JNIMember()
{
	PL_strfree(mName);
	PL_strfree(mSignature);
}

struct JNIField : JNIMember {
	jfieldID mFieldID;
	jni_type mFieldType;
	
	JNIField(const char* name, const char* sig, jfieldID fieldID);
};

JNIField::JNIField(const char* name, const char* sig, jfieldID fieldID)
	: JNIMember(name, sig), mFieldID(fieldID), mFieldType(get_jni_type(*sig))
{
}

struct JNIMethod : JNIMember {
	jmethodID mMethodID;
	PRUint32 mArgCount;
	jni_type* mArgTypes;
	jni_type mReturnType;
	
	JNIMethod(const char* name, const char* sig, jmethodID methodID);
	~JNIMethod();
	
	jvalue* marshallArgs(va_list args);
};

JNIMethod::JNIMethod(const char* name, const char* sig, jmethodID methodID)
	: JNIMember(name, sig), mMethodID(methodID), mArgCount(0), mArgTypes(NULL), mReturnType(jvoid_type)
{
	get_method_type(sig, mArgCount, mArgTypes, mReturnType);
}

JNIMethod::~JNIMethod()
{
	if (mArgTypes != NULL)
		delete[] mArgTypes;
}




jvalue* JNIMethod::marshallArgs(va_list args)
{
	PRUint32 argCount = mArgCount;
	jni_type* argTypes = mArgTypes;
	jvalue* jargs = new jvalue[argCount];
	if (jargs != NULL) {
		for (int i = 0; i < argCount; i++) {
			switch (argTypes[i]) {
			case jobject_type:
				jargs[i].l = va_arg(args, jobject);
				break;
			case jboolean_type:
				jargs[i].z = va_arg(args, jboolean);
				break;
			case jbyte_type:
				jargs[i].b = va_arg(args, jbyte);
				break;
			case jchar_type:
 				jargs[i].b = va_arg(args, jbyte);
				break;
			case jshort_type:
 				jargs[i].s = va_arg(args, jshort);
				break;
			case jint_type:
 				jargs[i].i = va_arg(args, jint);
				break;
			case jlong_type:
 				jargs[i].j = va_arg(args, jlong);
				break;
			case jfloat_type:
 				jargs[i].f = va_arg(args, jfloat);
				break;
			case jdouble_type:
 				jargs[i].d = va_arg(args, jdouble);
				break;
			}
		}
	}
	return jargs;
}





class MarshalledArgs {
public:
	MarshalledArgs(JNIMethod* forMethod, va_list args) : mArgs(forMethod->marshallArgs(args)) {}
	~MarshalledArgs() { delete[] mArgs; }

	operator jvalue* () { return mArgs; }
	
private:
	jvalue* mArgs;
};

class nsJNIEnv : public JNIEnv {
private:
	static JNINativeInterface_ theFuncs;
	static nsHashtable* theIDTable;
	nsISecureJNI2* mSecureEnv;
	nsISecurityContext* mContext;
	jobject mJavaThread;
	
	static nsJNIEnv& nsJNIEnvRef(JNIEnv* env) { return *(nsJNIEnv*)env; }

	nsISecureJNI2* operator->() { return mSecureEnv; }
	nsISecurityContext* getContext() { return mContext; }
	
	static jint JNICALL GetVersion(JNIEnv* env)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jint version = 0;
		nsresult result =  secureEnv->GetVersion(&version);
		return version;
	}

	static jclass JNICALL DefineClass(JNIEnv *env, const char *name, jobject loader, const jbyte *buf, jsize len)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jclass outClass = NULL;
		nsresult result = secureEnv->DefineClass(name, loader, buf, len, &outClass);
		return outClass;
	}

	static jclass JNICALL FindClass(JNIEnv *env, const char *name)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jclass outClass = NULL;
		nsresult result = secureEnv->FindClass(name, &outClass);
		return outClass;
	}

	static jclass JNICALL GetSuperclass(JNIEnv *env, jclass sub)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jclass outSuper = NULL;
		nsresult result = secureEnv->GetSuperclass(sub, &outSuper);
		return outSuper;
	}

	static jboolean JNICALL IsAssignableFrom(JNIEnv *env, jclass sub, jclass sup)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jboolean outIsAssignable = FALSE;
		nsresult result = secureEnv->IsAssignableFrom(sub, sup, &outIsAssignable);
		return outIsAssignable;
	}
	
	static jint Throw(JNIEnv *env, jthrowable obj)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jint outStatus = FALSE;
		nsresult result = secureEnv->Throw(obj, &outStatus);
		return outStatus;
	}
	
	static jint JNICALL ThrowNew(JNIEnv *env, jclass clazz, const char *msg)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jint outStatus = FALSE;
		nsresult result = secureEnv->ThrowNew(clazz, msg, &outStatus);
		return outStatus;
	}
	
	static jthrowable JNICALL ExceptionOccurred(JNIEnv *env)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jthrowable outThrowable = NULL;
		nsresult result = secureEnv->ExceptionOccurred(&outThrowable);
		return outThrowable;
	}

	static void JNICALL ExceptionDescribe(JNIEnv *env)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->ExceptionDescribe();
	}
	
	static void JNICALL ExceptionClear(JNIEnv *env)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->ExceptionClear();
	}
	
	static void JNICALL FatalError(JNIEnv *env, const char *msg)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->FatalError(msg);
	}

	static jobject JNICALL NewGlobalRef(JNIEnv *env, jobject lobj)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jobject outGlobalRef = NULL;
		nsresult result = secureEnv->NewGlobalRef(lobj, &outGlobalRef);
		return outGlobalRef;
	}
	
	static void JNICALL DeleteGlobalRef(JNIEnv *env, jobject gref)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->DeleteGlobalRef(gref);
	}
	
	static void JNICALL DeleteLocalRef(JNIEnv *env, jobject obj)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->DeleteLocalRef(obj);
	}
	
	static jboolean JNICALL IsSameObject(JNIEnv *env, jobject obj1, jobject obj2)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jboolean outIsSameObject = FALSE;
		nsresult result = secureEnv->IsSameObject(obj1, obj2, &outIsSameObject);
		return outIsSameObject;
	}

	static jobject JNICALL AllocObject(JNIEnv *env, jclass clazz)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jobject outObject = NULL;
		nsresult result = secureEnv->AllocObject(clazz, &outObject);
		return outObject;
	}
	
	static jobject JNICALL NewObject(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
	{
		va_list args; va_start(args, methodID);
		jobject outObject = NewObjectV(env, clazz, methodID, args);
		va_end(args);
		return outObject;
	}
	
	static jobject JNICALL NewObjectV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
	{
		jobject outObject = NULL;
		
		
		JNIMethod* method = (JNIMethod*)methodID;
		MarshalledArgs jargs(method, args);
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->NewObject(clazz, method->mMethodID, jargs, &outObject, secureEnv.getContext());
		
		return outObject;
	}

	static jobject JNICALL NewObjectA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args)
	{
		jobject outObject = NULL;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		JNIMethod* method = (JNIMethod*)methodID;
		nsresult result = secureEnv->NewObject(clazz, method->mMethodID, args, &outObject, secureEnv.getContext());
		return outObject;
	}

	static jclass JNICALL GetObjectClass(JNIEnv *env, jobject obj)
	{
		jclass outClass = NULL;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->GetObjectClass(obj, &outClass);
		return outClass;
	}
	
	static jboolean JNICALL IsInstanceOf(JNIEnv *env, jobject obj, jclass clazz)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jboolean outIsInstanceOf = FALSE;
		nsresult result = secureEnv->IsInstanceOf(obj, clazz, &outIsInstanceOf);
		return outIsInstanceOf;
	}

	static jmethodID JNICALL GetMethodID(JNIEnv *env, jclass clazz, const char *name, const char *sig)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jmethodID outMethodID = NULL;
		nsresult result = secureEnv->GetMethodID(clazz, name, sig, &outMethodID);
		if (result == NS_OK) {
			JNIHashKey key(outMethodID);
			JNIMethod* method = (JNIMethod*) theIDTable->Get(&key);
			if (method == NULL) {
				method = new JNIMethod(name, sig, outMethodID);
				theIDTable->Put(&key, method);
			}
			outMethodID = jmethodID(method);
		}
		return outMethodID;
	}
	
	


	
	
	
	static jvalue InvokeMethod(JNIEnv *env, jobject obj, JNIMethod* method, jvalue* args)
	{
		jvalue outValue = { NULL };
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->CallMethod(method->mReturnType, obj, method->mMethodID, args, &outValue, secureEnv.getContext());
		return outValue;
	}
	
	static jvalue InvokeMethod(JNIEnv *env, jobject obj, JNIMethod* method, va_list args)
	{
		
		MarshalledArgs jargs(method, args);
		return InvokeMethod(env, obj, method, jargs);
	}
	
	static void InvokeVoidMethod(JNIEnv *env, jobject obj, JNIMethod* method, jvalue* args)
	{
		jvalue unusedValue;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->CallMethod(jvoid_type, obj, method->mMethodID, args, &unusedValue, secureEnv.getContext());
	}

	static void InvokeVoidMethod(JNIEnv *env, jobject obj, JNIMethod* method, va_list args)
	{
		
		MarshalledArgs jargs(method, args);
		InvokeVoidMethod(env, obj, method, jargs);
	}

#define IMPLEMENT_METHOD_FAMILY(methodName, returnType, jvalueField) \
	static returnType JNICALL methodName(JNIEnv *env, jobject obj, jmethodID methodID, ...)					\
	{																										\
		va_list args; va_start(args, methodID);																\
		returnType result = InvokeMethod(env, obj, (JNIMethod*)methodID, args).jvalueField;					\
		va_end(args);																						\
		return result;																						\
	}																										\
																											\
	static returnType JNICALL methodName##V(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)		\
	{																										\
		return InvokeMethod(env, obj, (JNIMethod*)methodID, args).jvalueField;								\
	}																										\
																											\
	static returnType JNICALL methodName##A(JNIEnv *env, jobject obj, jmethodID methodID, jvalue * args)	\
	{																										\
		return InvokeMethod(env, obj, (JNIMethod*)methodID, args).jvalueField;								\
	}																										\

	
	


















	
	IMPLEMENT_METHOD_FAMILY(CallObjectMethod, jobject, l)
	IMPLEMENT_METHOD_FAMILY(CallBooleanMethod, jboolean, z)
	IMPLEMENT_METHOD_FAMILY(CallByteMethod, jbyte, b)
	IMPLEMENT_METHOD_FAMILY(CallCharMethod, jchar, c)
	IMPLEMENT_METHOD_FAMILY(CallShortMethod, jshort, s)
	IMPLEMENT_METHOD_FAMILY(CallIntMethod, jint, i)
	IMPLEMENT_METHOD_FAMILY(CallLongMethod, jlong, j)
	IMPLEMENT_METHOD_FAMILY(CallFloatMethod, jfloat, f)
	IMPLEMENT_METHOD_FAMILY(CallDoubleMethod, jdouble, d)

#undef IMPLEMENT_METHOD_FAMILY

	static void JNICALL CallVoidMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...)
	{
		va_list args; va_start(args, methodID);
		InvokeVoidMethod(env, obj, (JNIMethod*)methodID, args);
		va_end(args);
	}
	
	static void JNICALL CallVoidMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list args)
	{
		InvokeVoidMethod(env, obj, (JNIMethod*)methodID, args);
	}
	
	static void JNICALL CallVoidMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue * args)
	{
		InvokeVoidMethod(env, obj, (JNIMethod*)methodID, args);
	}

	

	static jvalue InvokeNonVirtualMethod(JNIEnv *env, jobject obj, jclass clazz, JNIMethod* method, jvalue* args)
	{
		jvalue outValue = { NULL };
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->CallNonvirtualMethod(method->mReturnType, obj, clazz, method->mMethodID, args, &outValue, secureEnv.getContext());
		return outValue;
	}
	
	static jvalue InvokeNonVirtualMethod(JNIEnv *env, jobject obj, jclass clazz, JNIMethod* method, va_list args)
	{
		
		MarshalledArgs jargs(method, args);
		return InvokeNonVirtualMethod(env, obj, clazz, method, jargs);
	}
	
	static void InvokeNonVirtualVoidMethod(JNIEnv *env, jobject obj, jclass clazz, JNIMethod* method, jvalue* args)
	{
		jvalue unusedValue;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->CallNonvirtualMethod(jvoid_type, obj, clazz, method->mMethodID, args, &unusedValue, secureEnv.getContext());
	}

	static void InvokeNonVirtualVoidMethod(JNIEnv *env, jobject obj, jclass clazz, JNIMethod* method, va_list args)
	{
		
		MarshalledArgs jargs(method, args);
		InvokeNonVirtualVoidMethod(env, obj, clazz, method, jargs);
	}

#define IMPLEMENT_METHOD_FAMILY(methodName, returnType, jvalueField) \
	static returnType JNICALL methodName(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)				\
	{																													\
		va_list args; va_start(args, methodID);																			\
		returnType result = InvokeNonVirtualMethod(env, obj, clazz, (JNIMethod*)methodID, args).jvalueField;			\
		va_end(args);																									\
		return result;																									\
	}																													\
																														\
	static returnType JNICALL methodName##V(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)	\
	{																													\
		return InvokeNonVirtualMethod(env, obj, clazz, (JNIMethod*)methodID, args).jvalueField;							\
	}																													\
																														\
	static returnType JNICALL methodName##A(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue * args)	\
	{																													\
		return InvokeNonVirtualMethod(env, obj, clazz, (JNIMethod*)methodID, args).jvalueField;							\
	}																													\

	IMPLEMENT_METHOD_FAMILY(CallNonvirtualObjectMethod, jobject, l)
	IMPLEMENT_METHOD_FAMILY(CallNonvirtualBooleanMethod, jboolean, z)
	IMPLEMENT_METHOD_FAMILY(CallNonvirtualByteMethod, jbyte, b)
	IMPLEMENT_METHOD_FAMILY(CallNonvirtualCharMethod, jchar, c)
	IMPLEMENT_METHOD_FAMILY(CallNonvirtualShortMethod, jshort, s)
	IMPLEMENT_METHOD_FAMILY(CallNonvirtualIntMethod, jint, i)
	IMPLEMENT_METHOD_FAMILY(CallNonvirtualLongMethod, jlong, j)
	IMPLEMENT_METHOD_FAMILY(CallNonvirtualFloatMethod, jfloat, f)
	IMPLEMENT_METHOD_FAMILY(CallNonvirtualDoubleMethod, jdouble, d)

#undef IMPLEMENT_METHOD_FAMILY

	static void JNICALL CallNonvirtualVoidMethod(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...)
	{
		va_list args; va_start(args, methodID);
		InvokeNonVirtualVoidMethod(env, obj, clazz, (JNIMethod*)methodID, args);
		va_end(args);
	}
	
	static void JNICALL CallNonvirtualVoidMethodV(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
	{
		InvokeNonVirtualVoidMethod(env, obj, clazz, (JNIMethod*)methodID, args);
	}
	
	static void JNICALL CallNonvirtualVoidMethodA(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, jvalue * args)
	{
		InvokeNonVirtualVoidMethod(env, obj, clazz, (JNIMethod*)methodID, args);
	}

	

	static jfieldID JNICALL GetFieldID(JNIEnv *env, jclass clazz, const char *name, const char *sig)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jfieldID outFieldID = NULL;
		nsresult result = secureEnv->GetFieldID(clazz, name, sig, &outFieldID);
		if (result == NS_OK) {
			JNIHashKey key(outFieldID);
			JNIField* field = (JNIField*) theIDTable->Get(&key);
			if (field == NULL) {
				field = new JNIField(name, sig, outFieldID);
				theIDTable->Put(&key, field);
			}
			outFieldID = jfieldID(field);
		}
		return outFieldID;
	}

	static jvalue GetField(JNIEnv* env, jobject obj, JNIField* field)
	{
		jvalue outValue = { NULL };
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->GetField(field->mFieldType, obj, field->mFieldID, &outValue, secureEnv.getContext());
		return outValue;
	}

#define IMPLEMENT_GET_FIELD(methodName, returnType, jvalueField)							\
	static returnType JNICALL methodName(JNIEnv *env, jobject obj, jfieldID fieldID)		\
	{																						\
		return GetField(env, obj, (JNIField*)fieldID).jvalueField;							\
	}																						\

	IMPLEMENT_GET_FIELD(GetObjectField, jobject, l)
	IMPLEMENT_GET_FIELD(GetBooleanField, jboolean, z)
	IMPLEMENT_GET_FIELD(GetByteField, jbyte, b)
	IMPLEMENT_GET_FIELD(GetCharField, jchar, c)
	IMPLEMENT_GET_FIELD(GetShortField, jshort, s)
	IMPLEMENT_GET_FIELD(GetIntField, jint, i)
	IMPLEMENT_GET_FIELD(GetLongField, jlong, j)
	IMPLEMENT_GET_FIELD(GetFloatField, jfloat, f)
	IMPLEMENT_GET_FIELD(GetDoubleField, jdouble, d)

#undef IMPLEMENT_GET_FIELD

	static void SetField(JNIEnv* env, jobject obj, JNIField* field, jvalue value)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->SetField(field->mFieldType, obj, field->mFieldID, value, secureEnv.getContext());
	}

#define IMPLEMENT_SET_FIELD(methodName, fieldType, jvalueField)										\
	static void JNICALL methodName(JNIEnv *env, jobject obj, jfieldID fieldID, fieldType value)		\
	{																								\
		jvalue fieldValue;																			\
		fieldValue.jvalueField = value;																\
		SetField(env, obj, (JNIField*)fieldID, fieldValue);											\
	}																								\

	IMPLEMENT_SET_FIELD(SetObjectField, jobject, l)
	IMPLEMENT_SET_FIELD(SetBooleanField, jboolean, z)
	IMPLEMENT_SET_FIELD(SetByteField, jbyte, b)
	IMPLEMENT_SET_FIELD(SetCharField, jchar, c)
	IMPLEMENT_SET_FIELD(SetShortField, jshort, s)
	IMPLEMENT_SET_FIELD(SetIntField, jint, i)
	IMPLEMENT_SET_FIELD(SetLongField, jlong, j)
	IMPLEMENT_SET_FIELD(SetFloatField, jfloat, f)
	IMPLEMENT_SET_FIELD(SetDoubleField, jdouble, d)

#undef IMPLEMENT_SET_FIELD

	

	static jmethodID JNICALL GetStaticMethodID(JNIEnv *env, jclass clazz, const char *name, const char *sig)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jmethodID outMethodID = NULL;
		nsresult result = secureEnv->GetStaticMethodID(clazz, name, sig, &outMethodID);
		if (result == NS_OK) {
			JNIHashKey key(outMethodID);
			JNIMethod* method = (JNIMethod*) theIDTable->Get(&key);
			if (method == NULL) {
				method = new JNIMethod(name, sig, outMethodID);
				theIDTable->Put(&key, method);
			}
			outMethodID = jmethodID(method);
		}
		return outMethodID;
	}

	
	
	
	
	static jvalue InvokeStaticMethod(JNIEnv *env, jclass clazz, JNIMethod* method, jvalue* args)
	{
		jvalue outValue = { NULL };
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->CallStaticMethod(method->mReturnType, clazz, method->mMethodID, args, &outValue, secureEnv.getContext());
		return outValue;
	}
	
	static jvalue InvokeStaticMethod(JNIEnv *env, jclass clazz, JNIMethod* method, va_list args)
	{
		
		MarshalledArgs jargs(method, args);
		return InvokeStaticMethod(env, clazz, method, jargs);
	}
	
	static void InvokeStaticVoidMethod(JNIEnv *env, jclass clazz, JNIMethod* method, jvalue* args)
	{
		jvalue unusedValue;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->CallStaticMethod(jvoid_type, clazz, method->mMethodID, args, &unusedValue, secureEnv.getContext());
	}

	static void InvokeStaticVoidMethod(JNIEnv *env, jclass clazz, JNIMethod* method, va_list args)
	{
		
		MarshalledArgs jargs(method, args);
		InvokeStaticVoidMethod(env, clazz, method, jargs);
	}

#define IMPLEMENT_METHOD_FAMILY(methodName, returnType, jvalueField) \
	static returnType JNICALL methodName(JNIEnv *env, jclass clazz, jmethodID methodID, ...)				\
	{																										\
		va_list args; va_start(args, methodID);																\
		returnType result = InvokeStaticMethod(env, clazz, (JNIMethod*)methodID, args).jvalueField;			\
		va_end(args);																						\
		return result;																						\
	}																										\
																											\
	static returnType JNICALL methodName##V(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)	\
	{																										\
		return InvokeStaticMethod(env, clazz, (JNIMethod*)methodID, args).jvalueField;						\
	}																										\
																											\
	static returnType JNICALL methodName##A(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue * args)	\
	{																										\
		return InvokeStaticMethod(env, clazz, (JNIMethod*)methodID, args).jvalueField;						\
	}																										\

	IMPLEMENT_METHOD_FAMILY(CallStaticObjectMethod, jobject, l)
	IMPLEMENT_METHOD_FAMILY(CallStaticBooleanMethod, jboolean, z)
	IMPLEMENT_METHOD_FAMILY(CallStaticByteMethod, jbyte, b)
	IMPLEMENT_METHOD_FAMILY(CallStaticCharMethod, jchar, c)
	IMPLEMENT_METHOD_FAMILY(CallStaticShortMethod, jshort, s)
	IMPLEMENT_METHOD_FAMILY(CallStaticIntMethod, jint, i)
	IMPLEMENT_METHOD_FAMILY(CallStaticLongMethod, jlong, j)
	IMPLEMENT_METHOD_FAMILY(CallStaticFloatMethod, jfloat, f)
	IMPLEMENT_METHOD_FAMILY(CallStaticDoubleMethod, jdouble, d)

#undef IMPLEMENT_METHOD_FAMILY

	static void JNICALL CallStaticVoidMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
	{
		va_list args; va_start(args, methodID);
		InvokeStaticVoidMethod(env, clazz, (JNIMethod*)methodID, args);
		va_end(args);
	}
	
	static void JNICALL CallStaticVoidMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
	{
		InvokeStaticVoidMethod(env, clazz, (JNIMethod*)methodID, args);
	}
	
	static void JNICALL CallStaticVoidMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue * args)
	{
		InvokeStaticVoidMethod(env, clazz, (JNIMethod*)methodID, args);
	}

	

	static jfieldID JNICALL GetStaticFieldID(JNIEnv *env, jclass clazz, const char *name, const char *sig)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		jfieldID outFieldID = NULL;
		nsresult result = secureEnv->GetStaticFieldID(clazz, name, sig, &outFieldID);
		if (result == NS_OK) {
			JNIHashKey key(outFieldID);
			JNIField* field = (JNIField*) theIDTable->Get(&key);
			if (field == NULL) {
				field = new JNIField(name, sig, outFieldID);
				theIDTable->Put(&key, field);
			}
			outFieldID = jfieldID(field);
		}
		return outFieldID;
	}

	static jvalue GetStaticField(JNIEnv* env, jclass clazz, JNIField* field)
	{
		jvalue outValue = { NULL };
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->GetStaticField(field->mFieldType, clazz, field->mFieldID, &outValue, secureEnv.getContext());
		return outValue;
	}

#define IMPLEMENT_GET_FIELD(methodName, returnType, jvalueField)							\
	static returnType JNICALL methodName(JNIEnv *env, jclass clazz, jfieldID fieldID)		\
	{																						\
		return GetStaticField(env, clazz, (JNIField*)fieldID).jvalueField;					\
	}																						\

	IMPLEMENT_GET_FIELD(GetStaticObjectField, jobject, l)
	IMPLEMENT_GET_FIELD(GetStaticBooleanField, jboolean, z)
	IMPLEMENT_GET_FIELD(GetStaticByteField, jbyte, b)
	IMPLEMENT_GET_FIELD(GetStaticCharField, jchar, c)
	IMPLEMENT_GET_FIELD(GetStaticShortField, jshort, s)
	IMPLEMENT_GET_FIELD(GetStaticIntField, jint, i)
	IMPLEMENT_GET_FIELD(GetStaticLongField, jlong, j)
	IMPLEMENT_GET_FIELD(GetStaticFloatField, jfloat, f)
	IMPLEMENT_GET_FIELD(GetStaticDoubleField, jdouble, d)

#undef IMPLEMENT_GET_FIELD

	static void SetStaticField(JNIEnv* env, jclass clazz, JNIField* field, jvalue value)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->SetStaticField(field->mFieldType, clazz, field->mFieldID, value, secureEnv.getContext());
	}

#define IMPLEMENT_SET_FIELD(methodName, fieldType, jvalueField)										\
	static void JNICALL methodName(JNIEnv *env, jclass clazz, jfieldID fieldID, fieldType value)	\
	{																								\
		jvalue fieldValue;																			\
		fieldValue.jvalueField = value;																\
		SetStaticField(env, clazz, (JNIField*)fieldID, fieldValue);									\
	}																								\

	IMPLEMENT_SET_FIELD(SetStaticObjectField, jobject, l)
	IMPLEMENT_SET_FIELD(SetStaticBooleanField, jboolean, z)
	IMPLEMENT_SET_FIELD(SetStaticByteField, jbyte, b)
	IMPLEMENT_SET_FIELD(SetStaticCharField, jchar, c)
	IMPLEMENT_SET_FIELD(SetStaticShortField, jshort, s)
	IMPLEMENT_SET_FIELD(SetStaticIntField, jint, i)
	IMPLEMENT_SET_FIELD(SetStaticLongField, jlong, j)
	IMPLEMENT_SET_FIELD(SetStaticFloatField, jfloat, f)
	IMPLEMENT_SET_FIELD(SetStaticDoubleField, jdouble, d)

#undef IMPLEMENT_SET_FIELD

	static jstring JNICALL NewString(JNIEnv *env, const jchar *unicode, jsize len)
	{
		jstring outString;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->NewString(unicode, len, &outString);
		return outString;
	}
	
	static jsize JNICALL GetStringLength(JNIEnv *env, jstring str)
	{
		jsize outLength;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->GetStringLength(str, &outLength);
		return outLength;
	}
	
	static const jchar* JNICALL GetStringChars(JNIEnv *env, jstring str, jboolean *isCopy)
	{
		jchar* outChars = NULL;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->GetStringChars(str, isCopy, &outChars);
		return outChars;
	}
	
	static void JNICALL ReleaseStringChars(JNIEnv *env, jstring str, const jchar *chars)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->ReleaseStringChars(str, chars);
	}

	static jstring JNICALL NewStringUTF(JNIEnv *env, const char *utf)
	{
		jstring outString;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->NewStringUTF(utf, &outString);
		return outString;
	}
	
	static jsize JNICALL GetStringUTFLength(JNIEnv *env, jstring str)
	{
		jsize outLength;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->GetStringUTFLength(str, &outLength);
		return outLength;
	}
	
	static const char* JNICALL GetStringUTFChars(JNIEnv *env, jstring str, jboolean *isCopy)
	{
		const char* outChars = NULL;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->GetStringUTFChars(str, isCopy, &outChars);
		return outChars;
	}
	
	static void JNICALL ReleaseStringUTFChars(JNIEnv *env, jstring str, const char* chars)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->ReleaseStringUTFChars(str, chars);
	}

	static jsize JNICALL GetArrayLength(JNIEnv *env, jarray array)
	{
		jsize outLength;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->GetArrayLength(array, &outLength);
		return outLength;
	}

	static jobjectArray JNICALL NewObjectArray(JNIEnv *env, jsize len, jclass clazz, jobject initVal)
	{
		jobjectArray outArray = NULL;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->NewObjectArray(len, clazz, initVal, &outArray);
		return outArray;
	}

	static jobject JNICALL GetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index)
	{
		jobject outObject = NULL;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->GetObjectArrayElement(array, index, &outObject);
		return outObject;
	}

	static void JNICALL SetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index, jobject val)
	{
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->SetObjectArrayElement(array, index, val);
	}

#define IMPLEMENT_NEW_ARRAY(methodName, type)														\
	static type##Array JNICALL methodName(JNIEnv *env, jsize len)									\
	{																								\
		type##Array outArray = NULL;																\
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);														\
		nsresult result = secureEnv->NewArray(type##_type, len, (jarray*)&outArray);				\
		return outArray;																			\
	}																								\

	IMPLEMENT_NEW_ARRAY(NewBooleanArray, jboolean)
	IMPLEMENT_NEW_ARRAY(NewByteArray, jbyte)
	IMPLEMENT_NEW_ARRAY(NewCharArray, jchar)
	IMPLEMENT_NEW_ARRAY(NewShortArray, jshort)
	IMPLEMENT_NEW_ARRAY(NewIntArray, jint)
	IMPLEMENT_NEW_ARRAY(NewLongArray, jlong)
	IMPLEMENT_NEW_ARRAY(NewFloatArray, jfloat)
	IMPLEMENT_NEW_ARRAY(NewDoubleArray, jdouble)

#undef IMPLEMENT_NEW_ARRAY

#define IMPLEMENT_GET_ARRAY_ELEMENTS(methodName, type)														\
	static type* JNICALL methodName(JNIEnv *env, type##Array array, jboolean *isCopy)						\
	{																										\
		type* outElements = NULL;																			\
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);																\
		nsresult result = secureEnv->GetArrayElements(type##_type, array, isCopy, &outElements);			\
		return outElements;																					\
	}																										\

	IMPLEMENT_GET_ARRAY_ELEMENTS(GetBooleanArrayElements, jboolean)
	IMPLEMENT_GET_ARRAY_ELEMENTS(GetByteArrayElements, jbyte)
	IMPLEMENT_GET_ARRAY_ELEMENTS(GetCharArrayElements, jchar)
	IMPLEMENT_GET_ARRAY_ELEMENTS(GetShortArrayElements, jshort)
	IMPLEMENT_GET_ARRAY_ELEMENTS(GetIntArrayElements, jint)
	IMPLEMENT_GET_ARRAY_ELEMENTS(GetLongArrayElements, jlong)
	IMPLEMENT_GET_ARRAY_ELEMENTS(GetFloatArrayElements, jfloat)
	IMPLEMENT_GET_ARRAY_ELEMENTS(GetDoubleArrayElements, jdouble)

#undef IMPLEMENT_GET_ARRAY_ELEMENTS

#define IMPLEMENT_RELEASE_ARRAY_ELEMENTS(methodName, type)													\
	static void JNICALL methodName(JNIEnv *env, type##Array array, type* elems, jint mode)					\
	{																										\
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);																\
		nsresult result = secureEnv->ReleaseArrayElements(type##_type, array, elems, mode);					\
	}																										\

	IMPLEMENT_RELEASE_ARRAY_ELEMENTS(ReleaseBooleanArrayElements, jboolean)
	IMPLEMENT_RELEASE_ARRAY_ELEMENTS(ReleaseByteArrayElements, jbyte)
	IMPLEMENT_RELEASE_ARRAY_ELEMENTS(ReleaseCharArrayElements, jchar)
	IMPLEMENT_RELEASE_ARRAY_ELEMENTS(ReleaseShortArrayElements, jshort)
	IMPLEMENT_RELEASE_ARRAY_ELEMENTS(ReleaseIntArrayElements, jint)
	IMPLEMENT_RELEASE_ARRAY_ELEMENTS(ReleaseLongArrayElements, jlong)
	IMPLEMENT_RELEASE_ARRAY_ELEMENTS(ReleaseFloatArrayElements, jfloat)
	IMPLEMENT_RELEASE_ARRAY_ELEMENTS(ReleaseDoubleArrayElements, jdouble)

#undef IMPLEMENT_RELEASE_ARRAY_ELEMENTS

#define IMPLEMENT_GET_ARRAY_REGION(methodName, type)														\
	static void JNICALL methodName(JNIEnv *env, type##Array array, jsize start, jsize len, type* buf)		\
	{																										\
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);																\
		nsresult result = secureEnv->GetArrayRegion(type##_type, array, start, len, buf);					\
	}																										\

	IMPLEMENT_GET_ARRAY_REGION(GetBooleanArrayRegion, jboolean)
	IMPLEMENT_GET_ARRAY_REGION(GetByteArrayRegion, jbyte)
	IMPLEMENT_GET_ARRAY_REGION(GetCharArrayRegion, jchar)
	IMPLEMENT_GET_ARRAY_REGION(GetShortArrayRegion, jshort)
	IMPLEMENT_GET_ARRAY_REGION(GetIntArrayRegion, jint)
	IMPLEMENT_GET_ARRAY_REGION(GetLongArrayRegion, jlong)
	IMPLEMENT_GET_ARRAY_REGION(GetFloatArrayRegion, jfloat)
	IMPLEMENT_GET_ARRAY_REGION(GetDoubleArrayRegion, jdouble)

#undef IMPLEMENT_GET_ARRAY_REGION

#define IMPLEMENT_SET_ARRAY_REGION(methodName, type)														\
	static void JNICALL methodName(JNIEnv *env, type##Array array, jsize start, jsize len, type* buf)		\
	{																										\
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);																\
		nsresult result = secureEnv->SetArrayRegion(type##_type, array, start, len, buf);					\
	}																										\

	IMPLEMENT_SET_ARRAY_REGION(SetBooleanArrayRegion, jboolean)
	IMPLEMENT_SET_ARRAY_REGION(SetByteArrayRegion, jbyte)
	IMPLEMENT_SET_ARRAY_REGION(SetCharArrayRegion, jchar)
	IMPLEMENT_SET_ARRAY_REGION(SetShortArrayRegion, jshort)
	IMPLEMENT_SET_ARRAY_REGION(SetIntArrayRegion, jint)
	IMPLEMENT_SET_ARRAY_REGION(SetLongArrayRegion, jlong)
	IMPLEMENT_SET_ARRAY_REGION(SetFloatArrayRegion, jfloat)
	IMPLEMENT_SET_ARRAY_REGION(SetDoubleArrayRegion, jdouble)

#undef IMPLEMENT_SET_ARRAY_REGION

	static jint JNICALL RegisterNatives(JNIEnv *env, jclass clazz, const JNINativeMethod *methods, jint nMethods)
	{
		jint outStatus = 0;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->RegisterNatives(clazz, methods, nMethods, &outStatus);
		return outStatus;
	}
	
	static jint JNICALL UnregisterNatives(JNIEnv *env, jclass clazz)
	{
		jint outStatus = 0;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->UnregisterNatives(clazz, &outStatus);
		return outStatus;
	}

	static jint JNICALL MonitorEnter(JNIEnv *env, jobject obj)
	{
		jint outStatus = 0;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->MonitorEnter(obj, &outStatus);
		return outStatus;
	}
	
	static jint JNICALL JNICALL MonitorExit(JNIEnv *env, jobject obj)
	{
		jint outStatus = 0;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->MonitorExit(obj, &outStatus);
		return outStatus;
	}

	static jint JNICALL GetJavaVM(JNIEnv *env, JavaVM **vm)
	{
		jint outStatus = 0;
		nsJNIEnv& secureEnv = nsJNIEnvRef(env);
		nsresult result = secureEnv->GetJavaVM(vm, &outStatus);
		return outStatus;
	}

public:
	nsJNIEnv(nsISecureJNI2* secureEnv);
	~nsJNIEnv();
};

JNINativeInterface_ nsJNIEnv::theFuncs = {
	NULL,	
	NULL,	
	NULL,	

	NULL,	

	
	&GetVersion,

	
	&DefineClass,
	
	
	&FindClass,

	NULL,	
	NULL,	
	NULL,	

	
	&GetSuperclass,
	
	
	&IsAssignableFrom,
	
	NULL, 

	
	&Throw,
	
	
	&ThrowNew,
	
	
	&ExceptionOccurred,
	
	
	&ExceptionDescribe,
	
	
	&ExceptionClear,
	
	
	&FatalError,
	
	NULL, 
	NULL, 

	
	&NewGlobalRef,
	
	
	&DeleteGlobalRef,
	
	
	&DeleteLocalRef,
	
	
	&IsSameObject,
	
	NULL, 
	NULL, 

	
	&AllocObject,
	
#define REFERENCE_METHOD_FAMILY(methodName) &methodName, &methodName##V, &methodName##A,

	
	
	
	REFERENCE_METHOD_FAMILY(NewObject)
	
	
	&GetObjectClass,
	
	
	&IsInstanceOf,

	
	&GetMethodID,

	
	
	
	REFERENCE_METHOD_FAMILY(CallObjectMethod)
	
	
	
	
	REFERENCE_METHOD_FAMILY(CallBooleanMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallByteMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallCharMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallShortMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallIntMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallLongMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallFloatMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallDoubleMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallVoidMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallNonvirtualObjectMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallNonvirtualBooleanMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallNonvirtualByteMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallNonvirtualCharMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallNonvirtualShortMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallNonvirtualIntMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallNonvirtualLongMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallNonvirtualFloatMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallNonvirtualDoubleMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallNonvirtualVoidMethod)

	
	&GetFieldID,

	
	
	
	
	
	
	
	
	
	&GetObjectField, &GetBooleanField, &GetByteField, &GetCharField, &GetShortField, &GetIntField, &GetLongField,
	&GetFloatField, &GetDoubleField,

	
	
	
	
	
	
	
	
	
	&SetObjectField, &SetBooleanField, &SetByteField, &SetCharField, &SetShortField, &SetIntField, &SetLongField,
	&SetFloatField, &SetDoubleField,

	
	&GetStaticMethodID,

	
	
	
	REFERENCE_METHOD_FAMILY(CallStaticObjectMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallStaticBooleanMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallStaticByteMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallStaticCharMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallStaticShortMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallStaticIntMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallStaticLongMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallStaticFloatMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallStaticDoubleMethod)

	
	
	
	REFERENCE_METHOD_FAMILY(CallStaticVoidMethod)

	
	&GetStaticFieldID,
	
	
	
	
	
	
	
	
	
	
	&GetStaticObjectField, &GetStaticBooleanField, &GetStaticByteField, &GetStaticCharField, &GetStaticShortField,
	&GetStaticIntField, &GetStaticLongField, &GetStaticFloatField, &GetStaticDoubleField,

	
	
	
	
	
	
	
	
	
	&SetStaticObjectField, &SetStaticBooleanField, &SetStaticByteField, &SetStaticCharField, &SetStaticShortField,
	&SetStaticIntField, &SetStaticLongField, &SetStaticFloatField, &SetStaticDoubleField,

	
	
	
	
	&NewString, &GetStringLength, &GetStringChars, &ReleaseStringChars,

	
	
	
	
	&NewStringUTF, &GetStringUTFLength, &GetStringUTFChars, &ReleaseStringUTFChars,

	
	&GetArrayLength,

	
	
	
	&NewObjectArray, &GetObjectArrayElement, &SetObjectArrayElement,

	
	
	
	
	
	
	
	
	&NewBooleanArray, &NewByteArray, &NewCharArray, &NewShortArray, 
	&NewIntArray, &NewLongArray, &NewFloatArray, &NewDoubleArray,

	
	
	
	
	
	
	
	
	&GetBooleanArrayElements, &GetByteArrayElements, &GetCharArrayElements, &GetShortArrayElements, 
	&GetIntArrayElements, &GetLongArrayElements, &GetFloatArrayElements, &GetDoubleArrayElements, 

	
	
	
	
	
	
	
	
	&ReleaseBooleanArrayElements, &ReleaseByteArrayElements, &ReleaseCharArrayElements, &ReleaseShortArrayElements, 
	&ReleaseIntArrayElements, &ReleaseLongArrayElements, &ReleaseFloatArrayElements, &ReleaseDoubleArrayElements, 

	
	
	
	
	
	
	
	
	&GetBooleanArrayRegion, &GetByteArrayRegion, &GetCharArrayRegion, &GetShortArrayRegion, 
	&GetIntArrayRegion, &GetLongArrayRegion, &GetFloatArrayRegion, &GetDoubleArrayRegion, 

	
	
	
	
	
	
	
	
	&SetBooleanArrayRegion, &SetByteArrayRegion, &SetCharArrayRegion, &SetShortArrayRegion, 
	&SetIntArrayRegion, &SetLongArrayRegion, &SetFloatArrayRegion, &SetDoubleArrayRegion, 

	
	
	&RegisterNatives, &UnregisterNatives,

	
	
	&MonitorEnter, &MonitorExit,

	
	&GetJavaVM
};

nsHashtable* nsJNIEnv::theIDTable = NULL;

nsJNIEnv::nsJNIEnv(nsISecureJNI2* secureEnv)
	:	mSecureEnv(secureEnv), mContext(NULL), mJavaThread(NULL)
{
	this->functions = &theFuncs;
	if (theIDTable == NULL)
		theIDTable = new nsHashtable();
}

nsJNIEnv::~nsJNIEnv()
{
	this->functions = NULL;
}
