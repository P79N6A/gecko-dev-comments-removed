




package org.mozilla.gecko;

import org.mozilla.gecko.prompts.Prompt;
import org.mozilla.gecko.util.EventCallback;
import org.mozilla.gecko.util.NativeEventListener;
import org.mozilla.gecko.util.NativeJSObject;
import org.mozilla.gecko.util.ThreadUtils;

import java.io.File;

import org.json.JSONException;
import org.json.JSONObject;

import android.app.KeyguardManager;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ListView;


public final class GuestSession {
    private static final String LOGTAG = "GeckoGuestSession";

    
    static boolean isSecureKeyguardLocked(Context context) {
        final KeyguardManager manager = (KeyguardManager) context.getSystemService(Context.KEYGUARD_SERVICE);

        if (AppConstants.Versions.preJB) {
            return false;
        }

        return manager.isKeyguardLocked() && manager.isKeyguardSecure();
    }

    




    public static boolean shouldUse(final Context context, final String args) {
        
        if (args != null && args.contains(BrowserApp.GUEST_BROWSING_ARG)) {
            return true;
        }

        
        final boolean keyguard = isSecureKeyguardLocked(context);
        if (keyguard) {
            return true;
        }

        
        final GeckoProfile profile = GeckoProfile.getGuestProfile(context);
        if (profile == null) {
            return false;
        }

        return profile.locked();
    }

    public static void configureWindow(Window window) {
        
        window.addFlags(WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD);
        window.addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
    }
}
