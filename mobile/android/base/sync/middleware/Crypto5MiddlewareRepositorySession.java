




































package org.mozilla.gecko.sync.middleware;

import java.io.UnsupportedEncodingException;
import java.util.concurrent.ExecutorService;

import org.mozilla.gecko.sync.CryptoRecord;
import org.mozilla.gecko.sync.crypto.CryptoException;
import org.mozilla.gecko.sync.crypto.KeyBundle;
import org.mozilla.gecko.sync.repositories.NoStoreDelegateException;
import org.mozilla.gecko.sync.repositories.RecordFactory;
import org.mozilla.gecko.sync.repositories.RepositorySession;
import org.mozilla.gecko.sync.repositories.RepositorySessionBundle;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionBeginDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFetchRecordsDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFinishDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionGuidsSinceDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionStoreDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionWipeDelegate;
import org.mozilla.gecko.sync.repositories.domain.Record;








































public class Crypto5MiddlewareRepositorySession extends RepositorySession {
  private KeyBundle keyBundle;
  private RepositorySession inner;
  private RecordFactory recordFactory;

  public Crypto5MiddlewareRepositorySession(RepositorySession session, Crypto5MiddlewareRepository repository, RecordFactory recordFactory) {
    super(repository);
    this.inner = session;
    this.keyBundle = repository.keyBundle;
    this.recordFactory = recordFactory;
  }

  public class DecryptingTransformingFetchDelegate implements RepositorySessionFetchRecordsDelegate {
    private RepositorySessionFetchRecordsDelegate next;
    private KeyBundle keyBundle;
    private RecordFactory recordFactory;

    DecryptingTransformingFetchDelegate(RepositorySessionFetchRecordsDelegate next, KeyBundle bundle, RecordFactory recordFactory) {
      this.next = next;
      this.keyBundle = bundle;
      this.recordFactory = recordFactory;
    }

    @Override
    public void onFetchFailed(Exception ex, Record record) {
      next.onFetchFailed(ex, record);
    }

    @Override
    public void onFetchedRecord(Record record) {
      CryptoRecord r;
      try {
        r = (CryptoRecord) record;
      } catch (ClassCastException e) {
        next.onFetchFailed(e, record);
        return;
      }
      r.keyBundle = keyBundle;
      try {
        r.decrypt();
      } catch (Exception e) {
        next.onFetchFailed(e, r);
        return;
      }
      Record transformed;
      try {
        transformed = this.recordFactory.createRecord(r);
      } catch (Exception e) {
        next.onFetchFailed(e, r);
        return;
      }
      next.onFetchedRecord(transformed);
    }

    @Override
    public void onFetchSucceeded(Record[] records, long fetchEnd) {
      for (Record record : records) {
        try {
          this.onFetchedRecord(record);
        } catch (Exception e) {
          this.onFetchFailed(e, record);
        }
      }
      this.onFetchCompleted(fetchEnd);
    }

    @Override
    public void onFetchCompleted(final long fetchEnd) {
      next.onFetchCompleted(fetchEnd);
    }

    @Override
    public RepositorySessionFetchRecordsDelegate deferredFetchDelegate(ExecutorService executor) {
      
      RepositorySessionFetchRecordsDelegate deferredNext = next.deferredFetchDelegate(executor);
      return new DecryptingTransformingFetchDelegate(deferredNext, keyBundle, recordFactory);
    }
  }

  private DecryptingTransformingFetchDelegate makeUnwrappingDelegate(RepositorySessionFetchRecordsDelegate inner) {
    if (inner == null) {
      throw new IllegalArgumentException("Inner delegate cannot be null!");
    }
    return new DecryptingTransformingFetchDelegate(inner, this.keyBundle, this.recordFactory);
  }

  @Override
  public void guidsSince(long timestamp,
                         RepositorySessionGuidsSinceDelegate delegate) {
    
    inner.guidsSince(timestamp, delegate);
  }

  @Override
  public void fetchSince(long timestamp,
                         RepositorySessionFetchRecordsDelegate delegate) {
    inner.fetchSince(timestamp, makeUnwrappingDelegate(delegate));
  }

  @Override
  public void fetch(String[] guids,
                    RepositorySessionFetchRecordsDelegate delegate) {
    inner.fetch(guids, makeUnwrappingDelegate(delegate));
  }

  @Override
  public void fetchAll(RepositorySessionFetchRecordsDelegate delegate) {
    inner.fetchAll(makeUnwrappingDelegate(delegate));
  }

  @Override
  public void setStoreDelegate(RepositorySessionStoreDelegate delegate) {
    
    inner.setStoreDelegate(delegate);
    this.delegate = delegate;             
  }

  @Override
  public void store(Record record) throws NoStoreDelegateException {
    if (delegate == null) {
      throw new NoStoreDelegateException();
    }
    CryptoRecord rec = record.getPayload();
    rec.keyBundle = this.keyBundle;
    try {
      rec.encrypt();
    } catch (UnsupportedEncodingException e) {
      delegate.onRecordStoreFailed(e);
      return;
    } catch (CryptoException e) {
      delegate.onRecordStoreFailed(e);
      return;
    }
    
    inner.store(rec);
  }

  @Override
  public void wipe(RepositorySessionWipeDelegate delegate) {
    inner.wipe(delegate);
  }

  @Override
  public void storeDone() {
    inner.storeDone();
  }

  @Override
  public void storeDone(long storeEnd) {
    inner.storeDone(storeEnd);
  }

  public class Crypto5MiddlewareRepositorySessionBeginDelegate implements RepositorySessionBeginDelegate {
    private Crypto5MiddlewareRepositorySession outerSession;
    private RepositorySessionBeginDelegate next;

    public Crypto5MiddlewareRepositorySessionBeginDelegate(Crypto5MiddlewareRepositorySession outerSession, RepositorySessionBeginDelegate next) {
      this.outerSession = outerSession;
      this.next = next;
    }

    @Override
    public void onBeginFailed(Exception ex) {
      next.onBeginFailed(ex);
    }

    @Override
    public void onBeginSucceeded(RepositorySession session) {
      outerSession.setStatus(SessionStatus.ACTIVE);
      next.onBeginSucceeded(outerSession);
    }

    @Override
    public RepositorySessionBeginDelegate deferredBeginDelegate(ExecutorService executor) {
      return this;
    }
  }

  public void begin(RepositorySessionBeginDelegate delegate) {
    inner.begin(new Crypto5MiddlewareRepositorySessionBeginDelegate(this, delegate));
  }

  public class Crypto5MiddlewareRepositorySessionFinishDelegate implements RepositorySessionFinishDelegate {
    private Crypto5MiddlewareRepositorySession outerSession;
    private RepositorySessionFinishDelegate next;

    public Crypto5MiddlewareRepositorySessionFinishDelegate(Crypto5MiddlewareRepositorySession outerSession, RepositorySessionFinishDelegate next) {
      this.outerSession = outerSession;
      this.next = next;
    }

    @Override
    public void onFinishFailed(Exception ex) {
      next.onFinishFailed(ex);
    }

    @Override
    public void onFinishSucceeded(RepositorySession session, RepositorySessionBundle bundle) {
      outerSession.setStatus(SessionStatus.DONE);
      next.onFinishSucceeded(outerSession, bundle);
    }

    @Override
    public RepositorySessionFinishDelegate deferredFinishDelegate(ExecutorService executor) {
      return this;
    }
  }

  @Override
  public void finish(RepositorySessionFinishDelegate delegate) {
    inner.finish(new Crypto5MiddlewareRepositorySessionFinishDelegate(this, delegate));
  }

  @Override
  public void abort() {
    setStatus(SessionStatus.ABORTED);
    inner.abort();
  }

  @Override
  public void abort(RepositorySessionFinishDelegate delegate) {
    this.status = SessionStatus.DONE; 
    inner.abort(new Crypto5MiddlewareRepositorySessionFinishDelegate(this, delegate));
  }
}
