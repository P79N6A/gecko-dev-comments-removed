




package org.mozilla.gecko.tabqueue;

import android.app.Service;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.PixelFormat;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;
import org.mozilla.gecko.BrowserApp;
import org.mozilla.gecko.R;
import org.mozilla.gecko.mozglue.ContextUtils;






















public class TabQueueService extends Service {
    private static final String LOGTAG = "Gecko" + TabQueueService.class.getSimpleName();
    private static final long TOAST_TIMEOUT = 3000;
    private WindowManager windowManager;
    private View toastLayout;
    private Button openNowButton;
    private Handler tabQueueHandler;
    private WindowManager.LayoutParams toastLayoutParams;
    private volatile StopServiceRunnable stopServiceRunnable;
    private HandlerThread handlerThread;

    @Override
    public IBinder onBind(Intent intent) {
        
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();

        handlerThread = new HandlerThread("TabQueueHandlerThread");
        handlerThread.start();
        tabQueueHandler = new Handler(handlerThread.getLooper());

        windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);

        LayoutInflater layoutInflater = (LayoutInflater) getSystemService(LAYOUT_INFLATER_SERVICE);
        toastLayout = layoutInflater.inflate(R.layout.button_toast, null);

        final Resources resources = getResources();

        TextView messageView = (TextView) toastLayout.findViewById(R.id.toast_message);
        messageView.setText(resources.getText(R.string.tab_queue_toast_message));

        openNowButton = (Button) toastLayout.findViewById(R.id.toast_button);
        openNowButton.setText(resources.getText(R.string.tab_queue_toast_action));

        toastLayoutParams = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.WRAP_CONTENT,
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
        if (stopServiceRunnable != null) {
            
            
            tabQueueHandler.removeCallbacks(stopServiceRunnable);
            stopServiceRunnable.run(false);
        } else {
            windowManager.addView(toastLayout, toastLayoutParams);
        }

        stopServiceRunnable = new StopServiceRunnable(startId) {
            @Override
            public void onRun() {
                addUrlToTabQueue(intent);
                stopServiceRunnable = null;
            }
        };

        openNowButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                tabQueueHandler.removeCallbacks(stopServiceRunnable);
                stopServiceRunnable = null;


                Intent forwardIntent = new Intent(intent);
                forwardIntent.setClass(getApplicationContext(), BrowserApp.class);
                forwardIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                startActivity(forwardIntent);

                removeView();
            }
        });

        tabQueueHandler.postDelayed(stopServiceRunnable, TOAST_TIMEOUT);

        return START_FLAG_REDELIVERY;
    }

    private void removeView() {
        windowManager.removeView(toastLayout);
    }

    private void addUrlToTabQueue(Intent intentParam) {
        if (intentParam == null) {
            
            Log.w(LOGTAG, "Error adding URL to tab queue - invalid intent passed in.");
            return;
        }
        final ContextUtils.SafeIntent intent = new ContextUtils.SafeIntent(intentParam);
        final String intentData = intent.getDataString();

        
        Log.d(LOGTAG, "Adding URL to tab queue: " + intentData);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        tabQueueHandler = null;
        handlerThread.quit();
    }

    



    private abstract class StopServiceRunnable implements Runnable {

        private final int startId;

        public StopServiceRunnable(int startId) {
            this.startId = startId;
        }

        public void run(boolean shouldStopService) {
            onRun();

            if (shouldStopService) {
                removeView();
            }

            stopSelfResult(startId);
        }

        public void run() {
            run(true);
        }

        public abstract void onRun();
    }
}