




package org.mozilla.gecko.health;

import android.content.SharedPreferences;

import org.json.JSONObject;





public class StubbedHealthRecorder implements HealthRecorder {
    public boolean isEnabled() { return false; }

    public void setCurrentSession(SessionInformation session) { }
    public void checkForOrphanSessions() { }

    public void recordGeckoStartupTime(long duration) { }
    public void recordJavaStartupTime(long duration) { }
    public void recordSearch(final String engineID, final String location) { }
    public void recordSessionEnd(String reason, SharedPreferences.Editor editor) { }
    public void recordSessionEnd(String reason, SharedPreferences.Editor editor, final int environment) { }

    public void onAppLocaleChanged(String to) { }
    public void onAddonChanged(String id, JSONObject json) { }
    public void onAddonUninstalling(String id) { }
    public void onEnvironmentChanged() { }
    public void onEnvironmentChanged(final boolean startNewSession, final String sessionEndReason) { }

    public void close() { }
}
