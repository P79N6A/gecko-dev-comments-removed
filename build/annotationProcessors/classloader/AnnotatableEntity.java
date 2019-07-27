



package org.mozilla.gecko.annotationProcessors.classloader;

import org.mozilla.gecko.annotationProcessors.AnnotationInfo;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;





public class AnnotatableEntity {
    public enum ENTITY_TYPE {METHOD, NATIVE, FIELD, CONSTRUCTOR}

    private final Member mMember;
    public final ENTITY_TYPE mEntityType;

    public final AnnotationInfo mAnnotationInfo;

    public AnnotatableEntity(Member aObject, AnnotationInfo aAnnotationInfo) {
        mMember = aObject;
        mAnnotationInfo = aAnnotationInfo;

        if (aObject instanceof Method) {
            if (Modifier.isNative(aObject.getModifiers())) {
                mEntityType = ENTITY_TYPE.NATIVE;
            } else {
                mEntityType = ENTITY_TYPE.METHOD;
            }
        } else if (aObject instanceof Field) {
            mEntityType = ENTITY_TYPE.FIELD;
        } else {
            mEntityType = ENTITY_TYPE.CONSTRUCTOR;
        }
    }

    public Method getMethod() {
        if (mEntityType != ENTITY_TYPE.METHOD && mEntityType != ENTITY_TYPE.NATIVE) {
            throw new UnsupportedOperationException("Attempt to cast to unsupported member type.");
        }
        return (Method) mMember;
    }
    public Field getField() {
        if (mEntityType != ENTITY_TYPE.FIELD) {
            throw new UnsupportedOperationException("Attempt to cast to unsupported member type.");
        }
        return (Field) mMember;
    }
    public Constructor<?> getConstructor() {
        if (mEntityType != ENTITY_TYPE.CONSTRUCTOR) {
            throw new UnsupportedOperationException("Attempt to cast to unsupported member type.");
        }
        return (Constructor<?>) mMember;
    }
}
