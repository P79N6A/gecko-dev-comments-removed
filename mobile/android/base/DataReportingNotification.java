




package org.mozilla.gecko;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.preferences.GeckoPreferences;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Typeface;
import android.support.v4.app.NotificationCompat;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.style.StyleSpan;

public class DataReportingNotification {

    private static final String LOGTAG = "DataReportNotification";

    public static final String ALERT_NAME_DATAREPORTING_NOTIFICATION = "datareporting-notification";

    private static final String PREFS_POLICY_NOTIFIED_TIME = "datareporting.policy.dataSubmissionPolicyNotifiedTime";
    private static final String PREFS_POLICY_VERSION = "datareporting.policy.dataSubmissionPolicyVersion";
    private static final int DATA_REPORTING_VERSION = 2;

    public static void checkAndNotifyPolicy(Context context) {
        SharedPreferences dataPrefs = GeckoSharedPrefs.forApp(context);
        final int currentVersion = dataPrefs.getInt(PREFS_POLICY_VERSION, -1);

        if (currentVersion < 1) {
            
            notifyDataPolicy(context, dataPrefs);

            
            if (AppConstants.MOZ_SERVICES_HEALTHREPORT) {
                SharedPreferences.Editor editor = dataPrefs.edit();
                editor.putBoolean(GeckoPreferences.PREFS_HEALTHREPORT_UPLOAD_ENABLED, true);
                editor.apply();
            }
            return;
        }

        if (currentVersion == 1) {
            
            if (TextUtils.equals("beta", AppConstants.MOZ_UPDATE_CHANNEL)) {
                notifyDataPolicy(context, dataPrefs);
            } else {
                
                SharedPreferences.Editor editor = dataPrefs.edit();
                editor.putInt(PREFS_POLICY_VERSION, DATA_REPORTING_VERSION);
                editor.apply();
            }
            return;
        }

        if (currentVersion >= DATA_REPORTING_VERSION) {
            
            return;
        }
    }

    


    private static void notifyDataPolicy(Context context, SharedPreferences sharedPrefs) {
        boolean result = false;
        try {
            
            Intent prefIntent = new Intent(GeckoApp.ACTION_LAUNCH_SETTINGS);
            prefIntent.setClassName(AppConstants.ANDROID_PACKAGE_NAME, AppConstants.BROWSER_INTENT_CLASS_NAME);

            GeckoPreferences.setResourceToOpen(prefIntent, "preferences_vendor");
            prefIntent.putExtra(ALERT_NAME_DATAREPORTING_NOTIFICATION, true);

            PendingIntent contentIntent = PendingIntent.getActivity(context, 0, prefIntent, PendingIntent.FLAG_UPDATE_CURRENT);
            final Resources resources = context.getResources();

            
            String notificationTitle = resources.getString(R.string.datareporting_notification_title);
            String notificationSummary;
            if (Versions.preJB) {
                notificationSummary = resources.getString(R.string.datareporting_notification_action);
            } else {
                
                notificationSummary = resources.getString(R.string.datareporting_notification_summary);
            }
            String notificationAction = resources.getString(R.string.datareporting_notification_action);
            String notificationBigSummary = resources.getString(R.string.datareporting_notification_summary);

            
            String tickerString = resources.getString(R.string.datareporting_notification_ticker_text);
            SpannableString tickerText = new SpannableString(tickerString);
            
            tickerText.setSpan(new StyleSpan(Typeface.BOLD), 0, notificationTitle.length(), Spannable.SPAN_INCLUSIVE_EXCLUSIVE);

            Notification notification = new NotificationCompat.Builder(context)
                                        .setContentTitle(notificationTitle)
                                        .setContentText(notificationSummary)
                                        .setSmallIcon(R.drawable.ic_status_logo)
                                        .setAutoCancel(true)
                                        .setContentIntent(contentIntent)
                                        .setStyle(new NotificationCompat.BigTextStyle()
                                                                        .bigText(notificationBigSummary))
                                        .addAction(R.drawable.firefox_settings_alert, notificationAction, contentIntent)
                                        .setTicker(tickerText)
                                        .build();

            NotificationManager notificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
            int notificationID = ALERT_NAME_DATAREPORTING_NOTIFICATION.hashCode();
            notificationManager.notify(notificationID, notification);

            
            SharedPreferences.Editor editor = sharedPrefs.edit();
            long now = System.currentTimeMillis();
            editor.putLong(PREFS_POLICY_NOTIFIED_TIME, now);
            editor.putInt(PREFS_POLICY_VERSION, DATA_REPORTING_VERSION);
            editor.apply();
            result = true;
        } finally {
            
            Telemetry.sendUIEvent(TelemetryContract.Event.POLICY_NOTIFICATION_SUCCESS, result);
        }
    }
}
