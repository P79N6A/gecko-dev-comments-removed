




































package org.mozilla.gecko;

import java.lang.Math;

import android.util.Log;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import android.telephony.TelephonyManager;












































public class GeckoNetworkManager
  extends BroadcastReceiver
{
  static private final GeckoNetworkManager sInstance = new GeckoNetworkManager();

  static private final double  kDefaultBandwidth    = -1.0;
  static private final boolean kDefaultCanBeMetered = false;

  static private final double  kMaxBandwidth = 20.0;

  static private final double  kNetworkSpeedEthernet = 20.0;           
  static private final double  kNetworkSpeedWifi     = 20.0;           
  static private final double  kNetworkSpeedWiMax    = 40.0;           
  static private final double  kNetworkSpeed_2_G     = 15.0 / 1024.0;  
  static private final double  kNetworkSpeed_2_5_G   = 60.0 / 1024.0;  
  static private final double  kNetworkSpeed_2_75_G  = 200.0 / 1024.0; 
  static private final double  kNetworkSpeed_3_G     = 300.0 / 1024.0; 
  static private final double  kNetworkSpeed_3_5_G   = 7.0;            
  static private final double  kNetworkSpeed_3_75_G  = 20.0;           
  static private final double  kNetworkSpeed_3_9_G   = 50.0;           

  private enum NetworkType {
    NETWORK_NONE,
    NETWORK_ETHERNET,
    NETWORK_WIFI,
    NETWORK_WIMAX,
    NETWORK_2_G,    
    NETWORK_2_5_G,  
    NETWORK_2_75_G, 
    NETWORK_3_G,    
    NETWORK_3_5_G,  
    NETWORK_3_75_G, 
    NETWORK_3_9_G,  
    NETWORK_UNKNOWN
  }

  private NetworkType  mNetworkType = NetworkType.NETWORK_NONE;
  private IntentFilter mNetworkFilter = new IntentFilter();
  
  private boolean mShouldBeListening = false;
  
  
  private boolean mShouldNotify      = false;

  public static GeckoNetworkManager getInstance() {
    return sInstance;
  }

  @Override
  public void onReceive(Context aContext, Intent aIntent) {
    updateNetworkType();
  }

  public void init() {
    mNetworkFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);

    mNetworkType = getNetworkType();
  }

  public void start() {
    mShouldBeListening = true;
    updateNetworkType();

    if (mShouldNotify) {
      startListening();
    }
  }

  private void startListening() {
    GeckoApp.mAppContext.registerReceiver(sInstance, mNetworkFilter);
  }

  public void stop() {
    mShouldBeListening = false;

    if (mShouldNotify) {
      stopListening();
    }
  }

  private void stopListening() {
    GeckoApp.mAppContext.unregisterReceiver(sInstance);
  }

  private void updateNetworkType() {
    NetworkType previousNetworkType = mNetworkType;
    mNetworkType = getNetworkType();

    if (mNetworkType == previousNetworkType || !mShouldNotify) {
      return;
    }

    GeckoAppShell.sendEventToGecko(GeckoEvent.createNetworkEvent(
                                       getNetworkSpeed(mNetworkType),
                                       isNetworkUsuallyMetered(mNetworkType)));
  }

  public double[] getCurrentInformation() {
    return new double[] { getNetworkSpeed(mNetworkType),
                          isNetworkUsuallyMetered(mNetworkType) ? 1.0 : 0.0 };
  }

  public void enableNotifications() {
    
    
    updateNetworkType();
    mShouldNotify = true;

    if (mShouldBeListening) {
      startListening();
    }
  }

  public void disableNotifications() {
    mShouldNotify = false;

    if (mShouldBeListening) {
      stopListening();
    }
  }

  private static NetworkType getNetworkType() {
    ConnectivityManager cm =
      (ConnectivityManager)GeckoApp.mAppContext.getSystemService(Context.CONNECTIVITY_SERVICE);

    if (cm.getActiveNetworkInfo() == null) {
      return NetworkType.NETWORK_NONE;
    }

    switch (cm.getActiveNetworkInfo().getType()) {
      case ConnectivityManager.TYPE_ETHERNET:
        return NetworkType.NETWORK_ETHERNET;
      case ConnectivityManager.TYPE_WIFI:
        return NetworkType.NETWORK_WIFI;
      case ConnectivityManager.TYPE_WIMAX:
        return NetworkType.NETWORK_WIMAX;
      case ConnectivityManager.TYPE_MOBILE:
        break; 
      default:
        Log.w("GeckoNetworkManager", "Ignoring the current network type.");
        return NetworkType.NETWORK_UNKNOWN;
    }

    TelephonyManager tm =
      (TelephonyManager)GeckoApp.mAppContext.getSystemService(Context.TELEPHONY_SERVICE);

    switch (tm.getNetworkType()) {
      case TelephonyManager.NETWORK_TYPE_IDEN:
      case TelephonyManager.NETWORK_TYPE_CDMA:
        return NetworkType.NETWORK_2_G;
      case TelephonyManager.NETWORK_TYPE_GPRS:
      case TelephonyManager.NETWORK_TYPE_1xRTT:
        return NetworkType.NETWORK_2_5_G;
      case TelephonyManager.NETWORK_TYPE_EDGE:
        return NetworkType.NETWORK_2_75_G;
      case TelephonyManager.NETWORK_TYPE_UMTS:
      case TelephonyManager.NETWORK_TYPE_EVDO_0:
        return NetworkType.NETWORK_3_G;
      case TelephonyManager.NETWORK_TYPE_HSPA:
      case TelephonyManager.NETWORK_TYPE_HSDPA:
      case TelephonyManager.NETWORK_TYPE_HSUPA:
      case TelephonyManager.NETWORK_TYPE_EVDO_A:
      case TelephonyManager.NETWORK_TYPE_EVDO_B:
      case TelephonyManager.NETWORK_TYPE_EHRPD:
        return NetworkType.NETWORK_3_5_G;
      case TelephonyManager.NETWORK_TYPE_HSPAP:
        return NetworkType.NETWORK_3_75_G;
      case TelephonyManager.NETWORK_TYPE_LTE:
        return NetworkType.NETWORK_3_9_G;
      case TelephonyManager.NETWORK_TYPE_UNKNOWN:
      default:
        Log.w("GeckoNetworkManager", "Connected to an unknown mobile network!");
        return NetworkType.NETWORK_UNKNOWN;
    }
  }

  private static double getNetworkSpeed(NetworkType aType) {
    switch (aType) {
      case NETWORK_NONE:
        return 0.0;
      case NETWORK_ETHERNET:
        return kNetworkSpeedEthernet;
      case NETWORK_WIFI:
        return kNetworkSpeedWifi;
      case NETWORK_WIMAX:
        return kNetworkSpeedWiMax;
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

  private static boolean isNetworkUsuallyMetered(NetworkType aType) {
    switch (aType) {
      case NETWORK_NONE:
      case NETWORK_UNKNOWN:
      case NETWORK_ETHERNET:
      case NETWORK_WIFI:
      case NETWORK_WIMAX:
        return false;
      case NETWORK_2_G:
      case NETWORK_2_5_G:
      case NETWORK_2_75_G:
      case NETWORK_3_G:
      case NETWORK_3_5_G:
      case NETWORK_3_75_G:
      case NETWORK_3_9_G:
        return true;
      default:
        Log.e("GeckoNetworkManager", "Got an unexpected network type!");
        return false;
    }
  }
}
