



package org.mozilla.gecko.background.healthreport.upload;

import org.mozilla.gecko.background.BackgroundService;
import org.mozilla.gecko.background.common.GlobalConstants;
import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.healthreport.HealthReportConstants;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.IBinder;










public class HealthReportUploadService extends BackgroundService {
  public static final String LOG_TAG = HealthReportUploadService.class.getSimpleName();
  public static final String WORKER_THREAD_NAME = LOG_TAG + "Worker";

  public HealthReportUploadService() {
    super(WORKER_THREAD_NAME);
  }

  @Override
  public IBinder onBind(Intent intent) {
    return null;
  }

  protected SharedPreferences getSharedPreferences() {
    return this.getSharedPreferences(HealthReportConstants.PREFS_BRANCH, GlobalConstants.SHARED_PREFERENCES_MODE);
  }

  @Override
  public void onHandleIntent(Intent intent) {
    Logger.setThreadLogTag(HealthReportConstants.GLOBAL_LOG_TAG);

    
    if (intent == null) {
      Logger.debug(LOG_TAG, "Short-circuiting on null intent.");
      return;
    }

    if (HealthReportConstants.UPLOAD_FEATURE_DISABLED) {
      Logger.debug(LOG_TAG, "Health report upload feature is compile-time disabled; not handling upload intent.");
      return;
    }

    Logger.debug(LOG_TAG, "Health report upload feature is compile-time enabled; handling upload intent.");

    String profileName = intent.getStringExtra("profileName");
    String profilePath = intent.getStringExtra("profilePath");

    if (profileName == null || profilePath == null) {
      Logger.warn(LOG_TAG, "Got intent without profilePath or profileName. Ignoring.");
      return;
    }

    if (!intent.hasExtra("uploadEnabled")) {
      Logger.warn(LOG_TAG, "Got intent without uploadEnabled. Ignoring.");
      return;
    }
    boolean uploadEnabled = intent.getBooleanExtra("uploadEnabled", false);

    
    if (!backgroundDataIsEnabled()) {
      Logger.debug(LOG_TAG, "Background data is not enabled; skipping.");
      return;
    }

    Logger.pii(LOG_TAG, "Ticking policy for profile " + profileName + " at " + profilePath + ".");

    final SharedPreferences sharedPrefs = getSharedPreferences();
    final ObsoleteDocumentTracker tracker = new ObsoleteDocumentTracker(sharedPrefs);
    SubmissionClient client = new AndroidSubmissionClient(this, sharedPrefs, profilePath);
    SubmissionPolicy policy = new SubmissionPolicy(sharedPrefs, client, tracker, uploadEnabled);

    final long now = System.currentTimeMillis();
    policy.tick(now);
  }
}
