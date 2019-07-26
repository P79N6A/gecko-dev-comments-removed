



package org.mozilla.gecko.background.announcements;

import java.util.List;
import java.util.Locale;

public interface AnnouncementsFetchDelegate {
  


  public long getLastFetch();

  


  public Locale getLocale();

  


  public String getUserAgent();

  


  public String getServiceURL();

  


  public void onNoNewAnnouncements(long fetched);
  public void onNewAnnouncements(List<Announcement> snippets, long fetched);
  public void onLocalError(Exception e);
  public void onRemoteError(Exception e);
  public void onRemoteFailure(int status);
  public void onBackoff(int retryAfterInSeconds);
}