



package org.mozilla.gecko.background.announcements;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.net.URI;
import java.util.List;
import java.util.Locale;

import org.mozilla.gecko.background.BackgroundConstants;
import org.mozilla.gecko.sync.Logger;

import android.app.IntentService;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.os.IBinder;































public class AnnouncementsService extends IntentService implements AnnouncementsFetchDelegate {
  private static final String WORKER_THREAD_NAME = "AnnouncementsServiceWorker";
  private static final String LOG_TAG = "AnnounceService";

  public AnnouncementsService() {
    super(WORKER_THREAD_NAME);
    Logger.setThreadLogTag(AnnouncementsConstants.GLOBAL_LOG_TAG);
    Logger.debug(LOG_TAG, "Creating AnnouncementsService.");
  }

  public boolean shouldFetchAnnouncements() {
    final long now = System.currentTimeMillis();

    if (!backgroundDataIsEnabled()) {
      Logger.debug(LOG_TAG, "Background data not possible. Skipping.");
      return false;
    }

    
    if (getEarliestNextFetch() > now) {
      return false;
    }

    
    final long lastFetch = getLastFetch();

    
    
    if ((now - lastFetch) < AnnouncementsConstants.MINIMUM_FETCH_INTERVAL_MSEC) {
      Logger.debug(LOG_TAG, "Returning: minimum fetch interval of " + AnnouncementsConstants.MINIMUM_FETCH_INTERVAL_MSEC + "ms not met.");
      return false;
    }

    return true;
  }

  


  protected void processAnnouncements(final List<Announcement> announcements) {
    if (announcements == null) {
      Logger.warn(LOG_TAG, "No announcements to present.");
      return;
    }

    boolean presented = false;
    for (Announcement an : announcements) {
      
      if (presented) {
        Logger.warn(LOG_TAG, "Skipping announcement \"" + an.getTitle() + "\": one already shown.");
        continue;
      }
      if (Announcement.isValidAnnouncement(an)) {
        presented = true;
        AnnouncementPresenter.displayAnnouncement(this, an);
      }
    }
  }

  





  @Override
  public void onHandleIntent(Intent intent) {
    Logger.setThreadLogTag(AnnouncementsConstants.GLOBAL_LOG_TAG);
    Logger.debug(LOG_TAG, "Running AnnouncementsService.");

    if (!shouldFetchAnnouncements()) {
      Logger.debug(LOG_TAG, "Not fetching.");
      return;
    }

    
    AnnouncementsFetcher.fetchAndProcessAnnouncements(getLastLaunch(), this);
  }

  @Override
  public void onDestroy() {
    super.onDestroy();
  }

  @Override
  public IBinder onBind(Intent intent) {
    return null;
  }

  



  protected boolean backgroundDataIsEnabled() {
    ConnectivityManager connectivity = (ConnectivityManager) this.getSystemService(Context.CONNECTIVITY_SERVICE);
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
      return connectivity.getBackgroundDataSetting();
    }
    NetworkInfo networkInfo = connectivity.getActiveNetworkInfo();
    if (networkInfo == null) {
      return false;
    }
    return networkInfo.isAvailable();
  }

  protected long getLastLaunch() {
    return getSharedPreferences().getLong(AnnouncementsConstants.PREF_LAST_LAUNCH, 0);
  }

  private SharedPreferences getSharedPreferences() {
    return this.getSharedPreferences(AnnouncementsConstants.PREFS_BRANCH, BackgroundConstants.SHARED_PREFERENCES_MODE);
  }

  @Override
  protected void dump(FileDescriptor fd, PrintWriter writer, String[] args) {
    super.dump(fd, writer, args);

    final long lastFetch = getLastFetch();
    final long lastLaunch = getLastLaunch();
    writer.write("AnnouncementsService: last fetch " + lastFetch +
                 ", last Firefox activity: " + lastLaunch + "\n");
  }

  protected void setEarliestNextFetch(final long earliestInMsec) {
    this.getSharedPreferences().edit().putLong(AnnouncementsConstants.PREF_EARLIEST_NEXT_ANNOUNCE_FETCH, earliestInMsec).commit();
  }

  protected long getEarliestNextFetch() {
    return this.getSharedPreferences().getLong(AnnouncementsConstants.PREF_EARLIEST_NEXT_ANNOUNCE_FETCH, 0L);
  }

  protected void setLastFetch(final long fetch) {
    this.getSharedPreferences().edit().putLong(AnnouncementsConstants.PREF_LAST_FETCH_LOCAL_TIME, fetch).commit();
  }

  public long getLastFetch() {
    return this.getSharedPreferences().getLong(AnnouncementsConstants.PREF_LAST_FETCH_LOCAL_TIME, 0L);
  }

  protected String setLastDate(final String fetch) {
    if (fetch == null) {
      this.getSharedPreferences().edit().remove(AnnouncementsConstants.PREF_LAST_FETCH_SERVER_DATE).commit();
      return null;
    }
    this.getSharedPreferences().edit().putString(AnnouncementsConstants.PREF_LAST_FETCH_SERVER_DATE, fetch).commit();
    return fetch;
  }

  @Override
  public String getLastDate() {
    return this.getSharedPreferences().getString(AnnouncementsConstants.PREF_LAST_FETCH_SERVER_DATE, null);
  }

  





  public void setAnnouncementsServerBaseURL(final URI url) {
    if (url == null) {
      throw new IllegalArgumentException("url cannot be null.");
    }
    final String scheme = url.getScheme();
    if (scheme == null) {
      throw new IllegalArgumentException("url must have a scheme.");
    }
    if (!scheme.equalsIgnoreCase("http") && !scheme.equalsIgnoreCase("https")) {
      throw new IllegalArgumentException("url must be http or https.");
    }
    SharedPreferences p = this.getSharedPreferences();
    p.edit().putString(AnnouncementsConstants.PREF_ANNOUNCE_SERVER_BASE_URL, url.toASCIIString()).commit();
  }

  




  @Override
  public String getServiceURL() {
    SharedPreferences p = this.getSharedPreferences();
    String base = p.getString(AnnouncementsConstants.PREF_ANNOUNCE_SERVER_BASE_URL, AnnouncementsConstants.DEFAULT_ANNOUNCE_SERVER_BASE_URL);
    return base + AnnouncementsConstants.ANNOUNCE_PATH_SUFFIX;
  }

  @Override
  public Locale getLocale() {
    return Locale.getDefault();
  }

  @Override
  public String getUserAgent() {
    return AnnouncementsConstants.ANNOUNCE_USER_AGENT;
  }

  protected void persistTimes(long fetched, String date) {
    setLastFetch(fetched);
    if (date != null) {
      setLastDate(date);
    }
  }

  @Override
  public void onNoNewAnnouncements(long fetched, String date) {
    Logger.info(LOG_TAG, "No new announcements to display.");
    persistTimes(fetched, date);
  }

  @Override
  public void onNewAnnouncements(List<Announcement> announcements, long fetched, String date) {
    Logger.info(LOG_TAG, "Processing announcements: " + announcements.size());
    persistTimes(fetched, date);
    processAnnouncements(announcements);
  }

  @Override
  public void onRemoteFailure(int status) {
    
    Logger.warn(LOG_TAG, "Got remote fetch status " + status + "; bumping fetch time.");
    setLastFetch(System.currentTimeMillis());
  }

  @Override
  public void onRemoteError(Exception e) {
    
    Logger.warn(LOG_TAG, "Error processing response.", e);
    setLastFetch(System.currentTimeMillis());
  }

  @Override
  public void onLocalError(Exception e) {
    Logger.error(LOG_TAG, "Got exception in fetch.", e);
    
  }

  @Override
  public void onBackoff(int retryAfterInSeconds) {
    Logger.info(LOG_TAG, "Got retry after: " + retryAfterInSeconds);
    final long delayInMsec = Math.max(retryAfterInSeconds * 1000, AnnouncementsConstants.DEFAULT_BACKOFF_MSEC);
    final long fuzzedBackoffInMsec = delayInMsec + Math.round(((double) delayInMsec * 0.25d * Math.random()));
    Logger.debug(LOG_TAG, "Fuzzed backoff: " + fuzzedBackoffInMsec + "ms.");
    setEarliestNextFetch(fuzzedBackoffInMsec + System.currentTimeMillis());
  }
}
