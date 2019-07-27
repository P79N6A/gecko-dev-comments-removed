



package org.mozilla.gecko.annotationProcessors.utils;

import java.lang.annotation.Annotation;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.HashMap;




public class Utils {

    
    private static final HashMap<String, String> sBasicCTypes = new HashMap<String, String>();

    static {
        sBasicCTypes.put("void", "void");
        sBasicCTypes.put("int", "int32_t");
        sBasicCTypes.put("boolean", "bool");
        sBasicCTypes.put("long", "int64_t");
        sBasicCTypes.put("double", "jdouble");
        sBasicCTypes.put("float", "jfloat");
        sBasicCTypes.put("char", "uint16_t");
        sBasicCTypes.put("byte", "int8_t");
        sBasicCTypes.put("short", "int16_t");
    }

    private static final HashMap<String, String> sArrayCTypes = new HashMap<String, String>();

    static {
        sArrayCTypes.put("int", "jintArray");
        sArrayCTypes.put("boolean", "jbooleanArray");
        sArrayCTypes.put("long", "jlongArray");
        sArrayCTypes.put("double", "jdoubleArray");
        sArrayCTypes.put("float", "jfloatArray");
        sArrayCTypes.put("char", "jcharArray");
        sArrayCTypes.put("byte", "jbyteArray");
        sArrayCTypes.put("short", "jshortArray");
    }

    private static final HashMap<String, String> sStaticCallTypes = new HashMap<String, String>();

    static {
        sStaticCallTypes.put("void", "CallStaticVoidMethod");
        sStaticCallTypes.put("int", "CallStaticIntMethod");
        sStaticCallTypes.put("boolean", "CallStaticBooleanMethod");
        sStaticCallTypes.put("long", "CallStaticLongMethod");
        sStaticCallTypes.put("double", "CallStaticDoubleMethod");
        sStaticCallTypes.put("float", "CallStaticFloatMethod");
        sStaticCallTypes.put("char", "CallStaticCharMethod");
        sStaticCallTypes.put("byte", "CallStaticByteMethod");
        sStaticCallTypes.put("short", "CallStaticShortMethod");
    }

    private static final HashMap<String, String> sInstanceCallTypes = new HashMap<String, String>();

    static {
        sInstanceCallTypes.put("void", "CallVoidMethod");
        sInstanceCallTypes.put("int", "CallIntMethod");
        sInstanceCallTypes.put("boolean", "CallBooleanMethod");
        sInstanceCallTypes.put("long", "CallLongMethod");
        sInstanceCallTypes.put("double", "CallDoubleMethod");
        sInstanceCallTypes.put("float", "CallFloatMethod");
        sInstanceCallTypes.put("char", "CallCharMethod");
        sInstanceCallTypes.put("byte", "CallByteMethod");
        sInstanceCallTypes.put("short", "CallShortMethod");
    }

    private static final HashMap<String, String> sFieldTypes = new HashMap<String, String>();

    static {
        sFieldTypes.put("int", "Int");
        sFieldTypes.put("boolean", "Boolean");
        sFieldTypes.put("long", "Long");
        sFieldTypes.put("double", "Double");
        sFieldTypes.put("float", "Float");
        sFieldTypes.put("char", "Char");
        sFieldTypes.put("byte", "Byte");
        sFieldTypes.put("short", "Short");
    }

    private static final HashMap<String, String> sFailureReturns = new HashMap<String, String>();

    static {
        sFailureReturns.put("java.lang.Void", "");
        sFailureReturns.put("void", "");
        sFailureReturns.put("int", " 0");
        sFailureReturns.put("boolean", " false");
        sFailureReturns.put("long", " 0");
        sFailureReturns.put("double", " 0.0");
        sFailureReturns.put("float", " 0.0");
        sFailureReturns.put("char", " 0");
        sFailureReturns.put("byte", " 0");
        sFailureReturns.put("short", " 0");
    }

    private static final HashMap<String, String> sCanonicalSignatureParts = new HashMap<String, String>();

    static {
        sCanonicalSignatureParts.put("java/lang/Void", "V");
        sCanonicalSignatureParts.put("void", "V");
        sCanonicalSignatureParts.put("int", "I");
        sCanonicalSignatureParts.put("boolean", "Z");
        sCanonicalSignatureParts.put("long", "J");
        sCanonicalSignatureParts.put("double", "D");
        sCanonicalSignatureParts.put("float", "F");
        sCanonicalSignatureParts.put("char", "C");
        sCanonicalSignatureParts.put("byte", "B");
        sCanonicalSignatureParts.put("short", "S");
    }


    private static final HashMap<String, String> sDefaultParameterValues = new HashMap<String, String>();

    static {
        sDefaultParameterValues.put("int", "0");
        sDefaultParameterValues.put("boolean", "false");
        sDefaultParameterValues.put("long", "0");
        sDefaultParameterValues.put("double", "0");
        sDefaultParameterValues.put("float", "0.0");
        sDefaultParameterValues.put("char", "0");
        sDefaultParameterValues.put("byte", "0");
        sDefaultParameterValues.put("short", "0");
    }

    






    public static String getCParameterType(Class<?> type, boolean aNarrowChars) {
        String name = type.getCanonicalName();
        if (sBasicCTypes.containsKey(name)) {
            return sBasicCTypes.get(name);
        }
        
        int len = name.length();
        if (name.endsWith("[]")) {
            
            name = name.substring(0, len - 2);
            if (name.endsWith("[]")) {
                return "jobjectArray";
            } else {
                
                if (sArrayCTypes.containsKey(name)) {
                    return sArrayCTypes.get(name);
                }
                return "jobjectArray";
            }
        }
        

        
        if (isCharSequence(type)) {
            if (aNarrowChars) {
                return "const nsACString&";
            }
            return "const nsAString&";
        }

        if (name.equals("java.lang.Class")) {
            
            
            return "jclass";
        }
        if (name.equals("java.lang.Throwable")) {
            return "jthrowable";
        }
        return "jobject";
    }

    





     public static String getCReturnType(Class<?> type, boolean aNarrowChars) {
        if (type.getCanonicalName().equals("java.lang.Void")) {
            return "void";
        }
        String cParameterType = getCParameterType(type, aNarrowChars);
        if (cParameterType.equals("const nsAString&") || cParameterType.equals("const nsACString&")) {
            return "jstring";
        } else {
            return cParameterType;
        }
    }

    





    public static String getFieldType(Class<?> aFieldType) {
        String name = aFieldType.getCanonicalName();

        if (sFieldTypes.containsKey(name)) {
            return sFieldTypes.get(name);
        }
        return "Object";
    }

    







    public static String getCallPrefix(Class<?> aReturnType, boolean isStatic) {
        String name = aReturnType.getCanonicalName();
        if (isStatic) {
            if (sStaticCallTypes.containsKey(name)) {
                return sStaticCallTypes.get(name);
            }
            return "CallStaticObjectMethod";
        } else {
            if (sInstanceCallTypes.containsKey(name)) {
                return sInstanceCallTypes.get(name);
            }
            return "CallObjectMethod";
        }
    }

    






    public static String getFailureReturnForType(Class<?> type) {
        String name = type.getCanonicalName();
        if (sFailureReturns.containsKey(name)) {
            return sFailureReturns.get(name);
        }
        return " nullptr";
    }

    








    private static String getTypeSignatureInternal(Class<?>[] arguments, Class<?> returnType) {
        StringBuilder sb = new StringBuilder();
        sb.append('(');

        
        for (int i = 0; i < arguments.length; i++) {
            writeTypeSignature(sb, arguments[i]);
        }
        sb.append(')');

        
        writeTypeSignature(sb, returnType);
        return sb.toString();
    }

    





    protected static String getTypeSignatureStringForField(Field aField) {
        StringBuilder sb = new StringBuilder();
        writeTypeSignature(sb, aField.getType());
        return sb.toString();
    }

    





    protected static String getTypeSignatureStringForMethod(Method aMethod) {
        Class<?>[] arguments = aMethod.getParameterTypes();
        Class<?> returnType = aMethod.getReturnType();
        return getTypeSignatureInternal(arguments, returnType);
    }

    





    protected static String getTypeSignatureStringForConstructor(Constructor<?> aConstructor) {
        Class<?>[] arguments = aConstructor.getParameterTypes();
        return getTypeSignatureInternal(arguments, Void.class);
    }

    public static String getTypeSignatureStringForMember(Member aMember) {
        if (aMember instanceof Method) {
            return getTypeSignatureStringForMethod((Method) aMember);
        } else if (aMember instanceof Field) {
            return getTypeSignatureStringForField((Field) aMember);
        } else {
            return getTypeSignatureStringForConstructor((Constructor<?>) aMember);
        }
    }

    public static String getTypeSignatureString(Constructor<?> aConstructor) {
        Class<?>[] arguments = aConstructor.getParameterTypes();
        StringBuilder sb = new StringBuilder();
        sb.append('(');

        
        for (int i = 0; i < arguments.length; i++) {
            writeTypeSignature(sb, arguments[i]);
        }

        
        sb.append(")V");
        return sb.toString();
    }

    






    private static void writeTypeSignature(StringBuilder sb, Class<?> c) {
        String name = c.getCanonicalName().replaceAll("\\.", "/");

        
        int len = name.length();
        while (name.endsWith("[]")) {
            sb.append('[');
            name = name.substring(0, len - 2);
            len = len - 2;
        }

        if (c.isArray()) {
            c = c.getComponentType();
        }

        Class<?> containerClass = c.getDeclaringClass();
        if (containerClass != null) {
            
            final int lastSlash = name.lastIndexOf('/');
            name = name.substring(0, lastSlash) + '$' + name.substring(lastSlash+1);
        }

        
        if (sCanonicalSignatureParts.containsKey(name)) {
            
            sb.append(sCanonicalSignatureParts.get(name));
        } else {
            
            sb.append('L');
            sb.append(name);
            sb.append(';');
        }
    }

    









    public static String getCImplementationMethodSignature(Class<?>[] aArgumentTypes, Class<?> aReturnType, String aCMethodName, String aCClassName, boolean aNarrowChars) {
        StringBuilder retBuffer = new StringBuilder();

        retBuffer.append(getCReturnType(aReturnType, aNarrowChars));
        retBuffer.append(' ');
        retBuffer.append(aCClassName);
        retBuffer.append("::");
        retBuffer.append(aCMethodName);
        retBuffer.append('(');

        
        for (int aT = 0; aT < aArgumentTypes.length; aT++) {
            retBuffer.append(getCParameterType(aArgumentTypes[aT], aNarrowChars));
            retBuffer.append(" a");
            
            
            
            retBuffer.append(aT);
            if (aT != aArgumentTypes.length - 1) {
                retBuffer.append(", ");
            }
        }
        retBuffer.append(')');
        return retBuffer.toString();
    }

    












    public static String getCHeaderMethodSignature(Class<?>[] aArgumentTypes, Annotation[][] aArgumentAnnotations, Class<?> aReturnType, String aCMethodName, String aCClassName, boolean aIsStaticStub, boolean aNarrowChars) {
        StringBuilder retBuffer = new StringBuilder();

        
        if (aIsStaticStub) {
            retBuffer.append("static ");
        }

        
        retBuffer.append(getCReturnType(aReturnType, aNarrowChars));
        retBuffer.append(' ');
        retBuffer.append(aCMethodName);
        retBuffer.append('(');

        
        for (int aT = 0; aT < aArgumentTypes.length; aT++) {
            retBuffer.append(getCParameterType(aArgumentTypes[aT], aNarrowChars));
            retBuffer.append(" a");
            
            
            
            retBuffer.append(aT);

            
            retBuffer.append(getDefaultValueString(aArgumentTypes[aT], aArgumentAnnotations[aT]));

            if (aT != aArgumentTypes.length - 1) {
                retBuffer.append(", ");
            }
        }
        retBuffer.append(')');
        return retBuffer.toString();
    }

    









    public static String getDefaultValueString(Class<?> aArgumentType, Annotation[] aArgumentAnnotations) {
        for (int i = 0; i < aArgumentAnnotations.length; i++) {
            Class<? extends Annotation> annotationType = aArgumentAnnotations[i].annotationType();
            final String annotationTypeName = annotationType.getName();
            if (annotationTypeName.equals("org.mozilla.gecko.mozglue.generatorannotations.OptionalGeneratedParameter")) {
                return " = " + getDefaultParameterValueForType(aArgumentType);
            }
        }
        return "";
    }

    








    private static String getDefaultParameterValueForType(Class<?> aArgumentType) {
        String typeName = aArgumentType.getCanonicalName();
        if (sDefaultParameterValues.containsKey(typeName)) {
            return sDefaultParameterValues.get(typeName);
        } else if (isCharSequence(aArgumentType)) {
            return "EmptyString()";
        } else {
            return "nullptr";
        }
    }

    





    public static int enumerateReferenceArguments(Class<?>[] aArgs) {
        int ret = 0;
        for (int i = 0; i < aArgs.length; i++) {
            String name = aArgs[i].getCanonicalName();
            if (!sBasicCTypes.containsKey(name)) {
                ret++;
            }
        }
        return ret;
    }

    





    public static boolean hasStringArgument(Method m) {
        Class<?>[] args = m.getParameterTypes();
        for (int i = 0; i < args.length; i++) {
            if (isCharSequence(args[i])) {
                return true;
            }
        }
        return false;
    }

    






    public static String getArrayArgumentMashallingLine(Class<?> type, String argName) {
        StringBuilder sb = new StringBuilder();

        String name = type.getCanonicalName();
        if (sCanonicalSignatureParts.containsKey(name)) {
            sb.append(sCanonicalSignatureParts.get(name).toLowerCase());
            sb.append(" = ").append(argName).append(";\n");
        } else {
            if (isCharSequence(type)) {
                sb.append("l = AndroidBridge::NewJavaString(env, ").append(argName).append(");\n");
            } else {
                sb.append("l = ").append(argName).append(";\n");
            }
        }

        return sb.toString();
    }

    





    public static boolean isObjectType(Class<?> aType) {
        return !sBasicCTypes.containsKey(aType.getCanonicalName());
    }

    





    public static String getClassReferenceName(Class<?> aClass) {
        String className = aClass.getSimpleName();
        return 'm' + className + "Class";
    }

    





    public static String getStartupLineForClass(Class<?> aClass) {
        StringBuilder sb = new StringBuilder();
        sb.append("    ");
        sb.append(getClassReferenceName(aClass));
        sb.append(" = getClassGlobalRef(\"");

        String name = aClass.getCanonicalName().replaceAll("\\.", "/");
        Class<?> containerClass = aClass.getDeclaringClass();
        if (containerClass != null) {
            
            final int lastSlash = name.lastIndexOf('/');
            name = name.substring(0, lastSlash) + '$' + name.substring(lastSlash+1);
        }

        sb.append(name);
        sb.append("\");\n");
        return sb.toString();
    }

    




    public static boolean isCharSequence(Class<?> aClass) {
        if (aClass.getCanonicalName().equals("java.lang.CharSequence")) {
            return true;
        }
        Class<?>[] interfaces = aClass.getInterfaces();
        for (Class<?> c : interfaces) {
            if (c.getCanonicalName().equals("java.lang.CharSequence")) {
                return true;
            }
        }
        return false;
    }

    




    public static boolean isMemberStatic(Member aMember) {
        int aMethodModifiers = aMember.getModifiers();
        return Modifier.isStatic(aMethodModifiers);
    }

    




    public static boolean isMemberFinal(Member aMember) {
        int aMethodModifiers = aMember.getModifiers();
        return Modifier.isFinal(aMethodModifiers);
    }
}
