



package org.mozilla.gecko.annotationProcessors;




public class AnnotationInfo {
    public final String wrapperName;
    public final boolean isMultithreaded;
    public final boolean noThrow;
    public final boolean narrowChars;

    public AnnotationInfo(String aWrapperName, boolean aIsMultithreaded,
                          boolean aNoThrow, boolean aNarrowChars) {
        wrapperName = aWrapperName;
        isMultithreaded = aIsMultithreaded;
        noThrow = aNoThrow;
        narrowChars = aNarrowChars;
    }
}
