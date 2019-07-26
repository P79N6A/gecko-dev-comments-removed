



package org.mozilla.gecko.annotationProcessors;




public class AnnotationInfo {
    public final String wrapperName;
    public final boolean isStatic;
    public final boolean isMultithreaded;
    public final boolean noThrow;

    public AnnotationInfo(String aWrapperName, boolean aIsStatic, boolean aIsMultithreaded,
                          boolean aNoThrow) {
        wrapperName = aWrapperName;
        isStatic = aIsStatic;
        isMultithreaded = aIsMultithreaded;
        noThrow = aNoThrow;
    }
}
