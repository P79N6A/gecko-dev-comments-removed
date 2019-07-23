




































#define IMPLEMENT_GetFieldID_METHOD(class_name, param_name, returnType)\
                         \
        jclass clazz = env->FindClass(class_name); \
        /*jobject obj = env->AllocObject(clazz); */    \
        jfieldID fieldID = env->GetFieldID(clazz, param_name, returnType ); \
        printf("fieldID = %d\n", (int)fieldID); \

#define IMPLEMENT_GetStaticFieldID_METHOD(class_name, param_name, returnType)\
                         \
        jclass clazz = env->FindClass(class_name); \
        /*jobject obj = env->AllocObject(clazz); */    \
        jfieldID fieldID = env->GetStaticFieldID(clazz, param_name, returnType ); \
        printf("fieldID = %d\n", (int)fieldID); \

#define IMPLEMENT_GetMethodID_METHOD(class_name, func_name, returnType)\
        jclass clazz = env->FindClass(class_name);\
        jobject obj = env->AllocObject(clazz);    \
        jmethodID MethodID = env->GetMethodID(clazz, func_name, returnType);\
        printf("ID of %s method = %d\n", func_name, (int)MethodID); 

#define IMPLEMENT_GetStaticMethodID_METHOD(class_name, func_name, returnType)\
        jclass clazz = env->FindClass(class_name);\
        jobject obj = env->AllocObject(clazz);    \
        jmethodID MethodID = env->GetStaticMethodID(clazz, func_name, returnType);\
        printf("ID of static %s method = %d\n", func_name, (int)MethodID); 
