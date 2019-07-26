


package org.mozilla.gecko.background.helpers;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.test.ServiceTestCase;
import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;
import java.util.UUID;

import org.mozilla.gecko.background.common.GlobalConstants;







public abstract class BackgroundServiceTestCase<T extends Service> extends ServiceTestCase<T> {
  private static final String SHARED_PREFS_PREFIX = "BackgroundServiceTestCase-";
  
  
  
  
  protected static String sharedPrefsName;

  private final Class<T> mServiceClass;

  protected static CyclicBarrier barrier;
  protected Intent intent;

  public BackgroundServiceTestCase(Class<T> serviceClass) {
    super(serviceClass);
    mServiceClass = serviceClass;
  }

  @Override
  public void setUp() throws Exception {
    barrier = new CyclicBarrier(2);
    intent = new Intent(getContext(), mServiceClass);
    sharedPrefsName = SHARED_PREFS_PREFIX + mServiceClass.getName() + "-" + UUID.randomUUID();
  }

  @Override
  public void tearDown() throws Exception {
    barrier = null;
    intent = null;
    clearSharedPrefs(); 
  }

  protected SharedPreferences getSharedPreferences() {
    return getContext().getSharedPreferences(sharedPrefsName,
        GlobalConstants.SHARED_PREFERENCES_MODE);
  }

  protected void clearSharedPrefs() {
    getSharedPreferences().edit()
        .clear()
        .commit();
  }

  protected void await() {
    try {
      barrier.await();
    } catch (InterruptedException e) {
      fail("Test runner thread should not be interrupted.");
    } catch (BrokenBarrierException e) {
      fail("Background services should not timeout or be interrupted");
    }
  }

  protected void cancelAlarm(Intent intent) {
    final AlarmManager am = (AlarmManager) getContext().getSystemService(Context.ALARM_SERVICE);
    final PendingIntent pi = PendingIntent.getService(getContext(), 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
    am.cancel(pi);
    pi.cancel();
  }

  protected boolean isServiceAlarmSet(Intent intent) {
    return PendingIntent.getService(getContext(), 0, intent, PendingIntent.FLAG_NO_CREATE) != null;
  }
}
