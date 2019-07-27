



package org.mozilla.search.providers;

import android.content.Context;
import android.content.SharedPreferences;
import android.text.TextUtils;
import android.util.Log;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.Locales;
import org.mozilla.gecko.R;
import org.mozilla.gecko.distribution.Distribution;
import org.mozilla.gecko.util.FileUtils;
import org.mozilla.gecko.util.GeckoJarReader;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.RawResource;
import org.mozilla.gecko.util.ThreadUtils;
import org.xmlpull.v1.XmlPullParserException;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Locale;

public class SearchEngineManager implements SharedPreferences.OnSharedPreferenceChangeListener {
    private static final String LOG_TAG = "GeckoSearchEngineManager";

    
    private static final String PREF_GECKO_DEFAULT_ENGINE = "browser.search.defaultenginename";

    
    private static final String PREF_GECKO_DEFAULT_LOCALE = "distribution.searchplugins.defaultLocale";

    
    private static final String PREF_DEFAULT_ENGINE_KEY = "search.engines.defaultname";

    
    private static final String PREF_REGION_KEY = "search.region";

    
    private static final String GEOIP_LOCATION_URL = "https://location.services.mozilla.com/v1/country?key=" + AppConstants.MOZ_MOZILLA_API_KEY;

    
    
    private static final String USER_AGENT = HardwareUtils.isTablet() ?
        AppConstants.USER_AGENT_FENNEC_TABLET : AppConstants.USER_AGENT_FENNEC_MOBILE;

    private Context context;
    private Distribution distribution;
    private SearchEngineCallback changeCallback;
    private SearchEngine engine;

    
    
    private String fallbackLocale;

    
    
    private String distributionLocale;

    public static interface SearchEngineCallback {
        public void execute(SearchEngine engine);
    }

    public SearchEngineManager(Context context, Distribution distribution) {
        this.context = context;
        this.distribution = distribution;
        GeckoSharedPrefs.forApp(context).registerOnSharedPreferenceChangeListener(this);
    }

    






    public void setChangeCallback(SearchEngineCallback changeCallback) {
        this.changeCallback = changeCallback;
    }

    






    public void getEngine(SearchEngineCallback callback) {
        if (engine != null) {
            callback.execute(engine);
        } else {
            getDefaultEngine(callback);
        }
    }

    public void destroy() {
        GeckoSharedPrefs.forApp(context).unregisterOnSharedPreferenceChangeListener(this);
        context = null;
        distribution = null;
        changeCallback = null;
        engine = null;
    }

    private int ignorePreferenceChange = 0;

    @Override
    public void onSharedPreferenceChanged(final SharedPreferences sharedPreferences, final String key) {
        if (!TextUtils.equals(PREF_DEFAULT_ENGINE_KEY, key)) {
            return;
        }

        if (ignorePreferenceChange > 0) {
            ignorePreferenceChange--;
            return;
        }

        getDefaultEngine(changeCallback);
    }

    


    private void runCallback(final SearchEngine engine, final SearchEngineCallback callback) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                
                SearchEngineManager.this.engine = engine;
                callback.execute(engine);
            }
        });
    }

    










    private void getDefaultEngine(final SearchEngineCallback callback) {
        
        distribution.addOnDistributionReadyCallback(new Distribution.ReadyCallback() {
            @Override
            public void distributionNotFound() {
                defaultBehavior();
            }

            @Override
            public void distributionFound(Distribution distribution) {
                defaultBehavior();
            }

            @Override
            public void distributionArrivedLate(Distribution distribution) {
                
                
                final String name = getDefaultEngineNameFromDistribution();

                if (name == null) {
                    return;
                }

                
                
                
                ignorePreferenceChange++;
                GeckoSharedPrefs.forApp(context)
                        .edit()
                        .putString(PREF_DEFAULT_ENGINE_KEY, name)
                        .apply();

                final SearchEngine engine = createEngineFromName(name);
                runCallback(engine, callback);
            }

            private void defaultBehavior() {
                
                String name = GeckoSharedPrefs.forApp(context).getString(PREF_DEFAULT_ENGINE_KEY, null);

                
                
                String region = GeckoSharedPrefs.forApp(context).getString(PREF_REGION_KEY, null);

                if (name != null && region != null) {
                    Log.d(LOG_TAG, "Found default engine name in SharedPreferences: " + name);
                } else {
                    
                    name = getDefaultEngineNameFromDistribution();
                    if (name == null) {
                        
                        name = getDefaultEngineNameFromLocale();
                    }

                    
                    
                    
                    ignorePreferenceChange++;
                    GeckoSharedPrefs.forApp(context)
                                    .edit()
                                    .putString(PREF_DEFAULT_ENGINE_KEY, name)
                                    .apply();
                }

                final SearchEngine engine = createEngineFromName(name);
                runCallback(engine, callback);
            }
        });
    }

    





    private String getDefaultEngineNameFromDistribution() {
        if (!distribution.exists()) {
            return null;
        }

        final File prefFile = distribution.getDistributionFile("preferences.json");
        if (prefFile == null) {
            return null;
        }

        try {
            final JSONObject all = new JSONObject(FileUtils.getFileContents(prefFile));

            
            if (all.has("Preferences")) {
                final JSONObject prefs = all.getJSONObject("Preferences");
                if (prefs.has(PREF_GECKO_DEFAULT_LOCALE)) {
                    Log.d(LOG_TAG, "Found default searchplugin locale in distribution Preferences.");
                    distributionLocale = prefs.getString(PREF_GECKO_DEFAULT_LOCALE);
                }
            }

            
            final String languageTag = Locales.getLanguageTag(Locale.getDefault());
            final String overridesKey = "LocalizablePreferences." + languageTag;
            if (all.has(overridesKey)) {
                final JSONObject overridePrefs = all.getJSONObject(overridesKey);
                if (overridePrefs.has(PREF_GECKO_DEFAULT_ENGINE)) {
                    Log.d(LOG_TAG, "Found default engine name in distribution LocalizablePreferences override.");
                    return overridePrefs.getString(PREF_GECKO_DEFAULT_ENGINE);
                }
            }

            
            if (all.has("LocalizablePreferences")) {
                final JSONObject localizablePrefs = all.getJSONObject("LocalizablePreferences");
                if (localizablePrefs.has(PREF_GECKO_DEFAULT_ENGINE)) {
                    Log.d(LOG_TAG, "Found default engine name in distribution LocalizablePreferences.");
                    return localizablePrefs.getString(PREF_GECKO_DEFAULT_ENGINE);
                }
            }
        } catch (IOException e) {
            Log.e(LOG_TAG, "Error getting search engine name from preferences.json", e);
        } catch (JSONException e) {
            Log.e(LOG_TAG, "Error parsing preferences.json", e);
        }
        return null;
    }

    





    private String getHttpResponse(HttpURLConnection conn) {
        InputStream is = null;
        try {
            is = new BufferedInputStream(conn.getInputStream());
            return new java.util.Scanner(is).useDelimiter("\\A").next();
        } catch (Exception e) {
            return "";
        } finally {
            if (is != null) {
                try {
                    is.close();
                } catch (IOException e) {
                    Log.e(LOG_TAG, "Error closing InputStream", e);
                }
            }
        }
    }

    






    private String fetchCountryCode() {
        
        final String region = GeckoSharedPrefs.forApp(context).getString(PREF_REGION_KEY, null);
        if (region != null) {
            return region;
        }

        
        try {
            String responseText = null;

            URL url = new URL(GEOIP_LOCATION_URL);
            HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
            try {
                
                final String message = "{}";

                urlConnection.setDoOutput(true);
                urlConnection.setConnectTimeout(10000);
                urlConnection.setReadTimeout(10000);
                urlConnection.setRequestMethod("POST");
                urlConnection.setRequestProperty("User-Agent", USER_AGENT);
                urlConnection.setRequestProperty("Content-Type", "application/json");
                urlConnection.setFixedLengthStreamingMode(message.getBytes().length);

                final OutputStream out = urlConnection.getOutputStream();
                out.write(message.getBytes());
                out.close();

                responseText = getHttpResponse(urlConnection);
            } finally {
                urlConnection.disconnect();
            }

            if (responseText == null) {
                Log.e(LOG_TAG, "Country code fetch failed");
                return null;
            }

            
            final JSONObject response = new JSONObject(responseText);
            return response.optString("country_code", null);
        } catch (Exception e) {
            Log.e(LOG_TAG, "Country code fetch failed", e);
        }

        return null;
    }

    




    private String getDefaultEngineNameFromLocale() {
        try {
            final JSONObject browsersearch = new JSONObject(RawResource.getAsString(context, R.raw.browsersearch));

            
            String region = fetchCountryCode();

            
            
            GeckoSharedPrefs.forApp(context)
                            .edit()
                            .putString(PREF_REGION_KEY, (region == null ? "" : region))
                            .apply();

            if (region != null) {
                if (browsersearch.has("regions")) {
                    final JSONObject regions = browsersearch.getJSONObject("regions");
                    if (regions.has(region)) {
                        final JSONObject regionData = regions.getJSONObject(region);
                        Log.d(LOG_TAG, "Found region-specific default engine name in browsersearch.json.");
                        return regionData.getString("default");
                    }
                }
            }

            
            if (browsersearch.has("default")) {
                Log.d(LOG_TAG, "Found default engine name in browsersearch.json.");
                return browsersearch.getString("default");
            }
        } catch (IOException e) {
            Log.e(LOG_TAG, "Error getting search engine name from browsersearch.json", e);
        } catch (JSONException e) {
            Log.e(LOG_TAG, "Error parsing browsersearch.json", e);
        }
        return null;
    }

    











    private SearchEngine createEngineFromName(String name) {
        
        SearchEngine engine = createEngineFromDistribution(name);

        
        if (engine == null) {
            engine = createEngineFromLocale(name);
        }

        
        if (engine == null) {
            engine = createEngineFromProfile(name);
        }

        if (engine == null) {
            Log.e(LOG_TAG, "Could not create search engine from name: " + name);
        }

        return engine;
    }

    










    private SearchEngine createEngineFromDistribution(String name) {
        if (!distribution.exists()) {
            return null;
        }

        final File pluginsDir = distribution.getDistributionFile("searchplugins");
        if (pluginsDir == null) {
            return null;
        }

        
        
        
        
        
        ArrayList<File> files = new ArrayList<>();

        
        final File[] commonFiles = (new File(pluginsDir, "common")).listFiles();
        if (commonFiles != null) {
            Collections.addAll(files, commonFiles);
        }

        
        final File localeDir = new File(pluginsDir, "locale");
        if (localeDir != null) {
            final String languageTag = Locales.getLanguageTag(Locale.getDefault());
            final File[] localeFiles = (new File(localeDir, languageTag)).listFiles();
            if (localeFiles != null) {
                Collections.addAll(files, localeFiles);
            } else {
                
                if (distributionLocale != null) {
                    final File[] defaultLocaleFiles = (new File(localeDir, distributionLocale)).listFiles();
                    if (defaultLocaleFiles != null) {
                        Collections.addAll(files, defaultLocaleFiles);
                    }
                }
            }
        }

        if (files.isEmpty()) {
            Log.e(LOG_TAG, "Could not find search plugin files in distribution directory");
            return null;
        }

        return createEngineFromFileList(files.toArray(new File[files.size()]), name);
    }

    










    private SearchEngine createEngineFromLocale(String name) {
        final InputStream in = getInputStreamFromSearchPluginsJar("list.txt");
        final BufferedReader br = getBufferedReader(in);

        try {
            String identifier;
            while ((identifier = br.readLine()) != null) {
                final InputStream pluginIn = getInputStreamFromSearchPluginsJar(identifier + ".xml");
                final SearchEngine engine = createEngineFromInputStream(identifier, pluginIn);
                if (engine != null && engine.getName().equals(name)) {
                    return engine;
                }
            }
        } catch (IOException e) {
            Log.e(LOG_TAG, "Error creating shipped search engine from name: " + name, e);
        } finally {
            try {
                br.close();
            } catch (IOException e) {
                
            }
        }
        return null;
    }

    








    private SearchEngine createEngineFromProfile(String name) {
        final File pluginsDir = GeckoProfile.get(context).getFile("searchplugins");
        if (pluginsDir == null) {
            return null;
        }

        final File[] files = pluginsDir.listFiles();
        if (files == null) {
            Log.e(LOG_TAG, "Could not find search plugin files in profile directory");
            return null;
        }
        return createEngineFromFileList(files, name);
    }

    







    private SearchEngine createEngineFromFileList(File[] files, String name) {
        for (int i = 0; i < files.length; i++) {
            try {
                final FileInputStream fis = new FileInputStream(files[i]);
                final SearchEngine engine = createEngineFromInputStream(null, fis);
                if (engine != null && engine.getName().equals(name)) {
                    return engine;
                }
            } catch (IOException e) {
                Log.e(LOG_TAG, "Error creating search engine from name: " + name, e);
            }
        }
        return null;
    }

    









    private SearchEngine createEngineFromInputStream(String identifier, InputStream in) {
        try {
            try {
                return new SearchEngine(identifier, in);
            } finally {
                in.close();
            }
        } catch (IOException | XmlPullParserException e) {
            Log.e(LOG_TAG, "Exception creating search engine", e);
        }

        return null;
    }

    





    private InputStream getInputStreamFromSearchPluginsJar(String fileName) {
        final Locale locale = Locale.getDefault();

        
        final String languageTag = Locales.getLanguageTag(locale);
        String url = getSearchPluginsJarURL(context, languageTag, fileName);

        InputStream in = GeckoJarReader.getStream(url);
        if (in != null) {
            return in;
        }

        
        final String language = Locales.getLanguage(locale);
        if (!languageTag.equals(language)) {
            url = getSearchPluginsJarURL(context, language, fileName);
            in = GeckoJarReader.getStream(url);
            if (in != null) {
                return in;
            }
        }

        
        url = getSearchPluginsJarURL(context, getFallbackLocale(), fileName);
        return GeckoJarReader.getStream(url);
    }

    





    private String getFallbackLocale() {
        if (fallbackLocale != null) {
            return fallbackLocale;
        }

        final InputStream in = GeckoJarReader.getStream(GeckoJarReader.getJarURL(context, "chrome/chrome.manifest"));
        final BufferedReader br = getBufferedReader(in);

        try {
            String line;
            while ((line = br.readLine()) != null) {
                
                
                if (line.startsWith("locale global ")) {
                    fallbackLocale = line.split(" ", 4)[2];
                    break;
                }
            }
        } catch (IOException e) {
            Log.e(LOG_TAG, "Error reading fallback locale from chrome registry", e);
        } finally {
            try {
                br.close();
            } catch (IOException e) {
                
            }
        }
        return fallbackLocale;
    }

    






    private static String getSearchPluginsJarURL(Context context, String locale, String fileName) {
        final String path = "chrome/" + locale + "/locale/" + locale + "/browser/searchplugins/" + fileName;
        return GeckoJarReader.getJarURL(context, path);
    }

    private BufferedReader getBufferedReader(InputStream in) {
        try {
            return new BufferedReader(new InputStreamReader(in, "UTF-8"));
        } catch (UnsupportedEncodingException e) {
            
            return null;
        }
    }
}
