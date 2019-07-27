


package org.mozilla.tests.browser.junit3;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.Stack;

import android.test.InstrumentationTestCase;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.util.FileUtils;
import org.mozilla.gecko.util.GeckoJarReader;

import android.content.Context;





public class TestJarReader extends InstrumentationTestCase {
    public void testJarReader() {
        final Context context = getInstrumentation().getTargetContext().getApplicationContext();
        String appPath = getInstrumentation().getTargetContext().getPackageResourcePath();
        assertNotNull(appPath);

        
        String url = "jar:file://" + appPath + "!/" + AppConstants.OMNIJAR_NAME;
        InputStream stream = GeckoJarReader.getStream(context, "jar:" + url + "!/chrome/chrome/content/branding/favicon32.png");
        assertNotNull(stream);

        
        url = "jar:file://" + appPath + "!/" + AppConstants.OMNIJAR_NAME;
        stream = GeckoJarReader.getStream(context, "jar:" + url + "!/chrome/chrome/content/branding/nonexistent_file.png");
        assertNull(stream);

        
        url = "jar:file://" + appPath + "!/" + "BAD" + AppConstants.OMNIJAR_NAME;
        stream = GeckoJarReader.getStream(context, "jar:" + url + "!/chrome/chrome/content/branding/favicon32.png");
        assertNull(stream);

        
        url = "jar:file://" + appPath + "!" + "!/" + AppConstants.OMNIJAR_NAME;
        stream = GeckoJarReader.getStream(context, "jar:" + url + "!/chrome/chrome/content/branding/nonexistent_file.png");
        assertNull(stream);

        
        url = "jar:file://" + appPath + "BAD" + "!/" + AppConstants.OMNIJAR_NAME;
        stream = GeckoJarReader.getStream(context, "jar:" + url + "!/chrome/chrome/content/branding/favicon32.png");
        assertNull(stream);
    }

    protected void assertExtractStream(String url) throws IOException {
        final File file = GeckoJarReader.extractStream(getInstrumentation().getTargetContext(), url, getInstrumentation().getContext().getCacheDir(), ".test");
        assertNotNull(file);
        try {
            assertTrue(file.getName().endsWith("temp"));
            final String contents = FileUtils.getFileContents(file);
            assertNotNull(contents);
            assertTrue(contents.length() > 0);
        } finally {
            file.delete();
        }
    }

    public void testExtractStream() throws IOException {
        String appPath = getInstrumentation().getTargetContext().getPackageResourcePath();
        assertNotNull(appPath);

        
        assertExtractStream("jar:file://" + appPath + "!/resources.arsc");

        final String url = GeckoJarReader.getJarURL(getInstrumentation().getTargetContext(), "chrome.manifest");
        assertExtractStream(url);

        
        final File file = GeckoJarReader.extractStream(getInstrumentation().getTargetContext(), url, getInstrumentation().getContext().getCacheDir(), ".test");
        assertNotNull(file);
        try {
            assertExtractStream("file://" + file.getAbsolutePath()); 
            assertExtractStream(file.getAbsolutePath()); 
        } finally {
            file.delete();
        }
    }

    protected void assertExtractStreamFails(String url) throws IOException {
        final File file = GeckoJarReader.extractStream(getInstrumentation().getTargetContext(), url, getInstrumentation().getContext().getCacheDir(), ".test");
        assertNull(file);
    }

    public void testExtractStreamFailureCases() throws IOException {
        String appPath = getInstrumentation().getTargetContext().getPackageResourcePath();
        assertNotNull(appPath);

        
        assertExtractStreamFails("jar:file://" + appPath + "BAD!/resources.arsc");

        
        assertExtractStreamFails("jar:file://" + appPath + "!/BADresources.arsc");

        
        final String badUrl = GeckoJarReader.getJarURL(getInstrumentation().getTargetContext(), "BADchrome.manifest");
        assertExtractStreamFails(badUrl);

        
        final String goodUrl = GeckoJarReader.getJarURL(getInstrumentation().getTargetContext(), "chrome.manifest");
        final File file = GeckoJarReader.extractStream(getInstrumentation().getTargetContext(), goodUrl, getInstrumentation().getContext().getCacheDir(), ".test");
        assertNotNull(file);
        try {
            assertExtractStreamFails("file://" + file.getAbsolutePath() + "BAD"); 
            assertExtractStreamFails(file.getAbsolutePath() + "BAD"); 
        } finally {
            file.delete();
        }
    }
}
