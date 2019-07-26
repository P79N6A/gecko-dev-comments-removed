



package org.mozilla.gecko.annotationProcessors;




public class AnnotationInfo {
    public final String wrapperName;
    public final boolean isStatic;
    public final boolean isMultithreaded;

    public AnnotationInfo(String aWrapperName, boolean aIsStatic, boolean aIsMultithreaded) {
        wrapperName = aWrapperName;
        isStatic = aIsStatic;
        isMultithreaded = aIsMultithreaded;
    }
}
