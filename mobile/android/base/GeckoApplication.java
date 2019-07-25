



package org.mozilla.gecko;

import java.util.ArrayList;

import android.app.Activity;
import android.app.Application;

public class GeckoApplication extends Application {

    private boolean mInBackground = false;
    private ArrayList<ApplicationLifecycleCallbacks> mListeners;

    @Override
    public void onCreate() {
        
        try {
            Class.forName("android.os.AsyncTask");
        } catch (ClassNotFoundException e) {}

        super.onCreate();
    }

    public interface ApplicationLifecycleCallbacks {
        public void onApplicationPause();
        public void onApplicationResume();
    }

    public void addApplicationLifecycleCallbacks(ApplicationLifecycleCallbacks callback) {
        if (mListeners == null)
            mListeners = new ArrayList<ApplicationLifecycleCallbacks>();

        mListeners.add(callback);
    }

    public void removeApplicationLifecycleCallbacks(ApplicationLifecycleCallbacks callback) {
        if (mListeners == null)
            return;

        mListeners.remove(callback);
    }

    public void onActivityPause(GeckoActivity activity) {
        if (activity.isGeckoActivityOpened())
            return;

        if (mListeners == null)
            return;

        mInBackground = true;

        for (ApplicationLifecycleCallbacks listener: mListeners)
            listener.onApplicationPause();
    }

    public void onActivityResume(GeckoActivity activity) {
        
        if (activity.isGeckoActivityOpened())
            return;

        if (mListeners == null)
            return;

        for (ApplicationLifecycleCallbacks listener: mListeners)
            listener.onApplicationResume();

        mInBackground = false;
    }

    public boolean isApplicationInBackground() {
        return mInBackground;
    }
}
