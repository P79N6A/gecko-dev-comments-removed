



package org.mozilla.gecko.background.announcements;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import org.mozilla.gecko.background.BackgroundConstants;
import org.mozilla.gecko.background.BackgroundService;
import org.mozilla.gecko.sync.GlobalConstants;
import org.mozilla.gecko.sync.Logger;

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

  private void toggleAlarm(final Context context, boolean enabled) {
    Logger.info(LOG_TAG, (enabled ? "R" : "Unr") + "egistering announcements broadcast receiver...");

    final PendingIntent pending = createPendingIntent(context, AnnouncementsStartReceiver.class);

    if (!enabled) {
      cancelAlarm(pending);
      return;
    }

    final long pollInterval = getPollInterval(context);
    scheduleAlarm(pollInterval, pending);
  }

  






  public static void recordLastLaunch(final Context context) {
    final long now = System.currentTimeMillis();
    final SharedPreferences preferences = context.getSharedPreferences(AnnouncementsConstants.PREFS_BRANCH, BackgroundConstants.SHARED_PREFERENCES_MODE);

    
    
    
    
    
    
    
    
    
    
    
    
    
    long previous = preferences.getLong(AnnouncementsConstants.PREF_LAST_LAUNCH, -1);
    if (previous == -1) {
      Logger.debug(LOG_TAG, "No previous launch recorded.");
    }

    if (now < GlobalConstants.BUILD_TIMESTAMP) {
      Logger.warn(LOG_TAG, "Current time " + now + " is older than build date " +
                           GlobalConstants.BUILD_TIMESTAMP + ". Ignoring until clock is corrected.");
      return;
    }

    if (now > AnnouncementsConstants.LATEST_ACCEPTED_LAUNCH_TIMESTAMP) {
      Logger.warn(LOG_TAG, "Launch time " + now + " is later than max sane launch timestamp " +
                           AnnouncementsConstants.LATEST_ACCEPTED_LAUNCH_TIMESTAMP +
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
    SharedPreferences preferences = context.getSharedPreferences(AnnouncementsConstants.PREFS_BRANCH, BackgroundConstants.SHARED_PREFERENCES_MODE);
    return preferences.getLong(AnnouncementsConstants.PREF_ANNOUNCE_FETCH_INTERVAL_MSEC, AnnouncementsConstants.DEFAULT_ANNOUNCE_FETCH_INTERVAL_MSEC);
  }

  public static void setPollInterval(final Context context, long interval) {
    SharedPreferences preferences = context.getSharedPreferences(AnnouncementsConstants.PREFS_BRANCH, BackgroundConstants.SHARED_PREFERENCES_MODE);
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
      handleSystemLifetimeIntent();
      return;
    }

    
    Logger.warn(LOG_TAG, "Unknown intent " + action);
  }

  











  protected void handleSystemLifetimeIntent() {
    
    try {
      Class<?> geckoPreferences = Class.forName(BackgroundConstants.GECKO_PREFERENCES_CLASS);
      Method broadcastSnippetsPref = geckoPreferences.getMethod(BackgroundConstants.GECKO_BROADCAST_METHOD, Context.class);
      broadcastSnippetsPref.invoke(null, this);
      return;
    } catch (ClassNotFoundException e) {
      Logger.error(LOG_TAG, "Class " + BackgroundConstants.GECKO_PREFERENCES_CLASS + " not found!");
      return;
    } catch (NoSuchMethodException e) {
      Logger.error(LOG_TAG, "Method " + BackgroundConstants.GECKO_PREFERENCES_CLASS + "/" + BackgroundConstants.GECKO_BROADCAST_METHOD + " not found!");
      return;
    } catch (IllegalArgumentException e) {
      Logger.error(LOG_TAG, "Got exception invoking " + BackgroundConstants.GECKO_BROADCAST_METHOD + ".");
    } catch (IllegalAccessException e) {
      Logger.error(LOG_TAG, "Got exception invoking " + BackgroundConstants.GECKO_BROADCAST_METHOD + ".");
    } catch (InvocationTargetException e) {
      Logger.error(LOG_TAG, "Got exception invoking " + BackgroundConstants.GECKO_BROADCAST_METHOD + ".");
    }
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
      final SharedPreferences sharedPreferences = this.getSharedPreferences(AnnouncementsConstants.PREFS_BRANCH,
                                                                            BackgroundConstants.SHARED_PREFERENCES_MODE);
      final Editor editor = sharedPreferences.edit();
      editor.remove(AnnouncementsConstants.PREF_LAST_FETCH_LOCAL_TIME);
      editor.remove(AnnouncementsConstants.PREF_EARLIEST_NEXT_ANNOUNCE_FETCH);
      editor.commit();
    }
  }
}
