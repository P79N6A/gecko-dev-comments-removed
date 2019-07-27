



package org.mozilla.gecko.util;

import android.content.Context;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.mozglue.NativeZip;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.util.Log;
import org.mozilla.gecko.mozglue.RobocopTarget;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Stack;




public final class GeckoJarReader {
    private static final String LOGTAG = "GeckoJarReader";

    private GeckoJarReader() {}

    public static Bitmap getBitmap(Resources resources, String url) {
        BitmapDrawable drawable = getBitmapDrawable(resources, url);
        return (drawable != null) ? drawable.getBitmap() : null;
    }

    public static BitmapDrawable getBitmapDrawable(Resources resources, String url) {
        Stack<String> jarUrls = parseUrl(url);
        InputStream inputStream = null;
        BitmapDrawable bitmap = null;

        NativeZip zip = null;
        try {
            
            zip = getZipFile(jarUrls.pop());
            inputStream = getStream(zip, jarUrls, url);
            if (inputStream != null) {
                bitmap = new BitmapDrawable(resources, inputStream);
            }
        } catch (IOException | URISyntaxException ex) {
            Log.e(LOGTAG, "Exception ", ex);
        } finally {
            if (inputStream != null) {
                try {
                    inputStream.close();
                } catch(IOException ex) {
                    Log.e(LOGTAG, "Error closing stream", ex);
                }
            }
            if (zip != null) {
                zip.close();
            }
        }

        return bitmap;
    }

    public static String getText(String url) {
        Stack<String> jarUrls = parseUrl(url);

        NativeZip zip = null;
        BufferedReader reader = null;
        String text = null;
        try {
            zip = getZipFile(jarUrls.pop());
            InputStream input = getStream(zip, jarUrls, url);
            if (input != null) {
                reader = new BufferedReader(new InputStreamReader(input));
                text = reader.readLine();
            }
        } catch (IOException | URISyntaxException ex) {
            Log.e(LOGTAG, "Exception ", ex);
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch(IOException ex) {
                    Log.e(LOGTAG, "Error closing reader", ex);
                }
            }
            if (zip != null) {
                zip.close();
            }
        }

        return text;
    }

    private static NativeZip getZipFile(String url) throws IOException, URISyntaxException {
        URI fileUrl = new URI(url);
        return new NativeZip(fileUrl.getPath());
    }

    @RobocopTarget
    public static InputStream getStream(String url) {
        Stack<String> jarUrls = parseUrl(url);
        try {
            NativeZip zip = getZipFile(jarUrls.pop());
            return getStream(zip, jarUrls, url);
        } catch (Exception ex) {
            
            
            
            Log.e(LOGTAG, "Exception getting input stream from jar URL: " + url, ex);
            return null;
        }
    }

    private static InputStream getStream(NativeZip zip, Stack<String> jarUrls, String origUrl) {
        InputStream inputStream = null;

        
        while (!jarUrls.empty()) {
            String fileName = jarUrls.pop();

            if (inputStream != null) {
                
                try {
                    zip = new NativeZip(inputStream);
                } catch (IllegalArgumentException e) {
                    String description = "!!! BUG 849589 !!! origUrl=" + origUrl;
                    Log.e(LOGTAG, description, e);
                    throw new IllegalArgumentException(description);
                }
            }

            inputStream = zip.getInputStream(fileName);
            if (inputStream == null) {
                Log.d(LOGTAG, "No Entry for " + fileName);
                return null;
            }
        }

        return inputStream;
    }

    








    private static Stack<String> parseUrl(String url) {
        return parseUrl(url, null);
    }

    private static Stack<String> parseUrl(String url, Stack<String> results) {
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

    public static String getJarURL(Context context, String pathInsideJAR) {
        
        
        
        final String resourcePath = context.getPackageResourcePath();
        return computeJarURI(resourcePath, pathInsideJAR);
    }

    


    public static String computeJarURI(String resourcePath, String pathInsideJAR) {
        final String resURI = new File(resourcePath).toURI().toString();

        
        return "jar:jar:" + resURI + "!/" + AppConstants.OMNIJAR_NAME + "!/" + pathInsideJAR;
    }
}
