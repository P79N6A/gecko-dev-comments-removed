



package org.mozilla.gecko;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

import org.mozilla.gecko.db.BrowserContract;

import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.os.RemoteException;

public class TestGeckoProfilesProvider extends BrowserTestCase {
    private static final String[] NAME_AND_PATH = new String[] { BrowserContract.Profiles.NAME, BrowserContract.Profiles.PATH };

    


    public void testQueryDefault() throws RemoteException {
        final ContentResolver contentResolver = getActivity().getContentResolver();
        final Uri uri = BrowserContract.PROFILES_AUTHORITY_URI.buildUpon().appendPath("profiles").build();
        final Cursor c = contentResolver.query(uri, NAME_AND_PATH, null, null, null);
        assertNotNull(c);
        try {
            assertTrue(c.moveToFirst());
            assertTrue(c.getCount() > 0);
            Map<String, String> profiles = new HashMap<String, String>();
            while (!c.isAfterLast()) {
                final String name = c.getString(0);
                final String path = c.getString(1);
                profiles.put(name, path);
                c.moveToNext();
            }

            assertTrue(profiles.containsKey("default"));
            final String path = profiles.get("default");
            assertTrue(path.endsWith(".default"));          
            assertTrue(path.startsWith("/data/"));          
            assertTrue(path.contains("/mozilla/"));         
            assertTrue(new File(path).exists());
        } finally {
            c.close();
        }
    }
}
