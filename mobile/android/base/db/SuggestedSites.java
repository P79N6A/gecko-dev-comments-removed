




package org.mozilla.gecko.db;

import android.content.Context;
import android.content.ContentResolver;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.database.MatrixCursor.RowBuilder;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Scanner;
import java.util.Set;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.mozilla.gecko.BrowserLocaleManager;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.R;
import org.mozilla.gecko.distribution.Distribution;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.preferences.GeckoPreferences;
import org.mozilla.gecko.util.RawResource;
import org.mozilla.gecko.util.ThreadUtils;




















@RobocopTarget
public class SuggestedSites {
    private static final String LOGTAG = "GeckoSuggestedSites";

    
    public static final String PREF_SUGGESTED_SITES_HIDDEN = "suggestedSites.hidden";

    
    public static final String PREF_SUGGESTED_SITES_LOCALE = "suggestedSites.locale";

    
    private static final String FILENAME = "suggestedsites.json";

    private static final String[] COLUMNS = new String[] {
        BrowserContract.SuggestedSites._ID,
        BrowserContract.SuggestedSites.URL,
        BrowserContract.SuggestedSites.TITLE
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

        public Site(JSONObject json) throws JSONException {
            this.url = json.getString(JSON_KEY_URL);
            this.title = json.getString(JSON_KEY_TITLE);
            this.imageUrl = json.getString(JSON_KEY_IMAGE_URL);
            this.bgColor = json.getString(JSON_KEY_BG_COLOR);

            validate();
        }

        public Site(String url, String title, String imageUrl, String bgColor) {
            this.url = url;
            this.title = title;
            this.imageUrl = imageUrl;
            this.bgColor = bgColor;

            validate();
        }

        private void validate() {
            
            if (TextUtils.isEmpty(url) ||
                TextUtils.isEmpty(title) ||
                TextUtils.isEmpty(imageUrl) ||
                TextUtils.isEmpty(bgColor)) {
                throw new IllegalStateException("Suggested sites must have a URL, title, " +
                                                "image URL, and background color.");
            }
        }

        @Override
        public String toString() {
            return "{ url = " + url + "\n" +
                     "title = " + title + "\n" +
                     "imageUrl = " + imageUrl + "\n" +
                     "bgColor = " + bgColor + " }";
        }

        public JSONObject toJSON() throws JSONException {
            final JSONObject json = new JSONObject();

            json.put(JSON_KEY_URL, url);
            json.put(JSON_KEY_TITLE, title);
            json.put(JSON_KEY_IMAGE_URL, imageUrl);
            json.put(JSON_KEY_BG_COLOR, bgColor);

            return json;
        }
    }

     final Context context;
     final Distribution distribution;
     final File file;
    private Map<String, Site> cachedSites;
    private Set<String> cachedBlacklist;

    public SuggestedSites(Context appContext) {
        this(appContext, null);
    }

    public SuggestedSites(Context appContext, Distribution distribution) {
        this(appContext, distribution,
             GeckoProfile.get(appContext).getFile(FILENAME));
    }

    public SuggestedSites(Context appContext, Distribution distribution, File file) {
        this.context = appContext;
        this.distribution = distribution;
        this.file = file;
    }

    private static boolean isNewLocale(Context context, Locale requestedLocale) {
        final SharedPreferences prefs = GeckoSharedPrefs.forProfile(context);

        String locale = prefs.getString(PREF_SUGGESTED_SITES_LOCALE, null);
        if (locale == null) {
            
            updateSuggestedSitesLocale(context);
            return true;
        }

        return !TextUtils.equals(requestedLocale.toString(), locale);
    }

    


    private static List<Locale> getAcceptableLocales() {
        final List<Locale> locales = new ArrayList<Locale>();

        final Locale defaultLocale = Locale.getDefault();
        locales.add(defaultLocale);

        if (!defaultLocale.equals(Locale.US)) {
            locales.add(Locale.US);
        }

        return locales;
    }

    private static Map<String, Site> loadSites(File f) throws IOException {
        Scanner scanner = null;

        try {
            scanner = new Scanner(f, "UTF-8");
            return loadSites(scanner.useDelimiter("\\A").next());
        } finally {
            if (scanner != null) {
                scanner.close();
            }
        }
    }

    private static Map<String, Site> loadSites(String jsonString) {
        if (TextUtils.isEmpty(jsonString)) {
            return null;
        }

        Map<String, Site> sites = null;

        try {
            final JSONArray jsonSites = new JSONArray(jsonString);
            sites = new LinkedHashMap<String, Site>(jsonSites.length());

            final int count = jsonSites.length();
            for (int i = 0; i < count; i++) {
                final Site site = new Site(jsonSites.getJSONObject(i));
                sites.put(site.url, site);
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Failed to refresh suggested sites", e);
            return null;
        }

        return sites;
    }

    



     static void saveSites(File f, Map<String, Site> sites) {
        ThreadUtils.assertNotOnUiThread();

        if (sites == null || sites.isEmpty()) {
            return;
        }

        OutputStreamWriter osw = null;

        try {
            final JSONArray jsonSites = new JSONArray();
            for (Site site : sites.values()) {
                jsonSites.put(site.toJSON());
            }

            osw = new OutputStreamWriter(new FileOutputStream(f), "UTF-8");

            final String jsonString = jsonSites.toString();
            osw.write(jsonString, 0, jsonString.length());
        } catch (Exception e) {
            Log.e(LOGTAG, "Failed to save suggested sites", e);
        } finally {
            if (osw != null) {
                try {
                    osw.close();
                } catch (IOException e) {
                    
                }
            }
        }
    }

    private void maybeWaitForDistribution() {
        if (distribution == null) {
            return;
        }

        distribution.addOnDistributionReadyCallback(new Runnable() {
            @Override
            public void run() {
                Log.d(LOGTAG, "Running post-distribution task: suggested sites.");

                
                
                if (!distribution.exists()) {
                    return;
                }

                
                
                Map<String, Site> sites = loadFromDistribution(distribution);
                if (sites == null) {
                    sites = new LinkedHashMap<String, Site>();
                }
                sites.putAll(loadFromResource());

                
                setCachedSites(sites);

                
                synchronized (file) {
                    saveSites(file, sites);
                }

                
                final ContentResolver cr = context.getContentResolver();
                cr.notifyChange(BrowserContract.SuggestedSites.CONTENT_URI, null);
            }
        });
    }

    






     static Map<String, Site> loadFromDistribution(Distribution dist) {
        for (Locale locale : getAcceptableLocales()) {
            try {
                final String languageTag = BrowserLocaleManager.getLanguageTag(locale);
                final String path = String.format("suggestedsites/locales/%s/%s",
                                                  languageTag, FILENAME);

                final File f = dist.getDistributionFile(path);
                if (f == null) {
                    Log.d(LOGTAG, "No suggested sites for locale: " + languageTag);
                    continue;
                }

                return loadSites(f);
            } catch (Exception e) {
                Log.e(LOGTAG, "Failed to open suggested sites for locale " +
                              locale + " in distribution.", e);
            }
        }

        return null;
    }

    private Map<String, Site> loadFromProfile() {
        try {
            synchronized (file) {
                return loadSites(file);
            }
        } catch (FileNotFoundException e) {
            maybeWaitForDistribution();
        } catch (IOException e) {
            
        }

        return null;
    }

     Map<String, Site> loadFromResource() {
        try {
            return loadSites(RawResource.getAsString(context, R.raw.suggestedsites));
        } catch (IOException e) {
            return null;
        }
    }

    private synchronized void setCachedSites(Map<String, Site> sites) {
        cachedSites = Collections.unmodifiableMap(sites);
        updateSuggestedSitesLocale(context);
    }

    




    private void refresh() {
        Log.d(LOGTAG, "Refreshing suggested sites from file");

        Map<String, Site> sites = loadFromProfile();
        if (sites == null) {
            sites = loadFromResource();
        }

        
        if (sites != null) {
            setCachedSites(sites);
        }
    }

    private static void updateSuggestedSitesLocale(Context context) {
        final Editor editor = GeckoSharedPrefs.forProfile(context).edit();
        editor.putString(PREF_SUGGESTED_SITES_LOCALE, Locale.getDefault().toString());
        editor.apply();
    }

    private boolean isEnabled() {
        final SharedPreferences prefs = GeckoSharedPrefs.forApp(context);
        return prefs.getBoolean(GeckoPreferences.PREFS_SUGGESTED_SITES, true);
    }

    private synchronized Site getSiteForUrl(String url) {
        if (cachedSites == null) {
            return null;
        }

        return cachedSites.get(url);
    }

    




    public Cursor get(int limit) {
        return get(limit, Locale.getDefault());
    }

    





    public Cursor get(int limit, Locale locale) {
        return get(limit, locale, null);
    }

    





    public Cursor get(int limit, List<String> excludeUrls) {
        return get(limit, Locale.getDefault(), excludeUrls);
    }

    






    public synchronized Cursor get(int limit, Locale locale, List<String> excludeUrls) {
        final MatrixCursor cursor = new MatrixCursor(COLUMNS);

        
        
        if (!isEnabled()) {
            return cursor;
        }

        final boolean isNewLocale = isNewLocale(context, locale);

        
        
        if (isNewLocale) {
            file.delete();
        }

        if (cachedSites == null || isNewLocale) {
            Log.d(LOGTAG, "No cached sites, refreshing.");
            refresh();
        }

        
        
        if (cachedSites == null || cachedSites.isEmpty()) {
            return cursor;
        }

        excludeUrls = includeBlacklist(excludeUrls);

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

    private Set<String> loadBlacklist() {
        Log.d(LOGTAG, "Loading blacklisted suggested sites from SharedPreferences.");
        final Set<String> blacklist = new HashSet<String>();

        final SharedPreferences preferences = GeckoSharedPrefs.forProfile(context);
        final String sitesString = preferences.getString(PREF_SUGGESTED_SITES_HIDDEN, null);

        if (sitesString != null) {
            for (String site : sitesString.trim().split(" ")) {
                blacklist.add(Uri.decode(site));
            }
        }

        return blacklist;
    }

    private List<String> includeBlacklist(List<String> originalList) {
        if (cachedBlacklist == null) {
            cachedBlacklist = loadBlacklist();
        }

        if (cachedBlacklist.isEmpty()) {
            return originalList;
        }

        if (originalList == null) {
            originalList = new ArrayList<String>();
        }

        originalList.addAll(cachedBlacklist);
        return originalList;
    }

    









    public synchronized boolean hideSite(String url) {
        ThreadUtils.assertNotOnUiThread();

        if (cachedSites == null) {
            refresh();
            if (cachedSites == null) {
                Log.w(LOGTAG, "Could not load suggested sites!");
                return false;
            }
        }

        if (cachedSites.containsKey(url)) {
            if (cachedBlacklist == null) {
                cachedBlacklist = loadBlacklist();
            }

            
            if (!cachedBlacklist.contains(url)) {

                saveToBlacklist(url);
                cachedBlacklist.add(url);

                return true;
            }
        }

        return false;
    }

    private void saveToBlacklist(String url) {
        final SharedPreferences prefs = GeckoSharedPrefs.forProfile(context);
        final String prefString = prefs.getString(PREF_SUGGESTED_SITES_HIDDEN, "");
        final String siteString = prefString.concat(" " + Uri.encode(url));
        prefs.edit().putString(PREF_SUGGESTED_SITES_HIDDEN, siteString).apply();
    }
}
