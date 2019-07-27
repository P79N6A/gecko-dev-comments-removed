package com.adjust.sdk;

import android.net.Uri;




public class AdjustInstance {

    private String referrer;
    private long referrerClickTime;
    private ActivityHandler activityHandler;

    private static ILogger getLogger() {
        return AdjustFactory.getLogger();
    }

    public void onCreate(AdjustConfig adjustConfig) {
        if (activityHandler != null) {
            getLogger().error("Adjust already initialized");
            return;
        }

        adjustConfig.referrer = this.referrer;
        adjustConfig.referrerClickTime = this.referrerClickTime;

        activityHandler = ActivityHandler.getInstance(adjustConfig);
    }

    public void trackEvent(AdjustEvent event) {
        if (!checkActivityHandler()) return;
        activityHandler.trackEvent(event);
    }

    public void onResume() {
        if (!checkActivityHandler()) return;
        activityHandler.trackSubsessionStart();
    }

    public void onPause() {
        if (!checkActivityHandler()) return;
        activityHandler.trackSubsessionEnd();
    }

    public void setEnabled(boolean enabled) {
        if (!checkActivityHandler()) return;
        activityHandler.setEnabled(enabled);
    }

    public boolean isEnabled() {
        if (!checkActivityHandler()) return false;
        return activityHandler.isEnabled();
    }

    public void appWillOpenUrl(Uri url) {
        if (!checkActivityHandler()) return;
        long clickTime = System.currentTimeMillis();
        activityHandler.readOpenUrl(url, clickTime);
    }

    public void sendReferrer(String referrer) {
        long clickTime = System.currentTimeMillis();
        
        if (activityHandler == null) {
            
            this.referrer = referrer;
            this.referrerClickTime = clickTime;
        } else {
            activityHandler.sendReferrer(referrer, clickTime);
        }
    }

    public void setOfflineMode(boolean enabled) {
        if (!checkActivityHandler()) return;
        activityHandler.setOfflineMode(enabled);
    }

    private boolean checkActivityHandler() {
        if (activityHandler == null) {
            getLogger().error("Please initialize Adjust by calling 'onCreate' before");
            return false;
        } else {
            return true;
        }
    }
}
