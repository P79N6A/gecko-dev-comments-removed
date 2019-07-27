




package org.mozilla.gecko;

import org.mozilla.gecko.mozglue.JNITarget;
import org.mozilla.gecko.util.NativeEventListener;
import org.mozilla.gecko.util.NativeJSObject;
import org.mozilla.gecko.util.EventCallback;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.DhcpInfo;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.telephony.TelephonyManager;
import android.util.Log;














public class GeckoNetworkManager extends BroadcastReceiver implements NativeEventListener {
    private static final String LOGTAG = "GeckoNetworkManager";

    static private final GeckoNetworkManager sInstance = new GeckoNetworkManager();

    public static void destroy() {
        if (sInstance != null) {
            sInstance.onDestroy();
        }
    }

    
    private enum ConnectionType {
        CELLULAR(0),
        BLUETOOTH(1),
        ETHERNET(2),
        WIFI(3),
        OTHER(4),
        NONE(5);

        public final int value;

        private ConnectionType(int value) {
            this.value = value;
        }
    }

    private enum InfoType {
        MCC,
        MNC
    }

    private GeckoNetworkManager() {
        EventDispatcher.getInstance().registerGeckoThreadListener(this, "Wifi:Enable");
    }

    private void onDestroy() {
        EventDispatcher.getInstance().unregisterGeckoThreadListener(this, "Wifi:Enable");
    }

    private ConnectionType mConnectionType = ConnectionType.NONE;
    private final IntentFilter mNetworkFilter = new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION);

    
    private boolean mShouldBeListening;

    
    
    private boolean mShouldNotify;

    
    
    private volatile Context mApplicationContext;

    public static GeckoNetworkManager getInstance() {
        return sInstance;
    }

    @Override
    public void onReceive(Context aContext, Intent aIntent) {
        updateConnectionType();
    }

    public void start(final Context context) {
        
        mApplicationContext = context.getApplicationContext();
        if (mConnectionType == ConnectionType.NONE) {
            mConnectionType = getConnectionType();
        }

        mShouldBeListening = true;
        updateConnectionType();

        if (mShouldNotify) {
            startListening();
        }
    }

    private void startListening() {
        final Context appContext = mApplicationContext;
        if (appContext == null) {
            Log.w(LOGTAG, "Not registering receiver: no context!");
            return;
        }

        Log.v(LOGTAG, "Registering receiver.");
        appContext.registerReceiver(this, mNetworkFilter);
    }

    public void stop() {
        mShouldBeListening = false;

        if (mShouldNotify) {
            stopListening();
        }
    }

    @Override
    public void handleMessage(final String event, final NativeJSObject message,
                              final EventCallback callback) {
        if (event.equals("Wifi:Enable")) {
            final WifiManager mgr = (WifiManager) mApplicationContext.getSystemService(Context.WIFI_SERVICE);

            if (!mgr.isWifiEnabled()) {
                mgr.setWifiEnabled(true);
            } else {
                
                Intent intent = new Intent(android.provider.Settings.ACTION_WIFI_SETTINGS);
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                mApplicationContext.startActivity(intent);
            }
        }
    }

    private void stopListening() {
        if (null == mApplicationContext) {
            return;
        }

        mApplicationContext.unregisterReceiver(this);
    }

    private int wifiDhcpGatewayAddress() {
        if (mConnectionType != ConnectionType.WIFI) {
            return 0;
        }

        if (null == mApplicationContext) {
            return 0;
        }

        try {
            WifiManager mgr = (WifiManager) mApplicationContext.getSystemService(Context.WIFI_SERVICE);
            DhcpInfo d = mgr.getDhcpInfo();
            if (d == null) {
                return 0;
            }

            return d.gateway;

        } catch (Exception ex) {
            
            
            
            return 0;
        }
    }

    private void updateConnectionType() {
        ConnectionType previousConnectionType = mConnectionType;
        mConnectionType = getConnectionType();

        if (mConnectionType == previousConnectionType || !mShouldNotify) {
            return;
        }

        GeckoAppShell.sendEventToGecko(GeckoEvent.createNetworkEvent(
                                       mConnectionType.value,
                                       mConnectionType == ConnectionType.WIFI,
                                       wifiDhcpGatewayAddress()));
    }

    public double[] getCurrentInformation() {
        return new double[] { mConnectionType.value,
                              (mConnectionType == ConnectionType.WIFI) ? 1.0 : 0.0,
                              wifiDhcpGatewayAddress()};
    }

    public void enableNotifications() {
        
        
        mConnectionType = ConnectionType.NONE; 
        updateConnectionType();
        mShouldNotify = true;

        if (mShouldBeListening) {
            startListening();
        }
    }

    public void disableNotifications() {
        mShouldNotify = false;

        if (mShouldBeListening) {
            stopListening();
        }
    }

    private ConnectionType getConnectionType() {
        final Context appContext = mApplicationContext;

        if (null == appContext) {
            return ConnectionType.NONE;
        }

        ConnectivityManager cm = (ConnectivityManager) appContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (cm == null) {
            Log.e(LOGTAG, "Connectivity service does not exist");
            return ConnectionType.NONE;
        }

        NetworkInfo ni = null;
        try {
            ni = cm.getActiveNetworkInfo();
        } catch (SecurityException se) {} 

        if (ni == null) {
            return ConnectionType.NONE;
        }

        switch (ni.getType()) {
        case ConnectivityManager.TYPE_BLUETOOTH:
            return ConnectionType.BLUETOOTH;
        case ConnectivityManager.TYPE_ETHERNET:
            return ConnectionType.ETHERNET;
        case ConnectivityManager.TYPE_MOBILE:
        case ConnectivityManager.TYPE_WIMAX:
            return ConnectionType.CELLULAR;
        case ConnectivityManager.TYPE_WIFI:
            return ConnectionType.WIFI;
        default:
            Log.w(LOGTAG, "Ignoring the current network type.");
            return ConnectionType.OTHER;
        }
    }

    private static int getNetworkOperator(InfoType type, Context context) {
        if (null == context) {
            return -1;
        }

        TelephonyManager tel = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        if (tel == null) {
            Log.e(LOGTAG, "Telephony service does not exist");
            return -1;
        }

        String networkOperator = tel.getNetworkOperator();
        if (networkOperator == null || networkOperator.length() <= 3) {
            return -1;
        }
        if (type == InfoType.MNC) {
            return Integer.parseInt(networkOperator.substring(3));
        } else if (type == InfoType.MCC) {
            return Integer.parseInt(networkOperator.substring(0, 3));
        }

        return -1;
    }

    





    @JNITarget
    public static int getMCC() {
        return getNetworkOperator(InfoType.MCC, GeckoAppShell.getContext().getApplicationContext());
    }

    @JNITarget
    public static int getMNC() {
        return getNetworkOperator(InfoType.MNC, GeckoAppShell.getContext().getApplicationContext());
    }
}
