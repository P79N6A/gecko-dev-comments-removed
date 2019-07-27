



package org.mozilla.search.providers;

import android.content.Context;
import android.content.SharedPreferences;
import android.text.TextUtils;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.BrowserLocaleManager;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.R;
import org.mozilla.gecko.util.FileUtils;
import org.mozilla.gecko.util.GeckoJarReader;
import org.mozilla.gecko.util.RawResource;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.distribution.Distribution;
import org.mozilla.search.Constants;
import org.xmlpull.v1.XmlPullParserException;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class SearchEngineManager implements SharedPreferences.OnSharedPreferenceChangeListener {
    private static final String LOG_TAG = "GeckoSearchEngineManager";

    
    private static final String PREF_GECKO_DEFAULT_ENGINE = "browser.search.defaultenginename";

    
    private static final String PREF_DEFAULT_ENGINE_KEY = "search.engines.defaultname";

    private Context context;
    private Distribution distribution;
    private SearchEngineCallback changeCallback;
    private SearchEngine engine;

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
        
        distribution.addOnDistributionReadyCallback(new Runnable() {
            @Override
            public void run() {
                
                String name = GeckoSharedPrefs.forApp(context).getString(PREF_DEFAULT_ENGINE_KEY, null);

                if (name != null) {
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

            
            final String languageTag = BrowserLocaleManager.getLanguageTag(Locale.getDefault());
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

    




    private String getDefaultEngineNameFromLocale() {
        try {
            final JSONObject browsersearch = new JSONObject(RawResource.getAsString(context, R.raw.browsersearch));
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

        final File[] files = (new File(pluginsDir, "common")).listFiles();
        return createEngineFromFileList(files, name);
    }

    










    private SearchEngine createEngineFromLocale(String name) {
        final InputStream in = getInputStreamFromSearchPluginsJar("list.txt");
        InputStreamReader isr = null;

        try {
            isr = new InputStreamReader(in);
            BufferedReader br = new BufferedReader(isr);
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
            if (isr != null) {
                try {
                    isr.close();
                } catch (IOException e) {
                    
                }
            }
            try {
                in.close();
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
                Log.e(LOG_TAG, "Error creating earch engine from name: " + name, e);
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

        
        final String languageTag = BrowserLocaleManager.getLanguageTag(locale);
        String url = getSearchPluginsJarURL(languageTag, fileName);

        InputStream in = GeckoJarReader.getStream(url);
        if (in != null) {
            return in;
        }

        
        final String language = BrowserLocaleManager.getLanguage(locale);
        if (!languageTag.equals(language)) {
            url = getSearchPluginsJarURL(language, fileName);
            in = GeckoJarReader.getStream(url);
            if (in != null) {
                return in;
            }
        }

        
        url = getSearchPluginsJarURL("en-US", fileName);
        return GeckoJarReader.getStream(url);
    }

    






    private String getSearchPluginsJarURL(String locale, String fileName) {
        final String path = "!/chrome/" + locale + "/locale/" + locale + "/browser/searchplugins/" + fileName;
        return "jar:jar:file://" + context.getPackageResourcePath() + "!/" + AppConstants.OMNIJAR_NAME + path;
    }
}
