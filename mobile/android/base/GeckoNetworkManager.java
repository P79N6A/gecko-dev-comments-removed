




package org.mozilla.gecko;

import org.mozilla.gecko.mozglue.JNITarget;

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














public class GeckoNetworkManager extends BroadcastReceiver {
    private static final String LOGTAG = "GeckoNetworkManager";

    static private final GeckoNetworkManager sInstance = new GeckoNetworkManager();

    
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

    static private final ConnectionType kDefaultConnectionType = ConnectionType.NONE;

    private Context mApplicationContext;
    private ConnectionType mConnectionType = ConnectionType.NONE;
    private final IntentFilter mNetworkFilter = new IntentFilter();

    
    private boolean mShouldBeListening = false;

    
    
    private boolean mShouldNotify = false;

    public static GeckoNetworkManager getInstance() {
        return sInstance;
    }

    @Override
    public void onReceive(Context aContext, Intent aIntent) {
        updateConnectionType();
    }

    public void start(final Context context) {
        
        if (mApplicationContext == null) {
            mApplicationContext = context.getApplicationContext();
            mNetworkFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
            mConnectionType = getConnectionType();
        }

        mShouldBeListening = true;
        updateConnectionType();

        if (mShouldNotify) {
            startListening();
        }
    }

    private void startListening() {
        mApplicationContext.registerReceiver(sInstance, mNetworkFilter);
    }

    public void stop() {
        mShouldBeListening = false;

        if (mShouldNotify) {
            stopListening();
        }
    }

    private void stopListening() {
        mApplicationContext.unregisterReceiver(sInstance);
    }

    private int wifiDhcpGatewayAddress() {
        if (mConnectionType != ConnectionType.WIFI) {
            return 0;
        }
        try {
            WifiManager mgr = (WifiManager)sInstance.mApplicationContext.getSystemService(Context.WIFI_SERVICE);
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

    private static ConnectionType getConnectionType() {
        ConnectivityManager cm =
            (ConnectivityManager)sInstance.mApplicationContext.getSystemService(Context.CONNECTIVITY_SERVICE);
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

    private static int getNetworkOperator(InfoType type) {
        TelephonyManager tel = (TelephonyManager)sInstance.mApplicationContext.getSystemService(Context.TELEPHONY_SERVICE);
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
        return getNetworkOperator(InfoType.MCC);
    }

    @JNITarget
    public static int getMNC() {
        return getNetworkOperator(InfoType.MNC);
    }
}
