


package org.mozilla.gecko.browser.tests;

import android.content.Context;
import android.content.res.Resources;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.test.mock.MockResources;
import android.test.RenamingDelegatingContext;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

import org.json.JSONArray;
import org.json.JSONObject;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.SuggestedSites;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.preferences.GeckoPreferences;

public class TestSuggestedSites extends BrowserTestCase {
    private static class TestContext extends RenamingDelegatingContext {
        private static final String PREFIX = "TestSuggestedSites-";

        private final Resources resources;
        private final Set<String> usedPrefs;

        public TestContext(Context context) {
            super(context, PREFIX);
            resources = new TestResources();
            usedPrefs = Collections.synchronizedSet(new HashSet<String>());
        }

        @Override
        public Resources getResources() {
            return resources;
        }

        @Override
        public SharedPreferences getSharedPreferences(String name, int mode) {
            usedPrefs.add(name);
            return super.getSharedPreferences(PREFIX + name, mode);
        }

        public void clearUsedPrefs() {
            for (String prefsName : usedPrefs) {
                getSharedPreferences(prefsName, 0).edit().clear().commit();
            }

            usedPrefs.clear();
        }
    }

    private static class TestResources extends MockResources {
        private String suggestedSites;

        @Override
        public InputStream openRawResource(int id) {
            if (id == R.raw.suggestedsites && suggestedSites != null) {
                return new ByteArrayInputStream(suggestedSites.getBytes());
            }

            return null;
        }

        public void setSuggestedSitesResource(String suggestedSites) {
            this.suggestedSites = suggestedSites;
        }
    }

    private static final int DEFAULT_LIMIT = 6;

    private TestContext context;
    private TestResources resources;

    private String generateSites(int n) {
        JSONArray sites = new JSONArray();

        try {
            for (int i = 0; i < n; i++) {
                JSONObject site = new JSONObject();
                site.put("url", "url" + i);
                site.put("title", "title" + i);
                site.put("imageurl", "imageUrl" + i);
                site.put("bgcolor", "bgColor" + i);

                sites.put(site);
            }
        } catch (Exception e) {
            return "";
        }

        return sites.toString();
    }

    private void checkCursorCount(String content, int expectedCount) {
        checkCursorCount(content, expectedCount, DEFAULT_LIMIT);
    }

    private void checkCursorCount(String content, int expectedCount, int limit) {
        resources.setSuggestedSitesResource(content);
        Cursor c = new SuggestedSites(context).get(limit);
        assertEquals(expectedCount, c.getCount());
        c.close();
    }

    protected void setUp() {
        context = new TestContext(getApplicationContext());
        resources = (TestResources) context.getResources();
    }

    protected void tearDown() {
        context.clearUsedPrefs();
    }

    public void testCount() {
        
        checkCursorCount(generateSites(0), 0);

        
        checkCursorCount(generateSites(2), 2);

        
        checkCursorCount(generateSites(10), 3, 3);
    }

    public void testEmptyCursor() {
        
        checkCursorCount(null, 0);

        
        checkCursorCount("", 0);

        
        checkCursorCount("{ broken: }", 0);
    }

    public void testCursorContent() {
        resources.setSuggestedSitesResource(generateSites(3));

        Cursor c = new SuggestedSites(context).get(DEFAULT_LIMIT);
        assertEquals(3, c.getCount());

        c.moveToPosition(-1);
        while (c.moveToNext()) {
            int position = c.getPosition();

            String url = c.getString(c.getColumnIndexOrThrow(BrowserContract.SuggestedSites.URL));
            assertEquals("url" + position, url);

            String title = c.getString(c.getColumnIndexOrThrow(BrowserContract.SuggestedSites.TITLE));
            assertEquals("title" + position, title);
        }

        c.close();
    }

    public void testExcludeUrls() {
        resources.setSuggestedSitesResource(generateSites(6));

        List<String> excludedUrls = new ArrayList<String>(3);
        excludedUrls.add("url1");
        excludedUrls.add("url3");
        excludedUrls.add("url5");

        List<String> includedUrls = new ArrayList<String>(3);
        includedUrls.add("url0");
        includedUrls.add("url2");
        includedUrls.add("url4");

        Cursor c = new SuggestedSites(context).get(DEFAULT_LIMIT, excludedUrls);

        c.moveToPosition(-1);
        while (c.moveToNext()) {
            String url = c.getString(c.getColumnIndexOrThrow(BrowserContract.SuggestedSites.URL));
            assertFalse(excludedUrls.contains(url));
            assertTrue(includedUrls.contains(url));
        }

        c.close();
    }

    public void testHiddenSites() {
        resources.setSuggestedSitesResource(generateSites(6));

        List<String> visibleUrls = new ArrayList<String>(3);
        visibleUrls.add("url3");
        visibleUrls.add("url4");
        visibleUrls.add("url5");

        List<String> hiddenUrls = new ArrayList<String>(3);
        hiddenUrls.add("url0");
        hiddenUrls.add("url1");
        hiddenUrls.add("url2");

        
        StringBuilder hiddenUrlBuilder = new StringBuilder();
        for (String s : hiddenUrls) {
            hiddenUrlBuilder.append(" ");
            hiddenUrlBuilder.append(Uri.encode(s));
        }

        final String hiddenPref = hiddenUrlBuilder.toString();
        GeckoSharedPrefs.forProfile(context).edit()
                                        .putString(SuggestedSites.PREF_SUGGESTED_SITES_HIDDEN, hiddenPref)
                                        .commit();

        Cursor c = new SuggestedSites(context).get(DEFAULT_LIMIT);
        assertEquals(Math.min(3, DEFAULT_LIMIT), c.getCount());

        c.moveToPosition(-1);
        while (c.moveToNext()) {
            String url = c.getString(c.getColumnIndexOrThrow(BrowserContract.SuggestedSites.URL));
            assertFalse(hiddenUrls.contains(url));
            assertTrue(visibleUrls.contains(url));
        }

        c.close();
    }

    public void testDisabledState() {
        resources.setSuggestedSitesResource(generateSites(3));

        Cursor c = new SuggestedSites(context).get(DEFAULT_LIMIT);
        assertEquals(3, c.getCount());
        c.close();

        
        GeckoSharedPrefs.forApp(context).edit()
                                        .putBoolean(GeckoPreferences.PREFS_SUGGESTED_SITES, false)
                                        .commit();

        c = new SuggestedSites(context).get(DEFAULT_LIMIT);
        assertNotNull(c);
        assertEquals(0, c.getCount());
        c.close();
    }

    public void testImageUrlAndBgColor() {
        final int count = 3;
        resources.setSuggestedSitesResource(generateSites(count));

        SuggestedSites suggestedSites = new SuggestedSites(context);

        
        for (int i = 0; i < count; i++) {
            String url = "url" + i;
            assertFalse(suggestedSites.contains(url));
            assertNull(suggestedSites.getImageUrlForUrl(url));
            assertNull(suggestedSites.getBackgroundColorForUrl(url));
        }

        Cursor c = suggestedSites.get(DEFAULT_LIMIT);
        c.moveToPosition(-1);

        
        while (c.moveToNext()) {
            String url = c.getString(c.getColumnIndexOrThrow(BrowserContract.SuggestedSites.URL));
            assertTrue(suggestedSites.contains(url));
            assertEquals("imageUrl" + c.getPosition(),
                         suggestedSites.getImageUrlForUrl(url));
            assertEquals("bgColor" + c.getPosition(),
                         suggestedSites.getBackgroundColorForUrl(url));
        }
        c.close();

        
        assertFalse(suggestedSites.contains("foo"));
        assertNull(suggestedSites.getImageUrlForUrl("foo"));
        assertNull(suggestedSites.getBackgroundColorForUrl("foo"));
    }

    public void testLocaleChanges() {
        resources.setSuggestedSitesResource(generateSites(3));

        SuggestedSites suggestedSites = new SuggestedSites(context);

        
        Cursor c = suggestedSites.get(DEFAULT_LIMIT, Locale.UK);
        assertEquals(3, c.getCount());
        c.close();

        resources.setSuggestedSitesResource(generateSites(5));

        
        
        c = suggestedSites.get(DEFAULT_LIMIT, Locale.UK);
        assertEquals(3, c.getCount());
        c.close();

        
        c = suggestedSites.get(DEFAULT_LIMIT, Locale.US);
        assertEquals(5, c.getCount());
        c.close();
    }
}
