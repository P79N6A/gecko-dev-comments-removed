




































package org.mozilla.gecko.sync.stage;

import java.net.URISyntaxException;

import org.mozilla.gecko.sync.repositories.ConstrainedServer11Repository;
import org.mozilla.gecko.sync.repositories.RecordFactory;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.android.AndroidBrowserBookmarksRepository;
import org.mozilla.gecko.sync.repositories.domain.BookmarkRecordFactory;

public class AndroidBrowserBookmarksServerSyncStage extends ServerSyncStage {

  
  
  private static final String BOOKMARKS_SORT          = "index";
  private static final long   BOOKMARKS_REQUEST_LIMIT = 5000;         

  @Override
  public void execute(org.mozilla.gecko.sync.GlobalSession session) throws NoSuchStageException {
    super.execute(session);
  }

  @Override
  protected String getCollection() {
    return "bookmarks";
  }
  @Override
  protected String getEngineName() {
    return "bookmarks";
  }

  @Override
  protected Repository getRemoteRepository() throws URISyntaxException {
    return new ConstrainedServer11Repository(session.config.getClusterURLString(),
                                             session.config.username,
                                             getCollection(),
                                             session,
                                             BOOKMARKS_REQUEST_LIMIT,
                                             BOOKMARKS_SORT);
  }

  @Override
  protected Repository getLocalRepository() {
    return new AndroidBrowserBookmarksRepository();
  }

  @Override
  protected RecordFactory getRecordFactory() {
    return new BookmarkRecordFactory();
  }
}