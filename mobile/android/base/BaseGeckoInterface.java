




package org.mozilla.gecko;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.prompts.PromptService;
import org.mozilla.gecko.util.ActivityUtils;
import org.mozilla.gecko.util.HardwareUtils;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.Activity;
import android.content.Context;
import android.graphics.RectF;
import android.hardware.SensorEventListener;
import android.location.LocationListener;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AbsoluteLayout;

public class BaseGeckoInterface implements GeckoAppShell.GeckoInterface {
    
    
    
    
    

    private final Context mContext;
    private GeckoProfile mProfile;

    public BaseGeckoInterface(Context context) {
        mContext = context;
    }

    @Override
    public GeckoProfile getProfile() {
        
        if (mProfile == null) {
            mProfile = GeckoProfile.get(mContext);
        }
        return mProfile;
    }

    
    @Override
    public PromptService getPromptService() {
        return null;
    }

    @Override
    public Activity getActivity() {
        return (Activity)mContext;
    }

    @Override
    public String getDefaultUAString() {
        return HardwareUtils.isTablet() ? AppConstants.USER_AGENT_FENNEC_TABLET :
                                          AppConstants.USER_AGENT_FENNEC_MOBILE;
    }

    
    @Override
    public LocationListener getLocationListener() {
        return null;
    }

    
    @Override
    public SensorEventListener getSensorEventListener() {
        return null;
    }

    
    @Override
    public void doRestart() {}

    @Override
    public void setFullScreen(final boolean fullscreen) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                ActivityUtils.setFullScreen(getActivity(), fullscreen);
            }
        });
    }

    
    @Override
    public void addPluginView(final View view, final RectF rect, final boolean isFullScreen) {}

    
    @Override
    public void removePluginView(final View view, final boolean isFullScreen) {}

    
    @Override
    public void enableCameraView() {}

    
    @Override
    public void disableCameraView() {}

    
    @Override
    public void addAppStateListener(GeckoAppShell.AppStateListener listener) {}

    
    @Override
    public void removeAppStateListener(GeckoAppShell.AppStateListener listener) {}

    
    @Override
    public View getCameraView() {
        return null;
    }

    
    @Override
    public void notifyWakeLockChanged(String topic, String state) {}

    
    @Override
    public FormAssistPopup getFormAssistPopup() {
        return null;
    }

    @Override
    public boolean areTabsShown() {
        return false;
    }

    
    @Override
    public AbsoluteLayout getPluginContainer() {
        return null;
    }

    @Override
    public void notifyCheckUpdateResult(String result) {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Update:CheckResult", result));
    }

    
    @Override
    public void invalidateOptionsMenu() {}
}
