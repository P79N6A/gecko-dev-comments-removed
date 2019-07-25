





































package org.mozilla.gecko.sync.repositories.android;

import org.mozilla.gecko.sync.repositories.BookmarksRepository;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCreationDelegate;

import android.content.Context;

public class AndroidBrowserBookmarksRepository extends AndroidBrowserRepository implements BookmarksRepository {

  @Override
  protected void sessionCreator(RepositorySessionCreationDelegate delegate, Context context) {
    AndroidBrowserBookmarksRepositorySession session = new AndroidBrowserBookmarksRepositorySession(AndroidBrowserBookmarksRepository.this, context);
    delegate.onSessionCreated(session);
  }

  @Override
  protected AndroidBrowserRepositoryDataAccessor getDataAccessor(Context context) {
    return new AndroidBrowserBookmarksDataAccessor(context);
  }
}
