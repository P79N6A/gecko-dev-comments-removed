



package org.mozilla.gecko.annotationProcessors.utils;

import org.mozilla.gecko.annotationProcessors.MethodWithAnnotationInfo;

import java.lang.annotation.Annotation;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;






public class GeneratableEntryPointIterator implements Iterator<MethodWithAnnotationInfo> {
    private final Method[] mMethods;
    private MethodWithAnnotationInfo mNextReturnValue;
    private int mMethodIndex;

    public GeneratableEntryPointIterator(Method[] aMethods) {
        
        
        Arrays.sort(aMethods, new AlphabeticMethodComparator());
        mMethods = aMethods;

        findNextValue();
    }

    



    private void findNextValue() {
        while (mMethodIndex < mMethods.length) {
            Method candidateMethod = mMethods[mMethodIndex];
            mMethodIndex++;
            for (Annotation annotation : candidateMethod.getDeclaredAnnotations()) {
                
                Class<? extends Annotation> annotationType = annotation.annotationType();
                final String annotationTypeName = annotationType.getName();

                if (annotationTypeName.equals("org.mozilla.gecko.mozglue.GeneratableAndroidBridgeTarget")) {
                    String stubName = null;
                    boolean isStaticStub = false;
                    boolean isMultithreadedStub = false;
                    try {
                        
                        final Method stubNameMethod = annotationType.getDeclaredMethod("stubName");
                        stubNameMethod.setAccessible(true);
                        stubName = (String) stubNameMethod.invoke(annotation);

                        
                        final Method staticStubMethod = annotationType.getDeclaredMethod("generateStatic");
                        staticStubMethod.setAccessible(true);
                        isStaticStub = (Boolean) staticStubMethod.invoke(annotation);

                        
                        final Method multithreadedStubMethod = annotationType.getDeclaredMethod("allowMultithread");
                        multithreadedStubMethod.setAccessible(true);
                        isMultithreadedStub = (Boolean) multithreadedStubMethod.invoke(annotation);
                    } catch (NoSuchMethodException e) {
                        System.err.println("Unable to find expected field on GeneratableAndroidBridgeTarget annotation. Did the signature change?");
                        e.printStackTrace(System.err);
                        System.exit(3);
                    } catch (IllegalAccessException e) {
                        System.err.println("IllegalAccessException reading fields on GeneratableAndroidBridgeTarget annotation. Seems the semantics of Reflection have changed...");
                        e.printStackTrace(System.err);
                        System.exit(4);
                    } catch (InvocationTargetException e) {
                        System.err.println("InvocationTargetException reading fields on GeneratableAndroidBridgeTarget annotation. This really shouldn't happen.");
                        e.printStackTrace(System.err);
                        System.exit(5);
                    }
                    
                    if (stubName.isEmpty()) {
                        String aMethodName = candidateMethod.getName();
                        stubName = aMethodName.substring(0, 1).toUpperCase() + aMethodName.substring(1);
                    }

                    mNextReturnValue = new MethodWithAnnotationInfo(candidateMethod, stubName, isStaticStub, isMultithreadedStub);
                    return;
                }
            }
        }
        mNextReturnValue = null;
    }

    @Override
    public boolean hasNext() {
        return mNextReturnValue != null;
    }

    @Override
    public MethodWithAnnotationInfo next() {
        MethodWithAnnotationInfo ret = mNextReturnValue;
        findNextValue();
        return ret;
    }

    @Override
    public void remove() {
        throw new UnsupportedOperationException("Removal of methods from GeneratableEntryPointIterator not supported.");
    }
}
