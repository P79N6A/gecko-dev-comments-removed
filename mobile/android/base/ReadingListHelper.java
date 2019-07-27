



package org.mozilla.gecko;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.db.BrowserContract.ReadingListItems;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.db.DBUtils;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.util.EventCallback;
import org.mozilla.gecko.util.NativeEventListener;
import org.mozilla.gecko.util.NativeJSObject;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UIAsyncTask;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.Uri;
import android.util.Log;
import android.widget.Toast;

public final class ReadingListHelper implements NativeEventListener {
    private static final String LOGTAG = "GeckoReadingListHelper";

    protected final Context context;
    private final BrowserDB db;

    private final Uri readingListUriWithProfile;
    private final ContentObserver contentObserver;

    public ReadingListHelper(Context context, GeckoProfile profile) {
        this.context = context;
        this.db = profile.getDB();

        EventDispatcher.getInstance().registerGeckoThreadListener((NativeEventListener) this,
            "Reader:AddToList", "Reader:UpdateList", "Reader:FaviconRequest", "Reader:ListStatusRequest", "Reader:RemoveFromList");

        readingListUriWithProfile = DBUtils.appendProfile(profile.getName(), ReadingListItems.CONTENT_URI);

        contentObserver = new ContentObserver(null) {
            @Override
            public void onChange(boolean selfChange) {
                fetchContent();
            }
        };

        context.getContentResolver().registerContentObserver(readingListUriWithProfile, false, contentObserver);
    }

    public void uninit() {
        EventDispatcher.getInstance().unregisterGeckoThreadListener((NativeEventListener) this,
            "Reader:AddToList", "Reader:UpdateList", "Reader:FaviconRequest", "Reader:ListStatusRequest", "Reader:RemoveFromList");

        context.getContentResolver().unregisterContentObserver(contentObserver);
    }

    @Override
    public void handleMessage(final String event, final NativeJSObject message,
                              final EventCallback callback) {
        switch(event) {
            case "Reader:AddToList": {
                handleAddToList(callback, message);
                break;
            }
            case "Reader:UpdateList": {
                handleUpdateList(message);
                break;
            }
            case "Reader:FaviconRequest": {
                handleReaderModeFaviconRequest(callback, message.getString("url"));
                break;
            }
            case "Reader:RemoveFromList": {
                handleRemoveFromList(message.getString("url"));
                break;
            }
            case "Reader:ListStatusRequest": {
                handleReadingListStatusRequest(callback, message.getString("url"));
                break;
            }
        }
    }

    





    private void handleAddToList(final EventCallback callback, final NativeJSObject message) {
        final ContentResolver cr = context.getContentResolver();
        final String url = message.getString("url");

        
        
        final ContentValues values = getContentValues(message);

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                if (db.isReadingListItem(cr, url)) {
                    showToast(R.string.reading_list_duplicate, Toast.LENGTH_SHORT);
                    callback.sendError("URL already in reading list: " + url);
                } else {
                    db.addReadingListItem(cr, values);
                    showToast(R.string.reading_list_added, Toast.LENGTH_SHORT);
                    callback.sendSuccess(url);
                }
            }
        });
    }

    


    private void handleUpdateList(final NativeJSObject message) {
        final ContentResolver cr = context.getContentResolver();
        final ContentValues values = getContentValues(message);

        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                db.updateReadingListItem(cr, values);
            }
        });
    }

    


    private ContentValues getContentValues(NativeJSObject message) {
        final ContentValues values = new ContentValues();
        if (message.has("id")) {
            values.put(ReadingListItems._ID, message.getInt("id"));
        }
        if (message.has("url")) {
            values.put(ReadingListItems.URL, message.getString("url"));
        }
        if (message.has("title")) {
            values.put(ReadingListItems.TITLE, message.getString("title"));
        }
        if (message.has("length")) {
            values.put(ReadingListItems.LENGTH, message.getInt("length"));
        }
        if (message.has("excerpt")) {
            values.put(ReadingListItems.EXCERPT, message.getString("excerpt"));
        }
        if (message.has("status")) {
            values.put(ReadingListItems.CONTENT_STATUS, message.getInt("status"));
        }
        return values;
    }

    



    private void handleReaderModeFaviconRequest(final EventCallback callback, final String url) {
        (new UIAsyncTask.WithoutParams<String>(ThreadUtils.getBackgroundHandler()) {
            @Override
            public String doInBackground() {
                return Favicons.getFaviconURLForPageURL(db, context.getContentResolver(), url);
            }

            @Override
            public void onPostExecute(String faviconUrl) {
                JSONObject args = new JSONObject();
                if (faviconUrl != null) {
                    try {
                        args.put("url", url);
                        args.put("faviconUrl", faviconUrl);
                    } catch (JSONException e) {
                        Log.w(LOGTAG, "Error building JSON favicon arguments.", e);
                    }
                }
                callback.sendSuccess(args.toString());
            }
        }).execute();
    }

    



    private void handleRemoveFromList(final String url) {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                db.removeReadingListItemWithURL(context.getContentResolver(), url);
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Reader:Removed", url));
                showToast(R.string.page_removed, Toast.LENGTH_SHORT);
            }
        });
    }

    



    private void handleReadingListStatusRequest(final EventCallback callback, final String url) {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                final int inReadingList = db.isReadingListItem(context.getContentResolver(), url) ? 1 : 0;

                final JSONObject json = new JSONObject();
                try {
                    json.put("url", url);
                    json.put("inReadingList", inReadingList);
                } catch (JSONException e) {
                    Log.e(LOGTAG, "JSON error - failed to return inReadingList status", e);
                }

                
                callback.sendSuccess(json.toString());
            }
        });
    }

    


    private void showToast(final int resId, final int duration) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(context, resId, duration).show();
            }
        });
    }

    private void fetchContent() {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                final Cursor c = db.getReadingListUnfetched(context.getContentResolver());
                try {
                    while (c.moveToNext()) {
                        JSONObject json = new JSONObject();
                        try {
                            json.put("id", c.getInt(c.getColumnIndexOrThrow(ReadingListItems._ID)));
                            json.put("url", c.getString(c.getColumnIndexOrThrow(ReadingListItems.URL)));
                            GeckoAppShell.sendEventToGecko(
                                GeckoEvent.createBroadcastEvent("Reader:FetchContent", json.toString()));
                        } catch (JSONException e) {
                            Log.e(LOGTAG, "Failed to fetch reading list content for item");
                        }
                    }
                } finally {
                    c.close();
                }
            }
        });
    }
}
