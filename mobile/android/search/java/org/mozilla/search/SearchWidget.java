




package org.mozilla.search;

import org.mozilla.gecko.AboutPages;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.TelemetryContract;

import android.annotation.SuppressLint;
import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.appwidget.AppWidgetProviderInfo;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.widget.RemoteViews;




public class SearchWidget extends AppWidgetProvider {
    final private static String LOGTAG = "GeckoSearchWidget";

    final public static String ACTION_LAUNCH_BROWSER = "org.mozilla.widget.LAUNCH_BROWSER";
    final public static String ACTION_LAUNCH_SEARCH = "org.mozilla.widget.LAUNCH_SEARCH";
    final public static String ACTION_LAUNCH_NEW_TAB = "org.mozilla.widget.LAUNCH_NEW_TAB";

    @SuppressLint("NewApi")
    @Override
    public void onUpdate(final Context context, final AppWidgetManager manager, final int[] ids) {
        for (int id : ids) {
            final Bundle bundle;
            if (AppConstants.Versions.feature16Plus) {
                bundle = manager.getAppWidgetOptions(id);
            } else {
                bundle = null;
            }
            addView(manager, context, id, bundle);
        }

        super.onUpdate(context, manager, ids);
    }

    @SuppressLint("NewApi")
    @Override
    public void onAppWidgetOptionsChanged(final Context context,
                                          final AppWidgetManager manager,
                                          final int id,
                                          final Bundle options) {
        addView(manager, context, id, options);
        if (AppConstants.Versions.feature16Plus) {
            super.onAppWidgetOptionsChanged(context, manager, id, options);
        }
    }

    @Override
    public void onReceive(final Context context, final Intent intent) {
        
        final Intent redirect;
        switch (intent.getAction()) {
            case ACTION_LAUNCH_BROWSER:
                redirect = buildRedirectIntent(Intent.ACTION_MAIN,
                                               AppConstants.ANDROID_PACKAGE_NAME,
                                               AppConstants.MOZ_ANDROID_BROWSER_INTENT_CLASS,
                                               intent);
                Telemetry.sendUIEvent(TelemetryContract.Event.LAUNCH,
                                      TelemetryContract.Method.WIDGET, "browser");
                break;
            case ACTION_LAUNCH_NEW_TAB:
                redirect = buildRedirectIntent(Intent.ACTION_VIEW,
                                               AppConstants.ANDROID_PACKAGE_NAME,
                                               AppConstants.MOZ_ANDROID_BROWSER_INTENT_CLASS,
                                               intent);
                Telemetry.sendUIEvent(TelemetryContract.Event.LAUNCH,
                                      TelemetryContract.Method.WIDGET, "new-tab");
                break;
            case ACTION_LAUNCH_SEARCH:
                redirect = buildRedirectIntent(Intent.ACTION_VIEW,
                                               AppConstants.ANDROID_PACKAGE_NAME,
                                               AppConstants.MOZ_ANDROID_SEARCH_INTENT_CLASS,
                                               intent);
                Telemetry.sendUIEvent(TelemetryContract.Event.LAUNCH,
                                      TelemetryContract.Method.WIDGET, "search");
                break;
            default:
                redirect = null;
        }

        if (redirect != null) {
            context.startActivity(redirect);
        }

        super.onReceive(context, intent);
    }

    
    private void addView(final AppWidgetManager manager, final Context context, final int id, final Bundle options) {
        final RemoteViews views = new RemoteViews(context.getPackageName(), R.layout.search_widget);

        addClickIntent(context, views, R.id.search_button, ACTION_LAUNCH_SEARCH);
        addClickIntent(context, views, R.id.new_tab_button, ACTION_LAUNCH_NEW_TAB);
        
        addClickIntent(context, views, R.id.logo_button, ACTION_LAUNCH_BROWSER);

        manager.updateAppWidget(id, views);
    }

    
    private void addClickIntent(final Context context, final RemoteViews views, final int viewId, final String action) {
        final Intent intent = new Intent(context, SearchWidget.class);
        intent.setAction(action);
        intent.setData(Uri.parse(AboutPages.HOME));
        final PendingIntent pendingIntent = PendingIntent.getBroadcast(context, 0, intent, 0);
        views.setOnClickPendingIntent(viewId, pendingIntent);
    }

    
    private Intent buildRedirectIntent(final String action, final String pkg, final String className, final Intent source) {
        final Intent activity = new Intent(action);
        if (pkg != null && className != null) {
            activity.setClassName(pkg, className);
        }
        activity.setData(source.getData());
        activity.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        return activity;
    }

}
