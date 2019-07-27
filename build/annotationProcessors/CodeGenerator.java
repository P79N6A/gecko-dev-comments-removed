



package org.mozilla.gecko.annotationProcessors;

import org.mozilla.gecko.annotationProcessors.classloader.AnnotatableEntity;
import org.mozilla.gecko.annotationProcessors.utils.Utils;

import java.lang.annotation.Annotation;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.HashMap;
import java.util.HashSet;

public class CodeGenerator {
    private static final Class<?>[] EMPTY_CLASS_ARRAY = new Class<?>[0];
    private static final Annotation[][] GETTER_ARGUMENT_ANNOTATIONS = new Annotation[0][0];
    private static final Annotation[][] SETTER_ARGUMENT_ANNOTATIONS = new Annotation[1][0];

    
    private final StringBuilder zeroingCode = new StringBuilder();
    private final StringBuilder wrapperStartupCode = new StringBuilder();
    private final StringBuilder wrapperMethodBodies = new StringBuilder();
    private final StringBuilder headerPublic = new StringBuilder();
    private final StringBuilder headerProtected = new StringBuilder();

    private final HashSet<String> seenClasses = new HashSet<String>();

    private final String mCClassName;

    private final Class<?> mClassToWrap;

    private boolean mHasEncounteredDefaultConstructor;

    
    private final HashMap<Member, String> mMembersToIds = new HashMap<Member, String>();
    private final HashSet<String> mTakenMemberNames = new HashSet<String>();
    private int mNameMunger;

    private final boolean mLazyInit;

    public CodeGenerator(Class<?> aClass, String aGeneratedName) {
        this(aClass, aGeneratedName, false);
    }

    public CodeGenerator(Class<?> aClass, String aGeneratedName, boolean aLazyInit) {
        mClassToWrap = aClass;
        mCClassName = aGeneratedName;
        mLazyInit = aLazyInit;

        
        
        
        
        wrapperStartupCode.append("void ").append(mCClassName).append("::InitStubs(JNIEnv *env) {\n");

        
        headerPublic.append("class ").append(mCClassName).append(" : public AutoGlobalWrappedJavaObject {\n" +
                            "public:\n" +
                            "    static void InitStubs(JNIEnv *env);\n");
        headerProtected.append("protected:");

        generateWrapperMethod();
    }

    



    private void generateWrapperMethod() {
        headerPublic.append("    static ").append(mCClassName).append("* Wrap(jobject obj);\n" +
                "    ").append(mCClassName).append("(jobject obj, JNIEnv* env) : AutoGlobalWrappedJavaObject(obj, env) {};\n");

        wrapperMethodBodies.append("\n").append(mCClassName).append("* ").append(mCClassName).append("::Wrap(jobject obj) {\n" +
                "    JNIEnv *env = GetJNIForThread();\n" +
                "    ").append(mCClassName).append("* ret = new ").append(mCClassName).append("(obj, env);\n" +
                "    env->DeleteLocalRef(obj);\n" +
                "    return ret;\n" +
                "}\n");
    }

    private void generateMemberCommon(Member theMethod, String aCMethodName, Class<?> aClass) {
        ensureClassHeaderAndStartup(aClass);
        writeMemberIdField(theMethod, aCMethodName);

        if (!mLazyInit) {
            writeMemberInit(theMethod, wrapperStartupCode);
        }
    }

    




    public void generateMethod(AnnotatableEntity aMethodTuple) {
        
        Method theMethod = aMethodTuple.getMethod();

        String CMethodName = aMethodTuple.mAnnotationInfo.wrapperName;

        generateMemberCommon(theMethod, CMethodName, mClassToWrap);

        boolean isFieldStatic = Utils.isMemberStatic(theMethod);

        Class<?>[] parameterTypes = theMethod.getParameterTypes();
        Class<?> returnType = theMethod.getReturnType();

        
        String implementationSignature = Utils.getCImplementationMethodSignature(parameterTypes, returnType, CMethodName,
            mCClassName, aMethodTuple.mAnnotationInfo.narrowChars, aMethodTuple.mAnnotationInfo.catchException);
        String headerSignature = Utils.getCHeaderMethodSignature(parameterTypes, theMethod.getParameterAnnotations(), returnType,
            CMethodName, mCClassName, isFieldStatic, aMethodTuple.mAnnotationInfo.narrowChars, aMethodTuple.mAnnotationInfo.catchException);

        
        writeSignatureToHeader(headerSignature);

        
        writeMethodBody(implementationSignature, theMethod, mClassToWrap,
                        aMethodTuple.mAnnotationInfo.isMultithreaded,
                        aMethodTuple.mAnnotationInfo.noThrow,
                        aMethodTuple.mAnnotationInfo.narrowChars,
                        aMethodTuple.mAnnotationInfo.catchException);
    }

    private void generateGetterOrSetterBody(Field aField, String aFieldName, boolean aIsFieldStatic, boolean isSetter, boolean aNarrowChars) {
        StringBuilder argumentContent = null;
        Class<?> fieldType = aField.getType();

        if (isSetter) {
            Class<?>[] setterArguments = new Class<?>[]{fieldType};
            
            argumentContent = getArgumentMarshalling(setterArguments);
        }

        if (mLazyInit) {
            writeMemberInit(aField, wrapperMethodBodies);
        }

        boolean isObjectReturningMethod = Utils.isObjectType(fieldType);
        wrapperMethodBodies.append("    ");
        if (isSetter) {
            wrapperMethodBodies.append("env->Set");
        } else {
            wrapperMethodBodies.append("return ");

            if (isObjectReturningMethod) {
                wrapperMethodBodies.append("static_cast<").append(Utils.getCReturnType(fieldType, aNarrowChars)).append(">(");
            }

            wrapperMethodBodies.append("env->Get");
        }

        if (aIsFieldStatic) {
            wrapperMethodBodies.append("Static");
        }
        wrapperMethodBodies.append(Utils.getFieldType(fieldType))
                           .append("Field(");

        
        if (aIsFieldStatic) {
            wrapperMethodBodies.append(Utils.getClassReferenceName(mClassToWrap));
        } else {
            wrapperMethodBodies.append("wrapped_obj");
        }
        wrapperMethodBodies.append(", j")
                           .append(aFieldName);
        if (isSetter) {
            wrapperMethodBodies.append(argumentContent);
        }

        if (!isSetter && isObjectReturningMethod) {
            wrapperMethodBodies.append(')');
        }
        wrapperMethodBodies.append(");\n" +
                               "}\n");
    }

    public void generateField(AnnotatableEntity aFieldTuple) {
        Field theField = aFieldTuple.getField();

        
        
        if (theField.getName().equals("$VALUES")) {
            return;
        }

        String CFieldName = aFieldTuple.mAnnotationInfo.wrapperName;

        Class<?> fieldType = theField.getType();

        generateMemberCommon(theField, CFieldName, mClassToWrap);

        boolean isFieldStatic = Utils.isMemberStatic(theField);
        boolean isFieldFinal = Utils.isMemberFinal(theField);

        String getterName = "get" + CFieldName;
        String getterSignature = Utils.getCImplementationMethodSignature(EMPTY_CLASS_ARRAY, fieldType, getterName, mCClassName, aFieldTuple.mAnnotationInfo.narrowChars, false);
        String getterHeaderSignature = Utils.getCHeaderMethodSignature(EMPTY_CLASS_ARRAY, GETTER_ARGUMENT_ANNOTATIONS, fieldType, getterName, mCClassName, isFieldStatic, aFieldTuple.mAnnotationInfo.narrowChars, false);

        writeSignatureToHeader(getterHeaderSignature);

        writeFunctionStartupBoilerPlate(getterSignature, true);

        generateGetterOrSetterBody(theField, CFieldName, isFieldStatic, false, aFieldTuple.mAnnotationInfo.narrowChars);

        
        if (!isFieldFinal) {
            String setterName = "set" + CFieldName;

            Class<?>[] setterArguments = new Class<?>[]{fieldType};

            String setterSignature = Utils.getCImplementationMethodSignature(setterArguments, Void.class, setterName, mCClassName, aFieldTuple.mAnnotationInfo.narrowChars, false);
            String setterHeaderSignature = Utils.getCHeaderMethodSignature(setterArguments, SETTER_ARGUMENT_ANNOTATIONS, Void.class, setterName, mCClassName, isFieldStatic, aFieldTuple.mAnnotationInfo.narrowChars, false);

            writeSignatureToHeader(setterHeaderSignature);

            writeFunctionStartupBoilerPlate(setterSignature, true);

            generateGetterOrSetterBody(theField, CFieldName, isFieldStatic, true, aFieldTuple.mAnnotationInfo.narrowChars);
        }
    }

    public void generateConstructor(AnnotatableEntity aCtorTuple) {
        
        Constructor<?> theCtor = aCtorTuple.getConstructor();
        String CMethodName = mCClassName;

        generateMemberCommon(theCtor, mCClassName, mClassToWrap);

        String implementationSignature = Utils.getCImplementationMethodSignature(theCtor.getParameterTypes(), Void.class, CMethodName,
            mCClassName, aCtorTuple.mAnnotationInfo.narrowChars, aCtorTuple.mAnnotationInfo.catchException);
        String headerSignature = Utils.getCHeaderMethodSignature(theCtor.getParameterTypes(), theCtor.getParameterAnnotations(), Void.class, CMethodName,
            mCClassName, false, aCtorTuple.mAnnotationInfo.narrowChars, aCtorTuple.mAnnotationInfo.catchException);

        
        headerSignature = headerSignature.substring(5);
        implementationSignature = implementationSignature.substring(5);

        
        writeSignatureToHeader(headerSignature);

        
        writeCtorBody(implementationSignature, theCtor,
            aCtorTuple.mAnnotationInfo.isMultithreaded,
            aCtorTuple.mAnnotationInfo.noThrow,
            aCtorTuple.mAnnotationInfo.catchException);

        if (theCtor.getParameterTypes().length == 0) {
            mHasEncounteredDefaultConstructor = true;
        }
    }

    public void generateMembers(Member[] members) {
        for (Member m : members) {
            if (!Modifier.isPublic(m.getModifiers())) {
                continue;
            }

            String name = m.getName();
            name = name.substring(0, 1).toUpperCase() + name.substring(1);

            AnnotationInfo info = new AnnotationInfo(name, true, true, true, true);
            AnnotatableEntity entity = new AnnotatableEntity(m, info);
            if (m instanceof Constructor) {
                generateConstructor(entity);
            } else if (m instanceof Method) {
                generateMethod(entity);
            } else if (m instanceof Field) {
                generateField(entity);
            } else {
                throw new IllegalArgumentException("expected member to be Constructor, Method, or Field");
            }
        }
    }

    





    private void ensureClassHeaderAndStartup(Class<?> aClass) {
        String className = aClass.getCanonicalName();
        if (seenClasses.contains(className)) {
            return;
        }

        zeroingCode.append("jclass ")
                   .append(mCClassName)
                   .append("::")
                   .append(Utils.getClassReferenceName(aClass))
                   .append(" = 0;\n");

        
        headerProtected.append("\n    static jclass ")
                       .append(Utils.getClassReferenceName(aClass))
                       .append(";\n");

        
        wrapperStartupCode.append(Utils.getStartupLineForClass(aClass));

        seenClasses.add(className);
    }

    


    private void writeFunctionStartupBoilerPlate(String methodSignature, boolean aIsThreaded) {
        
        wrapperMethodBodies.append('\n')
                           .append(methodSignature)
                           .append(" {\n");

        wrapperMethodBodies.append("    JNIEnv *env = ");
        if (!aIsThreaded) {
            wrapperMethodBodies.append("AndroidBridge::GetJNIEnv();\n");
        } else {
            wrapperMethodBodies.append("GetJNIForThread();\n");
        }
    }

    






    private void writeFramePushBoilerplate(Member aMethod,
            boolean aIsObjectReturningMethod, boolean aNoThrow) {
        if (aMethod instanceof Field) {
            throw new IllegalArgumentException("Tried to push frame for a FIELD?!");
        }

        Method m;
        Constructor<?> c;

        Class<?> returnType;

        int localReferencesNeeded;
        if (aMethod instanceof Method) {
            m = (Method) aMethod;
            returnType = m.getReturnType();
            localReferencesNeeded = Utils.enumerateReferenceArguments(m.getParameterTypes());
        } else {
            c = (Constructor<?>) aMethod;
            returnType = Void.class;
            localReferencesNeeded = Utils.enumerateReferenceArguments(c.getParameterTypes());
        }

        
        
        if (aIsObjectReturningMethod) {
            localReferencesNeeded++;
        }
        wrapperMethodBodies.append(
                "    if (env->PushLocalFrame(").append(localReferencesNeeded).append(") != 0) {\n");
        if (!aNoThrow) {
            wrapperMethodBodies.append(
                "        AndroidBridge::HandleUncaughtException(env);\n" +
                "        MOZ_CRASH(\"Exception should have caused crash.\");\n");
        } else {
            wrapperMethodBodies.append(
                "        return").append(Utils.getFailureReturnForType(returnType)).append(";\n");
        }
        wrapperMethodBodies.append(
                "    }\n\n");
    }

    private StringBuilder getArgumentMarshalling(Class<?>[] argumentTypes) {
        StringBuilder argumentContent = new StringBuilder();

        
        argumentContent.append(", ");
        if (argumentTypes.length > 2) {
            wrapperMethodBodies.append("    jvalue args[").append(argumentTypes.length).append("];\n");
            for (int aT = 0; aT < argumentTypes.length; aT++) {
                wrapperMethodBodies.append("    args[").append(aT).append("].")
                                   .append(Utils.getArrayArgumentMashallingLine(argumentTypes[aT], "a" + aT));
            }

            
            argumentContent.append("args");
            wrapperMethodBodies.append('\n');
        } else {
            
            boolean needsNewline = false;
            for (int aT = 0; aT < argumentTypes.length; aT++) {
                
                
                if (Utils.isCharSequence(argumentTypes[aT])) {
                    wrapperMethodBodies.append("    jstring j").append(aT).append(" = AndroidBridge::NewJavaString(env, a").append(aT).append(");\n");
                    needsNewline = true;
                    
                    
                    argumentContent.append('j').append(aT);
                } else {
                    argumentContent.append('a').append(aT);
                }
                if (aT != argumentTypes.length - 1) {
                    argumentContent.append(", ");
                }
            }
            if (needsNewline) {
                wrapperMethodBodies.append('\n');
            }
        }

        return argumentContent;
    }

    private void writeCatchException() {
        wrapperMethodBodies.append(
            "    if (env->ExceptionCheck()) {\n" +
            "        env->ExceptionClear();\n" +
            "        if (aResult) {\n" +
            "            *aResult = NS_ERROR_FAILURE;\n" +
            "        }\n" +
            "    } else if (aResult) {\n" +
            "        *aResult = NS_OK;\n" +
            "    }\n\n");
    }

    private void writeCtorBody(String implementationSignature, Constructor<?> theCtor,
            boolean aIsThreaded, boolean aNoThrow, boolean aCatchException) {
        Class<?>[] argumentTypes = theCtor.getParameterTypes();

        writeFunctionStartupBoilerPlate(implementationSignature, aIsThreaded);

        writeFramePushBoilerplate(theCtor, false, aNoThrow);

        if (mLazyInit) {
            writeMemberInit(theCtor, wrapperMethodBodies);
        }

        
        boolean hasArguments = argumentTypes.length != 0;

        StringBuilder argumentContent = new StringBuilder();
        if (hasArguments) {
            argumentContent = getArgumentMarshalling(argumentTypes);
        }

        
        wrapperMethodBodies.append("    Init(env->NewObject");
        if (argumentTypes.length > 2) {
            wrapperMethodBodies.append('A');
        }

        wrapperMethodBodies.append('(');


        
        wrapperMethodBodies.append(Utils.getClassReferenceName(mClassToWrap)).append(", ");

        wrapperMethodBodies.append(mMembersToIds.get(theCtor))
        
                           .append(argumentContent)
                           .append("), env);\n");

        
        if (aCatchException) {
            writeCatchException();
        }

        wrapperMethodBodies.append("    env->PopLocalFrame(nullptr);\n}\n");
    }

    







    private void writeMethodBody(String methodSignature, Method aMethod,
                                 Class<?> aClass, boolean aIsMultithreaded,
                                 boolean aNoThrow, boolean aNarrowChars,
                                 boolean aCatchException) {
        Class<?>[] argumentTypes = aMethod.getParameterTypes();
        Class<?> returnType = aMethod.getReturnType();

        writeFunctionStartupBoilerPlate(methodSignature, aIsMultithreaded);

        boolean isObjectReturningMethod = !returnType.getCanonicalName().equals("void") && Utils.isObjectType(returnType);

        writeFramePushBoilerplate(aMethod, isObjectReturningMethod, aNoThrow);

        if (mLazyInit) {
            writeMemberInit(aMethod, wrapperMethodBodies);
        }

        
        boolean hasArguments = argumentTypes.length != 0;

        
        
        
        
        
        StringBuilder argumentContent = new StringBuilder();
        if (hasArguments) {
            argumentContent = getArgumentMarshalling(argumentTypes);
        }

        
        wrapperMethodBodies.append("    ");
        if (!returnType.getCanonicalName().equals("void")) {
            if (isObjectReturningMethod) {
                wrapperMethodBodies.append("jobject");
            } else {
                wrapperMethodBodies.append(Utils.getCReturnType(returnType, aNarrowChars));
            }
            wrapperMethodBodies.append(" temp = ");
        }

        boolean isStaticJavaMethod = Utils.isMemberStatic(aMethod);

        
        wrapperMethodBodies.append("env->")
                           .append(Utils.getCallPrefix(returnType, isStaticJavaMethod));
        if (argumentTypes.length > 2) {
            wrapperMethodBodies.append('A');
        }

        wrapperMethodBodies.append('(');
        
        if (!isStaticJavaMethod) {
            wrapperMethodBodies.append("wrapped_obj, ");
        } else {
            
            
            wrapperMethodBodies.append(Utils.getClassReferenceName(aClass)).append(", ");
        }

        wrapperMethodBodies.append(mMembersToIds.get(aMethod));

        
        wrapperMethodBodies.append(argumentContent)
                           .append(");\n");

        
        if (!aNoThrow) {
            wrapperMethodBodies.append("    AndroidBridge::HandleUncaughtException(env);\n");
        }

        
        if (aCatchException) {
            writeCatchException();
        }

        
        
        if (isObjectReturningMethod) {
            wrapperMethodBodies.append("    ")
                               .append(Utils.getCReturnType(returnType, aNarrowChars))
                               .append(" ret = static_cast<").append(Utils.getCReturnType(returnType, aNarrowChars)).append(">(env->PopLocalFrame(temp));\n" +
                                       "    return ret;\n");
        } else if (!returnType.getCanonicalName().equals("void")) {
            
            
            wrapperMethodBodies.append("    env->PopLocalFrame(nullptr);\n" +
                                       "    return temp;\n");
        } else {
            
            wrapperMethodBodies.append("    env->PopLocalFrame(nullptr);\n");
        }
        wrapperMethodBodies.append("}\n");
    }

    





    private void writeMemberInit(Member aMember, StringBuilder aOutput) {
        if (mLazyInit) {
            aOutput.append("    if (!" + mMembersToIds.get(aMember) + ") {\n    ");
        }

        aOutput.append("    " + mMembersToIds.get(aMember)).append(" = AndroidBridge::Get");
        if (Utils.isMemberStatic(aMember)) {
            aOutput.append("Static");
        }

        boolean isField = aMember instanceof Field;
        if (isField) {
            aOutput.append("FieldID(env, " + Utils.getClassReferenceName(aMember.getDeclaringClass()) + ", \"");
        } else {
            aOutput.append("MethodID(env, " + Utils.getClassReferenceName(aMember.getDeclaringClass()) + ", \"");
        }

        if (aMember instanceof Constructor) {
            aOutput.append("<init>");
        } else {
            aOutput.append(aMember.getName());
        }

        aOutput.append("\", \"")
                          .append(Utils.getTypeSignatureStringForMember(aMember))
                          .append("\");\n");

        if (mLazyInit) {
            aOutput.append("    }\n\n");
        }
    }

    private void writeZeroingFor(Member aMember, final String aMemberName) {
        if (aMember instanceof Field) {
            zeroingCode.append("jfieldID ");
        } else {
            zeroingCode.append("jmethodID ");
        }
        zeroingCode.append(mCClassName)
                   .append("::")
                   .append(aMemberName)
                   .append(" = 0;\n");
    }

    




    private void writeMemberIdField(Member aMember, final String aCMethodName) {
        String memberName = 'j'+ aCMethodName;

        if (aMember instanceof Field) {
            headerProtected.append("    static jfieldID ");
        } else {
            headerProtected.append("    static jmethodID ");
        }

        while(mTakenMemberNames.contains(memberName)) {
            memberName = 'j' + aCMethodName + mNameMunger;
            mNameMunger++;
        }

        writeZeroingFor(aMember, memberName);
        mMembersToIds.put(aMember, memberName);
        mTakenMemberNames.add(memberName);

        headerProtected.append(memberName)
                       .append(";\n");
    }

    




    private void writeSignatureToHeader(String aSignature) {
        headerPublic.append("    ")
                    .append(aSignature)
                    .append(";\n");
    }

    




    public String getWrapperFileContents() {
        wrapperStartupCode.append("}\n");
        zeroingCode.append(wrapperStartupCode)
                   .append(wrapperMethodBodies);
        wrapperMethodBodies.setLength(0);
        wrapperStartupCode.setLength(0);
        return zeroingCode.toString();
    }

    




    public String getHeaderFileContents() {
        if (!mHasEncounteredDefaultConstructor) {
            headerPublic.append("    ").append(mCClassName).append("() : AutoGlobalWrappedJavaObject() {};\n");
        }
        headerProtected.append("};\n\n");
        headerPublic.append(headerProtected);
        headerProtected.setLength(0);
        return headerPublic.toString();
    }
}
