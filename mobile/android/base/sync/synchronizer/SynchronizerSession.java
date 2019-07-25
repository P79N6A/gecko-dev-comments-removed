



package org.mozilla.gecko.sync.synchronizer;


import java.util.concurrent.ExecutorService;

import org.mozilla.gecko.sync.Logger;
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
  private Synchronizer synchronizer;
  private SynchronizerSessionDelegate delegate;
  private Context context;

  


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

  
  public void abort() {
    this.delegate.onSynchronizeAborted(this);
  }

  


  public void synchronize() {
    
    if (!sessionA.dataAvailable() &&
        !sessionB.dataAvailable()) {
      Logger.info(LOG_TAG, "Neither session reports data available. Short-circuiting sync.");
      sessionA.abort();
      sessionB.abort();
      this.delegate.onSynchronizeSkipped(this);
      return;
    }

    final SynchronizerSession session = this;

    

    
    
    final RecordsChannel channelBToA = new RecordsChannel(this.sessionB, this.sessionA, this);

    
    RecordsChannelDelegate channelAToBDelegate = new RecordsChannelDelegate() {
      public void onFlowCompleted(RecordsChannel recordsChannel, long fetchEnd, long storeEnd) {
        Logger.info(LOG_TAG, "First RecordsChannel onFlowCompleted. Fetch end is " + fetchEnd +
             ". Store end is " + storeEnd + ". Starting next.");
        pendingATimestamp = fetchEnd;
        storeEndBTimestamp = storeEnd;
        flowAToBCompleted = true;
        channelBToA.flow();
      }

      @Override
      public void onFlowBeginFailed(RecordsChannel recordsChannel, Exception ex) {
        Logger.warn(LOG_TAG, "First RecordsChannel onFlowBeginFailed. Reporting session error.", ex);
        session.delegate.onSessionError(ex);
      }

      @Override
      public void onFlowFetchFailed(RecordsChannel recordsChannel, Exception ex) {
        
        Logger.warn(LOG_TAG, "First RecordsChannel onFlowFetchFailed. Reporting fetch error.", ex);
        session.delegate.onFetchError(ex);
      }

      @Override
      public void onFlowStoreFailed(RecordsChannel recordsChannel, Exception ex) {
        
        Logger.warn(LOG_TAG, "First RecordsChannel onFlowStoreFailed. Reporting store error.", ex);
        session.delegate.onStoreError(ex);
      }

      @Override
      public void onFlowFinishFailed(RecordsChannel recordsChannel, Exception ex) {
        Logger.warn(LOG_TAG, "First RecordsChannel onFlowFinishedFailed. Reporting session error.", ex);
        session.delegate.onSessionError(ex);
      }
    };

    
    final RecordsChannel channelAToB = new RecordsChannel(this.sessionA, this.sessionB, channelAToBDelegate);

    Logger.info(LOG_TAG, "Starting A to B flow. Channel is " + channelAToB);
    try {
      channelAToB.beginAndFlow();
    } catch (InvalidSessionTransitionException e) {
      onFlowBeginFailed(channelAToB, e);
    }
  }

  @Override
  public void onFlowCompleted(RecordsChannel channel, long fetchEnd, long storeEnd) {
    Logger.info(LOG_TAG, "Second RecordsChannel onFlowCompleted. Fetch end is " + fetchEnd +
         ". Store end is " + storeEnd + ". Finishing.");

    pendingBTimestamp = fetchEnd;
    storeEndATimestamp = storeEnd;
    flowBToACompleted = true;

    
    try {
      this.sessionA.finish(this);
    } catch (InactiveSessionException e) {
      this.onFinishFailed(e);
    }
  }

  @Override
  public void onFlowBeginFailed(RecordsChannel recordsChannel, Exception ex) {
    Logger.warn(LOG_TAG, "Second RecordsChannel onFlowBeginFailed. Reporting session error.", ex);
    this.delegate.onSessionError(ex);
  }

  @Override
  public void onFlowFetchFailed(RecordsChannel recordsChannel, Exception ex) {
    
    Logger.warn(LOG_TAG, "Second RecordsChannel onFlowFetchFailed. Reporting fetch error.", ex);
    this.delegate.onFetchError(ex);
  }

  






  @Override
  public void onFlowStoreFailed(RecordsChannel recordsChannel, Exception ex) {
    
    Logger.warn(LOG_TAG, "Second RecordsChannel onFlowStoreFailed. Ignoring store error.", ex);
  }

  @Override
  public void onFlowFinishFailed(RecordsChannel recordsChannel, Exception ex) {
    Logger.warn(LOG_TAG, "Second RecordsChannel onFlowFinishedFailed. Reporting session error.", ex);
    this.delegate.onSessionError(ex);
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
    this.delegate.onSessionError(ex);
  }

  





  
  @Override
  public void onSessionCreated(RepositorySession session) {
    if (session == null ||
        this.sessionA == session) {
      
      this.delegate.onSessionError(new UnexpectedSessionException(session));
      return;
    }
    if (this.sessionA == null) {
      this.sessionA = session;

      
      try {
        this.sessionA.unbundle(this.bundleA);
      } catch (Exception e) {
        this.delegate.onSessionError(new UnbundleError(e, sessionA));
        
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
        
        this.delegate.onSessionError(new UnbundleError(e, sessionB));
        return;
      }

      this.delegate.onInitialized(this);
      return;
    }
    
    this.delegate.onSessionError(new UnexpectedSessionException(session));
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
    Logger.info(LOG_TAG, "onFinishSucceeded. Flows? " + flowAToBCompleted + ", " + flowBToACompleted);

    if (session == sessionA) {
      if (flowAToBCompleted) {
        Logger.info(LOG_TAG, "onFinishSucceeded: bumping session A's timestamp to " + pendingATimestamp + " or " + storeEndATimestamp);
        bundle.bumpTimestamp(Math.max(pendingATimestamp, storeEndATimestamp));
        this.synchronizer.bundleA = bundle;
      } else {
        
        this.delegate.onSessionError(new UnexpectedSessionException(sessionA));
      }
      if (this.sessionB != null) {
        Logger.info(LOG_TAG, "Finishing session B.");
        
        try {
          this.sessionB.finish(this);
        } catch (InactiveSessionException e) {
          this.onFinishFailed(e);
        }
      }
    } else if (session == sessionB) {
      if (flowBToACompleted) {
        Logger.info(LOG_TAG, "onFinishSucceeded: bumping session B's timestamp to " + pendingBTimestamp + " or " + storeEndBTimestamp);
        bundle.bumpTimestamp(Math.max(pendingBTimestamp, storeEndBTimestamp));
        this.synchronizer.bundleB = bundle;
        Logger.info(LOG_TAG, "Notifying delegate.onSynchronized.");
        this.delegate.onSynchronized(this);
      } else {
        
        this.delegate.onSessionError(new UnexpectedSessionException(sessionB));
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
