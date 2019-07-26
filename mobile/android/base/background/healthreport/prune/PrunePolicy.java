



package org.mozilla.gecko.background.healthreport.prune;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.background.healthreport.HealthReportConstants;

import android.content.SharedPreferences;












public class PrunePolicy {
  public static final String LOG_TAG = PrunePolicy.class.getSimpleName();

  protected final PrunePolicyStorage storage;
  protected final SharedPreferences sharedPreferences;
  protected final Editor editor;

  public PrunePolicy(final PrunePolicyStorage storage, final SharedPreferences sharedPrefs) {
    this.storage = storage;
    this.sharedPreferences = sharedPrefs;
    this.editor = new Editor(this.sharedPreferences.edit());
  }

  protected SharedPreferences getSharedPreferences() {
    return this.sharedPreferences;
  }

  public void tick(final long time) {
    try {
      try {
        boolean pruned = attemptPruneBySize(time);
        pruned = attemptExpiration(time) ? true : pruned;
        
        if (pruned) {
          attemptStorageCleanup(time);
        }
      } catch (Exception e) {
        
        
        
        Logger.error(LOG_TAG, "Got exception pruning document.", e);
      } finally {
        editor.commit();
      }
    } catch (Exception e) {
      Logger.error(LOG_TAG, "Got exception committing to SharedPreferences.", e);
    } finally {
      storage.close();
    }
  }

  protected boolean attemptPruneBySize(final long time) {
    final long nextPrune = getNextPruneBySizeTime();
    if (nextPrune < 0) {
      Logger.debug(LOG_TAG, "Initializing prune-by-size time.");
      editor.setNextPruneBySizeTime(time + getMinimumTimeBetweenPruneBySizeChecks());
      return false;
    }

    
    
    if (nextPrune > getMinimumTimeBetweenPruneBySizeChecks() + time) {
      Logger.debug(LOG_TAG, "Clock skew detected - resetting prune-by-size time.");
      editor.setNextPruneBySizeTime(time + getMinimumTimeBetweenPruneBySizeChecks());
      return false;
    }

    if (nextPrune > time) {
      Logger.debug(LOG_TAG, "Skipping prune-by-size - wait period has not yet elapsed.");
      return false;
    }

    Logger.debug(LOG_TAG, "Attempting prune-by-size.");

    
    
    
    final int environmentCount = storage.getEnvironmentCount();
    if (environmentCount > getMaxEnvironmentCount()) {
      final int environmentPruneCount = environmentCount - getEnvironmentCountAfterPrune();
      Logger.debug(LOG_TAG, "Pruning " + environmentPruneCount + " environments.");
      storage.pruneEnvironments(environmentPruneCount);
    }

    final int eventCount = storage.getEventCount();
    if (eventCount > getMaxEventCount()) {
      final int eventPruneCount = eventCount - getEventCountAfterPrune();
      Logger.debug(LOG_TAG, "Pruning up to " + eventPruneCount + " events.");
      storage.pruneEvents(eventPruneCount);
    }
    editor.setNextPruneBySizeTime(time + getMinimumTimeBetweenPruneBySizeChecks());
    return true;
  }

  protected boolean attemptExpiration(final long time) {
    final long nextPrune = getNextExpirationTime();
    if (nextPrune < 0) {
      Logger.debug(LOG_TAG, "Initializing expiration time.");
      editor.setNextExpirationTime(time + getMinimumTimeBetweenExpirationChecks());
      return false;
    }

    
    
    if (nextPrune > getMinimumTimeBetweenExpirationChecks() + time) {
      Logger.debug(LOG_TAG, "Clock skew detected - resetting expiration time.");
      editor.setNextExpirationTime(time + getMinimumTimeBetweenExpirationChecks());
      return false;
    }

    if (nextPrune > time) {
      Logger.debug(LOG_TAG, "Skipping expiration - wait period has not yet elapsed.");
      return false;
    }

    final long oldEventTime = time - getEventExistenceDuration();
    Logger.debug(LOG_TAG, "Pruning data older than " + oldEventTime + ".");
    storage.deleteDataBefore(oldEventTime);
    editor.setNextExpirationTime(time + getMinimumTimeBetweenExpirationChecks());
    return true;
  }

  protected boolean attemptStorageCleanup(final long time) {
    
    final long nextCleanup = getNextCleanupTime();
    if (nextCleanup < 0) {
      Logger.debug(LOG_TAG, "Initializing cleanup time.");
      editor.setNextCleanupTime(time + getMinimumTimeBetweenCleanupChecks());
      return false;
    }

    
    
    if (nextCleanup > getMinimumTimeBetweenCleanupChecks() + time) {
      Logger.debug(LOG_TAG, "Clock skew detected - resetting cleanup time.");
      editor.setNextCleanupTime(time + getMinimumTimeBetweenCleanupChecks());
      return false;
    }

    if (nextCleanup > time) {
      Logger.debug(LOG_TAG, "Skipping cleanup - wait period has not yet elapsed.");
      return false;
    }

    editor.setNextCleanupTime(time + getMinimumTimeBetweenCleanupChecks());
    Logger.debug(LOG_TAG, "Cleaning up storage.");
    storage.cleanup();
    return true;
  }

  protected static class Editor {
    protected final SharedPreferences.Editor editor;

    public Editor(final SharedPreferences.Editor editor) {
      this.editor = editor;
    }

    public void commit() {
      editor.commit();
    }

    public Editor setNextExpirationTime(final long time) {
      editor.putLong(HealthReportConstants.PREF_EXPIRATION_TIME, time);
      return this;
    }

    public Editor setNextPruneBySizeTime(final long time) {
      editor.putLong(HealthReportConstants.PREF_PRUNE_BY_SIZE_TIME, time);
      return this;
    }

    public Editor setNextCleanupTime(final long time) {
      editor.putLong(HealthReportConstants.PREF_CLEANUP_TIME, time);
      return this;
    }
  }

  private long getNextExpirationTime() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_EXPIRATION_TIME, -1L);
  }

  private long getEventExistenceDuration() {
    return HealthReportConstants.EVENT_EXISTENCE_DURATION;
  }

  private long getMinimumTimeBetweenExpirationChecks() {
    return HealthReportConstants.MINIMUM_TIME_BETWEEN_EXPIRATION_CHECKS_MILLIS;
  }

  private long getNextPruneBySizeTime() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_PRUNE_BY_SIZE_TIME, -1L);
  }

  private long getMinimumTimeBetweenPruneBySizeChecks() {
    return HealthReportConstants.MINIMUM_TIME_BETWEEN_PRUNE_BY_SIZE_CHECKS_MILLIS;
  }

  private int getMaxEnvironmentCount() {
    return HealthReportConstants.MAX_ENVIRONMENT_COUNT;
  }

  private int getEnvironmentCountAfterPrune() {
    return HealthReportConstants.ENVIRONMENT_COUNT_AFTER_PRUNE;
  }

  private int getMaxEventCount() {
    return HealthReportConstants.MAX_EVENT_COUNT;
  }

  private int getEventCountAfterPrune() {
    return HealthReportConstants.EVENT_COUNT_AFTER_PRUNE;
  }

  private long getNextCleanupTime() {
    return getSharedPreferences().getLong(HealthReportConstants.PREF_CLEANUP_TIME, -1L);
  }

  private long getMinimumTimeBetweenCleanupChecks() {
    return HealthReportConstants.MINIMUM_TIME_BETWEEN_CLEANUP_CHECKS_MILLIS;
  }
}
