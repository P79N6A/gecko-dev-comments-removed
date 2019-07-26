



package org.mozilla.gecko;

import android.content.Context;
import android.util.Log;
import android.view.OrientationEventListener;
import android.view.Surface;
import android.content.pm.ActivityInfo;

public class GeckoScreenOrientationListener
{
  private static final String LOGTAG = "GeckoScreenOrientationListener";

  static class OrientationEventListenerImpl extends OrientationEventListener {
    public OrientationEventListenerImpl(Context c) {
      super(c);
    }

    @Override
    public void onOrientationChanged(int aOrientation) {
      GeckoScreenOrientationListener.getInstance().updateScreenOrientation();
    }
  }

  static private GeckoScreenOrientationListener sInstance = null;

  
  static public final short eScreenOrientation_None               = 0;
  static public final short eScreenOrientation_PortraitPrimary    = 1; 
  static public final short eScreenOrientation_PortraitSecondary  = 2; 
  static public final short eScreenOrientation_LandscapePrimary   = 4; 
  static public final short eScreenOrientation_LandscapeSecondary = 8; 

  private short mOrientation;
  private OrientationEventListenerImpl mListener = null;

  
  private boolean mShouldBeListening = false;
  
  private boolean mShouldNotify      = false;

  private GeckoScreenOrientationListener() {
    mListener = new OrientationEventListenerImpl(GeckoApp.mAppContext);
  }

  public static GeckoScreenOrientationListener getInstance() {
    if (sInstance == null) {
      sInstance = new GeckoScreenOrientationListener();
    }

    return sInstance;
  }

  public void start() {
    mShouldBeListening = true;
    updateScreenOrientation();

    if (mShouldNotify) {
      startListening();
    }
  }

  public void stop() {
    mShouldBeListening = false;

    if (mShouldNotify) {
      stopListening();
    }
  }

  public void enableNotifications() {
    updateScreenOrientation();
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

  private void startListening() {
    mListener.enable();
  }

  private void stopListening() {
    mListener.disable();
  }

  
  
  public void updateScreenOrientation() {
    int rotation = GeckoApp.mAppContext.getWindowManager().getDefaultDisplay().getRotation();
    short previousOrientation = mOrientation;

    if (rotation == Surface.ROTATION_0) {
      mOrientation = eScreenOrientation_PortraitPrimary;
    } else if (rotation == Surface.ROTATION_180) {
      mOrientation = eScreenOrientation_PortraitSecondary;
    } else if (rotation == Surface.ROTATION_270) {
      mOrientation = eScreenOrientation_LandscapeSecondary;
    } else if (rotation == Surface.ROTATION_90) {
      mOrientation = eScreenOrientation_LandscapePrimary;
    } else {
      Log.e(LOGTAG, "Unexpected value received! (" + rotation + ")");
      return;
    }

    if (mShouldNotify && mOrientation != previousOrientation) {
      GeckoAppShell.sendEventToGecko(new GeckoEvent(mOrientation));
    }
  }

  public short getScreenOrientation() {
    return mOrientation;
  }

  public void lockScreenOrientation(int aOrientation) {
    int orientation = 0;

    switch (aOrientation) {
      case eScreenOrientation_PortraitPrimary:
        orientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
        break;
      case eScreenOrientation_PortraitSecondary:
        orientation = ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT;
        break;
      case eScreenOrientation_PortraitPrimary | eScreenOrientation_PortraitSecondary:
        orientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;
        break;
      case eScreenOrientation_LandscapePrimary:
        orientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
        break;
      case eScreenOrientation_LandscapeSecondary:
        orientation = ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
        break;
      case eScreenOrientation_LandscapePrimary | eScreenOrientation_LandscapeSecondary:
        orientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
        break;
      default:
        Log.e(LOGTAG, "Unexpected value received! (" + aOrientation + ")");
    }

    GeckoApp.mAppContext.setRequestedOrientation(orientation);
    updateScreenOrientation();
  }

  public void unlockScreenOrientation() {
    GeckoApp.mAppContext.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR);
    updateScreenOrientation();
  }
}
