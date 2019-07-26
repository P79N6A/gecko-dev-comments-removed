



package org.mozilla.gecko.sync.stage;

import java.net.URISyntaxException;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.JSONRecordFetcher;
import org.mozilla.gecko.sync.MetaGlobalException;
import org.mozilla.gecko.sync.repositories.RecordFactory;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserBookmarksRepository;
import org.mozilla.gecko.sync.repositories.android.FennecControlHelper;
import org.mozilla.gecko.sync.repositories.domain.BookmarkRecordFactory;
import org.mozilla.gecko.sync.repositories.domain.VersionConstants;

public class AndroidBrowserBookmarksServerSyncStage extends ServerSyncStage {
  protected static final String LOG_TAG = "BookmarksStage";

  
  
  private static final String BOOKMARKS_SORT          = "index";
  private static final long   BOOKMARKS_REQUEST_LIMIT = 5000;         

  @Override
  protected String getCollection() {
    return "bookmarks";
  }

  @Override
  protected String getEngineName() {
    return "bookmarks";
  }

  @Override
  public Integer getStorageVersion() {
    return VersionConstants.BOOKMARKS_ENGINE_VERSION;
  }

  @Override
  protected Repository getRemoteRepository() throws URISyntaxException {
    
    
    final JSONRecordFetcher countsFetcher = new JSONRecordFetcher(session.config.infoCollectionCountsURL(), session.credentials());
    return new SafeConstrainedServer11Repository(session.config.getClusterURLString(),
                                                 session.config.username,
                                                 getCollection(),
                                                 session,
                                                 BOOKMARKS_REQUEST_LIMIT,
                                                 BOOKMARKS_SORT,
                                                 countsFetcher);
  }

  @Override
  protected Repository getLocalRepository() {
    return new AndroidBrowserBookmarksRepository();
  }

  @Override
  protected RecordFactory getRecordFactory() {
    return new BookmarkRecordFactory();
  }

  @Override
  protected boolean isEnabled() throws MetaGlobalException {
    if (session.getContext() == null) {
      return false;
    }
    boolean migrated = FennecControlHelper.areBookmarksMigrated(session.getContext());
    if (!migrated) {
      Logger.warn(LOG_TAG, "Not enabling bookmarks engine since Fennec bookmarks are not migrated.");
    }
    return super.isEnabled() && migrated;
  }
}