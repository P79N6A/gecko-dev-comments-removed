




































package org.mozilla.gecko;

import java.lang.Math;
import java.util.Date;

import android.util.Log;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import android.os.BatteryManager;

public class GeckoBatteryManager
  extends BroadcastReceiver
{
    private static final String LOGTAG = "GeckoBatteryManager";

  
  
  private final static double  kDefaultLevel         = 1.0;
  private final static boolean kDefaultCharging      = true;
  private final static double  kDefaultRemainingTime = -1.0;
  private final static double  kUnknownRemainingTime = -1.0;

  private static Date    sLastLevelChange            = new Date(0);
  private static boolean sNotificationsEnabled       = false;
  private static double  sLevel                      = kDefaultLevel;
  private static boolean sCharging                   = kDefaultCharging;
  private static double  sRemainingTime              = kDefaultRemainingTime;;

  @Override
  public void onReceive(Context context, Intent intent) {
    if (!intent.getAction().equals(Intent.ACTION_BATTERY_CHANGED)) {
      Log.e(LOGTAG, "Got an unexpected intent!");
      return;
    }

    boolean previousCharging = isCharging();
    double previousLevel = getLevel();

    if (intent.getBooleanExtra(BatteryManager.EXTRA_PRESENT, false)) {
      int plugged = intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);
      if (plugged == -1) {
        sCharging = kDefaultCharging;
        Log.e(LOGTAG, "Failed to get the plugged status!");
      } else {
        
        
        sCharging = plugged != 0;
      }

      if (sCharging != previousCharging) {
        sRemainingTime = kUnknownRemainingTime;
        
        
        sLastLevelChange = new Date(0);
      }

      
      double current =  (double)intent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
      double max = (double)intent.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
      if (current == -1 || max == -1) {
        Log.e(LOGTAG, "Failed to get battery level!");
        sLevel = kDefaultLevel;
      } else {
        sLevel = current / max;
      }

      if (sLevel == 1.0 && sCharging) {
        sRemainingTime = 0.0;
      } else if (sLevel != previousLevel) {
        
        if (sLastLevelChange.getTime() != 0) {
          Date currentTime = new Date();
          long dt = (currentTime.getTime() - sLastLevelChange.getTime()) / 1000;
          double dLevel = sLevel - previousLevel;

          if (sCharging) {
            if (dLevel < 0) {
              Log.w(LOGTAG, "When charging, level should increase!");
              sRemainingTime = kUnknownRemainingTime;
            } else {
              sRemainingTime = Math.round(dt / dLevel * (1.0 - sLevel));
            }
          } else {
            if (dLevel > 0) {
              Log.w(LOGTAG, "When discharging, level should decrease!");
              sRemainingTime = kUnknownRemainingTime;
            } else {
              sRemainingTime = Math.round(dt / -dLevel * sLevel);
            }
          }

          sLastLevelChange = currentTime;
        } else {
          
          sLastLevelChange = new Date();
        }
      }
    } else {
      sLevel = kDefaultLevel;
      sCharging = kDefaultCharging;
      sRemainingTime = kDefaultRemainingTime;
    }

    










    if (sNotificationsEnabled &&
        (previousCharging != isCharging() || previousLevel != getLevel())) {
      GeckoAppShell.notifyBatteryChange(getLevel(), isCharging(), getRemainingTime());
    }
  }

  public static boolean isCharging() {
    return sCharging;
  }

  public static double getLevel() {
    return sLevel;
  }

  public static double getRemainingTime() {
    return sRemainingTime;
  }

  public static void enableNotifications() {
    sNotificationsEnabled = true;
  }

  public static void disableNotifications() {
    sNotificationsEnabled = false;
  }

  public static double[] getCurrentInformation() {
    return new double[] { getLevel(), isCharging() ? 1.0 : 0.0, getRemainingTime() };
  }
}
