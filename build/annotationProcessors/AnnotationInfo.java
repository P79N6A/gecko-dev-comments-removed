



package org.mozilla.gecko.annotationProcessors;




public class AnnotationInfo {
    public final String wrapperName;
    public final boolean isStatic;
    public final boolean isMultithreaded;
    public final boolean noThrow;
    public final boolean narrowChars;

    public AnnotationInfo(String aWrapperName, boolean aIsStatic, boolean aIsMultithreaded,
                          boolean aNoThrow, boolean aNarrowChars) {
        wrapperName = aWrapperName;
        isStatic = aIsStatic;
        isMultithreaded = aIsMultithreaded;
        noThrow = aNoThrow;
        narrowChars = aNarrowChars;
    }
}
