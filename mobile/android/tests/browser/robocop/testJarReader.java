



package org.mozilla.gecko.tests;

import java.io.InputStream;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.util.GeckoJarReader;

import android.content.Context;





public class testJarReader extends BaseTest {
    public void testGetJarURL() {
        
        final String s = GeckoJarReader.computeJarURI("some[1].apk", "something/else");
        mAsserter.ok(!s.contains("["), "Illegal characters are escaped away.", null);
        mAsserter.ok(!s.toLowerCase().contains("%2f"), "Path characters aren't escaped.", null);
    }

    public void testJarReader() {
        final Context context = getInstrumentation().getTargetContext().getApplicationContext();
        String appPath = getActivity().getApplication().getPackageResourcePath();
        mAsserter.isnot(appPath, null, "getPackageResourcePath is non-null");

        
        String url = "jar:file://" + appPath + "!/" + AppConstants.OMNIJAR_NAME;
        InputStream stream = GeckoJarReader.getStream(context, "jar:" + url + "!/chrome/chrome/content/branding/favicon32.png");
        mAsserter.isnot(stream, null, "JarReader returned non-null for valid file in valid jar");

        
        url = "jar:file://" + appPath + "!/" + AppConstants.OMNIJAR_NAME;
        stream = GeckoJarReader.getStream(context, "jar:" + url + "!/chrome/chrome/content/branding/nonexistent_file.png");
        mAsserter.is(stream, null, "JarReader returned null for non-existent file in valid jar");

        
        url = "jar:file://" + appPath + "!/" + "BAD" + AppConstants.OMNIJAR_NAME;
        stream = GeckoJarReader.getStream(context, "jar:" + url + "!/chrome/chrome/content/branding/favicon32.png");
        mAsserter.is(stream, null, "JarReader returned null for valid file in invalid jar file");

        
        url = "jar:file://" + appPath + "!" + "!/" + AppConstants.OMNIJAR_NAME;
        stream = GeckoJarReader.getStream(context, "jar:" + url + "!/chrome/chrome/content/branding/nonexistent_file.png");
        mAsserter.is(stream, null, "JarReader returned null for bad jar url");

        
        url = "jar:file://" + appPath + "BAD" + "!/" + AppConstants.OMNIJAR_NAME;
        stream = GeckoJarReader.getStream(context, "jar:" + url + "!/chrome/chrome/content/branding/favicon32.png");
        mAsserter.is(stream, null, "JarReader returned null for a non-existent APK");

        
        
        
        blockForGeckoReady();
    }

    private String getData(InputStream stream) {
        return new java.util.Scanner(stream).useDelimiter("\\A").next();
    }

}
