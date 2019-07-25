



package org.mozilla.gecko;

import android.content.Context;
import android.util.Log;
import android.view.OrientationEventListener;
import android.view.Surface;

public class GeckoScreenOrientationListener
{
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
      Log.e("GeckoScreenOrientationListener", "Unexpected value received! (" + rotation + ")");
      return;
    }

    if (mShouldNotify && mOrientation != previousOrientation) {
      GeckoAppShell.sendEventToGecko(new GeckoEvent(mOrientation));
    }
  }

  public short getScreenOrientation() {
    return mOrientation;
  }
}
