




package org.mozilla.gecko.webapp;

import java.io.Closeable;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.Context;
import android.graphics.Bitmap;
import android.net.Uri;
import android.util.Log;

public class InstallHelper implements GeckoEventListener {
    private static final String LOGTAG = "GeckoInstallHelper";
    private static final String[] INSTALL_EVENT_NAMES = new String[] {"WebApps:PostInstall"};
    private final Context mContext;
    private final InstallCallback mCallback;
    private final ApkResources mApkResources;

    public static interface InstallCallback {
        
        void installCompleted(InstallHelper installHelper, String event, JSONObject message);

        
        void installErrored(InstallHelper installHelper, Exception exception);
    }

    public InstallHelper(Context context, ApkResources apkResources, InstallCallback cb) {
        mContext = context;
        mCallback = cb;
        mApkResources = apkResources;
    }

    public void startInstall(String profileName) throws IOException {
        startInstall(profileName, null);
    }

    public void startInstall(final String profileName, final JSONObject message) throws IOException {
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                try {
                    install(profileName, message);
                } catch (IOException e) {
                    handleException(e);
                }
            }
        });
    }

    protected void handleException(Exception e) {
        if (mCallback != null) {
            mCallback.installErrored(this, e);
        } else {
            Log.e(LOGTAG, "mozApps.install failed", e);
        }
    }

    private void install(String profileName, JSONObject message) throws IOException {
        if (message == null) {
            message = new JSONObject();
        }

        
        GeckoProfile profile = GeckoProfile.get(mContext, profileName);

        try {
            message.put("apkPackageName", mApkResources.getPackageName());
            message.put("manifestUrl", mApkResources.getManifestUrl());
            message.put("title", mApkResources.getAppName());
            message.put("manifest", new JSONObject(mApkResources.getManifest(mContext)));

            String appType = mApkResources.getWebAppType();
            message.putOpt("type", appType);
            if ("packaged".equals(appType)) {
                message.putOpt("updateManifest", new JSONObject(mApkResources.getMiniManifest(mContext)));
            }

            message.putOpt("profilePath", profile.getDir());

            if (mApkResources.isPackaged()) {
                File zipFile = copyApplicationZipFile();
                message.putOpt("zipFilePath", Uri.fromFile(zipFile).toString());
            }
        } catch (JSONException e) {
            handleException(e);
            return;
        }

        registerGeckoListener();

        GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Webapps:AutoInstall", message.toString()));
        calculateColor();
    }

    public File copyApplicationZipFile() throws IOException {
        if (!mApkResources.isPackaged()) {
            return null;
        }

        Uri uri = mApkResources.getZipFileUri();

        InputStream in = null;
        OutputStream out = null;
        File destPath = new File(mApkResources.getFileDirectory(), "application.zip");
        try {
            in = mContext.getContentResolver().openInputStream(uri);
            out = new FileOutputStream(destPath);
            byte[] buffer = new byte[1024];
            int read = 0;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
            out.flush();
        } catch (IOException e) {
            throw e;
        } finally {
            close(in);
            close(out);
        }
        return destPath;
    }

    private static void close(Closeable close) {
        if (close == null) {
            return;
        }
        try {
            close.close();
        } catch (IOException e) {
            
        }
    }

    public void registerGeckoListener() {
        for (String eventName : INSTALL_EVENT_NAMES) {
            GeckoAppShell.registerEventListener(eventName, this);
        }
    }

    private void calculateColor() {
        ThreadUtils.assertOnBackgroundThread();
        WebAppAllocator slots = WebAppAllocator.getInstance(mContext);
        int index = slots.getIndexForApp(mApkResources.getPackageName());
        Bitmap bitmap = BitmapUtils.getBitmapFromDrawable(mApkResources.getAppIcon());
        slots.updateColor(index, BitmapUtils.getDominantColor(bitmap));
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        for (String eventName : INSTALL_EVENT_NAMES) {
            GeckoAppShell.unregisterEventListener(eventName, this);
        }

        if (mCallback != null) {
            mCallback.installCompleted(this, event, message);
        }
    }
}
