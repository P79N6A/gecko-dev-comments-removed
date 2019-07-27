




package org.mozilla.gecko.health;

import android.content.SharedPreferences;

import org.json.JSONObject;





public class StubbedHealthRecorder implements HealthRecorder {
    @Override
    public boolean isEnabled() { return false; }

    @Override
    public void setCurrentSession(SessionInformation session) { }
    @Override
    public void checkForOrphanSessions() { }

    @Override
    public void recordGeckoStartupTime(long duration) { }
    @Override
    public void recordJavaStartupTime(long duration) { }
    @Override
    public void recordSearch(final String engineID, final String location) { }
    @Override
    public void recordSessionEnd(String reason, SharedPreferences.Editor editor) { }
    @Override
    public void recordSessionEnd(String reason, SharedPreferences.Editor editor, final int environment) { }

    @Override
    public void onAppLocaleChanged(String to) { }
    @Override
    public void onAddonChanged(String id, JSONObject json) { }
    @Override
    public void onAddonUninstalling(String id) { }
    @Override
    public void onEnvironmentChanged() { }
    @Override
    public void onEnvironmentChanged(final boolean startNewSession, final String sessionEndReason) { }

    @Override
    public void close() { }

    @Override
    public void processDelayed() { }
}
