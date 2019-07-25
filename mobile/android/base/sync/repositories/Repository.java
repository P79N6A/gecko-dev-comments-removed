





































package org.mozilla.gecko.sync.repositories;

import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCleanDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCreationDelegate;

import android.content.Context;

public abstract class Repository {
  public abstract void createSession(RepositorySessionCreationDelegate delegate, Context context);

  public void clean(boolean success, RepositorySessionCleanDelegate delegate, Context context) {
    delegate.onCleaned(this);
  }
}