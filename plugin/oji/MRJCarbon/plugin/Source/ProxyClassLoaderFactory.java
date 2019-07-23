





































package netscape.oji;

import java.net.URLClassLoader;
import java.net.URL;
import java.net.MalformedURLException;

import java.io.*;
import java.util.zip.*;
import java.util.WeakHashMap;

import java.security.CodeSource;

public abstract class ProxyClassLoaderFactory {
    static void debug(String message) {
        System.out.println("<<< " + message + " >>>");
    }

    


    private static byte[] getMRJPluginClassFile(String path) {
        try {
    		String homeDir = System.getProperty("netscape.oji.plugin.home");
    		ZipFile jarFile = new ZipFile(new File(homeDir, "MRJPlugin.jar"));
    		ZipEntry classEntry = jarFile.getEntry(path);
    		int size = (int) classEntry.getSize();
    		if (size > 0) {
    			byte[] data = new byte[size];
    			DataInputStream input = new DataInputStream(jarFile.getInputStream(classEntry));
    			input.readFully(data);
    			input.close();
    			jarFile.close();
    			return data;
    		}
        } catch (IOException ioe) {
        }
		return null;
    }
    
    




    private static class ProxyClassLoader extends URLClassLoader {
        private static byte[] data = getMRJPluginClassFile("netscape/oji/LiveConnectProxy.class");
        ProxyClassLoader(URL[] documentURLs) {
            super(documentURLs);
            if (data != null) {
                Class proxyClass = defineClass("netscape.oji.LiveConnectProxy",
                                               data, 0, data.length,
                                               new CodeSource(documentURLs[0], null));
                debug("ProxyClassLoader: defined LiveConnectProxy class.");
                debug("Here're the permisssions you've got:");
                debug(proxyClass.getProtectionDomain().getPermissions().toString());
            } else {
                debug("ProxyClassLoader: failed to define LiveConnectProxy class.");
            }
        }
    }
    
    
    private static WeakHashMap mClassLoaders = new WeakHashMap();

    public static ClassLoader createClassLoader(final String documentURL) throws MalformedURLException {
        ClassLoader loader = (ClassLoader) mClassLoaders.get(documentURL);
        if (loader == null) {
            try {
                URL[] documentURLs = new URL[] { new URL(documentURL) };
                loader = new ProxyClassLoader(documentURLs);
                mClassLoaders.put(documentURL, loader);
    		} catch (MalformedURLException e) {
            }
        }
        return loader;
    }
    
    public static void destroyClassLoader(final String documentURL) {
        mClassLoaders.remove(documentURL);
    }
}
