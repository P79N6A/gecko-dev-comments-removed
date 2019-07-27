



package org.mozilla.search.providers;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.text.TextUtils;
import android.util.Log;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.util.GeckoJarReader;
import org.mozilla.search.Constants;
import org.mozilla.search.SearchPreferenceActivity;
import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.io.InputStream;

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
                final String identifier = GeckoSharedPrefs.forApp(context)
                        .getString(SearchPreferenceActivity.PREF_SEARCH_ENGINE_KEY, Constants.DEFAULT_SEARCH_ENGINE)
                        .toLowerCase();
                return createEngine(identifier);
            }

            @Override
            protected void onPostExecute(SearchEngine engine) {
                
                SearchEngineManager.this.engine = engine;
                if (callback != null) {
                    callback.execute(engine);
                }
            }
        };
        task.execute();
    }

    






    private SearchEngine createEngine(String identifier) {
        InputStream in = getEngineFromJar(identifier);

        
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
            return context.getResources().getAssets().open(identifier + ".xml");
        } catch (IOException e) {
            Log.e(LOG_TAG, "Exception getting search engine from assets", e);
            return null;
        }
    }

    






    private InputStream getEngineFromJar(String identifier) {
        
        final String locale = "en-US";

        final String path = "!/chrome/" + locale + "/locale/" + locale + "/browser/searchplugins/" + identifier + ".xml";
        final String url = "jar:jar:file://" + context.getPackageResourcePath() + "!/" + AppConstants.OMNIJAR_NAME + path;

        return GeckoJarReader.getStream(url);
    }
}
