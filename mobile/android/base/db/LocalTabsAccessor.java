



package org.mozilla.gecko.db;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.regex.Pattern;

import org.json.JSONArray;
import org.json.JSONException;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UIAsyncTask;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.util.Log;

public class LocalTabsAccessor implements TabsAccessor {
    private static final String LOGTAG = "GeckoTabsAccessor";

    public static final String[] TABS_PROJECTION_COLUMNS = new String[] {
                                                                BrowserContract.Tabs.TITLE,
                                                                BrowserContract.Tabs.URL,
                                                                BrowserContract.Clients.GUID,
                                                                BrowserContract.Clients.NAME,
                                                                BrowserContract.Tabs.LAST_USED,
                                                                BrowserContract.Clients.LAST_MODIFIED,
                                                                BrowserContract.Clients.DEVICE_TYPE,
                                                            };

    public static final String[] CLIENTS_PROJECTION_COLUMNS = new String[] {
                                                                    BrowserContract.Clients.GUID,
                                                                    BrowserContract.Clients.NAME,
                                                                    BrowserContract.Clients.LAST_MODIFIED,
                                                                    BrowserContract.Clients.DEVICE_TYPE
                                                            };

    private static final String REMOTE_CLIENTS_SELECTION = BrowserContract.Clients.GUID + " IS NOT NULL";
    private static final String LOCAL_TABS_SELECTION = BrowserContract.Tabs.CLIENT_GUID + " IS NULL";
    private static final String REMOTE_TABS_SELECTION = BrowserContract.Tabs.CLIENT_GUID + " IS NOT NULL";

    private static final String REMOTE_TABS_SORT_ORDER =
            
            BrowserContract.Clients.LAST_MODIFIED + " DESC, " +
            
            
            BrowserContract.Clients.GUID + " DESC, " +
            
            BrowserContract.Tabs.LAST_USED + " DESC";

    private static final String LOCAL_CLIENT_SELECTION = BrowserContract.Clients.GUID + " IS NULL";

    private static final Pattern FILTERED_URL_PATTERN = Pattern.compile("^(about|chrome|wyciwyg|file):");

    private final Uri clientsRecencyUriWithProfile;
    private final Uri tabsUriWithProfile;
    private final Uri clientsUriWithProfile;

    public LocalTabsAccessor(String profileName) {
        tabsUriWithProfile = DBUtils.appendProfileWithDefault(profileName, BrowserContract.Tabs.CONTENT_URI);
        clientsUriWithProfile = DBUtils.appendProfileWithDefault(profileName, BrowserContract.Clients.CONTENT_URI);
        clientsRecencyUriWithProfile = DBUtils.appendProfileWithDefault(profileName, BrowserContract.Clients.CONTENT_RECENCY_URI);
    }

    



    @Override
    public List<RemoteClient> getClientsWithoutTabsByRecencyFromCursor(Cursor cursor) {
        final ArrayList<RemoteClient> clients = new ArrayList<>(cursor.getCount());

        final int originalPosition = cursor.getPosition();
        try {
            if (!cursor.moveToFirst()) {
                return clients;
            }

            final int clientGuidIndex = cursor.getColumnIndex(BrowserContract.Clients.GUID);
            final int clientNameIndex = cursor.getColumnIndex(BrowserContract.Clients.NAME);
            final int clientLastModifiedIndex = cursor.getColumnIndex(BrowserContract.Clients.LAST_MODIFIED);
            final int clientDeviceTypeIndex = cursor.getColumnIndex(BrowserContract.Clients.DEVICE_TYPE);

            while (!cursor.isAfterLast()) {
                final String clientGuid = cursor.getString(clientGuidIndex);
                final String clientName = cursor.getString(clientNameIndex);
                final String deviceType = cursor.getString(clientDeviceTypeIndex);
                final long lastModified = cursor.getLong(clientLastModifiedIndex);

                clients.add(new RemoteClient(clientGuid, clientName, lastModified, deviceType));

                cursor.moveToNext();
            }
        } finally {
            cursor.moveToPosition(originalPosition);
        }
        return clients;
    }

    











    @Override
    public List<RemoteClient> getClientsFromCursor(final Cursor cursor) {
        final ArrayList<RemoteClient> clients = new ArrayList<RemoteClient>();

        final int originalPosition = cursor.getPosition();
        try {
            if (!cursor.moveToFirst()) {
                return clients;
            }

            final int tabTitleIndex = cursor.getColumnIndex(BrowserContract.Tabs.TITLE);
            final int tabUrlIndex = cursor.getColumnIndex(BrowserContract.Tabs.URL);
            final int tabLastUsedIndex = cursor.getColumnIndex(BrowserContract.Tabs.LAST_USED);
            final int clientGuidIndex = cursor.getColumnIndex(BrowserContract.Clients.GUID);
            final int clientNameIndex = cursor.getColumnIndex(BrowserContract.Clients.NAME);
            final int clientLastModifiedIndex = cursor.getColumnIndex(BrowserContract.Clients.LAST_MODIFIED);
            final int clientDeviceTypeIndex = cursor.getColumnIndex(BrowserContract.Clients.DEVICE_TYPE);

            
            
            
            RemoteClient lastClient = null;
            while (!cursor.isAfterLast()) {
                final String clientGuid = cursor.getString(clientGuidIndex);
                if (lastClient == null || !TextUtils.equals(lastClient.guid, clientGuid)) {
                    final String clientName = cursor.getString(clientNameIndex);
                    final long lastModified = cursor.getLong(clientLastModifiedIndex);
                    final String deviceType = cursor.getString(clientDeviceTypeIndex);
                    lastClient = new RemoteClient(clientGuid, clientName, lastModified, deviceType);
                    clients.add(lastClient);
                }

                final String tabTitle = cursor.getString(tabTitleIndex);
                final String tabUrl = cursor.getString(tabUrlIndex);
                final long tabLastUsed = cursor.getLong(tabLastUsedIndex);
                lastClient.tabs.add(new RemoteTab(tabTitle, tabUrl, tabLastUsed));

                cursor.moveToNext();
            }
        } finally {
            cursor.moveToPosition(originalPosition);
        }

        return clients;
    }

    @Override
    public Cursor getRemoteClientsByRecencyCursor(Context context) {
        final Uri uri = clientsRecencyUriWithProfile;
        return context.getContentResolver().query(uri, CLIENTS_PROJECTION_COLUMNS,
                REMOTE_CLIENTS_SELECTION, null, null);
    }

    @Override
    public Cursor getRemoteTabsCursor(Context context) {
        return getRemoteTabsCursor(context, -1);
    }

    @Override
    public Cursor getRemoteTabsCursor(Context context, int limit) {
        Uri uri = tabsUriWithProfile;

        if (limit > 0) {
            uri = uri.buildUpon()
                     .appendQueryParameter(BrowserContract.PARAM_LIMIT, String.valueOf(limit))
                     .build();
        }

        final Cursor cursor =  context.getContentResolver().query(uri,
                                                            TABS_PROJECTION_COLUMNS,
                                                            REMOTE_TABS_SELECTION,
                                                            null,
                                                            REMOTE_TABS_SORT_ORDER);
        return cursor;
    }

    
    
    @Override
    public void getTabs(final Context context, final OnQueryTabsCompleteListener listener) {
        getTabs(context, 0, listener);
    }

    
    
    @Override
    public void getTabs(final Context context, final int limit, final OnQueryTabsCompleteListener listener) {
        
        if (listener == null)
            return;

        (new UIAsyncTask.WithoutParams<List<RemoteClient>>(ThreadUtils.getBackgroundHandler()) {
            @Override
            protected List<RemoteClient> doInBackground() {
                final Cursor cursor = getRemoteTabsCursor(context, limit);
                if (cursor == null)
                    return null;

                try {
                    return Collections.unmodifiableList(getClientsFromCursor(cursor));
                } finally {
                    cursor.close();
                }
            }

            @Override
            protected void onPostExecute(List<RemoteClient> clients) {
                listener.onQueryTabsComplete(clients);
            }
        }).execute();
    }

    
    private void updateLocalClient(final ContentResolver cr) {
        ContentValues values = new ContentValues();
        values.put(BrowserContract.Clients.LAST_MODIFIED, System.currentTimeMillis());

        cr.update(clientsUriWithProfile, values, LOCAL_CLIENT_SELECTION, null);
    }

    
    private void deleteLocalTabs(final ContentResolver cr) {
        cr.delete(tabsUriWithProfile, LOCAL_TABS_SELECTION, null);
    }

    









    private void insertLocalTabs(final ContentResolver cr, final Iterable<Tab> tabs) {
        
        JSONArray history = new JSONArray();
        ArrayList<ContentValues> valuesToInsert = new ArrayList<ContentValues>();

        int position = 0;
        for (Tab tab : tabs) {
            
            String url = tab.getURL();
            if (url == null || tab.isPrivate() || isFilteredURL(url))
                continue;

            ContentValues values = new ContentValues();
            values.put(BrowserContract.Tabs.URL, url);
            values.put(BrowserContract.Tabs.TITLE, tab.getTitle());
            values.put(BrowserContract.Tabs.LAST_USED, tab.getLastUsed());

            String favicon = tab.getFaviconURL();
            if (favicon != null)
                values.put(BrowserContract.Tabs.FAVICON, favicon);
            else
                values.putNull(BrowserContract.Tabs.FAVICON);

            
            
            try {
                history.put(0, tab.getURL());
                values.put(BrowserContract.Tabs.HISTORY, history.toString());
            } catch (JSONException e) {
                Log.w(LOGTAG, "JSONException adding URL to tab history array.", e);
            }

            values.put(BrowserContract.Tabs.POSITION, position++);

            
            values.putNull(BrowserContract.Tabs.CLIENT_GUID);

            valuesToInsert.add(values);
        }

        ContentValues[] valuesToInsertArray = valuesToInsert.toArray(new ContentValues[valuesToInsert.size()]);
        cr.bulkInsert(tabsUriWithProfile, valuesToInsertArray);
    }

    
    @Override
    public synchronized void persistLocalTabs(final ContentResolver cr, final Iterable<Tab> tabs) {
        deleteLocalTabs(cr);
        insertLocalTabs(cr, tabs);
        updateLocalClient(cr);
    }

    




    private boolean isFilteredURL(String url) {
        return FILTERED_URL_PATTERN.matcher(url).lookingAt();
    }
}
