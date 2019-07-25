




































package org.mozilla.gecko.sync.synchronizer;


import java.util.concurrent.ExecutorService;

import org.mozilla.gecko.sync.repositories.RepositorySession;
import org.mozilla.gecko.sync.repositories.RepositorySessionBundle;
import org.mozilla.gecko.sync.repositories.delegates.DeferrableRepositorySessionCreationDelegate;
import org.mozilla.gecko.sync.repositories.delegates.DeferredRepositorySessionFinishDelegate;
import org.mozilla.gecko.sync.repositories.delegates.RepositorySessionFinishDelegate;

import android.content.Context;
import android.util.Log;

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
  private boolean flowAToBCompleted = false;
  private boolean flowBToACompleted = false;

  private static void warn(String msg, Exception e) {
    System.out.println("WARN: " + msg);
    e.printStackTrace(System.err);
    Log.w(LOG_TAG, msg, e);
  }
  private static void warn(String msg) {
    System.out.println("WARN: " + msg);
    Log.w(LOG_TAG, msg);
  }
  private static void info(String msg) {
    System.out.println("INFO: " + msg);
    Log.i(LOG_TAG, msg);
  }

  


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
      info("Neither session reports data available. Short-circuiting sync.");
      sessionA.abort();
      sessionB.abort();
      this.delegate.onSynchronizeSkipped(this);
      return;
    }

    final SynchronizerSession session = this;

    
    final RecordsChannel channelBToA = new RecordsChannel(this.sessionB, this.sessionA, this);
    RecordsChannelDelegate channelDelegate = new RecordsChannelDelegate() {
      public void onFlowCompleted(RecordsChannel recordsChannel, long end) {
        info("First RecordsChannel flow completed. End is " + end + ". Starting next.");
        pendingATimestamp = end;
        flowAToBCompleted = true;
        channelBToA.flow();
      }

      @Override
      public void onFlowBeginFailed(RecordsChannel recordsChannel, Exception ex) {
        warn("First RecordsChannel flow failed to begin.");
        session.delegate.onSessionError(ex);
      }

      @Override
      public void onFlowStoreFailed(RecordsChannel recordsChannel, Exception ex) {
        
        warn("First RecordsChannel flow failed.");
        session.delegate.onStoreError(ex);
      }

      @Override
      public void onFlowFinishFailed(RecordsChannel recordsChannel, Exception ex) {
        warn("onFlowFinishedFailed. Reporting store error.", ex);
        session.delegate.onStoreError(ex);
      }
    };
    final RecordsChannel channelAToB = new RecordsChannel(this.sessionA, this.sessionB, channelDelegate);
    info("Starting A to B flow. Channel is " + channelAToB);
    channelAToB.beginAndFlow();
  }

  @Override
  public void onFlowCompleted(RecordsChannel channel, long end) {
    info("Second RecordsChannel flow completed. End is " + end + ". Finishing.");
    pendingBTimestamp = end;
    flowBToACompleted = true;

    
    this.sessionA.finish(this);
  }

  @Override
  public void onFlowBeginFailed(RecordsChannel recordsChannel, Exception ex) {
    warn("Second RecordsChannel flow failed to begin.", ex);
  }

  @Override
  public void onFlowStoreFailed(RecordsChannel recordsChannel, Exception ex) {
    
    warn("Second RecordsChannel flow failed.");
  }

  @Override
  public void onFlowFinishFailed(RecordsChannel recordsChannel, Exception ex) {
    
    warn("First RecordsChannel flow failed to finish.");
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
      
      warn("Got exception cleaning up first after second session creation failed.", ex);
      return;
    }
    String session = (this.sessionA == null) ? "B" : "A";
    this.delegate.onSynchronizeFailed(this, ex, "Finish of session " + session + " failed.");
  }

  @Override
  public void onFinishSucceeded(RepositorySession session,
                                RepositorySessionBundle bundle) {
    info("onFinishSucceeded. Flows? " +
         flowAToBCompleted + ", " + flowBToACompleted);

    if (session == sessionA) {
      if (flowAToBCompleted) {
        info("onFinishSucceeded: bumping session A's timestamp to " + pendingATimestamp);
        bundle.bumpTimestamp(pendingATimestamp);
        this.synchronizer.bundleA = bundle;
      }
      if (this.sessionB != null) {
        info("Finishing session B.");
        
        this.sessionB.finish(this);
      }
    } else if (session == sessionB) {
      if (flowBToACompleted) {
        info("onFinishSucceeded: bumping session B's timestamp to " + pendingBTimestamp);
        bundle.bumpTimestamp(pendingBTimestamp);
        this.synchronizer.bundleB = bundle;
        info("Notifying delegate.onSynchronized.");
        this.delegate.onSynchronized(this);
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
