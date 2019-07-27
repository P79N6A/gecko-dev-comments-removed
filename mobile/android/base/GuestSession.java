



package org.mozilla.gecko;

import android.app.KeyguardManager;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.support.v4.app.NotificationCompat;
import android.view.Window;
import android.view.WindowManager;


public class GuestSession {
    public static final String NOTIFICATION_INTENT = "org.mozilla.gecko.GUEST_SESSION_INPROGRESS";
    private static final String LOGTAG = "GeckoGuestSession";

    
    static boolean isSecureKeyguardLocked(Context context) {
        final KeyguardManager manager = (KeyguardManager) context.getSystemService(Context.KEYGUARD_SERVICE);

        
        if (AppConstants.Versions.preJB || manager == null) {
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
        
        window.addFlags(WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD | WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
    }

    public static void unconfigureWindow(Window window) {
        
        window.clearFlags(WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD | WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
    }

    private static PendingIntent getNotificationIntent(Context context) {
        Intent intent = new Intent(NOTIFICATION_INTENT);
        intent.setClass(context, BrowserApp.class);
        return PendingIntent.getActivity(context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
    }

    public static void showNotification(Context context) {
        final NotificationCompat.Builder builder = new NotificationCompat.Builder(context);
        final Resources res = context.getResources();
        builder.setContentTitle(res.getString(R.string.guest_browsing_notification_title))
               .setContentText(res.getString(R.string.guest_browsing_notification_text))
               .setSmallIcon(R.drawable.alert_guest)
               .setOngoing(true)
               .setContentIntent(getNotificationIntent(context));

        final NotificationManager manager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        manager.notify(R.id.guestNotification, builder.build());
    }

    public static void hideNotification(Context context) {
        final NotificationManager manager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        manager.cancel(R.id.guestNotification);
    }

    public static void handleIntent(BrowserApp context, Intent intent) {
        context.showGuestModeDialog(BrowserApp.GuestModeDialog.LEAVING);
    }

}
