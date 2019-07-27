




package org.mozilla.gecko.tabqueue;

import org.mozilla.gecko.BrowserApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.R;
import org.mozilla.gecko.preferences.GeckoPreferences;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.support.v4.app.NotificationCompat;
import android.util.Log;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class TabQueueHelper {
    private static final String LOGTAG = "Gecko" + TabQueueHelper.class.getSimpleName();

    public static final String FILE_NAME = "tab_queue_url_list.json";
    public static final String LOAD_URLS_ACTION = "TAB_QUEUE_LOAD_URLS_ACTION";
    public static final int TAB_QUEUE_NOTIFICATION_ID = R.id.tabQueueNotification;

    public static final String PREF_TAB_QUEUE_COUNT = "tab_queue_count";

    








    public static int queueURL(final GeckoProfile profile, final String url, final String filename) {
        ThreadUtils.assertNotOnUiThread();

        JSONArray jsonArray = profile.readJSONArrayFromFile(filename);

        jsonArray.put(url);

        profile.writeFile(filename, jsonArray.toString());

        return jsonArray.length();
    }

    






    public static void showNotification(final Context context, final int tabsQueued) {
        ThreadUtils.assertNotOnUiThread();

        Intent resultIntent = new Intent(context, BrowserApp.class);
        resultIntent.setAction(TabQueueHelper.LOAD_URLS_ACTION);

        PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, resultIntent, PendingIntent.FLAG_CANCEL_CURRENT);

        String title, text;
        final Resources resources = context.getResources();
        if (tabsQueued == 1) {
            title = resources.getString(R.string.tab_queue_notification_title_singular);
            text = resources.getString(R.string.tab_queue_notification_text_singular);
        } else {
            title = resources.getString(R.string.tab_queue_notification_title_plural);
            text = resources.getString(R.string.tab_queue_notification_text_plural, tabsQueued);
        }

        NotificationCompat.Builder builder = new NotificationCompat.Builder(context)
                                                     .setSmallIcon(R.drawable.ic_status_logo)
                                                     .setContentTitle(title)
                                                     .setContentText(text)
                                                     .setContentIntent(pendingIntent);

        NotificationManager notificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.notify(TabQueueHelper.TAB_QUEUE_NOTIFICATION_ID, builder.build());
    }

    public static boolean shouldOpenTabQueueUrls(final Context context) {
        ThreadUtils.assertNotOnUiThread();

        
        final SharedPreferences prefs = GeckoSharedPrefs.forApp(context);

        boolean tabQueueEnabled = prefs.getBoolean(GeckoPreferences.PREFS_TAB_QUEUE, false);
        int tabsQueued = prefs.getInt(PREF_TAB_QUEUE_COUNT, 0);

        return tabQueueEnabled && tabsQueued > 0;
    }

    public static int getTabQueueLength(final Context context) {
        ThreadUtils.assertNotOnUiThread();

        
        final SharedPreferences prefs = GeckoSharedPrefs.forApp(context);
        return prefs.getInt(PREF_TAB_QUEUE_COUNT, 0);
    }

    public static void openQueuedUrls(final Context context, final GeckoProfile profile, final String filename, boolean shouldPerformJavaScriptCallback) {
        ThreadUtils.assertNotOnUiThread();

        
        NotificationManager notificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancel(TAB_QUEUE_NOTIFICATION_ID);

        
        if (getTabQueueLength(context) < 1) {
            return;
        }

        JSONArray jsonArray = profile.readJSONArrayFromFile(filename);

        if (jsonArray.length() > 0) {
            JSONObject data = new JSONObject();
            try {
                data.put("urls", jsonArray);
                data.put("shouldNotifyTabsOpenedToJava", shouldPerformJavaScriptCallback);
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Tabs:OpenMultiple", data.toString()));
            } catch (JSONException e) {
                
                Log.e(LOGTAG, "Error sending tab queue data", e);
            }
        }

        try {
            profile.deleteFileFromProfileDir(filename);
        } catch (IllegalArgumentException e) {
            Log.e(LOGTAG, "Error deleting Tab Queue data file.", e);
        }

        
        final SharedPreferences prefs = GeckoSharedPrefs.forApp(context);
        prefs.edit().remove(PREF_TAB_QUEUE_COUNT).apply();
    }
}