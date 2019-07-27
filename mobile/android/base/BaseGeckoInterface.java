




package org.mozilla.gecko;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.prompts.PromptService;
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

    public GeckoProfile getProfile() {
        
        if (mProfile == null) {
            mProfile = GeckoProfile.get(mContext);
        }
        return mProfile;
    }

    
    public PromptService getPromptService() {
        return null;
    }

    public Activity getActivity() {
        return (Activity)mContext;
    }

    public String getDefaultUAString() {
        return HardwareUtils.isTablet() ? AppConstants.USER_AGENT_FENNEC_TABLET :
                                          AppConstants.USER_AGENT_FENNEC_MOBILE;
    }

    
    public LocationListener getLocationListener() {
        return null;
    }

    
    public SensorEventListener getSensorEventListener() {
        return null;
    }

    
    public void doRestart() {}

    public void setFullScreen(final boolean fullscreen) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                
                Window window = getActivity().getWindow();
                window.setFlags(fullscreen ?
                                WindowManager.LayoutParams.FLAG_FULLSCREEN : 0,
                                WindowManager.LayoutParams.FLAG_FULLSCREEN);

                if (Versions.feature11Plus) {
                    window.getDecorView().setSystemUiVisibility(fullscreen ? 1 : 0);
                }
            }
        });
    }

    
    public void addPluginView(final View view, final RectF rect, final boolean isFullScreen) {}

    
    public void removePluginView(final View view, final boolean isFullScreen) {}

    
    public void enableCameraView() {}

    
    public void disableCameraView() {}

    
    public void addAppStateListener(GeckoAppShell.AppStateListener listener) {}

    
    public void removeAppStateListener(GeckoAppShell.AppStateListener listener) {}

    
    public View getCameraView() {
        return null;
    }

    
    public void notifyWakeLockChanged(String topic, String state) {}

    
    public FormAssistPopup getFormAssistPopup() {
        return null;
    }

    public boolean areTabsShown() {
        return false;
    }

    
    public AbsoluteLayout getPluginContainer() {
        return null;
    }

    public void notifyCheckUpdateResult(String result) {
        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Update:CheckResult", result));
    }

    public boolean hasTabsSideBar() {
        return false;
    }

    
    public void invalidateOptionsMenu() {}
}
