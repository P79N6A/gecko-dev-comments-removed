


package org.mozilla.tests.browser.junit3;

import java.io.InputStream;

import android.test.InstrumentationTestCase;
import org.mozilla.gecko.AppConstants;
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
}
