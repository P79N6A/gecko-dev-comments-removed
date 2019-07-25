




































package org.mozilla.gecko;

import java.lang.Math;

import android.util.Log;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import android.os.BatteryManager;

public class GeckoBatteryManager
  extends BroadcastReceiver
{
  private final static float   kDefaultLevel       = 1.0f;
  private final static boolean kDefaultCharging    = true;
  private final static float   kMinimumLevelChange = 0.01f;

  private static boolean sNotificationsEnabled     = false;
  private static float   sLevel                    = kDefaultLevel;
  private static boolean sCharging                 = kDefaultCharging;

  @Override
  public void onReceive(Context context, Intent intent) {
    if (!intent.getAction().equals(Intent.ACTION_BATTERY_CHANGED)) {
      Log.e("GeckoBatteryManager", "Got an unexpected intent!");
      return;
    }

    boolean previousCharging = isCharging();
    float previousLevel = getLevel();

    if (intent.getBooleanExtra(BatteryManager.EXTRA_PRESENT, false)) {
      int plugged = intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);
      if (plugged == -1) {
        sCharging = kDefaultCharging;
        Log.e("GeckoBatteryManager", "Failed to get the plugged status!");
      } else {
        
        
        sCharging = plugged != 0;
      }

      
      float current =  (float)intent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
      float max = (float)intent.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
      if (current == -1 || max == -1) {
        Log.e("GeckoBatteryManager", "Failed to get battery level!");
        sLevel = kDefaultLevel;
      } else {
        sLevel = current / max;
      }
    } else {
      sLevel = kDefaultLevel;
      sCharging = kDefaultCharging;
    }

    






    if (sNotificationsEnabled &&
        (previousCharging != isCharging() ||
         Math.abs(previousLevel - getLevel()) >= kMinimumLevelChange)) {
      GeckoAppShell.notifyBatteryChange(sLevel, sCharging);
    }
  }

  public static boolean isCharging() {
    return sCharging;
  }

  public static float getLevel() {
    return sLevel;
  }

  public static void enableNotifications() {
    sNotificationsEnabled = true;
  }

  public static void disableNotifications() {
    sNotificationsEnabled = false;
  }

  public static float[] getCurrentInformation() {
    return new float[] { getLevel(), isCharging() ? 1.0f : 0.0f };
  }
}
