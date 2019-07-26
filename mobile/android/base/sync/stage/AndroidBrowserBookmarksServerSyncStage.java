



package org.mozilla.gecko.sync.stage;

import java.net.URISyntaxException;

import org.mozilla.gecko.sync.JSONRecordFetcher;
import org.mozilla.gecko.sync.MetaGlobalException;
import org.mozilla.gecko.sync.net.AuthHeaderProvider;
import org.mozilla.gecko.sync.repositories.RecordFactory;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserBookmarksRepository;
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
    
    
    AuthHeaderProvider authHeaderProvider = session.getAuthHeaderProvider();
    final JSONRecordFetcher countsFetcher = new JSONRecordFetcher(session.config.infoCollectionCountsURL(), authHeaderProvider);
    String collection = getCollection();
    return new SafeConstrainedServer11Repository(
                                                 collection,
                                                 session.config.storageURL(),
                                                 session.getAuthHeaderProvider(),
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
    if (session == null || session.getContext() == null) {
      return false;
    }
    return super.isEnabled();
  }
}
