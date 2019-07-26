




package org.mozilla.gecko;

import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.GeckoEventListener;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.support.v4.app.NotificationCompat;
import android.util.Log;

import java.util.Set;
import java.util.HashSet;

public class NotificationHelper implements GeckoEventListener {
    public static final String NOTIFICATION_ID = "NotificationHelper_ID";
    private static final String LOGTAG = "GeckoNotificationManager";
    private Context mContext;
    private Set<String> mShowing;

    public NotificationHelper(Context context) {
        mContext = context;
        mShowing = new HashSet<String>();
        registerEventListener("Notification:Show");
        registerEventListener("Notification:Hide");
    }

    private void registerEventListener(String event) {
        GeckoAppShell.getEventDispatcher().registerEventListener(event, this);
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        if (event.equals("Notification:Show")) {
            showNotification(message);
        } else if (event.equals("Notification:Hide")) {
            hideNotification(message);
        }
    }

    private void showNotification(JSONObject message) {
        NotificationCompat.Builder builder = new NotificationCompat.Builder(mContext);

        
        final String id;
        try {
            builder.setContentTitle(message.getString("title"));
            builder.setContentText(message.getString("text"));
            id = message.getString("id");
        } catch (JSONException ex) {
            Log.i(LOGTAG, "Error parsing", ex);
            return;
        }

        Uri imageUri = Uri.parse(message.optString("smallicon"));
        builder.setSmallIcon(BitmapUtils.getResource(imageUri, R.drawable.ic_status_logo));

        JSONArray light = message.optJSONArray("light");
        if (light != null && light.length() == 3) {
            try {
                builder.setLights(light.getInt(0),
                                  light.getInt(1),
                                  light.getInt(2));
            } catch (JSONException ex) {
                Log.i(LOGTAG, "Error parsing", ex);
            }
        }

        boolean ongoing = message.optBoolean("ongoing");
        builder.setOngoing(ongoing);

        if (message.has("when")) {
            int when = message.optInt("when");
            builder.setWhen(when);
        }

        if (message.has("largeicon")) {
            Bitmap b = BitmapUtils.getBitmapFromDataURI(message.optString("largeicon"));
            builder.setLargeIcon(b);
        }

        
        
        Intent notificationIntent = new Intent(GeckoApp.ACTION_ALERT_CALLBACK);
        String app = mContext.getClass().getName();
        notificationIntent.setClassName(AppConstants.ANDROID_PACKAGE_NAME, app);
        notificationIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        
        
        if (!ongoing) {
            notificationIntent.putExtra(NOTIFICATION_ID, id);
        }

        PendingIntent pi = PendingIntent.getActivity(mContext, 0, notificationIntent, 0);
        builder.setContentIntent(pi);

        GeckoAppShell.sNotificationClient.add(id.hashCode(), builder.build());
        if (!mShowing.contains(id)) {
            mShowing.add(id);
        }
    }

    private void hideNotification(JSONObject message) {
        String id;
        try {
            id = message.getString("id");
        } catch (JSONException ex) {
            Log.i(LOGTAG, "Error parsing", ex);
            return;
        }

        hideNotification(id);
    }

    public void hideNotification(String id) {
        GeckoAppShell.sNotificationClient.remove(id.hashCode());
        mShowing.remove(id);
    }

    private void clearAll() {
        for (String id : mShowing) {
            hideNotification(id);
        }
    }

    public void destroy() {
        clearAll();
    }
}
