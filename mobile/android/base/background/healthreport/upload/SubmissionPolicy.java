



package org.mozilla.gecko.background.healthreport.upload;

import java.net.MalformedURLException;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.Collection;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.healthreport.HealthReportConstants;
import org.mozilla.gecko.background.healthreport.HealthReportUtils;
import org.mozilla.gecko.background.healthreport.upload.SubmissionClient.Delegate;

import android.content.SharedPreferences;




























public class SubmissionPolicy {
  public static final String LOG_TAG = SubmissionPolicy.class.getSimpleName();

  protected final SharedPreferences sharedPreferences;
  protected final SubmissionClient client;
  protected final boolean uploadEnabled;
  protected final ObsoleteDocumentTracker tracker;

  public SubmissionPolicy(final SharedPreferences sharedPreferences,
      final SubmissionClient client,
      final ObsoleteDocumentTracker tracker,
      boolean uploadEnabled) {
    if (sharedPreferences == null) {
      throw new IllegalArgumentException("sharedPreferences must not be null");
    }
    this.sharedPreferences = sharedPreferences;
    this.client = client;
    this.tracker = tracker;
    this.uploadEnabled = uploadEnabled;
  }

  






  public boolean tick(final long localTime) {
    final long nextUpload = getNextSubmission();

    
    
    
    
    
    if (nextUpload >= localTime + 3 * getMinimumTimeBetweenUploads()) {
      Logger.warn(LOG_TAG, "Next upload scheduled far in the future; system clock reset? " + nextUpload + " > " + localTime);
      
      editor()
        .setNextSubmission(localTime + getMinimumTimeBetweenUploads())
        .commit();
      return false;
    }

    
    if (localTime < nextUpload) {
      Logger.debug(LOG_TAG, "We uploaded less than an interval ago; skipping. " + nextUpload + " > " + localTime);
      return false;
    }

    if (!uploadEnabled) {
      
      
      
      
      
      
      final String obsoleteId = tracker.getNextObsoleteId();
      if (obsoleteId == null) {
        return false;
      }

      Editor editor = editor();
      editor.setLastDeleteRequested(localTime); 
      client.delete(localTime, obsoleteId, new DeleteDelegate(editor));
      return true;
    }

    long firstRun = getFirstRunLocalTime();
    if (firstRun < 0) {
      firstRun = localTime;
      
      editor()
        .setFirstRunLocalTime(firstRun)
        .setNextSubmission(localTime + getMinimumTimeBeforeFirstSubmission())
        .setCurrentDayFailureCount(0)
        .commit();
    }

    if (localTime < firstRun + getMinimumTimeBeforeFirstSubmission()) {
      Logger.info(LOG_TAG, "Need to wait " + getMinimumTimeBeforeFirstSubmission() + " before first upload.");
      return false;
    }

    String id = HealthReportUtils.generateDocumentId();
    Collection<String> oldIds = tracker.getBatchOfObsoleteIds();
    tracker.addObsoleteId(id);

    Editor editor = editor();
    editor.setLastUploadRequested(localTime); 
    client.upload(localTime, id, oldIds, new UploadDelegate(editor, oldIds));
    return true;
  }

  







  protected boolean isLocalException(Exception e) {
    return (e instanceof MalformedURLException) ||
           (e instanceof SocketException) ||
           (e instanceof UnknownHostException);
  }

  protected class UploadDelegate implements Delegate {
    protected final Editor editor;
    protected final Collection<String> oldIds;

    public UploadDelegate(Editor editor, Collection<String> oldIds) {
      this.editor = editor;
      this.oldIds = oldIds;
    }

    @Override
    public void onSuccess(long localTime, String id) {
      long next = localTime + getMinimumTimeBetweenUploads();
      tracker.markIdAsUploaded(id);
      tracker.purgeObsoleteIds(oldIds);
      editor
        .setNextSubmission(next)
        .setLastUploadSucceeded(localTime)
        .setCurrentDayFailureCount(0)
        .commit();
      if (Logger.LOG_PERSONAL_INFORMATION) {
        Logger.pii(LOG_TAG, "Successful upload with id " + id + " obsoleting "
            + oldIds.size() + " old records reported at " + localTime + "; next upload at " + next + ".");
      } else {
        Logger.info(LOG_TAG, "Successful upload obsoleting " + oldIds.size()
            + " old records reported at " + localTime + "; next upload at " + next + ".");
      }
    }

    @Override
    public void onHardFailure(long localTime, String id, String reason, Exception e) {
      long next = localTime + getMinimumTimeBetweenUploads();
      if (isLocalException(e)) {
        Logger.info(LOG_TAG, "Hard failure caused by local exception; not tracking id and not decrementing attempts.");
        tracker.removeObsoleteId(id);
      } else {
        tracker.decrementObsoleteIdAttempts(oldIds);
      }
      editor
        .setNextSubmission(next)
        .setLastUploadFailed(localTime)
        .setCurrentDayFailureCount(0)
        .commit();
      Logger.warn(LOG_TAG, "Hard failure reported at " + localTime + ": " + reason + " Next upload at " + next + ".", e);
    }

    @Override
    public void onSoftFailure(long localTime, String id, String reason, Exception e) {
      int failuresToday = getCurrentDayFailureCount();
      Logger.warn(LOG_TAG, "Soft failure reported at " + localTime + ": " + reason + " Previously failed " + failuresToday + " time(s) today.");

      if (failuresToday >= getMaximumFailuresPerDay()) {
        onHardFailure(localTime, id, "Reached the limit of daily upload attempts: " + failuresToday, e);
        return;
      }

      long next = localTime + getMinimumTimeAfterFailure();
      if (isLocalException(e)) {
        Logger.info(LOG_TAG, "Soft failure caused by local exception; not tracking id and not decrementing attempts.");
        tracker.removeObsoleteId(id);
      } else {
        tracker.decrementObsoleteIdAttempts(oldIds);
      }
      editor
        .setNextSubmission(next)
        .setLastUploadFailed(localTime)
        .setCurrentDayFailureCount(failuresToday + 1)
        .commit();
      Logger.info(LOG_TAG, "Retrying upload at " + next + ".");
    }
  }

  protected class DeleteDelegate implements Delegate {
    protected final Editor editor;

    public DeleteDelegate(Editor editor) {
      this.editor = editor;
    }

    @Override
    public void onSoftFailure(final long localTime, String id, String reason, Exception e) {
      long next = localTime + getMinimumTimeBetweenDeletes();
      if (isLocalException(e)) {
        Logger.info(LOG_TAG, "Soft failure caused by local exception; not decrementing attempts.");
      } else {
        tracker.decrementObsoleteIdAttempts(id);
      }
      editor
        .setNextSubmission(next)
        .setLastDeleteFailed(localTime)
        .commit();

      if (Logger.LOG_PERSONAL_INFORMATION) {
        Logger.info(LOG_TAG, "Got soft failure at " + localTime + " deleting obsolete document with id " + id + ": " + reason + " Trying again later.");
      } else {
        Logger.info(LOG_TAG, "Got soft failure at " + localTime + " deleting obsolete document: " + reason + " Trying again later.");
      }
    }

    @Override
    public void onHardFailure(final long localTime, String id, String reason, Exception e) {
      
      long next = localTime + getMinimumTimeBetweenDeletes();
      tracker.removeObsoleteId(id);
      editor
        .setNextSubmission(next)
        .setLastDeleteFailed(localTime)
        .commit();

      if (Logger.LOG_PERSONAL_INFORMATION) {
        Logger.warn(LOG_TAG, "Got hard failure at " + localTime + " deleting obsolete document with id " + id + ": " + reason + " Abandoning delete request.", e);
      } else {
        Logger.warn(LOG_TAG, "Got hard failure at " + localTime + " deleting obsolete document: " + reason + " Abandoning delete request.", e);
      }
    }

    @Override
    public void onSuccess(final long localTime, String id) {
      long next = localTime + getMinimumTimeBetweenDeletes();
      tracker.removeObsoleteId(id);
      editor
        .setNextSubmission(next)
        .setLastDeleteSucceeded(localTime)
        .commit();

      if (Logger.LOG_PERSONAL_INFORMATION) {
        Logger.pii(LOG_TAG, "Deleted an obsolete document with id " + id + " at " + localTime + ".");
      } else {
        Logger.info(LOG_TAG, "Deleted an obsolete document at " + localTime + ".");
      }
    }
  }

  public SharedPreferences getSharedPreferences() {
    return this.sharedPreferences;
  }

  public long getMinimumTimeBetweenUploads() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_MINIMUM_TIME_BETWEEN_UPLOADS, HealthReportConstants.DEFAULT_MINIMUM_TIME_BETWEEN_UPLOADS);
  }

  public long getMinimumTimeBeforeFirstSubmission() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_MINIMUM_TIME_BEFORE_FIRST_SUBMISSION, HealthReportConstants.DEFAULT_MINIMUM_TIME_BEFORE_FIRST_SUBMISSION);
  }

  public long getMinimumTimeAfterFailure() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_MINIMUM_TIME_AFTER_FAILURE, HealthReportConstants.DEFAULT_MINIMUM_TIME_AFTER_FAILURE);
  }

  public long getMaximumFailuresPerDay() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_MAXIMUM_FAILURES_PER_DAY, HealthReportConstants.DEFAULT_MAXIMUM_FAILURES_PER_DAY);
  }

  
  public long getFirstRunLocalTime() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_FIRST_RUN, -1);
  }

  
  public long getNextSubmission() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_NEXT_SUBMISSION, -1);
  }

  
  public int getCurrentDayFailureCount() {
    return getSharedPreferences().getInt(HealthReportConstants.PREF_CURRENT_DAY_FAILURE_COUNT, 0);
  }

  



  protected Editor editor() {
    return new Editor(getSharedPreferences().edit());
  }

  protected static class Editor {
    protected final SharedPreferences.Editor editor;

    public Editor(SharedPreferences.Editor editor) {
      this.editor = editor;
    }

    public void commit() {
      editor.commit();
    }

    
    public Editor setFirstRunLocalTime(long localTime) {
      editor.putLong(HealthReportConstants.PREF_FIRST_RUN, localTime);
      return this;
    }

    
    public Editor setNextSubmission(long localTime) {
      editor.putLong(HealthReportConstants.PREF_NEXT_SUBMISSION, localTime);
      return this;
    }

    
    public Editor setCurrentDayFailureCount(int failureCount) {
      editor.putInt(HealthReportConstants.PREF_CURRENT_DAY_FAILURE_COUNT, failureCount);
      return this;
    }

    
    public Editor setLastUploadRequested(long localTime) {
      editor.putLong(HealthReportConstants.PREF_LAST_UPLOAD_REQUESTED, localTime);
      return this;
    }

    
    public Editor setLastUploadSucceeded(long localTime) {
      editor.putLong(HealthReportConstants.PREF_LAST_UPLOAD_SUCCEEDED, localTime);
      return this;
    }

    
    public Editor setLastUploadFailed(long localTime) {
      editor.putLong(HealthReportConstants.PREF_LAST_UPLOAD_FAILED, localTime);
      return this;
    }

    
    public Editor setLastDeleteRequested(long localTime) {
      editor.putLong(HealthReportConstants.PREF_LAST_DELETE_REQUESTED, localTime);
      return this;
    }

    
    public Editor setLastDeleteSucceeded(long localTime) {
      editor.putLong(HealthReportConstants.PREF_LAST_DELETE_SUCCEEDED, localTime);
      return this;
    }

    
    public Editor setLastDeleteFailed(long localTime) {
      editor.putLong(HealthReportConstants.PREF_LAST_DELETE_FAILED, localTime);
      return this;
    }
  }

  
  public long getLastUploadRequested() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_LAST_UPLOAD_REQUESTED, -1);
  }

  
  public long getLastUploadSucceeded() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_LAST_UPLOAD_SUCCEEDED, -1);
  }

  
  public long getLastUploadFailed() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_LAST_UPLOAD_FAILED, -1);
  }

  
  public long getLastDeleteRequested() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_LAST_DELETE_REQUESTED, -1);
  }

  
  public long getLastDeleteSucceeded() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_LAST_DELETE_SUCCEEDED, -1);
  }

  
  public long getLastDeleteFailed() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_LAST_DELETE_FAILED, -1);
  }

  public long getMinimumTimeBetweenDeletes() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_MINIMUM_TIME_BETWEEN_DELETES, HealthReportConstants.DEFAULT_MINIMUM_TIME_BETWEEN_DELETES);
  }
}
