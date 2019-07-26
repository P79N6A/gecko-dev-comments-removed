



package org.mozilla.gecko.annotationProcessors;

import org.mozilla.gecko.annotationProcessors.classloader.IterableJarLoadingURLClassLoader;
import org.mozilla.gecko.annotationProcessors.utils.GeneratableEntryPointIterator;

import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Arrays;
import java.util.Iterator;

public class AnnotationProcessor {
    public static final String OUTFILE = "GeneratedJNIWrappers.cpp";
    public static final String HEADERFILE = "GeneratedJNIWrappers.h";

    public static void main(String[] args) {
        
        if (args.length <= 1) {
            System.err.println("Usage: java AnnotationProcessor jarfiles ...");
            System.exit(1);
        }

        System.out.println("Processing annotations...");

        
        
        Arrays.sort(args);

        
        long s = System.currentTimeMillis();

        
        Iterator<Class<?>> jarClassIterator = IterableJarLoadingURLClassLoader.getIteratorOverJars(args);

        CodeGenerator generatorInstance = new CodeGenerator();

        while (jarClassIterator.hasNext()) {
            Class<?> aClass = jarClassIterator.next();

            
            Iterator<MethodWithAnnotationInfo> methodIterator = new GeneratableEntryPointIterator(aClass.getDeclaredMethods());

            
            while (methodIterator.hasNext()) {
                MethodWithAnnotationInfo aMethodTuple = methodIterator.next();
                generatorInstance.generateMethod(aMethodTuple, aClass);
            }

        }

        writeOutputFiles(generatorInstance);
        long e = System.currentTimeMillis();
        System.out.println("Annotation processing complete in " + (e - s) + "ms");
    }

    private static void writeOutputFiles(CodeGenerator aGenerator) {
        try {
            FileOutputStream outStream = new FileOutputStream(OUTFILE);
            outStream.write(aGenerator.getWrapperFileContents());
        } catch (IOException e) {
            System.err.println("Unable to write " + OUTFILE + ". Perhaps a permissions issue?");
            e.printStackTrace(System.err);
        }

        try {
            FileOutputStream headerStream = new FileOutputStream(HEADERFILE);
            headerStream.write(aGenerator.getHeaderFileContents());
        } catch (IOException e) {
            System.err.println("Unable to write " + OUTFILE + ". Perhaps a permissions issue?");
            e.printStackTrace(System.err);
        }
    }
}