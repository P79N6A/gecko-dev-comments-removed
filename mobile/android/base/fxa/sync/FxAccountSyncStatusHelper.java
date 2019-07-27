



package org.mozilla.gecko.fxa.sync;

import java.util.Map;
import java.util.Map.Entry;
import java.util.WeakHashMap;

import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.authenticator.AndroidFxAccount;

import android.content.ContentResolver;
import android.content.SyncStatusObserver;






public class FxAccountSyncStatusHelper implements SyncStatusObserver {
  @SuppressWarnings("unused")
  private static final String LOG_TAG = FxAccountSyncStatusHelper.class.getSimpleName();

  protected static FxAccountSyncStatusHelper sInstance;

  public synchronized static FxAccountSyncStatusHelper getInstance() {
    if (sInstance == null) {
      sInstance = new FxAccountSyncStatusHelper();
    }
    return sInstance;
  }

  
  protected Object handle;

  
  
  protected Map<FirefoxAccounts.SyncStatusListener, Boolean> delegates = new WeakHashMap<FirefoxAccounts.SyncStatusListener, Boolean>();

  @Override
  public synchronized void onStatusChanged(int which) {
    for (Entry<FirefoxAccounts.SyncStatusListener, Boolean> entry : delegates.entrySet()) {
      final FirefoxAccounts.SyncStatusListener delegate = entry.getKey();
      final AndroidFxAccount fxAccount = new AndroidFxAccount(delegate.getContext(), delegate.getAccount());
      final boolean active = fxAccount.isCurrentlySyncing();
      
      boolean wasActiveLastTime = entry.getValue();
      
      entry.setValue(active);

      if (active && !wasActiveLastTime) {
        
        delegate.onSyncStarted();
      }
      if (!active && wasActiveLastTime) {
        
        delegate.onSyncFinished();
      }
    }
  }

  protected void addListener() {
    final int mask = ContentResolver.SYNC_OBSERVER_TYPE_ACTIVE;
    if (this.handle != null) {
      throw new IllegalStateException("Already registered this as an observer?");
    }
    this.handle = ContentResolver.addStatusChangeListener(mask, this);
  }

  protected void removeListener() {
    Object handle = this.handle;
    this.handle = null;
    if (handle != null) {
      ContentResolver.removeStatusChangeListener(handle);
    }
  }

  public synchronized void startObserving(FirefoxAccounts.SyncStatusListener delegate) {
    if (delegate == null) {
      throw new IllegalArgumentException("delegate must not be null");
    }
    if (delegates.containsKey(delegate)) {
      return;
    }
    
    if (delegates.isEmpty()) {
      addListener();
    }
    delegates.put(delegate, Boolean.FALSE);
  }

  public synchronized void stopObserving(FirefoxAccounts.SyncStatusListener delegate) {
    delegates.remove(delegate);
    
    if (delegates.isEmpty()) {
      removeListener();
    }
  }
}
