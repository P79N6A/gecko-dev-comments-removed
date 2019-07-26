















package com.googlecode.eyesfree.braille.selfbraille;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteException;
import android.util.Log;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;








public class SelfBrailleClient {
    private static final String LOG_TAG =
            SelfBrailleClient.class.getSimpleName();
    private static final String ACTION_SELF_BRAILLE_SERVICE =
            "com.googlecode.eyesfree.braille.service.ACTION_SELF_BRAILLE_SERVICE";
    private static final String BRAILLE_BACK_PACKAGE =
            "com.googlecode.eyesfree.brailleback";
    private static final Intent mServiceIntent =
            new Intent(ACTION_SELF_BRAILLE_SERVICE)
            .setPackage(BRAILLE_BACK_PACKAGE);
    






    
    private static final byte[] EYES_FREE_CERT_SHA1 = new byte[] {
        (byte) 0x9B, (byte) 0x42, (byte) 0x4C, (byte) 0x2D,
        (byte) 0x27, (byte) 0xAD, (byte) 0x51, (byte) 0xA4,
        (byte) 0x2A, (byte) 0x33, (byte) 0x7E, (byte) 0x0B,
        (byte) 0xB6, (byte) 0x99, (byte) 0x1C, (byte) 0x76,
        (byte) 0xEC, (byte) 0xA4, (byte) 0x44, (byte) 0x61
    };
    



    private static final int REBIND_DELAY_MILLIS = 500;
    private static final int MAX_REBIND_ATTEMPTS = 5;

    private final Binder mIdentity = new Binder();
    private final Context mContext;
    private final boolean mAllowDebugService;
    private final SelfBrailleHandler mHandler = new SelfBrailleHandler();
    private boolean mShutdown = false;

    



    private volatile Connection mConnection;
    
    private int mNumFailedBinds = 0;

    






    public SelfBrailleClient(Context context, boolean allowDebugService) {
        mContext = context;
        mAllowDebugService = allowDebugService;
        doBindService();
    }

    



    public void shutdown() {
        mShutdown = true;
        doUnbindService();
    }

    public void write(WriteData writeData) {
        writeData.validate();
        ISelfBrailleService localService = getSelfBrailleService();
        if (localService != null) {
            try {
                localService.write(mIdentity, writeData);
            } catch (RemoteException ex) {
                Log.e(LOG_TAG, "Self braille write failed", ex);
            }
        }
    }

    private void doBindService() {
        Connection localConnection = new Connection();
        if (!mContext.bindService(mServiceIntent, localConnection,
                Context.BIND_AUTO_CREATE)) {
            Log.e(LOG_TAG, "Failed to bind to service");
            mHandler.scheduleRebind();
            return;
        }
        mConnection = localConnection;
        Log.i(LOG_TAG, "Bound to self braille service");
    }

    private void doUnbindService() {
        if (mConnection != null) {
            ISelfBrailleService localService = getSelfBrailleService();
            if (localService != null) {
                try {
                    localService.disconnect(mIdentity);
                } catch (RemoteException ex) {
                    
                }
            }
            mContext.unbindService(mConnection);
            mConnection = null;
        }
    }

    private ISelfBrailleService getSelfBrailleService() {
        Connection localConnection = mConnection;
        if (localConnection != null) {
            return localConnection.mService;
        }
        return null;
    }

    private boolean verifyPackage() {
        PackageManager pm = mContext.getPackageManager();
        PackageInfo pi;
        try {
            pi = pm.getPackageInfo(BRAILLE_BACK_PACKAGE,
                    PackageManager.GET_SIGNATURES);
        } catch (PackageManager.NameNotFoundException ex) {
            Log.w(LOG_TAG, "Can't verify package " + BRAILLE_BACK_PACKAGE,
                    ex);
            return false;
        }
        MessageDigest digest;
        try {
            digest = MessageDigest.getInstance("SHA-1");
        } catch (NoSuchAlgorithmException ex) {
            Log.e(LOG_TAG, "SHA-1 not supported", ex);
            return false;
        }
        
        for (Signature signature : pi.signatures) {
            digest.update(signature.toByteArray());
            if (MessageDigest.isEqual(EYES_FREE_CERT_SHA1, digest.digest())) {
                return true;
            }
            digest.reset();
        }
        if (mAllowDebugService) {
            Log.w(LOG_TAG, String.format(
                "*** %s connected to BrailleBack with invalid (debug?) "
                + "signature ***",
                mContext.getPackageName()));
            return true;
        }
        return false;
    }
    private class Connection implements ServiceConnection {
        
        private volatile ISelfBrailleService mService;

        @Override
        public void onServiceConnected(ComponentName className,
                IBinder binder) {
            if (!verifyPackage()) {
                Log.w(LOG_TAG, String.format("Service certificate mismatch "
                                + "for %s, dropping connection",
                                BRAILLE_BACK_PACKAGE));
                mHandler.unbindService();
                return;
            }
            Log.i(LOG_TAG, "Connected to self braille service");
            mService = ISelfBrailleService.Stub.asInterface(binder);
            synchronized (mHandler) {
                mNumFailedBinds = 0;
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName className) {
            Log.e(LOG_TAG, "Disconnected from self braille service");
            mService = null;
            
            mHandler.scheduleRebind();
        }
    }

    private class SelfBrailleHandler extends Handler {
        private static final int MSG_REBIND_SERVICE = 1;
        private static final int MSG_UNBIND_SERVICE = 2;

        public void scheduleRebind() {
            synchronized (this) {
                if (mNumFailedBinds < MAX_REBIND_ATTEMPTS) {
                    int delay = REBIND_DELAY_MILLIS << mNumFailedBinds;
                    sendEmptyMessageDelayed(MSG_REBIND_SERVICE, delay);
                    ++mNumFailedBinds;
                }
            }
        }

        public void unbindService() {
            sendEmptyMessage(MSG_UNBIND_SERVICE);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_REBIND_SERVICE:
                    handleRebindService();
                    break;
                case MSG_UNBIND_SERVICE:
                    handleUnbindService();
                    break;
            }
        }

        private void handleRebindService() {
            if (mShutdown) {
                return;
            }
            if (mConnection != null) {
                doUnbindService();
            }
            doBindService();
        }

        private void handleUnbindService() {
            doUnbindService();
        }
    }
}
