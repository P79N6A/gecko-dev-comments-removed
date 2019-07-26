



package org.mozilla.gecko.background.healthreport;

import org.mozilla.gecko.background.BackgroundService;
import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.healthreport.prune.HealthReportPruneService;
import org.mozilla.gecko.background.healthreport.upload.HealthReportUploadService;
import org.mozilla.gecko.background.healthreport.upload.ObsoleteDocumentTracker;

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

  public long getSubmissionPollInterval() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_SUBMISSION_INTENT_INTERVAL_MSEC, HealthReportConstants.DEFAULT_SUBMISSION_INTENT_INTERVAL_MSEC);
  }

  public void setSubmissionPollInterval(final long interval) {
    getSharedPreferences().edit().putLong(HealthReportConstants.PREF_SUBMISSION_INTENT_INTERVAL_MSEC, interval).commit();
  }

  public long getPrunePollInterval() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_PRUNE_INTENT_INTERVAL_MSEC,
        HealthReportConstants.DEFAULT_PRUNE_INTENT_INTERVAL_MSEC);
  }

  public void setPrunePollInterval(final long interval) {
    getSharedPreferences().edit().putLong(HealthReportConstants.PREF_PRUNE_INTENT_INTERVAL_MSEC,
        interval).commit();
  }

  
















  protected void toggleSubmissionAlarm(final Context context, String profileName, String profilePath,
      boolean enabled, boolean serviceEnabled) {
    final Class<?> serviceClass = HealthReportUploadService.class;
    Logger.info(LOG_TAG, (serviceEnabled ? "R" : "Unr") + "egistering " +
        serviceClass.getSimpleName() + ".");

    
    
    
    
    final Intent service = new Intent(context, serviceClass);
    service.setAction("upload"); 
    service.putExtra("uploadEnabled", enabled);
    service.putExtra("profileName", profileName);
    service.putExtra("profilePath", profilePath);
    final PendingIntent pending = PendingIntent.getService(context, 0, service, PendingIntent.FLAG_CANCEL_CURRENT);

    if (!serviceEnabled) {
      cancelAlarm(pending);
      return;
    }

    final long pollInterval = getSubmissionPollInterval();
    scheduleAlarm(pollInterval, pending);
  }

  @Override
  protected void onHandleIntent(Intent intent) {
    Logger.setThreadLogTag(HealthReportConstants.GLOBAL_LOG_TAG);

    
    boolean handled = attemptHandleIntentForUpload(intent);
    handled = attemptHandleIntentForPrune(intent) ? true : handled;

    if (!handled) {
      Logger.warn(LOG_TAG, "Unhandled intent with action " + intent.getAction() + ".");
    }
  }

  


  protected boolean attemptHandleIntentForUpload(final Intent intent) {
    if (HealthReportConstants.UPLOAD_FEATURE_DISABLED) {
      Logger.debug(LOG_TAG, "Health report upload feature is compile-time disabled; not handling intent.");
      return false;
    }

    final String action = intent.getAction();
    Logger.debug(LOG_TAG, "Health report upload feature is compile-time enabled; handling intent with action " + action + ".");

    if (HealthReportConstants.ACTION_HEALTHREPORT_UPLOAD_PREF.equals(action)) {
      handleUploadPrefIntent(intent);
      return true;
    }

    if (Intent.ACTION_BOOT_COMPLETED.equals(action) ||
        Intent.ACTION_EXTERNAL_APPLICATIONS_AVAILABLE.equals(action)) {
      BackgroundService.reflectContextToFennec(this,
          GlobalConstants.GECKO_PREFERENCES_CLASS,
          GlobalConstants.GECKO_BROADCAST_HEALTHREPORT_UPLOAD_PREF_METHOD);
      return true;
    }

    return false;
  }

  




  protected void handleUploadPrefIntent(Intent intent) {
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

    Logger.pii(LOG_TAG, "Updating health report upload alarm for profile " + profileName + " at " +
        profilePath + ".");

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
    toggleSubmissionAlarm(this, profileName, profilePath, enabled, serviceEnabled);
  }

  


  protected boolean attemptHandleIntentForPrune(final Intent intent) {
    final String action = intent.getAction();
    Logger.debug(LOG_TAG, "Prune: Handling intent with action, " + action + ".");

    if (HealthReportConstants.ACTION_HEALTHREPORT_PRUNE.equals(action)) {
      handlePruneIntent(intent);
      return true;
    }

    if (Intent.ACTION_BOOT_COMPLETED.equals(action) ||
        Intent.ACTION_EXTERNAL_APPLICATIONS_AVAILABLE.equals(action)) {
      BackgroundService.reflectContextToFennec(this,
          GlobalConstants.GECKO_PREFERENCES_CLASS,
          GlobalConstants.GECKO_BROADCAST_HEALTHREPORT_PRUNE_METHOD);
      return true;
    }

    return false;
  }

  protected void handlePruneIntent(final Intent intent) {
    final String profileName = intent.getStringExtra("profileName");
    final String profilePath = intent.getStringExtra("profilePath");

    if (profileName == null || profilePath == null) {
      Logger.warn(LOG_TAG, "Got " + HealthReportConstants.ACTION_HEALTHREPORT_PRUNE + " intent " +
          "without profilePath or profileName. Ignoring.");
      return;
    }

    final Class<?> serviceClass = HealthReportPruneService.class;
    final Intent service = new Intent(this, serviceClass);
    service.setAction("prune"); 
    service.putExtra("profileName", profileName);
    service.putExtra("profilePath", profilePath);
    final PendingIntent pending = PendingIntent.getService(this, 0, service,
        PendingIntent.FLAG_CANCEL_CURRENT);

    
    
    
    
    
    Logger.info(LOG_TAG, "Registering " + serviceClass.getSimpleName() + ".");
    final long pollInterval = getPrunePollInterval();
    scheduleAlarm(pollInterval, pending);
  }
}
