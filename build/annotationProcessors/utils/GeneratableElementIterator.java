



package org.mozilla.gecko.annotationProcessors.utils;

import org.mozilla.gecko.annotationProcessors.AnnotationInfo;
import org.mozilla.gecko.annotationProcessors.classloader.AnnotatableEntity;

import java.lang.annotation.Annotation;
import java.lang.reflect.AnnotatedElement;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Iterator;






public class GeneratableElementIterator implements Iterator<AnnotatableEntity> {
    private final Member[] mObjects;
    private AnnotatableEntity mNextReturnValue;
    private int mElementIndex;

    private boolean mIterateEveryEntry;

    public GeneratableElementIterator(Class<?> aClass) {
        
        Member[] aMethods = aClass.getDeclaredMethods();
        Member[] aFields = aClass.getDeclaredFields();
        Member[] aCtors = aClass.getConstructors();

        
        Member[] objs = new Member[aMethods.length + aFields.length + aCtors.length];

        int offset = 0;
        System.arraycopy(aMethods, 0, objs, 0, aMethods.length);
        offset += aMethods.length;
        System.arraycopy(aFields, 0, objs, offset, aFields.length);
        offset += aFields.length;
        System.arraycopy(aCtors, 0, objs, offset, aCtors.length);

        
        Arrays.sort(objs, new AlphabeticAnnotatableEntityComparator());
        mObjects = objs;

        
        for (Annotation annotation : aClass.getDeclaredAnnotations()) {
            final String annotationTypeName = annotation.annotationType().getName();
            if (annotationTypeName.equals("org.mozilla.gecko.mozglue.generatorannotations.WrapEntireClassForJNI")) {
                mIterateEveryEntry = true;
                break;
            }
        }

        findNextValue();
    }

    



    private void findNextValue() {
        while (mElementIndex < mObjects.length) {
            Member candidateElement = mObjects[mElementIndex];
            mElementIndex++;
            for (Annotation annotation : ((AnnotatedElement) candidateElement).getDeclaredAnnotations()) {
                
                Class<? extends Annotation> annotationType = annotation.annotationType();
                final String annotationTypeName = annotationType.getName();
                if (annotationTypeName.equals("org.mozilla.gecko.mozglue.generatorannotations.WrapElementForJNI")) {
                    String stubName = null;
                    boolean isMultithreadedStub = false;
                    boolean noThrow = false;
                    boolean narrowChars = false;
                    try {
                        
                        final Method stubNameMethod = annotationType.getDeclaredMethod("stubName");
                        stubNameMethod.setAccessible(true);
                        stubName = (String) stubNameMethod.invoke(annotation);

                        
                        final Method multithreadedStubMethod = annotationType.getDeclaredMethod("allowMultithread");
                        multithreadedStubMethod.setAccessible(true);
                        isMultithreadedStub = (Boolean) multithreadedStubMethod.invoke(annotation);

                        
                        final Method noThrowMethod = annotationType.getDeclaredMethod("noThrow");
                        noThrowMethod.setAccessible(true);
                        noThrow = (Boolean) noThrowMethod.invoke(annotation);

                        
                        final Method narrowCharsMethod = annotationType.getDeclaredMethod("narrowChars");
                        narrowCharsMethod.setAccessible(true);
                        narrowChars = (Boolean) narrowCharsMethod.invoke(annotation);

                    } catch (NoSuchMethodException e) {
                        System.err.println("Unable to find expected field on WrapElementForJNI annotation. Did the signature change?");
                        e.printStackTrace(System.err);
                        System.exit(3);
                    } catch (IllegalAccessException e) {
                        System.err.println("IllegalAccessException reading fields on WrapElementForJNI annotation. Seems the semantics of Reflection have changed...");
                        e.printStackTrace(System.err);
                        System.exit(4);
                    } catch (InvocationTargetException e) {
                        System.err.println("InvocationTargetException reading fields on WrapElementForJNI annotation. This really shouldn't happen.");
                        e.printStackTrace(System.err);
                        System.exit(5);
                    }

                    
                    if (stubName.isEmpty()) {
                        String aMethodName = candidateElement.getName();
                        stubName = aMethodName.substring(0, 1).toUpperCase() + aMethodName.substring(1);
                    }

                    AnnotationInfo annotationInfo = new AnnotationInfo(
                        stubName, isMultithreadedStub, noThrow, narrowChars);
                    mNextReturnValue = new AnnotatableEntity(candidateElement, annotationInfo);
                    return;
                }
            }

            
            
            if (mIterateEveryEntry) {
                AnnotationInfo annotationInfo = new AnnotationInfo(
                    candidateElement.getName(), false, false, false);
                mNextReturnValue = new AnnotatableEntity(candidateElement, annotationInfo);
                return;
            }
        }
        mNextReturnValue = null;
    }

    @Override
    public boolean hasNext() {
        return mNextReturnValue != null;
    }

    @Override
    public AnnotatableEntity next() {
        AnnotatableEntity ret = mNextReturnValue;
        findNextValue();
        return ret;
    }

    @Override
    public void remove() {
        throw new UnsupportedOperationException("Removal of methods from GeneratableElementIterator not supported.");
    }
}
