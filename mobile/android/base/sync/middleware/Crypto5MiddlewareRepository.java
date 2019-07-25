




































package org.mozilla.gecko.sync.middleware;

import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.repositories.IdentityRecordFactory;
import org.mozilla.gecko.sync.repositories.RecordFactory;
import org.mozilla.gecko.sync.repositories.Repository;
import org.mozilla.gecko.sync.repositories.RepositorySession;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCleanDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionCreationDelegate;

import android.content.Context;








public class Crypto5MiddlewareRepository extends MiddlewareRepository {

  public RecordFactory recordFactory = new IdentityRecordFactory();

  public class Crypto5MiddlewareRepositorySessionCreationDelegate extends MiddlewareRepository.SessionCreationDelegate {
    private Crypto5MiddlewareRepository repository;
    private RepositorySessionCreationDelegate outerDelegate;

    public Crypto5MiddlewareRepositorySessionCreationDelegate(Crypto5MiddlewareRepository repository, RepositorySessionCreationDelegate outerDelegate) {
      this.repository = repository;
      this.outerDelegate = outerDelegate;
    }
    public void onSessionCreateFailed(Exception ex) {
      this.outerDelegate.onSessionCreateFailed(ex);
    }

    @Override
    public void onSessionCreated(RepositorySession session) {
      
      Crypto5MiddlewareRepositorySession cryptoSession;
      try {
        
        cryptoSession = new Crypto5MiddlewareRepositorySession(session, this.repository, recordFactory);
      } catch (Exception ex) {
        this.outerDelegate.onSessionCreateFailed(ex);
        return;
      }
      this.outerDelegate.onSessionCreated(cryptoSession);
    }
  }

  public KeyBundle keyBundle;
  private Repository inner;

  public Crypto5MiddlewareRepository(Repository inner, KeyBundle keys) {
    super();
    this.inner = inner;
    this.keyBundle = keys;
  }
  @Override
  public void createSession(RepositorySessionCreationDelegate delegate, Context context) {
    Crypto5MiddlewareRepositorySessionCreationDelegate delegateWrapper = new Crypto5MiddlewareRepositorySessionCreationDelegate(this, delegate);
    inner.createSession(delegateWrapper, context);
  }

  @Override
  public void clean(boolean success, RepositorySessionCleanDelegate delegate,
                    Context context) {
    this.inner.clean(success, delegate, context);
  }
}
