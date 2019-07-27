



package org.mozilla.gecko;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.db.BrowserContract.ReadingListItems;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.favicons.Favicons;
import org.mozilla.gecko.util.EventCallback;
import org.mozilla.gecko.util.NativeEventListener;
import org.mozilla.gecko.util.NativeJSObject;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.util.UIAsyncTask;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.util.Log;
import android.widget.Toast;

public final class ReadingListHelper implements NativeEventListener {
    private static final String LOGTAG = "ReadingListHelper";

    protected final Context context;

    public ReadingListHelper(Context context) {
        this.context = context;

        EventDispatcher.getInstance().registerGeckoThreadListener((NativeEventListener) this,
            "Reader:AddToList", "Reader:FaviconRequest", "Reader:ListStatusRequest", "Reader:RemoveFromList");
    }

    public void uninit() {
        EventDispatcher.getInstance().unregisterGeckoThreadListener((NativeEventListener) this,
            "Reader:AddToList", "Reader:FaviconRequest", "Reader:ListStatusRequest", "Reader:RemoveFromList");
    }

    @Override
    public void handleMessage(final String event, final NativeJSObject message,
                              final EventCallback callback) {
        switch(event) {
            case "Reader:AddToList": {
                handleAddToList(callback, message);
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

        
        
        final ContentValues values = new ContentValues();
        values.put(ReadingListItems.URL, url);
        values.put(ReadingListItems.TITLE, message.getString("title"));
        values.put(ReadingListItems.LENGTH, message.getInt("length"));
        values.put(ReadingListItems.EXCERPT, message.getString("excerpt"));
        values.put(ReadingListItems.CONTENT_STATUS, message.getInt("status"));

        final BrowserDB db = GeckoProfile.get(context).getDB();
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

    



    private void handleReaderModeFaviconRequest(final EventCallback callback, final String url) {
        final BrowserDB db = GeckoProfile.get(context).getDB();
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
        final BrowserDB db = GeckoProfile.get(context).getDB();
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
        final BrowserDB db = GeckoProfile.get(context).getDB();
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
}
