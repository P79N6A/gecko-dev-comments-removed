



package org.mozilla.gecko.annotationProcessors;

import java.lang.reflect.Method;




public class MethodWithAnnotationInfo {
    public final Method method;
    public final String wrapperName;
    public final boolean isStatic;
    public final boolean isMultithreaded;

    public MethodWithAnnotationInfo(Method aMethod, String aWrapperName, boolean aIsStatic, boolean aIsMultithreaded) {
        method = aMethod;
        wrapperName = aWrapperName;
        isStatic = aIsStatic;
        isMultithreaded = aIsMultithreaded;
    }
}
