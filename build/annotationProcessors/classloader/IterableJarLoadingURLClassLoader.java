



package org.mozilla.gecko.annotationProcessors.classloader;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;








public class IterableJarLoadingURLClassLoader extends URLClassLoader {
    LinkedList<String> classNames = new LinkedList<String>();

    







    public static Iterator<ClassWithOptions> getIteratorOverJars(String[] args) {
        URL[] urlArray = new URL[args.length];
        LinkedList<String> aClassNames = new LinkedList<String>();

        for (int i = 0; i < args.length; i++) {
            try {
                urlArray[i] = (new File(args[i])).toURI().toURL();

                Enumeration<JarEntry> entries = new JarFile(args[i]).entries();
                while (entries.hasMoreElements()) {
                    JarEntry e = entries.nextElement();
                    String fName = e.getName();
                    if (!fName.endsWith(".class")) {
                        continue;
                    }
                    final String className = fName.substring(0, fName.length() - 6).replace('/', '.');

                    aClassNames.add(className);
                }
            } catch (IOException e) {
                System.err.println("Error loading jar file \"" + args[i] + '"');
                e.printStackTrace(System.err);
            }
        }
        Collections.sort(aClassNames);
        return new JarClassIterator(new IterableJarLoadingURLClassLoader(urlArray, aClassNames));
    }

    






    protected IterableJarLoadingURLClassLoader(URL[] urls, LinkedList<String> aClassNames) {
        super(urls);
        classNames = aClassNames;
    }
}
