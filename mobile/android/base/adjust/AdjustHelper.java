




package org.mozilla.gecko.adjust;

import android.content.Context;
import android.content.Intent;

import com.adjust.sdk.Adjust;
import com.adjust.sdk.AdjustConfig;
import com.adjust.sdk.AdjustReferrerReceiver;
import com.adjust.sdk.LogLevel;

public class AdjustHelper implements AdjustHelperInterface {
    public void onCreate(final Context context, final String maybeAppToken) {
        final String environment;
        final String appToken;
        if (maybeAppToken != null) {
            environment = AdjustConfig.ENVIRONMENT_PRODUCTION;
            appToken = maybeAppToken;
        } else {
            environment = AdjustConfig.ENVIRONMENT_SANDBOX;
            appToken = "ABCDEFGHIJKL";
        }
        AdjustConfig config = new AdjustConfig(context, appToken, environment);
        config.setLogLevel(LogLevel.VERBOSE);
        Adjust.onCreate(config);
    }

    public void onReceive(final Context context, final Intent intent) {
        new AdjustReferrerReceiver().onReceive(context, intent);
    }
}
