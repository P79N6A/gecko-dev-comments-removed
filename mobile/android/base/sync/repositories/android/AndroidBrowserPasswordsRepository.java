




































package org.mozilla.gecko.sync.repositories.android;

import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCreationDelegate;

import android.content.Context;

public class AndroidBrowserPasswordsRepository extends AndroidBrowserRepository {

  @Override
  protected AndroidBrowserRepositoryDataAccessor getDataAccessor(Context context) {
    return new AndroidBrowserPasswordsDataAccessor(context);
  }

  @Override
  protected void sessionCreator(RepositorySessionCreationDelegate delegate,
      Context context) {
    AndroidBrowserPasswordsRepositorySession session = new AndroidBrowserPasswordsRepositorySession(AndroidBrowserPasswordsRepository.this, context);
    delegate.onSessionCreated(session);
  }

}
