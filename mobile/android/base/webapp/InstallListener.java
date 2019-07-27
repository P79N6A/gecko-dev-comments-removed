




package org.mozilla.gecko.webapp;

import java.io.File;
import java.io.IOException;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.GeckoThread;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;
import android.text.TextUtils;
import android.util.Log;

public class InstallListener extends BroadcastReceiver {

    private static String LOGTAG = "GeckoWebappInstallListener";
    private JSONObject mData;
    private String mManifestUrl;
    private boolean mReceived;
    private File mApkFile;

    public InstallListener(String manifestUrl, JSONObject data, File apkFile) {
        mData = data;
        mApkFile = apkFile;
        mManifestUrl = manifestUrl;
        assert mManifestUrl != null;
        assert mApkFile != null && mApkFile.exists();
    }

    public boolean isReceived() {
        return mReceived;
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        String packageName = intent.getData().getSchemeSpecificPart();

        if (TextUtils.isEmpty(packageName)) {
            Log.i(LOGTAG, "No package name defined in intent");
            return;
        }

        ApkResources apkResources = null;
        try {
            apkResources = new ApkResources(context, packageName);
        } catch (NameNotFoundException e) {
            Log.e(LOGTAG, "Can't find package that's just been installed");
            return;
        }

        String manifestUrl = apkResources.getManifestUrl();
        if (TextUtils.isEmpty(manifestUrl)) {
            Log.i(LOGTAG, "No manifest URL present in metadata");
            return;
        } else if (!isCorrectManifest(manifestUrl)) {
            
            
            
            Log.i(LOGTAG, "Manifest URL is for a different install; ignoring");
            return;
        }

        
        mReceived = true;
        context.unregisterReceiver(this);

        if (mApkFile != null && mApkFile.delete()) {
            Log.i(LOGTAG, "Downloaded APK file deleted");
        }


        if (GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
            InstallHelper installHelper = new InstallHelper(context, apkResources, null);
            try {
                JSONObject dataObject = new JSONObject();
                dataObject.put("request", mData);

                Allocator slots = Allocator.getInstance(context);
                int i = slots.findOrAllocatePackage(packageName);
                installHelper.startInstall("webapp" + i, dataObject);
            } catch (JSONException e) {
                Log.e(LOGTAG, "Couldn't parse data from mozApps.install()", e);
            } catch (IOException e) {
                Log.e(LOGTAG, "Couldn't install packaged app", e);
            }
        }
    }

    public boolean isCorrectManifest(String manifestUrl) {
        
        
        try {
            String registeredUrl = mManifestUrl.split("\\?")[0];
            String observedUrl = manifestUrl.split("\\?")[0];
            return registeredUrl.equals(observedUrl);
        } catch (NullPointerException e) {
            Log.e(LOGTAG, "One or both of the manifest URLs is null", e);
        }
        return false;
    }

}
