



package org.mozilla.gecko.background.healthreport.upload;

import org.mozilla.gecko.background.BackgroundService;
import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.healthreport.HealthReportConstants;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;






public class HealthReportBroadcastService extends BackgroundService {
  public static final String LOG_TAG = HealthReportBroadcastService.class.getSimpleName();
  public static final String WORKER_THREAD_NAME = LOG_TAG + "Worker";

  public HealthReportBroadcastService() {
    super(WORKER_THREAD_NAME);
  }

  protected SharedPreferences getSharedPreferences() {
    return this.getSharedPreferences(HealthReportConstants.PREFS_BRANCH, GlobalConstants.SHARED_PREFERENCES_MODE);
  }

  public long getPollInterval() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_SUBMISSION_INTENT_INTERVAL_MSEC, HealthReportConstants.DEFAULT_SUBMISSION_INTENT_INTERVAL_MSEC);
  }

  public void setPollInterval(long interval) {
    getSharedPreferences().edit().putLong(HealthReportConstants.PREF_SUBMISSION_INTENT_INTERVAL_MSEC, interval).commit();
  }

  
















  protected void toggleAlarm(final Context context, String profileName, String profilePath, boolean enabled, boolean serviceEnabled) {
    Logger.info(LOG_TAG, (serviceEnabled ? "R" : "Unr") + "egistering health report start broadcast receiver.");

    
    
    
    
    final Intent service = new Intent(context, HealthReportUploadStartReceiver.class);
    service.setAction("upload"); 
    service.putExtra("uploadEnabled", enabled);
    service.putExtra("profileName", profileName);
    service.putExtra("profilePath", profilePath);
    final PendingIntent pending = PendingIntent.getBroadcast(context, 0, service, PendingIntent.FLAG_CANCEL_CURRENT);

    if (!serviceEnabled) {
      cancelAlarm(pending);
      return;
    }

    final long pollInterval = getPollInterval();
    scheduleAlarm(pollInterval, pending);
  }

  @Override
  protected void onHandleIntent(Intent intent) {
    Logger.setThreadLogTag(HealthReportConstants.GLOBAL_LOG_TAG);

    if (HealthReportConstants.UPLOAD_FEATURE_DISABLED) {
      Logger.debug(LOG_TAG, "Health report upload feature is compile-time disabled; not handling intent.");
      return;
    }

    final String action = intent.getAction();
    Logger.debug(LOG_TAG, "Health report upload feature is compile-time enabled; handling intent with action " + action + ".");

    if (HealthReportConstants.ACTION_HEALTHREPORT_UPLOAD_PREF.equals(action)) {
      handlePrefIntent(intent);
      return;
    }

    if (Intent.ACTION_BOOT_COMPLETED.equals(action) ||
        Intent.ACTION_EXTERNAL_APPLICATIONS_AVAILABLE.equals(action)) {
      BackgroundService.reflectContextToFennec(this,
          GlobalConstants.GECKO_PREFERENCES_CLASS,
          GlobalConstants.GECKO_BROADCAST_HEALTHREPORT_UPLOAD_PREF_METHOD);
      return;
    }

    
    Logger.warn(LOG_TAG, "Unknown intent " + action + ".");
  }

  




  protected void handlePrefIntent(Intent intent) {
    if (!intent.hasExtra("enabled")) {
      Logger.warn(LOG_TAG, "Got " + HealthReportConstants.ACTION_HEALTHREPORT_UPLOAD_PREF + " intent without enabled. Ignoring.");
      return;
    }

    final boolean enabled = intent.getBooleanExtra("enabled", true);
    Logger.debug(LOG_TAG, intent.getStringExtra("branch") + "/" +
                          intent.getStringExtra("pref")   + " = " +
                          (intent.hasExtra("enabled") ? enabled : ""));

    String profileName = intent.getStringExtra("profileName");
    String profilePath = intent.getStringExtra("profilePath");

    if (profileName == null || profilePath == null) {
      Logger.warn(LOG_TAG, "Got " + HealthReportConstants.ACTION_HEALTHREPORT_UPLOAD_PREF + " intent without profilePath or profileName. Ignoring.");
      return;
    }

    Logger.pii(LOG_TAG, "Updating health report alarm for profile " + profileName + " at " + profilePath + ".");

    final SharedPreferences sharedPrefs = getSharedPreferences();
    final ObsoleteDocumentTracker tracker = new ObsoleteDocumentTracker(sharedPrefs);
    final boolean hasObsoleteIds = tracker.hasObsoleteIds();

    if (!enabled) {
      final Editor editor = sharedPrefs.edit();
      editor.remove(HealthReportConstants.PREF_LAST_UPLOAD_DOCUMENT_ID);

      if (hasObsoleteIds) {
        Logger.debug(LOG_TAG, "Health report upload disabled; scheduling deletion of " + tracker.numberOfObsoleteIds() + " documents.");
        tracker.limitObsoleteIds();
      } else {
        
        Logger.debug(LOG_TAG, "Health report upload disabled and no deletes to schedule: clearing prefs.");
        editor.remove(HealthReportConstants.PREF_FIRST_RUN);
        editor.remove(HealthReportConstants.PREF_NEXT_SUBMISSION);
      }

      editor.commit();
    }

    
    
    final boolean serviceEnabled = hasObsoleteIds || enabled;
    toggleAlarm(this, profileName, profilePath, enabled, serviceEnabled);
  }
}
