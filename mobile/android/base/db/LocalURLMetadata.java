




package org.mozilla.gecko.db;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.json.JSONObject;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.support.v4.util.LruCache;
import android.util.Log;


public class LocalURLMetadata implements URLMetadata {
    private static final String LOGTAG = "GeckoURLMetadata";
    private final Uri uriWithProfile;

    public LocalURLMetadata(String mProfile) {
        uriWithProfile = DBUtils.appendProfileWithDefault(mProfile, URLMetadataTable.CONTENT_URI);
    }

    
    @SuppressWarnings("serial")
    private final Set<String> getModel() {
        return new HashSet<String>() {{
            add(URLMetadataTable.URL_COLUMN);
            add(URLMetadataTable.TILE_IMAGE_URL_COLUMN);
            add(URLMetadataTable.TILE_COLOR_COLUMN);
        }};
    }

    
    private static final int CACHE_SIZE = 9;
    
    private final LruCache<String, Map<String, Object>> cache = new LruCache<String, Map<String, Object>>(CACHE_SIZE);

    



    @Override
    public Map<String, Object> fromJSON(JSONObject obj) {
        Map<String, Object> data = new HashMap<String, Object>();

        Set<String> model = getModel();
        for (String key : model) {
            if (obj.has(key)) {
                data.put(key, obj.optString(key));
            }
        }

        return Collections.unmodifiableMap(data);
    }

    




    private Map<String, Object> fromCursor(Cursor c) {
        Map<String, Object> data = new HashMap<String, Object>();

        Set<String> model = getModel();
        String[] columns = c.getColumnNames();
        for (String column : columns) {
            if (model.contains(column)) {
                try {
                    data.put(column, c.getString(c.getColumnIndexOrThrow(column)));
                } catch (Exception ex) {
                    Log.i(LOGTAG, "Error getting data for " + column, ex);
                }
            }
        }

        return Collections.unmodifiableMap(data);
    }

    



    @Override
    public Map<String, Map<String, Object>> getForURLs(final ContentResolver cr,
                                                       final List<String> urls,
                                                       final List<String> columns) {
        ThreadUtils.assertNotOnUiThread();
        ThreadUtils.assertNotOnGeckoThread();

        final Map<String, Map<String, Object>> data = new HashMap<String, Map<String, Object>>();

        
        if (urls.isEmpty() || columns.isEmpty()) {
            Log.e(LOGTAG, "Queried metadata for nothing");
            return data;
        }

        
        List<String> urlsToQuery = new ArrayList<String>();
        for (String url : urls) {
            final Map<String, Object> hit = cache.get(url);
            if (hit != null) {
                
                data.put(url, hit);
            } else {
                urlsToQuery.add(url);
            }
        }

        Telemetry.addToHistogram("FENNEC_TILES_CACHE_HIT", data.size());

        
        if (urlsToQuery.size() == 0) {
            return Collections.unmodifiableMap(data);
        }

        final String selection = DBUtils.computeSQLInClause(urlsToQuery.size(), URLMetadataTable.URL_COLUMN);
        
        if (!columns.contains(URLMetadataTable.URL_COLUMN)) {
            columns.add(URLMetadataTable.URL_COLUMN);
        }

        final Cursor cursor = cr.query(uriWithProfile,
                                       columns.toArray(new String[columns.size()]), 
                                       selection, 
                                       urlsToQuery.toArray(new String[urlsToQuery.size()]), 
                                       null);
        try {
            if (!cursor.moveToFirst()) {
                return Collections.unmodifiableMap(data);
            }

            do {
                final Map<String, Object> metadata = fromCursor(cursor);
                final String url = cursor.getString(cursor.getColumnIndexOrThrow(URLMetadataTable.URL_COLUMN));

                data.put(url, metadata);
                cache.put(url, metadata);
            } while(cursor.moveToNext());

        } finally {
            cursor.close();
        }

        return Collections.unmodifiableMap(data);
    }

    




    @Override
    public void save(final ContentResolver cr, final String url, final Map<String, Object> data) {
        ThreadUtils.assertNotOnUiThread();
        ThreadUtils.assertNotOnGeckoThread();

        try {
            ContentValues values = new ContentValues();

            Set<String> model = getModel();
            for (String key : model) {
                if (data.containsKey(key)) {
                    values.put(key, (String) data.get(key));
                }
            }

            if (values.size() == 0) {
                return;
            }

            Uri uri = uriWithProfile.buildUpon()
                                 .appendQueryParameter(BrowserContract.PARAM_INSERT_IF_NEEDED, "true")
                                 .build();
            cr.update(uri, values, URLMetadataTable.URL_COLUMN + "=?", new String[] {
                (String) data.get(URLMetadataTable.URL_COLUMN)
            });
        } catch (Exception ex) {
            Log.e(LOGTAG, "error saving", ex);
        }
    }
}
