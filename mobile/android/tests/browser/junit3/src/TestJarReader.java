


package org.mozilla.gecko;

import java.io.InputStream;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.util.GeckoJarReader;





public class TestJarReader extends BrowserTestCase {
    public void testJarReader() {
        String appPath = getActivity().getApplication().getPackageResourcePath();
        assertNotNull(appPath);

        
        String url = "jar:file://" + appPath + "!/" + AppConstants.OMNIJAR_NAME;
        InputStream stream = GeckoJarReader.getStream("jar:" + url + "!/chrome/chrome/content/branding/favicon32.png");
        assertNotNull(stream);

        
        url = "jar:file://" + appPath + "!/" + AppConstants.OMNIJAR_NAME;
        stream = GeckoJarReader.getStream("jar:" + url + "!/chrome/chrome/content/branding/nonexistent_file.png");
        assertNull(stream);

        
        url = "jar:file://" + appPath + "!/" + "BAD" + AppConstants.OMNIJAR_NAME;
        stream = GeckoJarReader.getStream("jar:" + url + "!/chrome/chrome/content/branding/favicon32.png");
        assertNull(stream);

        
        url = "jar:file://" + appPath + "!" + "!/" + AppConstants.OMNIJAR_NAME;
        stream = GeckoJarReader.getStream("jar:" + url + "!/chrome/chrome/content/branding/nonexistent_file.png");
        assertNull(stream);

        
        url = "jar:file://" + appPath + "BAD" + "!/" + AppConstants.OMNIJAR_NAME;
        stream = GeckoJarReader.getStream("jar:" + url + "!/chrome/chrome/content/branding/favicon32.png");
        assertNull(stream);
    }
}
