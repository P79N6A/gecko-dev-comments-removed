



package org.mozilla.gecko.fxa.sync;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

public class FxAccountSyncService extends Service {
  private static final Object syncAdapterLock = new Object();
  private static FxAccountSyncAdapter syncAdapter = null;

  @Override
  public void onCreate() {
    synchronized (syncAdapterLock) {
      if (syncAdapter == null) {
        syncAdapter = new FxAccountSyncAdapter(getApplicationContext(), true);
      }
    }
  }

  @Override
  public IBinder onBind(Intent intent) {
    return syncAdapter.getSyncAdapterBinder();
  }
}
