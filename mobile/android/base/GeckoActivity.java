



package org.mozilla.gecko;

import android.content.ComponentName;
import android.content.Intent;
import android.support.v4.app.FragmentActivity;

interface GeckoActivityStatus {
    public boolean isGeckoActivityOpened();
    public boolean isFinishing();  
};

public class GeckoActivity extends FragmentActivity implements GeckoActivityStatus {
    
    private boolean mGeckoActivityOpened = false;

    @Override
    public void onPause() {
        super.onPause();

        if (getApplication() instanceof GeckoApplication) {
            ((GeckoApplication) getApplication()).onActivityPause(this);
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        if (getApplication() instanceof GeckoApplication) {
            ((GeckoApplication) getApplication()).onActivityResume(this);
            mGeckoActivityOpened = false;
        }
    }

    @Override
    public void onCreate(android.os.Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (AppConstants.MOZ_ANDROID_ANR_REPORTER) {
            ANRReporter.register(getApplicationContext());
        }
    }

    @Override
    public void onDestroy() {
        if (AppConstants.MOZ_ANDROID_ANR_REPORTER) {
            ANRReporter.unregister();
        }
        super.onDestroy();
    }

    @Override
    public void startActivity(Intent intent) {
        mGeckoActivityOpened = checkIfGeckoActivity(intent);
        super.startActivity(intent);
    }

    @Override
    public void startActivityForResult(Intent intent, int request) {
        mGeckoActivityOpened = checkIfGeckoActivity(intent);
        super.startActivityForResult(intent, request);
    }

    private static boolean checkIfGeckoActivity(Intent intent) {
        
        
        
        ComponentName component = intent.getComponent();
        return (component != null &&
                AppConstants.ANDROID_PACKAGE_NAME.equals(component.getPackageName()));
    }

    @Override
    public boolean isGeckoActivityOpened() {
        return mGeckoActivityOpened;
    }

    public boolean isApplicationInBackground() {
        return ((GeckoApplication) getApplication()).isApplicationInBackground();
    }

    @Override
    public void onLowMemory() {
        MemoryMonitor.getInstance().onLowMemory();
        super.onLowMemory();
    }

    @Override
    public void onTrimMemory(int level) {
        MemoryMonitor.getInstance().onTrimMemory(level);
        super.onTrimMemory(level);
    }

    public LightweightTheme getLightweightTheme() {
        return ((GeckoApplication) getApplication()).getLightweightTheme();
    }
}
