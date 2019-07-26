



package org.mozilla.gecko.background.announcements;

import org.mozilla.gecko.background.BackgroundService;
import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.background.common.log.Logger;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;






public class AnnouncementsBroadcastService extends BackgroundService {
  private static final String WORKER_THREAD_NAME = "AnnouncementsBroadcastServiceWorker";
  private static final String LOG_TAG = "AnnounceBrSvc";

  public AnnouncementsBroadcastService() {
    super(WORKER_THREAD_NAME);
  }

  protected static SharedPreferences getSharedPreferences(Context context) {
    return context.getSharedPreferences(AnnouncementsConstants.PREFS_BRANCH,
        GlobalConstants.SHARED_PREFERENCES_MODE);
  }

  protected SharedPreferences getSharedPreferences() {
    return this.getSharedPreferences(AnnouncementsConstants.PREFS_BRANCH,
        GlobalConstants.SHARED_PREFERENCES_MODE);
  }

  private void toggleAlarm(final Context context, boolean enabled) {
    final Class<?> serviceClass = AnnouncementsService.class;
    Logger.info(LOG_TAG, (enabled ? "R" : "Unr") + "egistering " + serviceClass.getSimpleName() +
        ".");

    final Intent service = new Intent(context, serviceClass);
    final PendingIntent pending =  PendingIntent.getService(context, 0, service,
        PendingIntent.FLAG_CANCEL_CURRENT);

    if (!enabled) {
      cancelAlarm(pending);
      return;
    }

    final long pollInterval = getPollInterval(context);
    scheduleAlarm(pollInterval, pending);
  }

  






  public static void recordLastLaunch(final Context context) {
    final long now = System.currentTimeMillis();
    final SharedPreferences preferences = getSharedPreferences(context);

    
    
    
    
    
    
    
    
    
    
    
    
    
    long previous = preferences.getLong(AnnouncementsConstants.PREF_LAST_LAUNCH, -1);
    if (previous == -1) {
      Logger.debug(LOG_TAG, "No previous launch recorded.");
    }

    if (now < GlobalConstants.BUILD_TIMESTAMP_MSEC) {
      Logger.warn(LOG_TAG, "Current time " + now + " is older than build date " +
                           GlobalConstants.BUILD_TIMESTAMP_MSEC + ". Ignoring until clock is corrected.");
      return;
    }

    if (now > AnnouncementsConstants.LATEST_ACCEPTED_LAUNCH_TIMESTAMP_MSEC) {
      Logger.warn(LOG_TAG, "Launch time " + now + " is later than max sane launch timestamp " +
                           AnnouncementsConstants.LATEST_ACCEPTED_LAUNCH_TIMESTAMP_MSEC +
                           ". Ignoring until clock is corrected.");
      return;
    }

    if (previous > now) {
      Logger.debug(LOG_TAG, "Previous launch " + previous + " later than current time " +
                            now + ", but new time is sane. Accepting new time.");
    }

    preferences.edit().putLong(AnnouncementsConstants.PREF_LAST_LAUNCH, now).commit();
  }

  public static long getPollInterval(final Context context) {
    final SharedPreferences preferences = getSharedPreferences(context);
    return preferences.getLong(AnnouncementsConstants.PREF_ANNOUNCE_FETCH_INTERVAL_MSEC, AnnouncementsConstants.DEFAULT_ANNOUNCE_FETCH_INTERVAL_MSEC);
  }

  public static void setPollInterval(final Context context, long interval) {
    final SharedPreferences preferences = getSharedPreferences(context);
    preferences.edit().putLong(AnnouncementsConstants.PREF_ANNOUNCE_FETCH_INTERVAL_MSEC, interval).commit();
  }

  @Override
  protected void onHandleIntent(Intent intent) {
    Logger.setThreadLogTag(AnnouncementsConstants.GLOBAL_LOG_TAG);
    final String action = intent.getAction();
    Logger.debug(LOG_TAG, "Broadcast onReceive. Intent is " + action);

    if (AnnouncementsConstants.ACTION_ANNOUNCEMENTS_PREF.equals(action)) {
      handlePrefIntent(intent);
      return;
    }

    if (Intent.ACTION_BOOT_COMPLETED.equals(action) ||
        Intent.ACTION_EXTERNAL_APPLICATIONS_AVAILABLE.equals(action)) {
      BackgroundService.reflectContextToFennec(this,
          GlobalConstants.GECKO_PREFERENCES_CLASS,
          GlobalConstants.GECKO_BROADCAST_ANNOUNCEMENTS_PREF_METHOD);
      return;
    }

    
    Logger.warn(LOG_TAG, "Unknown intent " + action);
  }

  




  protected void handlePrefIntent(Intent intent) {
    if (!intent.hasExtra("enabled")) {
      Logger.warn(LOG_TAG, "Got ANNOUNCEMENTS_PREF intent without enabled. Ignoring.");
      return;
    }

    final boolean enabled = intent.getBooleanExtra("enabled", true);
    Logger.debug(LOG_TAG, intent.getStringExtra("branch") + "/" +
                          intent.getStringExtra("pref")   + " = " +
                          (intent.hasExtra("enabled") ? enabled : ""));

    toggleAlarm(this, enabled);

    
    if (!enabled) {
      Logger.info(LOG_TAG, "!enabled: clearing last fetch.");
      final SharedPreferences sharedPreferences = getSharedPreferences();
      final Editor editor = sharedPreferences.edit();
      editor.remove(AnnouncementsConstants.PREF_LAST_FETCH_LOCAL_TIME);
      editor.remove(AnnouncementsConstants.PREF_EARLIEST_NEXT_ANNOUNCE_FETCH);
      editor.commit();
    }
  }
}
