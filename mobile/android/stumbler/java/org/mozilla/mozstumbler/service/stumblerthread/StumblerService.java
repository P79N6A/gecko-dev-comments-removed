



package org.mozilla.mozstumbler.service.stumblerthread;

import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.os.AsyncTask;
import android.util.Log;
import java.io.IOException;
import java.util.concurrent.atomic.AtomicBoolean;

import org.mozilla.mozstumbler.service.AppGlobals;
import org.mozilla.mozstumbler.service.Prefs;
import org.mozilla.mozstumbler.service.stumblerthread.blocklist.WifiBlockListInterface;
import org.mozilla.mozstumbler.service.stumblerthread.datahandling.DataStorageManager;
import org.mozilla.mozstumbler.service.stumblerthread.scanners.ScanManager;
import org.mozilla.mozstumbler.service.uploadthread.UploadAlarmReceiver;
import org.mozilla.mozstumbler.service.utils.PersistentIntentService;




public class StumblerService extends PersistentIntentService
        implements DataStorageManager.StorageIsEmptyTracker {
    private static final String LOG_TAG = AppGlobals.makeLogTag(StumblerService.class.getSimpleName());
    public static final String ACTION_BASE = AppGlobals.ACTION_NAMESPACE;
    public static final String ACTION_START_PASSIVE = ACTION_BASE + ".START_PASSIVE";
    public static final String ACTION_EXTRA_MOZ_API_KEY = ACTION_BASE + ".MOZKEY";
    public static final String ACTION_EXTRA_USER_AGENT = ACTION_BASE + ".USER_AGENT";
    public static final String ACTION_NOT_FROM_HOST_APP = ACTION_BASE + ".NOT_FROM_HOST";
    public static final AtomicBoolean sFirefoxStumblingEnabled = new AtomicBoolean();
    protected final ScanManager mScanManager = new ScanManager();
    protected final Reporter mReporter = new Reporter();

    
    
    private static final int DELAY_IN_SEC_BEFORE_STARTING_UPLOAD_IN_PASSIVE_MODE = 2;

    
    private static final int FREQUENCY_IN_SEC_OF_UPLOAD_IN_ACTIVE_MODE = 5 * 60;

    
    private static final long PASSIVE_UPLOAD_FREQ_GUARD_MSEC = 5 * 60 * 1000;

    public StumblerService() {
        this("StumblerService");
    }

    public StumblerService(String name) {
        super(name);
    }

    public boolean isScanning() {
        return mScanManager.isScanning();
    }

    public void startScanning() {
        mScanManager.startScanning(this);
    }

    
    
    public void setWifiBlockList(WifiBlockListInterface list) {
        mScanManager.setWifiBlockList(list);
    }

    public Prefs getPrefs(Context c) {
        return Prefs.getInstance(c);
    }

    public void checkPrefs() {
        mScanManager.checkPrefs();
    }

    public int getLocationCount() {
        return mScanManager.getLocationCount();
    }

    public double getLatitude() {
        return mScanManager.getLatitude();
    }

    public double getLongitude() {
        return mScanManager.getLongitude();
    }

    public Location getLocation() {
        return mScanManager.getLocation();
    }

    public int getWifiStatus() {
        return mScanManager.getWifiStatus();
    }

    public int getAPCount() {
        return mScanManager.getAPCount();
    }

    public int getVisibleAPCount() {
        return mScanManager.getVisibleAPCount();
    }

    public int getCellInfoCount() {
        return mScanManager.getCellInfoCount();
    }

    public boolean isGeofenced () {
        return mScanManager.isGeofenced();
    }

    
    
    
    protected void init() {
        
        Prefs.getInstance(this);
        DataStorageManager.createGlobalInstance(this, this);

        mReporter.startup(this);
    }

    
    @Override
    public void onCreate() {
        super.onCreate();
        setIntentRedelivery(true);
    }

    
    @Override
    public void onDestroy() {
        super.onDestroy();

        if (!mScanManager.isScanning()) {
            return;
        }

        
        
        
        new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... params) {
                if (AppGlobals.isDebug) {
                    Log.d(LOG_TAG, "onDestroy");
                }

                if (!sFirefoxStumblingEnabled.get()) {
                    Prefs.getInstance(StumblerService.this).setFirefoxScanEnabled(false);
                }

                if (DataStorageManager.getInstance() != null) {
                    try {
                        DataStorageManager.getInstance().saveCurrentReportsToDisk();
                    } catch (IOException ex) {
                        AppGlobals.guiLogInfo(ex.toString());
                        Log.e(LOG_TAG, "Exception in onDestroy saving reports" + ex.toString());
                    }
                }
                return null;
            }
        }.execute();

        mReporter.shutdown();
        mScanManager.stopScanning();
    }

    
    @Override
    protected void onHandleIntent(Intent intent) {
        
        init();

        
        mScanManager.setPassiveMode(true);

        if (intent == null) {
            return;
        }

        final boolean isScanEnabledInPrefs = Prefs.getInstance(this).getFirefoxScanEnabled();

        if (!isScanEnabledInPrefs && intent.getBooleanExtra(ACTION_NOT_FROM_HOST_APP, false)) {
            stopSelf();
            return;
        }

        boolean hasFilesWaiting = !DataStorageManager.getInstance().isDirEmpty();
        if (AppGlobals.isDebug) {
            Log.d(LOG_TAG, "Files waiting:" + hasFilesWaiting);
        }
        if (hasFilesWaiting) {
            
            
            
            
            final long lastAttemptedTime = Prefs.getInstance(this).getLastAttemptedUploadTime();
            final long timeNow = System.currentTimeMillis();

            if (timeNow - lastAttemptedTime < PASSIVE_UPLOAD_FREQ_GUARD_MSEC) {
                
                if (AppGlobals.isDebug) {
                    Log.d(LOG_TAG, "Upload attempt too frequent.");
                }
            } else {
                Prefs.getInstance(this).setLastAttemptedUploadTime(timeNow);
                UploadAlarmReceiver.scheduleAlarm(this, DELAY_IN_SEC_BEFORE_STARTING_UPLOAD_IN_PASSIVE_MODE, false );
            }
        }

        if (!isScanEnabledInPrefs) {
            Prefs.getInstance(this).setFirefoxScanEnabled(true);
        }

        String apiKey = intent.getStringExtra(ACTION_EXTRA_MOZ_API_KEY);
        if (apiKey != null && !apiKey.equals(Prefs.getInstance(this).getMozApiKey())) {
            Prefs.getInstance(this).setMozApiKey(apiKey);
        }

        String userAgent = intent.getStringExtra(ACTION_EXTRA_USER_AGENT);
        if (userAgent != null && !userAgent.equals(Prefs.getInstance(this).getUserAgent())) {
            Prefs.getInstance(this).setUserAgent(userAgent);
        }

        if (!mScanManager.isScanning()) {
            startScanning();
        }
    }

    
    @Override
    public void notifyStorageStateEmpty(boolean isEmpty) {
        if (isEmpty) {
            UploadAlarmReceiver.cancelAlarm(this, !mScanManager.isPassiveMode());
        } else if (!mScanManager.isPassiveMode()) {
            UploadAlarmReceiver.scheduleAlarm(this, FREQUENCY_IN_SEC_OF_UPLOAD_IN_ACTIVE_MODE, true );
        }
    }
}
