



package org.mozilla.gecko.sync.synchronizer;


import java.util.concurrent.ExecutorService;
import java.util.concurrent.atomic.AtomicInteger;

import org.mozilla.gecko.background.common.log.Logger;
import org.mozilla.gecko.sync.repositories.InactiveSessionException;
import org.mozilla.gecko.sync.repositories.InvalidSessionTransitionException;
import org.mozilla.gecko.sync.repositories.RepositorySession;
import org.mozilla.gecko.sync.repositories.RepositorySessionBundle;
import org.mozilla.gecko.sync.repositories.delegates.DeferrableRepositorySessionCreationDelegate;
import org.mozilla.gecko.sync.repositories.delegates.DeferredRepositorySessionFinishDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFinishDelegate;

import android.content.Context;




























public class SynchronizerSession
extends DeferrableRepositorySessionCreationDelegate
implements RecordsChannelDelegate,
           RepositorySessionFinishDelegate {

  protected static final String LOG_TAG = "SynchronizerSession";
  protected Synchronizer synchronizer;
  protected SynchronizerSessionDelegate delegate;
  protected Context context;

  


  private RepositorySession sessionA;
  private RepositorySession sessionB;
  private RepositorySessionBundle bundleA;
  private RepositorySessionBundle bundleB;

  
  
  
  
  private long pendingATimestamp = -1;
  private long pendingBTimestamp = -1;
  private long storeEndATimestamp = -1;
  private long storeEndBTimestamp = -1;
  private boolean flowAToBCompleted = false;
  private boolean flowBToACompleted = false;

  protected final AtomicInteger numInboundRecords = new AtomicInteger(-1);
  protected final AtomicInteger numOutboundRecords = new AtomicInteger(-1);

  


  public SynchronizerSession(Synchronizer synchronizer, SynchronizerSessionDelegate delegate) {
    this.setSynchronizer(synchronizer);
    this.delegate = delegate;
  }

  public Synchronizer getSynchronizer() {
    return synchronizer;
  }

  public void setSynchronizer(Synchronizer synchronizer) {
    this.synchronizer = synchronizer;
  }

  public void init(Context context, RepositorySessionBundle bundleA, RepositorySessionBundle bundleB) {
    this.context = context;
    this.bundleA = bundleA;
    this.bundleB = bundleB;
    
    this.getSynchronizer().repositoryA.createSession(this, context);
  }

  







  public int getInboundCount() {
    return numInboundRecords.get();
  }

  







  public int getOutboundCount() {
    return numOutboundRecords.get();
  }

  
  
  protected RecordsChannel channelAToB;
  protected RecordsChannel channelBToA;

  


  public synchronized void synchronize() {
    numInboundRecords.set(-1);
    numOutboundRecords.set(-1);

    
    if (sessionA.shouldSkip() ||
        sessionB.shouldSkip()) {
      Logger.info(LOG_TAG, "Session requested skip. Short-circuiting sync.");
      sessionA.abort();
      sessionB.abort();
      this.delegate.onSynchronizeSkipped(this);
      return;
    }

    if (!sessionA.dataAvailable() &&
        !sessionB.dataAvailable()) {
      Logger.info(LOG_TAG, "Neither session reports data available. Short-circuiting sync.");
      sessionA.abort();
      sessionB.abort();
      this.delegate.onSynchronizeSkipped(this);
      return;
    }

    final SynchronizerSession session = this;

    

    
    
    channelBToA = new RecordsChannel(this.sessionB, this.sessionA, this);

    
    RecordsChannelDelegate channelAToBDelegate = new RecordsChannelDelegate() {
      public void onFlowCompleted(RecordsChannel recordsChannel, long fetchEnd, long storeEnd) {
        session.onFirstFlowCompleted(recordsChannel, fetchEnd, storeEnd);
      }

      @Override
      public void onFlowBeginFailed(RecordsChannel recordsChannel, Exception ex) {
        Logger.warn(LOG_TAG, "First RecordsChannel onFlowBeginFailed. Logging session error.", ex);
        session.delegate.onSynchronizeFailed(session, ex, "Failed to begin first flow.");
      }

      @Override
      public void onFlowFetchFailed(RecordsChannel recordsChannel, Exception ex) {
        Logger.warn(LOG_TAG, "First RecordsChannel onFlowFetchFailed. Logging remote fetch error.", ex);
      }

      @Override
      public void onFlowStoreFailed(RecordsChannel recordsChannel, Exception ex, String recordGuid) {
        Logger.warn(LOG_TAG, "First RecordsChannel onFlowStoreFailed. Logging local store error.", ex);
      }

      @Override
      public void onFlowFinishFailed(RecordsChannel recordsChannel, Exception ex) {
        Logger.warn(LOG_TAG, "First RecordsChannel onFlowFinishedFailed. Logging session error.", ex);
        session.delegate.onSynchronizeFailed(session, ex, "Failed to finish first flow.");
      }
    };

    
    channelAToB = new RecordsChannel(this.sessionA, this.sessionB, channelAToBDelegate);

    Logger.trace(LOG_TAG, "Starting A to B flow. Channel is " + channelAToB);
    try {
      channelAToB.beginAndFlow();
    } catch (InvalidSessionTransitionException e) {
      onFlowBeginFailed(channelAToB, e);
    }
  }

  







  public void onFirstFlowCompleted(RecordsChannel recordsChannel, long fetchEnd, long storeEnd) {
    Logger.trace(LOG_TAG, "First RecordsChannel onFlowCompleted.");
    Logger.debug(LOG_TAG, "Fetch end is " + fetchEnd + ". Store end is " + storeEnd + ". Starting next.");
    pendingATimestamp = fetchEnd;
    storeEndBTimestamp = storeEnd;
    numInboundRecords.set(recordsChannel.getFetchCount());
    flowAToBCompleted = true;
    channelBToA.flow();
  }

  







  public void onSecondFlowCompleted(RecordsChannel recordsChannel, long fetchEnd, long storeEnd) {
    Logger.trace(LOG_TAG, "Second RecordsChannel onFlowCompleted.");
    Logger.debug(LOG_TAG, "Fetch end is " + fetchEnd + ". Store end is " + storeEnd + ". Finishing.");

    pendingBTimestamp = fetchEnd;
    storeEndATimestamp = storeEnd;
    numOutboundRecords.set(recordsChannel.getFetchCount());
    flowBToACompleted = true;

    
    try {
      this.sessionA.finish(this);
    } catch (InactiveSessionException e) {
      this.onFinishFailed(e);
      return;
    }
  }

  @Override
  public void onFlowCompleted(RecordsChannel recordsChannel, long fetchEnd, long storeEnd) {
    onSecondFlowCompleted(recordsChannel, fetchEnd, storeEnd);
  }

  @Override
  public void onFlowBeginFailed(RecordsChannel recordsChannel, Exception ex) {
    Logger.warn(LOG_TAG, "Second RecordsChannel onFlowBeginFailed. Logging session error.", ex);
    this.delegate.onSynchronizeFailed(this, ex, "Failed to begin second flow.");
  }

  @Override
  public void onFlowFetchFailed(RecordsChannel recordsChannel, Exception ex) {
    Logger.warn(LOG_TAG, "Second RecordsChannel onFlowFetchFailed. Logging local fetch error.", ex);
  }

  @Override
  public void onFlowStoreFailed(RecordsChannel recordsChannel, Exception ex, String recordGuid) {
    Logger.warn(LOG_TAG, "Second RecordsChannel onFlowStoreFailed. Logging remote store error.", ex);
  }

  @Override
  public void onFlowFinishFailed(RecordsChannel recordsChannel, Exception ex) {
    Logger.warn(LOG_TAG, "Second RecordsChannel onFlowFinishedFailed. Logging session error.", ex);
    this.delegate.onSynchronizeFailed(this, ex, "Failed to finish second flow.");
  }

  



  





  @Override
  public void onSessionCreateFailed(Exception ex) {
    
    if (this.sessionA != null) {
      try {
        
        this.context = null;
        this.sessionA.finish(this);
      } catch (Exception e) {
        
      }
    }
    
    this.context = null;
    this.delegate.onSynchronizeFailed(this, ex, "Failed to create session");
  }

  





  
  @Override
  public void onSessionCreated(RepositorySession session) {
    if (session == null ||
        this.sessionA == session) {
      
      this.delegate.onSynchronizeFailed(this, new UnexpectedSessionException(session), "Failed to create session.");
      return;
    }
    if (this.sessionA == null) {
      this.sessionA = session;

      
      try {
        this.sessionA.unbundle(this.bundleA);
      } catch (Exception e) {
        this.delegate.onSynchronizeFailed(this, new UnbundleError(e, sessionA), "Failed to unbundle first session.");
        
        return;
      }
      this.getSynchronizer().repositoryB.createSession(this, this.context);
      return;
    }
    if (this.sessionB == null) {
      this.sessionB = session;
      
      this.context = null;

      
      try {
        this.sessionB.unbundle(this.bundleB);
      } catch (Exception e) {
        this.delegate.onSynchronizeFailed(this, new UnbundleError(e, sessionA), "Failed to unbundle second session.");
        return;
      }

      this.delegate.onInitialized(this);
      return;
    }
    
    this.delegate.onSynchronizeFailed(this, new UnexpectedSessionException(session), "Failed to create session.");
  }

  



  




  @Override
  public void onFinishFailed(Exception ex) {
    if (this.sessionB == null) {
      
      Logger.warn(LOG_TAG, "Got exception cleaning up first after second session creation failed.", ex);
      return;
    }
    String session = (this.sessionA == null) ? "B" : "A";
    this.delegate.onSynchronizeFailed(this, ex, "Finish of session " + session + " failed.");
  }

  







  @Override
  public void onFinishSucceeded(RepositorySession session,
                                RepositorySessionBundle bundle) {
    Logger.debug(LOG_TAG, "onFinishSucceeded. Flows? " + flowAToBCompleted + ", " + flowBToACompleted);

    if (session == sessionA) {
      if (flowAToBCompleted) {
        Logger.debug(LOG_TAG, "onFinishSucceeded: bumping session A's timestamp to " + pendingATimestamp + " or " + storeEndATimestamp);
        bundle.bumpTimestamp(Math.max(pendingATimestamp, storeEndATimestamp));
        this.synchronizer.bundleA = bundle;
      } else {
        
        this.delegate.onSynchronizeFailed(this, new UnexpectedSessionException(sessionA), "Failed to finish first session.");
        return;
      }
      if (this.sessionB != null) {
        Logger.trace(LOG_TAG, "Finishing session B.");
        
        try {
          this.sessionB.finish(this);
        } catch (InactiveSessionException e) {
          this.onFinishFailed(e);
          return;
        }
      }
    } else if (session == sessionB) {
      if (flowBToACompleted) {
        Logger.debug(LOG_TAG, "onFinishSucceeded: bumping session B's timestamp to " + pendingBTimestamp + " or " + storeEndBTimestamp);
        bundle.bumpTimestamp(Math.max(pendingBTimestamp, storeEndBTimestamp));
        this.synchronizer.bundleB = bundle;
        Logger.trace(LOG_TAG, "Notifying delegate.onSynchronized.");
        this.delegate.onSynchronized(this);
      } else {
        
        this.delegate.onSynchronizeFailed(this, new UnexpectedSessionException(sessionB), "Failed to finish second session.");
        return;
      }
    } else {
      
    }

    if (this.sessionB == null) {
      this.sessionA = null; 
    }
  }

  @Override
  public RepositorySessionFinishDelegate deferredFinishDelegate(final ExecutorService executor) {
    return new DeferredRepositorySessionFinishDelegate(this, executor);
  }
}
