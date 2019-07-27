




package org.mozilla.gecko.tabqueue;

import org.mozilla.gecko.BrowserApp;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.R;
import org.mozilla.gecko.mozglue.ContextUtils;
import org.mozilla.gecko.preferences.GeckoPreferences;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.PixelFormat;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.text.TextUtils;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;






















public class TabQueueService extends Service {
    private static final String LOGTAG = "Gecko" + TabQueueService.class.getSimpleName();

    private static final long TOAST_TIMEOUT = 3000;
    private static final long TOAST_DOUBLE_TAP_TIMEOUT_MILLIS = 6000;

    private WindowManager windowManager;
    private View toastLayout;
    private Button openNowButton;
    private Handler tabQueueHandler;
    private WindowManager.LayoutParams toastLayoutParams;
    private volatile StopServiceRunnable stopServiceRunnable;
    private HandlerThread handlerThread;
    private ExecutorService executorService;

    @Override
    public IBinder onBind(Intent intent) {
        
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        executorService = Executors.newSingleThreadExecutor();

        handlerThread = new HandlerThread("TabQueueHandlerThread");
        handlerThread.start();
        tabQueueHandler = new Handler(handlerThread.getLooper());

        windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);

        LayoutInflater layoutInflater = (LayoutInflater) getSystemService(LAYOUT_INFLATER_SERVICE);
        toastLayout = layoutInflater.inflate(R.layout.tab_queue_toast, null);

        final Resources resources = getResources();

        TextView messageView = (TextView) toastLayout.findViewById(R.id.toast_message);
        messageView.setText(resources.getText(R.string.tab_queue_toast_message));

        openNowButton = (Button) toastLayout.findViewById(R.id.toast_button);
        openNowButton.setText(resources.getText(R.string.tab_queue_toast_action));

        toastLayoutParams = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.WRAP_CONTENT,
                WindowManager.LayoutParams.TYPE_PHONE,
                WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL |
                        WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH |
                        WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
                PixelFormat.TRANSLUCENT);

        toastLayoutParams.gravity = Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL;
    }

    @Override
    public int onStartCommand(final Intent intent, final int flags, final int startId) {
        
        
        if (flags != START_FLAG_REDELIVERY) {
            final Context applicationContext = getApplicationContext();
            final SharedPreferences sharedPreferences = GeckoSharedPrefs.forApp(applicationContext);

            final String lastUrl = sharedPreferences.getString(GeckoPreferences.PREFS_TAB_QUEUE_LAST_SITE, "");

            final ContextUtils.SafeIntent safeIntent = new ContextUtils.SafeIntent(intent);
            final String intentUrl = safeIntent.getDataString();

            final long lastRunTime = sharedPreferences.getLong(GeckoPreferences.PREFS_TAB_QUEUE_LAST_TIME, 0);
            final boolean isWithinDoubleTapTimeLimit = System.currentTimeMillis() - lastRunTime < TOAST_DOUBLE_TAP_TIMEOUT_MILLIS;

            if (!TextUtils.isEmpty(lastUrl) && lastUrl.equals(intentUrl) && isWithinDoubleTapTimeLimit) {
                
                tabQueueHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        
                        
                        
                        
                        if (stopServiceRunnable != null) {
                            tabQueueHandler.removeCallbacks(stopServiceRunnable);
                            stopSelfResult(stopServiceRunnable.getStartId());
                            stopServiceRunnable = null;
                            removeView();
                        } else {
                            TabQueueHelper.removeURLFromFile(applicationContext, intentUrl, TabQueueHelper.FILE_NAME);
                        }
                        openNow(safeIntent.getUnsafe());
                        stopSelfResult(startId);
                    }
                });

                return START_REDELIVER_INTENT;
            }

            sharedPreferences.edit().putString(GeckoPreferences.PREFS_TAB_QUEUE_LAST_SITE, intentUrl)
                                    .putLong(GeckoPreferences.PREFS_TAB_QUEUE_LAST_TIME, System.currentTimeMillis())
                                    .apply();
        }

        if (stopServiceRunnable != null) {
            
            
            tabQueueHandler.removeCallbacks(stopServiceRunnable);
            stopServiceRunnable.run(false);
        } else {
            windowManager.addView(toastLayout, toastLayoutParams);
        }

        stopServiceRunnable = new StopServiceRunnable(startId) {
            @Override
            public void onRun() {
                addURLToTabQueue(intent, TabQueueHelper.FILE_NAME);
                stopServiceRunnable = null;
            }
        };

        openNowButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(final View view) {
                tabQueueHandler.removeCallbacks(stopServiceRunnable);
                stopServiceRunnable = null;
                removeView();
                openNow(intent);
                stopSelfResult(startId);
            }
        });

        tabQueueHandler.postDelayed(stopServiceRunnable, TOAST_TIMEOUT);

        return START_REDELIVER_INTENT;
    }

    private void openNow(Intent intent) {
        Intent forwardIntent = new Intent(intent);
        forwardIntent.setClass(getApplicationContext(), BrowserApp.class);
        forwardIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(forwardIntent);

        GeckoSharedPrefs.forApp(getApplicationContext()).edit().remove(GeckoPreferences.PREFS_TAB_QUEUE_LAST_SITE)
                                                               .remove(GeckoPreferences.PREFS_TAB_QUEUE_LAST_TIME)
                                                               .apply();
    }

    private void removeView() {
        windowManager.removeView(toastLayout);
    }

    private void addURLToTabQueue(final Intent intent, final String filename) {
        if (intent == null) {
            
            Log.w(LOGTAG, "Error adding URL to tab queue - invalid intent passed in.");
            return;
        }
        final ContextUtils.SafeIntent safeIntent = new ContextUtils.SafeIntent(intent);
        final String intentData = safeIntent.getDataString();

        
        executorService.submit(new Runnable() {
            @Override
            public void run() {
                Context applicationContext = getApplicationContext();
                final GeckoProfile profile = GeckoProfile.get(applicationContext);
                int tabsQueued = TabQueueHelper.queueURL(profile, intentData, filename);
                TabQueueHelper.showNotification(applicationContext, tabsQueued);

                
                
                
                final SharedPreferences prefs = GeckoSharedPrefs.forApp(applicationContext);

                prefs.edit().putInt(TabQueueHelper.PREF_TAB_QUEUE_COUNT, tabsQueued).apply();
            }
        });
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        tabQueueHandler = null;
        handlerThread.quit();
    }

    



    private abstract class StopServiceRunnable implements Runnable {

        private final int startId;

        public StopServiceRunnable(final int startId) {
            this.startId = startId;
        }

        public void run() {
            run(true);
        }

        public void run(final boolean shouldRemoveView) {
            onRun();

            if (shouldRemoveView) {
                removeView();
            }

            stopSelfResult(startId);
        }

        public int getStartId() {
            return startId;
        }

        public abstract void onRun();
    }
}