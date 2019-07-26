




package org.mozilla.gecko.webapp;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.text.TextUtils;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONArray;

import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;

import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.ArrayList;

public class UninstallListener extends BroadcastReceiver {

    private static String LOGTAG = "GeckoWebAppUninstallListener";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent.getBooleanExtra(Intent.EXTRA_REPLACING, false)) {
            Log.i(LOGTAG, "Package is being replaced; ignoring removal intent");
            return;
        }

        String packageName = intent.getData().getSchemeSpecificPart();

        if (TextUtils.isEmpty(packageName)) {
            Log.i(LOGTAG, "No package name defined in intent");
            return;
        }

        Allocator allocator = Allocator.getInstance(context);
        ArrayList<String> installedPackages = allocator.getInstalledPackageNames();

        if (installedPackages.contains(packageName)) {
            JSONObject message = new JSONObject();
            JSONArray packageNames = new JSONArray();
            try {
                packageNames.put(packageName);
                message.put("apkPackageNames", packageNames);
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Webapps:AutoUninstall", message.toString()));
            } catch (JSONException e) {
                Log.e(LOGTAG, "JSON EXCEPTION " + e);
            }
        }
    }

    public static void initUninstallPackageScan(Context context) {
        
        Allocator allocator = Allocator.getInstance(context);
        ArrayList<String> fennecPackages = allocator.getInstalledPackageNames();
        ArrayList<String> uninstalledPackages = new ArrayList<String>();

        final PackageManager pm = context.getPackageManager();
        
        List<ApplicationInfo> packages = pm.getInstalledApplications(PackageManager.GET_META_DATA);
        Set<String> allInstalledPackages = new HashSet<String>();

        for (ApplicationInfo packageInfo : packages) {
            
            allInstalledPackages.add(packageInfo.packageName);
        }

        for (String packageName : fennecPackages) {
            if (!allInstalledPackages.contains(packageName)) {
                uninstalledPackages.add(packageName);
            }
        }

        if (uninstalledPackages.size() > 0) {
            JSONObject message = new JSONObject();
            JSONArray packageNames = new JSONArray();
            try {
                for (String packageName : uninstalledPackages) {
                    packageNames.put(packageName);
                }
                message.put("apkPackageNames", packageNames);
                GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Webapps:AutoUninstall", message.toString()));
            } catch (JSONException e) {
                Log.e(LOGTAG, "JSON EXCEPTION " + e);
            }
        }
    }
}
