


package org.mozilla.gecko.browser.tests;

import android.content.Context;
import android.content.res.Resources;
import android.database.Cursor;
import android.test.mock.MockContext;
import android.test.mock.MockResources;

import java.io.ByteArrayInputStream;
import java.io.InputStream;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.SuggestedSites;
import org.mozilla.gecko.util.RawResource;

public class TestSuggestedSites extends BrowserTestCase {
    private static class TestContext extends MockContext {
        private final Resources resources;

        public TestContext() {
            resources = new TestResources();
        }

        @Override
        public Resources getResources() {
            return resources;
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
        context = new TestContext();
        resources = (TestResources) context.getResources();
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

            String imageUrl = c.getString(c.getColumnIndexOrThrow(BrowserContract.SuggestedSites.IMAGE_URL));
            assertEquals("imageUrl" + position, imageUrl);

            String bgColor = c.getString(c.getColumnIndexOrThrow(BrowserContract.SuggestedSites.BG_COLOR));
            assertEquals("bgColor" + position, bgColor);
        }

        c.close();
    }
}
