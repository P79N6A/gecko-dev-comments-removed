



package org.mozilla.gecko.annotationProcessors.classloader;

public class ClassWithOptions {
    public final Class<?> wrappedClass;
    public final String generatedName;

    public ClassWithOptions(Class<?> someClass, String name) {
        wrappedClass = someClass;
        generatedName = name;
    }
}
