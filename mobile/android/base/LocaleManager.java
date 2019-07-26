



package org.mozilla.gecko;

import java.util.Locale;

import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;







public interface LocaleManager {
    void initialize(Context context);
    Locale getCurrentLocale(Context context);
    String getAndApplyPersistedLocale(Context context);
    void correctLocale(Context context, Resources resources, Configuration newConfig);
    void updateConfiguration(Context context, Locale locale);
    String setSelectedLocale(Context context, String localeCode);
    boolean systemLocaleDidChange();
    void resetToSystemLocale(Context context);

    





    Locale onSystemConfigurationChanged(Context context, Resources resources, Configuration configuration, Locale currentActivityLocale);
}
