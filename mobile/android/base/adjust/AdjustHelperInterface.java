




package org.mozilla.gecko.adjust;

import android.content.Context;
import android.content.Intent;

public interface AdjustHelperInterface {
    



    void onCreate(final Context context, final String appToken);
    void onReceive(final Context context, final Intent intent);
}
