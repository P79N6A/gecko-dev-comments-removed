



package org.mozilla.gecko;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.regex.Pattern;

import org.json.JSONArray;
import org.json.JSONException;
import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UIAsyncTask;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Parcel;
import android.os.Parcelable;
import android.text.TextUtils;
import android.text.format.DateUtils;
import android.util.Log;

public final class TabsAccessor {
    private static final String LOGTAG = "GeckoTabsAccessor";

    public static final String[] TABS_PROJECTION_COLUMNS = new String[] {
                                                                BrowserContract.Tabs.TITLE,
                                                                BrowserContract.Tabs.URL,
                                                                BrowserContract.Clients.GUID,
                                                                BrowserContract.Clients.NAME,
                                                                BrowserContract.Clients.LAST_MODIFIED,
                                                                BrowserContract.Clients.DEVICE_TYPE,
                                                            };

    private static final String LOCAL_TABS_SELECTION = BrowserContract.Tabs.CLIENT_GUID + " IS NULL";
    private static final String REMOTE_TABS_SELECTION = BrowserContract.Tabs.CLIENT_GUID + " IS NOT NULL";

    private static final String REMOTE_TABS_SORT_ORDER =
            
            BrowserContract.Clients.LAST_MODIFIED + " DESC, " +
            
            
            BrowserContract.Clients.GUID + " DESC, " +
            
            BrowserContract.Tabs.LAST_USED + " DESC";

    private static final String LOCAL_CLIENT_SELECTION = BrowserContract.Clients.GUID + " IS NULL";

    private static final Pattern FILTERED_URL_PATTERN = Pattern.compile("^(about|chrome|wyciwyg|file):");

    





    public static class RemoteClient implements Parcelable {
        public final String guid;
        public final String name;
        public final long lastModified;
        public final String deviceType;
        public final ArrayList<RemoteTab> tabs;

        public RemoteClient(String guid, String name, long lastModified, String deviceType) {
            this.guid = guid;
            this.name = name;
            this.lastModified = lastModified;
            this.deviceType = deviceType;
            this.tabs = new ArrayList<RemoteTab>();
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel parcel, int flags) {
            parcel.writeString(guid);
            parcel.writeString(name);
            parcel.writeLong(lastModified);
            parcel.writeString(deviceType);
            parcel.writeTypedList(tabs);
        }

        public static final Creator<RemoteClient> CREATOR = new Creator<RemoteClient>() {
            @Override
            public RemoteClient createFromParcel(final Parcel source) {
                final String guid = source.readString();
                final String name = source.readString();
                final long lastModified = source.readLong();
                final String deviceType = source.readString();

                final RemoteClient client = new RemoteClient(guid, name, lastModified, deviceType);
                source.readTypedList(client.tabs, RemoteTab.CREATOR);

                return client;
            }

            @Override
            public RemoteClient[] newArray(final int size) {
                return new RemoteClient[size];
            }
        };
    }

    






    public static class RemoteTab implements Parcelable {
        public final String title;
        public final String url;

        public RemoteTab(String title, String url) {
            this.title = title;
            this.url = url;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel parcel, int flags) {
            parcel.writeString(title);
            parcel.writeString(url);
        }

        public static final Creator<RemoteTab> CREATOR = new Creator<RemoteTab>() {
            @Override
            public RemoteTab createFromParcel(final Parcel source) {
                final String title = source.readString();
                final String url = source.readString();

                return new RemoteTab(title, url);
            }

            @Override
            public RemoteTab[] newArray(final int size) {
                return new RemoteTab[size];
            }
        };

        @Override
        public int hashCode() {
            final int prime = 31;
            int result = 1;
            result = prime * result + ((title == null) ? 0 : title.hashCode());
            result = prime * result + ((url == null) ? 0 : url.hashCode());
            return result;
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (obj == null) {
                return false;
            }
            if (getClass() != obj.getClass()) {
                return false;
            }
            RemoteTab other = (RemoteTab) obj;
            if (title == null) {
                if (other.title != null) {
                    return false;
                }
            } else if (!title.equals(other.title)) {
                return false;
            }
            if (url == null) {
                if (other.url != null) {
                    return false;
                }
            } else if (!url.equals(other.url)) {
                return false;
            }
            return true;
        }
    }

    











    public static List<RemoteClient> getClientsFromCursor(final Cursor cursor) {
        final ArrayList<RemoteClient> clients = new ArrayList<TabsAccessor.RemoteClient>();

        final int originalPosition = cursor.getPosition();
        try {
            if (!cursor.moveToFirst()) {
                return clients;
            }

            final int tabTitleIndex = cursor.getColumnIndex(BrowserContract.Tabs.TITLE);
            final int tabUrlIndex = cursor.getColumnIndex(BrowserContract.Tabs.URL);
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
                lastClient.tabs.add(new RemoteTab(tabTitle, tabUrl));

                cursor.moveToNext();
            }
        } finally {
            cursor.moveToPosition(originalPosition);
        }

        return clients;
    }

    public static Cursor getRemoteTabsCursor(Context context) {
        return getRemoteTabsCursor(context, -1);
    }

    public static Cursor getRemoteTabsCursor(Context context, int limit) {
        Uri uri = BrowserContract.Tabs.CONTENT_URI;

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

    public interface OnQueryTabsCompleteListener {
        public void onQueryTabsComplete(List<RemoteClient> clients);
    }

    
    
    public static void getTabs(final Context context, final OnQueryTabsCompleteListener listener) {
        getTabs(context, 0, listener);
    }

    
    
    public static void getTabs(final Context context, final int limit, final OnQueryTabsCompleteListener listener) {
        
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

    
    private static void updateLocalClient(final ContentResolver cr) {
        ContentValues values = new ContentValues();
        values.put(BrowserContract.Clients.LAST_MODIFIED, System.currentTimeMillis());
        cr.update(BrowserContract.Clients.CONTENT_URI, values, LOCAL_CLIENT_SELECTION, null);
    }

    
    private static void deleteLocalTabs(final ContentResolver cr) {
        cr.delete(BrowserContract.Tabs.CONTENT_URI, LOCAL_TABS_SELECTION, null);
    }

    









    private static void insertLocalTabs(final ContentResolver cr, final Iterable<Tab> tabs) {
        
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
        cr.bulkInsert(BrowserContract.Tabs.CONTENT_URI, valuesToInsertArray);
    }

    
    public static synchronized void persistLocalTabs(final ContentResolver cr, final Iterable<Tab> tabs) {
        deleteLocalTabs(cr);
        insertLocalTabs(cr, tabs);
        updateLocalClient(cr);
    }

    




    private static boolean isFilteredURL(String url) {
        return FILTERED_URL_PATTERN.matcher(url).lookingAt();
    }

    






    public static String getLastSyncedString(Context context, long now, long time) {
        final CharSequence relativeTimeSpanString = DateUtils.getRelativeTimeSpanString(time, now, DateUtils.MINUTE_IN_MILLIS);
        return context.getResources().getString(R.string.remote_tabs_last_synced, relativeTimeSpanString);
    }
}
