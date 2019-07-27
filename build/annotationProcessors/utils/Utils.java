



package org.mozilla.gecko.annotationProcessors.utils;

import org.mozilla.gecko.annotationProcessors.AnnotationInfo;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.HashMap;




public class Utils {

    
    private static final HashMap<String, String> NATIVE_TYPES = new HashMap<String, String>();

    static {
        NATIVE_TYPES.put("void", "void");
        NATIVE_TYPES.put("boolean", "bool");
        NATIVE_TYPES.put("byte", "int8_t");
        NATIVE_TYPES.put("char", "char16_t");
        NATIVE_TYPES.put("short", "int16_t");
        NATIVE_TYPES.put("int", "int32_t");
        NATIVE_TYPES.put("long", "int64_t");
        NATIVE_TYPES.put("float", "float");
        NATIVE_TYPES.put("double", "double");
    }

    private static final HashMap<String, String> NATIVE_ARRAY_TYPES = new HashMap<String, String>();

    static {
        NATIVE_ARRAY_TYPES.put("boolean", "mozilla::jni::BooleanArray");
        NATIVE_ARRAY_TYPES.put("byte", "mozilla::jni::ByteArray");
        NATIVE_ARRAY_TYPES.put("char", "mozilla::jni::CharArray");
        NATIVE_ARRAY_TYPES.put("short", "mozilla::jni::ShortArray");
        NATIVE_ARRAY_TYPES.put("int", "mozilla::jni::IntArray");
        NATIVE_ARRAY_TYPES.put("long", "mozilla::jni::LongArray");
        NATIVE_ARRAY_TYPES.put("float", "mozilla::jni::FloatArray");
        NATIVE_ARRAY_TYPES.put("double", "mozilla::jni::DoubleArray");
    }

    private static final HashMap<String, String> CLASS_DESCRIPTORS = new HashMap<String, String>();

    static {
        CLASS_DESCRIPTORS.put("void", "V");
        CLASS_DESCRIPTORS.put("boolean", "Z");
        CLASS_DESCRIPTORS.put("byte", "B");
        CLASS_DESCRIPTORS.put("char", "C");
        CLASS_DESCRIPTORS.put("short", "S");
        CLASS_DESCRIPTORS.put("int", "I");
        CLASS_DESCRIPTORS.put("long", "J");
        CLASS_DESCRIPTORS.put("float", "F");
        CLASS_DESCRIPTORS.put("double", "D");
    }

    





    public static String getNativeParameterType(Class<?> type, AnnotationInfo info) {
        final String name = type.getName().replace('.', '/');

        if (NATIVE_TYPES.containsKey(name)) {
            return NATIVE_TYPES.get(name);
        }

        if (type.isArray()) {
            final String compName = type.getComponentType().getName();
            if (NATIVE_ARRAY_TYPES.containsKey(compName)) {
                return NATIVE_ARRAY_TYPES.get(compName) + "::Param";
            }
            return "mozilla::jni::ObjectArray::Param";
        }

        if (type == String.class || type == CharSequence.class) {
            return "mozilla::jni::String::Param";
        }

        if (type == Class.class) {
            
            
            return "mozilla::jni::ClassObject::Param";
        }

        if (type == Throwable.class) {
            return "mozilla::jni::Throwable::Param";
        }

        return "mozilla::jni::Object::Param";
    }

    public static String getNativeReturnType(Class<?> type, AnnotationInfo info) {
        final String name = type.getName().replace('.', '/');

        if (NATIVE_TYPES.containsKey(name)) {
            return NATIVE_TYPES.get(name);
        }

        if (type.isArray()) {
            final String compName = type.getComponentType().getName();
            if (NATIVE_ARRAY_TYPES.containsKey(compName)) {
                return NATIVE_ARRAY_TYPES.get(compName) + "::LocalRef";
            }
            return "mozilla::jni::ObjectArray::LocalRef";
        }

        if (type == String.class) {
            return "mozilla::jni::String::LocalRef";
        }

        if (type == Class.class) {
            
            
            return "mozilla::jni::ClassObject::LocalRef";
        }

        if (type == Throwable.class) {
            return "mozilla::jni::Throwable::LocalRef";
        }

        return "mozilla::jni::Object::LocalRef";
    }

    





    public static String getClassDescriptor(Class<?> type) {
        final String name = type.getName().replace('.', '/');

        if (CLASS_DESCRIPTORS.containsKey(name)) {
            return CLASS_DESCRIPTORS.get(name);
        }

        if (type.isArray()) {
            
            return name;
        }

        return "L" + name + ';';
    }

    





    public static String getSignature(Member member) {
        return member instanceof Field ?  getSignature((Field) member) :
               member instanceof Method ? getSignature((Method) member) :
                                          getSignature((Constructor<?>) member);
    }

    





    public static String getSignature(Field member) {
        return getClassDescriptor(member.getType());
    }

    private static String getSignature(Class<?>[] args, Class<?> ret) {
        final StringBuilder sig = new StringBuilder("(");
        for (int i = 0; i < args.length; i++) {
            sig.append(getClassDescriptor(args[i]));
        }
        return sig.append(')').append(getClassDescriptor(ret)).toString();
    }

    





    public static String getSignature(Method member) {
        return getSignature(member.getParameterTypes(), member.getReturnType());
    }

    





    public static String getSignature(Constructor<?> member) {
        return getSignature(member.getParameterTypes(), void.class);
    }

    





    public static String getNativeName(Member member) {
        final String name = getMemberName(member);
        return name.substring(0, 1).toUpperCase() + name.substring(1);
    }

    





    public static String getMemberName(Member member) {
        if (member instanceof Constructor) {
            return "<init>";
        }
        return member.getName();
    }

    





    public static boolean isStatic(final Member member) {
        return Modifier.isStatic(member.getModifiers());
    }

    





    public static boolean isFinal(final Member member) {
        return Modifier.isFinal(member.getModifiers());
    }
}
