



package org.mozilla.search.providers;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.text.TextUtils;
import android.util.Log;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.BrowserLocaleManager;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.util.GeckoJarReader;
import org.mozilla.search.Constants;
import org.mozilla.search.R;
import org.mozilla.search.SearchPreferenceActivity;
import org.xmlpull.v1.XmlPullParserException;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class SearchEngineManager implements SharedPreferences.OnSharedPreferenceChangeListener {
    private static final String LOG_TAG = "SearchEngineManager";

    private Context context;
    private SearchEngineCallback changeCallback;
    private SearchEngine engine;

    public static interface SearchEngineCallback {
        public void execute(SearchEngine engine);
    }

    public SearchEngineManager(Context context) {
        this.context = context;
        GeckoSharedPrefs.forApp(context).registerOnSharedPreferenceChangeListener(this);
    }

    public void setChangeCallback(SearchEngineCallback changeCallback) {
        this.changeCallback = changeCallback;
    }

    






    public void getEngine(SearchEngineCallback callback) {
        if (engine != null) {
            callback.execute(engine);
        } else {
            getEngineFromPrefs(callback);
        }
    }

    public void destroy() {
        GeckoSharedPrefs.forApp(context).unregisterOnSharedPreferenceChangeListener(this);
        context = null;
        changeCallback = null;
        engine = null;
    }

    @Override
    public void onSharedPreferenceChanged(final SharedPreferences sharedPreferences, final String key) {
        if (!TextUtils.equals(SearchPreferenceActivity.PREF_SEARCH_ENGINE_KEY, key)) {
            return;
        }
        getEngineFromPrefs(changeCallback);
    }

    






    private void getEngineFromPrefs(final SearchEngineCallback callback) {
        final AsyncTask<Void, Void, SearchEngine> task = new AsyncTask<Void, Void, SearchEngine>() {
            @Override
            protected SearchEngine doInBackground(Void... params) {
                String identifier = GeckoSharedPrefs.forApp(context).getString(SearchPreferenceActivity.PREF_SEARCH_ENGINE_KEY, null);
                if (!TextUtils.isEmpty(identifier)) {
                    try {
                        return createEngine(identifier);
                    } catch (IllegalArgumentException e) {
                        Log.e(LOG_TAG, "Exception creating search engine from pref. Falling back to default engine.", e);
                    }
                }

                try {
                    return createEngine(Constants.DEFAULT_ENGINE_IDENTIFIER);
                } catch (IllegalArgumentException e) {
                    Log.e(LOG_TAG, "Exception creating search engine from default identifier. " +
                            "This will happen if the locale doesn't contain the default search plugin.", e);
                }

                return null;
            }

            @Override
            protected void onPostExecute(SearchEngine engine) {
                if (engine != null) {
                    
                    SearchEngineManager.this.engine = engine;
                    if (callback != null) {
                        callback.execute(engine);
                    }
                }
            }
        };
        task.execute();
    }

    





    public List<SearchEngine> getAllEngines() {
        
        InputStream in = getInputStreamFromJar("list.txt");

        
        if (in == null) {
            try {
                in = context.getResources().getAssets().open("engines/list.txt");
            } catch (IOException e) {
                throw new IllegalStateException("Error reading list.txt");
            }
        }

        final List<SearchEngine> list = new ArrayList<SearchEngine>();
        InputStreamReader isr = null;

        try {
            isr = new InputStreamReader(in);
            BufferedReader br = new BufferedReader(isr);
            String identifier;
            while ((identifier = br.readLine()) != null) {
                list.add(createEngine(identifier));
            }
        } catch (IOException e) {
            throw new IllegalStateException("Error creating all search engines from list.txt");
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
        return list;
    }

    






    private SearchEngine createEngine(String identifier) {
        InputStream in = getInputStreamFromJar(identifier + ".xml");

        
        if (in == null) {
            in = getEngineFromAssets(identifier);
        }

        if (in == null) {
            throw new IllegalArgumentException("Couldn't find search engine for identifier: " + identifier);
        }

        try {
            try {
                return new SearchEngine(identifier, in);
            } finally {
                in.close();
            }
        } catch (IOException e) {
            Log.e(LOG_TAG, "Exception creating search engine", e);
        } catch (XmlPullParserException e) {
            Log.e(LOG_TAG, "Exception creating search engine", e);
        }

        return null;
    }

    






    private InputStream getEngineFromAssets(String identifier) {
        try {
            return context.getResources().getAssets().open("engines/" + identifier + ".xml");
        } catch (IOException e) {
            Log.e(LOG_TAG, "Exception getting search engine from assets", e);
            return null;
        }
    }

    






    private InputStream getInputStreamFromJar(String fileName) {
        final Locale locale = Locale.getDefault();

        
        final String languageTag = BrowserLocaleManager.getLanguageTag(locale);
        String url = getSearchPluginsJarURL(languageTag, fileName);

        final InputStream in = GeckoJarReader.getStream(url);
        if (in != null) {
            return in;
        }

        
        final String language = BrowserLocaleManager.getLanguage(locale);
        if (languageTag.equals(language)) {
            
            return null;
        }

        url = getSearchPluginsJarURL(language, fileName);
        return GeckoJarReader.getStream(url);
    }

    






    private String getSearchPluginsJarURL(String locale, String fileName) {
        final String path = "!/chrome/" + locale + "/locale/" + locale + "/browser/searchplugins/" + fileName;
        return "jar:jar:file://" + context.getPackageResourcePath() + "!/" + AppConstants.OMNIJAR_NAME + path;
    }
}
