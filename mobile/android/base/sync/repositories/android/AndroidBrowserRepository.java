



package org.mozilla.gecko.sync.repositories.android;

import org.mozilla.gecko.sync.repositories.NullCursorException;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCleanDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCreationDelegate;

import android.content.Context;

public abstract class AndroidBrowserRepository extends Repository {

  @Override
  public void createSession(RepositorySessionCreationDelegate delegate, Context context) {
    new CreateSessionThread(delegate, context).start();
  }

  @Override
  public void clean(boolean success, RepositorySessionCleanDelegate delegate, Context context) {
    
    if (success) {
      new CleanThread(delegate, context).start();
    }
  }

  class CleanThread extends Thread {
    private final RepositorySessionCleanDelegate delegate;
    private final Context context;

    public CleanThread(RepositorySessionCleanDelegate delegate, Context context) {
      if (context == null) {
        throw new IllegalArgumentException("context is null");
      }
      this.delegate = delegate;
      this.context = context;
    }

    @Override
    public void run() {
      try {
        getDataAccessor(context).purgeDeleted();
      } catch (NullCursorException e) {
        delegate.onCleanFailed(AndroidBrowserRepository.this, e);
        return;
      } catch (Exception e) {
        delegate.onCleanFailed(AndroidBrowserRepository.this, e);
        return;
      }
      delegate.onCleaned(AndroidBrowserRepository.this);
    }
  }

  protected abstract AndroidBrowserRepositoryDataAccessor getDataAccessor(Context context);
  protected abstract void sessionCreator(RepositorySessionCreationDelegate delegate, Context context);

  class CreateSessionThread extends Thread {
    private final RepositorySessionCreationDelegate delegate;
    private final Context context;

    public CreateSessionThread(RepositorySessionCreationDelegate delegate, Context context) {
      if (context == null) {
        throw new IllegalArgumentException("context is null.");
      }
      this.delegate = delegate;
      this.context = context;
    }

    @Override
    public void run() {
      sessionCreator(delegate, context);
    }
  }

}
