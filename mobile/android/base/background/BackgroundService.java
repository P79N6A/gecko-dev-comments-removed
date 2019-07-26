



package org.mozilla.gecko.background;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import org.mozilla.gecko.background.common.log.Logger;

import android.app.AlarmManager;
import android.app.IntentService;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;

public abstract class BackgroundService extends IntentService {
  private static final String LOG_TAG = BackgroundService.class.getSimpleName();

  protected BackgroundService() {
    super(LOG_TAG);
  }

  protected BackgroundService(String threadName) {
    super(threadName);
  }

  public static void runIntentInService(Context context, Intent intent, Class<? extends BackgroundService> serviceClass) {
    Intent service = new Intent(context, serviceClass);
    service.setAction(intent.getAction());
    service.putExtras(intent);
    context.startService(service);
  }

  



  protected boolean backgroundDataIsEnabled() {
    ConnectivityManager connectivity = (ConnectivityManager) this.getSystemService(Context.CONNECTIVITY_SERVICE);
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
      return connectivity.getBackgroundDataSetting();
    }
    NetworkInfo networkInfo = connectivity.getActiveNetworkInfo();
    if (networkInfo == null) {
      return false;
    }
    return networkInfo.isAvailable();
  }

  protected AlarmManager getAlarmManager() {
    return getAlarmManager(this.getApplicationContext());
  }

  protected static AlarmManager getAlarmManager(Context context) {
    return (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
  }

  protected void scheduleAlarm(long pollInterval, PendingIntent pendingIntent) {
    Logger.info(LOG_TAG, "Setting inexact repeating alarm for interval " + pollInterval);
    if (pollInterval <= 0) {
        throw new IllegalArgumentException("pollInterval " + pollInterval + " must be positive");
    }
    final AlarmManager alarm = getAlarmManager();
    final long firstEvent = System.currentTimeMillis();
    alarm.setInexactRepeating(AlarmManager.RTC, firstEvent, pollInterval, pendingIntent);
  }

  protected void cancelAlarm(PendingIntent pendingIntent) {
    final AlarmManager alarm = getAlarmManager();
    alarm.cancel(pendingIntent);
    
    pendingIntent.cancel();
  }

  











  protected static void reflectContextToFennec(Context context, String className, String methodName) {
    
    try {
      Class<?> geckoPreferences = Class.forName(className);
      Method broadcastSnippetsPref = geckoPreferences.getMethod(methodName, Context.class);
      broadcastSnippetsPref.invoke(null, context);
      return;
    } catch (ClassNotFoundException e) {
      Logger.error(LOG_TAG, "Class " + className + " not found!");
      return;
    } catch (NoSuchMethodException e) {
      Logger.error(LOG_TAG, "Method " + className + "/" + methodName + " not found!");
      return;
    } catch (IllegalArgumentException e) {
      Logger.error(LOG_TAG, "Got exception invoking " + methodName + ".");
    } catch (IllegalAccessException e) {
      Logger.error(LOG_TAG, "Got exception invoking " + methodName + ".");
    } catch (InvocationTargetException e) {
      Logger.error(LOG_TAG, "Got exception invoking " + methodName + ".");
    }
  }
}
