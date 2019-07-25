




































package org.mozilla.gecko.sync.repositories.android;

import org.mozilla.gecko.sync.repositories.HistoryRepository;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCreationDelegate;

import android.content.Context;

public class AndroidBrowserHistoryRepository extends AndroidBrowserRepository implements HistoryRepository {

  @Override
  protected void sessionCreator(RepositorySessionCreationDelegate delegate, Context context) {
    AndroidBrowserHistoryRepositorySession session = new AndroidBrowserHistoryRepositorySession(AndroidBrowserHistoryRepository.this, context);
    delegate.onSessionCreated(session);
  }

  @Override
  protected AndroidBrowserRepositoryDataAccessor getDataAccessor(Context context) {
    return new AndroidBrowserHistoryDataAccessor(context);
  }

}
