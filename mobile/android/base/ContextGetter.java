




package org.mozilla.gecko;

import android.content.Context;
import android.content.SharedPreferences;

public interface ContextGetter {
    Context getContext();
    SharedPreferences getSharedPreferences();
}

