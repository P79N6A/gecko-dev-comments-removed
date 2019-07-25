



package org.mozilla.gecko;

import java.io.File;
import java.net.URL;
import java.util.EmptyStackException;
import java.util.Stack;
import java.util.zip.ZipFile;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipEntry;
import java.io.InputStream;
import java.io.IOException;

import android.util.Log;




public class GeckoJarReader {
    static private String LOGTAG = "GeckoJarReader";

    static public InputStream getStream(String url) {
        Stack<String> jarUrls = parseUrl(url);
        ZipInputStream inputStream = null;

        try {
            
            URL fileUrl = new URL(jarUrls.pop());
            File file = new File(fileUrl.getPath());
            ZipFile zip = new ZipFile(file);
            ZipEntry entry = null;

            
            while (jarUrls.peek() != null) {
                String fileName = jarUrls.pop();

                if (inputStream != null) {
                    entry = getEntryFromStream(inputStream, fileName);
                } else {
                    entry = zip.getEntry(fileName);
                }

                
                jarUrls.peek();

                if (inputStream != null) {
                    inputStream = new ZipInputStream(inputStream);
                } else {
                    inputStream = new ZipInputStream(zip.getInputStream(entry));
                }
  
                if (entry == null) {
                    Log.d(LOGTAG, "No Entry for " + fileName);
                    return null;
                }
            }
        } catch (EmptyStackException ex) {
            Log.d(LOGTAG, "Reached Jar reader reached end of stack");
        } catch (IOException ex) {
            Log.e(LOGTAG, "Exception ", ex);
        } catch (Exception ex) {
            Log.e(LOGTAG, "Exception ", ex);
        }

        return inputStream;
    }

    
    static private ZipEntry getEntryFromStream(ZipInputStream zipStream, String entryName) {
        ZipEntry entry = null;

        try {
            entry = zipStream.getNextEntry();
            while(entry != null && !entry.getName().equals(entryName)) {
                entry = zipStream.getNextEntry();
            }
        } catch (IOException ex) {
            Log.e(LOGTAG, "Exception getting stream entry", ex);
        }

        return entry;
    }

    








    static private Stack<String> parseUrl(String url) {
        return parseUrl(url, null);
    }

    static private Stack<String> parseUrl(String url, Stack<String> results) {
        if (results == null) {
            results = new Stack<String>();
        }

        if (url.startsWith("jar:")) {
            int jarEnd = url.lastIndexOf("!");
            String subStr = url.substring(4, jarEnd);
            results.push(url.substring(jarEnd+2)); 
            return parseUrl(subStr, results);
        } else {
            results.push(url);
            return results;
        }
    }
}
