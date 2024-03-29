



package org.mozilla.gecko.annotationProcessors;

import org.mozilla.gecko.annotationProcessors.classloader.AnnotatableEntity;
import org.mozilla.gecko.annotationProcessors.classloader.ClassWithOptions;
import org.mozilla.gecko.annotationProcessors.classloader.IterableJarLoadingURLClassLoader;
import org.mozilla.gecko.annotationProcessors.utils.GeneratableElementIterator;

import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.Iterator;

public class AnnotationProcessor {
    public static final String SOURCE_FILE = "GeneratedJNIWrappers.cpp";
    public static final String HEADER_FILE = "GeneratedJNIWrappers.h";
    public static final String NATIVES_FILE = "GeneratedJNINatives.h";

    public static final String GENERATED_COMMENT =
            "// GENERATED CODE\n" +
            "// Generated by the Java program at /build/annotationProcessors at compile time\n" +
            "// from annotations on Java methods. To update, change the annotations on the\n" +
            "// corresponding Java methods and rerun the build. Manually updating this file\n" +
            "// will cause your build to fail.\n" +
            "\n";

    public static void main(String[] args) {
        
        if (args.length <= 1) {
            System.err.println("Usage: java AnnotationProcessor jarfiles ...");
            System.exit(1);
        }

        System.out.println("Processing annotations...");

        
        
        Arrays.sort(args);

        
        long s = System.currentTimeMillis();

        
        Iterator<ClassWithOptions> jarClassIterator = IterableJarLoadingURLClassLoader.getIteratorOverJars(args);

        StringBuilder headerFile = new StringBuilder(GENERATED_COMMENT);
        headerFile.append(
                "#ifndef " + getHeaderGuardName(HEADER_FILE) + "\n" +
                "#define " + getHeaderGuardName(HEADER_FILE) + "\n" +
                "\n" +
                "#include \"mozilla/jni/Refs.h\"\n" +
                "\n" +
                "namespace mozilla {\n" +
                "namespace widget {\n" +
                "\n");

        StringBuilder implementationFile = new StringBuilder(GENERATED_COMMENT);
        implementationFile.append(
                "#include \"GeneratedJNIWrappers.h\"\n" +
                "#include \"mozilla/jni/Accessors.h\"\n" +
                "\n" +
                "namespace mozilla {\n" +
                "namespace widget {\n" +
                "\n");

        StringBuilder nativesFile = new StringBuilder(GENERATED_COMMENT);
        nativesFile.append(
                "#ifndef " + getHeaderGuardName(NATIVES_FILE) + "\n" +
                "#define " + getHeaderGuardName(NATIVES_FILE) + "\n" +
                "\n" +
                "#include \"GeneratedJNIWrappers.h\"\n" +
                "#include \"mozilla/jni/Natives.h\"\n" +
                "\n" +
                "namespace mozilla {\n" +
                "namespace widget {\n" +
                "\n");

        while (jarClassIterator.hasNext()) {
            ClassWithOptions aClassTuple = jarClassIterator.next();

            CodeGenerator generatorInstance;

            
            Iterator<AnnotatableEntity> methodIterator = new GeneratableElementIterator(aClassTuple.wrappedClass);

            if (!methodIterator.hasNext()) {
                continue;
            }
            generatorInstance = new CodeGenerator(aClassTuple);

            
            while (methodIterator.hasNext()) {
                AnnotatableEntity aElementTuple = methodIterator.next();
                switch (aElementTuple.mEntityType) {
                    case METHOD:
                        generatorInstance.generateMethod(aElementTuple);
                        break;
                    case NATIVE:
                        generatorInstance.generateNative(aElementTuple);
                        break;
                    case FIELD:
                        generatorInstance.generateField(aElementTuple);
                        break;
                    case CONSTRUCTOR:
                        generatorInstance.generateConstructor(aElementTuple);
                        break;
                }
            }

            headerFile.append(generatorInstance.getHeaderFileContents());
            implementationFile.append(generatorInstance.getWrapperFileContents());
            nativesFile.append(generatorInstance.getNativesFileContents());
        }

        implementationFile.append(
                "\n" +
                "} /* widget */\n" +
                "} /* mozilla */\n");

        headerFile.append(
                "\n" +
                "} /* widget */\n" +
                "} /* mozilla */\n" +
                "#endif // " + getHeaderGuardName(HEADER_FILE) + "\n");

        nativesFile.append(
                "\n" +
                "} /* widget */\n" +
                "} /* mozilla */\n" +
                "#endif // " + getHeaderGuardName(NATIVES_FILE) + "\n");

        writeOutputFile(SOURCE_FILE, implementationFile);
        writeOutputFile(HEADER_FILE, headerFile);
        writeOutputFile(NATIVES_FILE, nativesFile);
        long e = System.currentTimeMillis();
        System.out.println("Annotation processing complete in " + (e - s) + "ms");
    }

    private static String getHeaderGuardName(final String name) {
        return name.replaceAll("\\W", "_");
    }

    private static void writeOutputFile(final String name,
                                        final StringBuilder content) {
        FileOutputStream outStream = null;
        try {
            outStream = new FileOutputStream(name);
            outStream.write(content.toString().getBytes());
        } catch (IOException e) {
            System.err.println("Unable to write " + name + ". Perhaps a permissions issue?");
            e.printStackTrace(System.err);
        } finally {
            if (outStream != null) {
                try {
                    outStream.close();
                } catch (IOException e) {
                    System.err.println("Unable to close outStream due to "+e);
                    e.printStackTrace(System.err);
                }
            }
        }
    }
}
