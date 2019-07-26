




package org.mozilla.gecko.health;

import android.content.SharedPreferences;




public interface HealthRecorder {
    public void setCurrentSession(SessionInformation session);
    public void recordSessionEnd(String reason, SharedPreferences.Editor editor);

    public void recordGeckoStartupTime(long duration);
    public void recordJavaStartupTime(long duration);

    public void onAppLocaleChanged(String to);
    public void onEnvironmentChanged(final boolean startNewSession, final String sessionEndReason);

    public void close();
}
