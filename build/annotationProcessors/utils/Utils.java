



package org.mozilla.gecko.annotationProcessors.utils;

import java.lang.annotation.Annotation;
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

    private static final HashMap<String, String> sFailureReturns = new HashMap<String, String>();

    static {
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

    






    public static String getCParameterType(Class<?> type) {
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

    





    public static String getCReturnType(Class<?> type) {
        
        String cParameterType = getCParameterType(type);
        if (cParameterType.equals("const nsAString&")) {
            return "jstring";
        } else {
            return cParameterType;
        }
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

    





    public static String getTypeSignatureString(Method aMethod) {
        Class<?>[] arguments = aMethod.getParameterTypes();
        Class<?> returnType = aMethod.getReturnType();

        StringBuilder sb = new StringBuilder();
        sb.append('(');
        
        for (int i = 0; i < arguments.length; i++) {
            writeTypeSignature(sb, arguments[i]);
        }
        sb.append(')');
        
        writeTypeSignature(sb, returnType);
        return sb.toString();
    }

    






    private static void writeTypeSignature(StringBuilder sb, Class<?> c) {
        String name = c.getCanonicalName().replaceAll("\\.", "/");
        
        int len = name.length();
        while (name.endsWith("[]")) {
            sb.append('[');
            name = name.substring(0, len - 2);
        }

        
        if (sCanonicalSignatureParts.containsKey(name)) {
            
            sb.append(sCanonicalSignatureParts.get(name));
        } else {
            
            sb.append('L');
            sb.append(name);
            sb.append(';');
        }
    }

    







    public static String getCImplementationMethodSignature(Method aMethod, String aCMethodName) {
        Class<?>[] argumentTypes = aMethod.getParameterTypes();
        Class<?> returnType = aMethod.getReturnType();

        StringBuilder retBuffer = new StringBuilder();
        
        retBuffer.append(getCReturnType(returnType));
        retBuffer.append(" AndroidBridge::");
        retBuffer.append(aCMethodName);
        retBuffer.append('(');

        
        if (!isMethodStatic(aMethod)) {
            retBuffer.append("jobject aTarget");
            if (argumentTypes.length > 0) {
                retBuffer.append(", ");
            }
        }

        
        for (int aT = 0; aT < argumentTypes.length; aT++) {
            retBuffer.append(getCParameterType(argumentTypes[aT]));
            retBuffer.append(" a");
            
            
            
            retBuffer.append(aT);
            if (aT != argumentTypes.length - 1) {
                retBuffer.append(", ");
            }
        }
        retBuffer.append(')');
        return retBuffer.toString();
    }

    







    public static String getCHeaderMethodSignature(Method aMethod, String aCMethodName, boolean aIsStaticStub) {
        Class<?>[] argumentTypes = aMethod.getParameterTypes();

        
        
        Annotation[][] argumentAnnotations = aMethod.getParameterAnnotations();
        Class<?> returnType = aMethod.getReturnType();

        StringBuilder retBuffer = new StringBuilder();

        
        if (aIsStaticStub) {
            retBuffer.append("static ");
        }

        
        retBuffer.append(getCReturnType(returnType));
        retBuffer.append(' ');
        retBuffer.append(aCMethodName);
        retBuffer.append('(');

        
        if (!isMethodStatic(aMethod)) {
            retBuffer.append("jobject aTarget");
            if (argumentTypes.length > 0) {
                retBuffer.append(", ");
            }
        }

        
        for (int aT = 0; aT < argumentTypes.length; aT++) {
            retBuffer.append(getCParameterType(argumentTypes[aT]));
            retBuffer.append(" a");
            
            
            
            retBuffer.append(aT);

            
            retBuffer.append(getDefaultValueString(argumentTypes[aT], argumentAnnotations[aT]));

            if (aT != argumentTypes.length - 1) {
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
            if (annotationTypeName.equals("org.mozilla.gecko.mozglue.OptionalGeneratedParameter")) {
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

    





    public static int enumerateReferenceArguments(Method m) {
        int ret = 0;
        Class<?>[] args = m.getParameterTypes();
        for (int i = 0; i < args.length; i++) {
            String name = args[i].getCanonicalName();
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
                sb.append("l = NewJavaString(env, ").append(argName).append(");\n");
            } else {
                sb.append("l = ").append(argName).append(";\n");
            }
        }

        return sb.toString();
    }

    






    public static boolean doesReturnObjectType(Method aMethod) {
        Class<?> returnType = aMethod.getReturnType();
        return !sBasicCTypes.containsKey(returnType.getCanonicalName());
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
        sb.append(aClass.getCanonicalName().replaceAll("\\.", "/"));
        sb.append("\");\n");
        return sb.toString();
    }

    




    public static boolean isCharSequence(Class aClass) {
        if (aClass.getCanonicalName().equals("java.lang.CharSequence")) {
            return true;
        }
        Class[] interfaces = aClass.getInterfaces();
        for (Class c : interfaces) {
            if (c.getCanonicalName().equals("java.lang.CharSequence")) {
                return true;
            }
        }
        return false;
    }

    




    public static boolean isMethodStatic(Method aMethod) {
        int aMethodModifiers = aMethod.getModifiers();
        return Modifier.isStatic(aMethodModifiers);
    }
}
