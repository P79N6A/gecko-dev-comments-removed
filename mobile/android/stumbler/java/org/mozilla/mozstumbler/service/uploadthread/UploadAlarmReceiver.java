



package org.mozilla.mozstumbler.service.uploadthread;

import android.app.AlarmManager;
import android.app.IntentService;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import org.mozilla.mozstumbler.service.AppGlobals;
import org.mozilla.mozstumbler.service.Prefs;
import org.mozilla.mozstumbler.service.stumblerthread.datahandling.DataStorageManager;
import org.mozilla.mozstumbler.service.utils.NetworkUtils;













public class UploadAlarmReceiver extends BroadcastReceiver {
    private static final String LOG_TAG = AppGlobals.makeLogTag(UploadAlarmReceiver.class.getSimpleName());
    private static final String EXTRA_IS_REPEATING = "is_repeating";
    private static boolean sIsAlreadyScheduled;

    public UploadAlarmReceiver() {}

    public static class UploadAlarmService extends IntentService {

        public UploadAlarmService(String name) {
            super(name);
            
            setIntentRedelivery(false);
        }

        public UploadAlarmService() {
            this(LOG_TAG);
        }

        @Override
        protected void onHandleIntent(Intent intent) {
            if (intent == null) {
                return;
            }
            boolean isRepeating = intent.getBooleanExtra(EXTRA_IS_REPEATING, true);
            if (DataStorageManager.getInstance() == null) {
                DataStorageManager.createGlobalInstance(this, null);
            }
            upload(isRepeating);
        }

        void upload(boolean isRepeating) {
            if (!isRepeating) {
                sIsAlreadyScheduled = false;
            }

            
            long oldestMs = DataStorageManager.getInstance().getOldestBatchTimeMs();
            int maxWeeks = DataStorageManager.getInstance().getMaxWeeksStored();
            if (oldestMs > 0) {
                long currentTime = System.currentTimeMillis();
                long msPerWeek = 604800 * 1000;
                if (currentTime - oldestMs > maxWeeks * msPerWeek) {
                    DataStorageManager.getInstance().deleteAll();
                    UploadAlarmReceiver.cancelAlarm(this, isRepeating);
                    return;
                }
            }

            NetworkUtils networkUtils = new NetworkUtils(this);
            if (networkUtils.isWifiAvailable() &&
                !AsyncUploader.isUploading()) {
                Log.d(LOG_TAG, "Alarm upload(), call AsyncUploader");
                AsyncUploader.AsyncUploadArgs settings =
                    new AsyncUploader.AsyncUploadArgs(networkUtils,
                            Prefs.getInstance(this).getWifiScanAlways(),
                            Prefs.getInstance(this).getUseWifiOnly());
                AsyncUploader uploader = new AsyncUploader(settings, null);
                uploader.setNickname(Prefs.getInstance(this).getNickname());
                uploader.execute();
                
            }
        }
    }

    static PendingIntent createIntent(Context c, boolean isRepeating) {
        Intent intent = new Intent(c, UploadAlarmReceiver.class);
        intent.putExtra(EXTRA_IS_REPEATING, isRepeating);
        PendingIntent pi = PendingIntent.getBroadcast(c, 0, intent, 0);
        return pi;
    }

    public static void cancelAlarm(Context c, boolean isRepeating) {
        Log.d(LOG_TAG, "cancelAlarm");
        
        sIsAlreadyScheduled = false;
        AlarmManager alarmManager = (AlarmManager) c.getSystemService(Context.ALARM_SERVICE);
        PendingIntent pi = createIntent(c, isRepeating);
        alarmManager.cancel(pi);
    }

    public static void scheduleAlarm(Context c, long secondsToWait, boolean isRepeating) {
        if (sIsAlreadyScheduled) {
            return;
        }

        long intervalMsec = secondsToWait * 1000;
        Log.d(LOG_TAG, "schedule alarm (ms):" + intervalMsec);

        sIsAlreadyScheduled = true;
        AlarmManager alarmManager = (AlarmManager) c.getSystemService(Context.ALARM_SERVICE);
        PendingIntent pi = createIntent(c, isRepeating);

        long triggerAtMs = System.currentTimeMillis() + intervalMsec;
        if (isRepeating) {
            alarmManager.setInexactRepeating(AlarmManager.RTC, triggerAtMs, intervalMsec, pi);
        } else {
            alarmManager.set(AlarmManager.RTC, triggerAtMs, pi);
        }
    }

    @Override
    public void onReceive(final Context context, Intent intent) {
        Intent startServiceIntent = new Intent(context, UploadAlarmService.class);
        context.startService(startServiceIntent);
    }
}
