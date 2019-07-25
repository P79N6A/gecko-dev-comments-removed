




































package org.mozilla.gecko;

import java.lang.Math;

import android.util.Log;

import android.content.Context;

import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import android.telephony.TelephonyManager;












































public class GeckoNetworkManager
{
  static private final double  kDefaultBandwidth    = -1.0;
  static private final boolean kDefaultCanBeMetered = false;

  static private final double  kMaxBandwidth = 20.0;

  static private final double  kNetworkSpeed_2_G    = 15.0 / 1024.0;  
  static private final double  kNetworkSpeed_2_5_G  = 60.0 / 1024.0;  
  static private final double  kNetworkSpeed_2_75_G = 200.0 / 1024.0; 
  static private final double  kNetworkSpeed_3_G    = 300.0 / 1024.0; 
  static private final double  kNetworkSpeed_3_5_G  = 7.0;            
  static private final double  kNetworkSpeed_3_75_G = 20.0;           
  static private final double  kNetworkSpeed_3_9_G  = 50.0;           

  private enum MobileNetworkType {
    NETWORK_2_G,    
    NETWORK_2_5_G,  
    NETWORK_2_75_G, 
    NETWORK_3_G,    
    NETWORK_3_5_G,  
    NETWORK_3_75_G, 
    NETWORK_3_9_G,  
    NETWORK_UNKNOWN
  }

  private static MobileNetworkType getMobileNetworkType() {
    TelephonyManager tm =
      (TelephonyManager)GeckoApp.mAppContext.getSystemService(Context.TELEPHONY_SERVICE);

    switch (tm.getNetworkType()) {
      case TelephonyManager.NETWORK_TYPE_IDEN:
      case TelephonyManager.NETWORK_TYPE_CDMA:
        return MobileNetworkType.NETWORK_2_G;
      case TelephonyManager.NETWORK_TYPE_GPRS:
      case TelephonyManager.NETWORK_TYPE_1xRTT:
        return MobileNetworkType.NETWORK_2_5_G;
      case TelephonyManager.NETWORK_TYPE_EDGE:
        return MobileNetworkType.NETWORK_2_75_G;
      case TelephonyManager.NETWORK_TYPE_UMTS:
      case TelephonyManager.NETWORK_TYPE_EVDO_0:
        return MobileNetworkType.NETWORK_3_G;
      case TelephonyManager.NETWORK_TYPE_HSPA:
      case TelephonyManager.NETWORK_TYPE_HSDPA:
      case TelephonyManager.NETWORK_TYPE_HSUPA:
      case TelephonyManager.NETWORK_TYPE_EVDO_A:
      case TelephonyManager.NETWORK_TYPE_EVDO_B:
      case TelephonyManager.NETWORK_TYPE_EHRPD:
        return MobileNetworkType.NETWORK_3_5_G;
      case TelephonyManager.NETWORK_TYPE_HSPAP:
        return MobileNetworkType.NETWORK_3_75_G;
      case TelephonyManager.NETWORK_TYPE_LTE:
        return MobileNetworkType.NETWORK_3_9_G;
      case TelephonyManager.NETWORK_TYPE_UNKNOWN:
      default:
        Log.w("GeckoNetworkManager", "Connected to an unknown mobile network!");
        return MobileNetworkType.NETWORK_UNKNOWN;
    }
  }

  public static double getMobileNetworkSpeed(MobileNetworkType aType) {
    switch (aType) {
      case NETWORK_2_G:
        return kNetworkSpeed_2_G;
      case NETWORK_2_5_G:
        return kNetworkSpeed_2_5_G;
      case NETWORK_2_75_G:
        return kNetworkSpeed_2_75_G;
      case NETWORK_3_G:
        return kNetworkSpeed_3_G;
      case NETWORK_3_5_G:
        return kNetworkSpeed_3_5_G;
      case NETWORK_3_75_G:
        return kNetworkSpeed_3_75_G;
      case NETWORK_3_9_G:
        return kNetworkSpeed_3_9_G;
      case NETWORK_UNKNOWN:
      default:
        return kDefaultBandwidth;
    }
  }

  public static double[] getCurrentInformation() {
    ConnectivityManager cm =
      (ConnectivityManager)GeckoApp.mAppContext.getSystemService(Context.CONNECTIVITY_SERVICE);

    if (cm.getActiveNetworkInfo() == null) {
      return new double[] { 0.0, 1.0 };
    }

    int type = cm.getActiveNetworkInfo().getType();
    double bandwidth = kDefaultBandwidth;
    boolean canBeMetered = kDefaultCanBeMetered;

    switch (type) {
      case ConnectivityManager.TYPE_ETHERNET:
      case ConnectivityManager.TYPE_WIFI:
      case ConnectivityManager.TYPE_WIMAX:
        bandwidth = kMaxBandwidth;
        canBeMetered = false;
        break;
      case ConnectivityManager.TYPE_MOBILE:
        bandwidth = Math.min(getMobileNetworkSpeed(getMobileNetworkType()), kMaxBandwidth);
        canBeMetered = true;
        break;
      default:
        Log.w("GeckoNetworkManager", "Ignoring the current network type.");
        break;
    }

    return new double[] { bandwidth, canBeMetered ? 1.0 : 0.0 };
  }
}
