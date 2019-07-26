




package org.mozilla.gecko.db;

import android.content.Context;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.database.MatrixCursor.RowBuilder;
import android.text.TextUtils;
import android.util.Log;

import java.io.IOException;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.preferences.GeckoPreferences;
import org.mozilla.gecko.util.RawResource;


















@RobocopTarget
public class SuggestedSites {
    private static final String LOGTAG = "GeckoSuggestedSites";

    private static final String[] COLUMNS = new String[] {
        BrowserContract.SuggestedSites._ID,
        BrowserContract.SuggestedSites.URL,
        BrowserContract.SuggestedSites.TITLE,
        BrowserContract.SuggestedSites.IMAGE_URL,
        BrowserContract.SuggestedSites.BG_COLOR
    };

    private static final String JSON_KEY_URL = "url";
    private static final String JSON_KEY_TITLE = "title";
    private static final String JSON_KEY_IMAGE_URL = "imageurl";
    private static final String JSON_KEY_BG_COLOR = "bgcolor";

    private static class Site {
        public final String url;
        public final String title;
        public final String imageUrl;
        public final String bgColor;

        public Site(String url, String title, String imageUrl, String bgColor) {
            this.url = url;
            this.title = title;
            this.imageUrl = imageUrl;
            this.bgColor = bgColor;
        }

        @Override
        public String toString() {
            return "{ url = " + url + "\n" +
                     "title = " + title + "\n" +
                     "imageUrl = " + imageUrl + "\n" +
                     "bgColor = " + bgColor + " }";
        }
    }

    private final Context context;
    private Map<String, Site> cachedSites;

    public SuggestedSites(Context appContext) {
        context = appContext;
    }

    private String loadFromFile() {
        
        return null;
    }

    private String loadFromResource() {
        try {
            return RawResource.getAsString(context, R.raw.suggestedsites);
        } catch (IOException e) {
            return null;
        }
    }

    




    private void refresh() {
        Log.d(LOGTAG, "Refreshing tiles from file");

        String jsonString = loadFromFile();
        if (TextUtils.isEmpty(jsonString)) {
            Log.d(LOGTAG, "No suggested sites file, loading from resource.");
            jsonString = loadFromResource();
        }

        Map<String, Site> sites = null;

        try {
            final JSONArray jsonSites = new JSONArray(jsonString);
            sites = new LinkedHashMap<String, Site>(jsonSites.length());

            final int count = jsonSites.length();
            for (int i = 0; i < count; i++) {
                final JSONObject jsonSite = (JSONObject) jsonSites.get(i);
                final String url = jsonSite.getString(JSON_KEY_URL);

                final Site site = new Site(url,
                                           jsonSite.getString(JSON_KEY_TITLE),
                                           jsonSite.getString(JSON_KEY_IMAGE_URL),
                                           jsonSite.getString(JSON_KEY_BG_COLOR));

                sites.put(url, site);
            }

            Log.d(LOGTAG, "Successfully parsed suggested sites.");
        } catch (Exception e) {
            Log.e(LOGTAG, "Failed to refresh suggested sites", e);
            return;
        }

        
        cachedSites = Collections.unmodifiableMap(sites);
    }

    private boolean isEnabled() {
        final SharedPreferences prefs = GeckoSharedPrefs.forApp(context);
        return prefs.getBoolean(GeckoPreferences.PREFS_SUGGESTED_SITES, true);
    }

    private Site getSiteForUrl(String url) {
        if (cachedSites == null) {
            return null;
        }

        return cachedSites.get(url);
    }

    




    public Cursor get(int limit) {
        return get(limit, null);
    }

    





    public Cursor get(int limit, List<String> excludeUrls) {
        final MatrixCursor cursor = new MatrixCursor(COLUMNS);

        
        
        if (!isEnabled()) {
            return cursor;
        }

        if (cachedSites == null) {
            Log.d(LOGTAG, "No cached sites, refreshing.");
            refresh();
        }

        
        
        if (cachedSites == null || cachedSites.isEmpty()) {
            return cursor;
        }

        final int sitesCount = cachedSites.size();
        Log.d(LOGTAG, "Number of suggested sites: " + sitesCount);

        final int maxCount = Math.min(limit, sitesCount);
        for (Site site : cachedSites.values()) {
            if (cursor.getCount() == maxCount) {
                break;
            }

            if (excludeUrls != null && excludeUrls.contains(site.url)) {
                continue;
            }

            final RowBuilder row = cursor.newRow();
            row.add(-1);
            row.add(site.url);
            row.add(site.title);
            row.add(site.imageUrl);
            row.add(site.bgColor);
        }

        cursor.setNotificationUri(context.getContentResolver(),
                                  BrowserContract.SuggestedSites.CONTENT_URI);

        return cursor;
    }

    public boolean contains(String url) {
        return (getSiteForUrl(url) != null);
    }

    public String getImageUrlForUrl(String url) {
        final Site site = getSiteForUrl(url);
        return (site != null ? site.imageUrl : null);
    }

    public String getBackgroundColorForUrl(String url) {
        final Site site = getSiteForUrl(url);
        return (site != null ? site.bgColor : null);
    }
}