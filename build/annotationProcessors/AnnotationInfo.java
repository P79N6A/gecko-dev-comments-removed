



package org.mozilla.gecko.annotationProcessors;




public class AnnotationInfo {
    public final String wrapperName;
    public final boolean isMultithreaded;
    public final boolean noThrow;
    public final boolean narrowChars;
    public final boolean catchException;

    public AnnotationInfo(String aWrapperName, boolean aIsMultithreaded,
                          boolean aNoThrow, boolean aNarrowChars, boolean aCatchException) {
        wrapperName = aWrapperName;
        isMultithreaded = aIsMultithreaded;
        noThrow = aNoThrow;
        narrowChars = aNarrowChars;
        catchException = aCatchException;

        if (!noThrow && catchException) {
            
            throw new IllegalArgumentException("noThrow and catchException are not allowed together");
        }
    }
}
