




package org.mozilla.gecko.webapp;

import org.mozilla.gecko.ActivityHandlerHelper;
import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.favicons.decoders.FaviconDecoder;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.ActivityResultHandler;
import org.mozilla.gecko.EventDispatcher;
import org.mozilla.gecko.util.EventCallback;
import org.mozilla.gecko.util.NativeEventListener;
import org.mozilla.gecko.util.NativeJSObject;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.WebappAllocator;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Bitmap;
import android.net.Uri;
import android.util.Log;

import java.io.File;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.json.JSONException;
import org.json.JSONObject;

public class EventListener implements NativeEventListener  {

    private static final String LOGTAG = "GeckoWebappEventListener";

    public void registerEvents() {
        EventDispatcher.getInstance().registerGeckoThreadListener(this,
            "Webapps:Preinstall",
            "Webapps:InstallApk",
            "Webapps:Postinstall",
            "Webapps:Open",
            "Webapps:Uninstall",
            "Webapps:GetApkVersions");
    }

    public void unregisterEvents() {
        EventDispatcher.getInstance().unregisterGeckoThreadListener(this,
            "Webapps:Preinstall",
            "Webapps:InstallApk",
            "Webapps:Postinstall",
            "Webapps:Open",
            "Webapps:Uninstall",
            "Webapps:GetApkVersions");
    }

    @Override
    public void handleMessage(String event, NativeJSObject message, EventCallback callback) {
        try {
            if (AppConstants.MOZ_ANDROID_SYNTHAPKS && event.equals("Webapps:InstallApk")) {
                installApk(GeckoAppShell.getGeckoInterface().getActivity(), message, callback);
            } else if (event.equals("Webapps:Postinstall")) {
                if (AppConstants.MOZ_ANDROID_SYNTHAPKS) {
                    postInstallWebapp(message.getString("apkPackageName"), message.getString("origin"));
                } else {
                    postInstallWebapp(message.getString("name"),
                                      message.getString("manifestURL"),
                                      message.getString("origin"),
                                      message.getString("iconURL"),
                                      message.getString("originalOrigin"));
                }
            } else if (event.equals("Webapps:Open")) {
                Intent intent = GeckoAppShell.getWebappIntent(message.getString("manifestURL"),
                                                              message.getString("origin"),
                                                              "", null);
                if (intent == null) {
                    return;
                }
                GeckoAppShell.getGeckoInterface().getActivity().startActivity(intent);
            } else if (!AppConstants.MOZ_ANDROID_SYNTHAPKS && event.equals("Webapps:Uninstall")) {
                uninstallWebapp(message.getString("origin"));
            } else if (!AppConstants.MOZ_ANDROID_SYNTHAPKS && event.equals("Webapps:Preinstall")) {
                String name = message.getString("name");
                String manifestURL = message.getString("manifestURL");
                String origin = message.getString("origin");

                JSONObject obj = new JSONObject();
                obj.put("profile", preInstallWebapp(name, manifestURL, origin).toString());
                callback.sendSuccess(obj);
            } else if (event.equals("Webapps:GetApkVersions")) {
                JSONObject obj = new JSONObject();
                obj.put("versions", getApkVersions(GeckoAppShell.getGeckoInterface().getActivity(),
                                                   message.getStringArray("packageNames")));
                callback.sendSuccess(obj);
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    
    public static File preInstallWebapp(String aTitle, String aURI, String aOrigin) {
        int index = WebappAllocator.getInstance(GeckoAppShell.getContext()).findAndAllocateIndex(aOrigin, aTitle, (String) null);
        GeckoProfile profile = GeckoProfile.get(GeckoAppShell.getContext(), "webapp" + index);
        return profile.getDir();
    }

    
    public static void postInstallWebapp(String aTitle, String aURI, String aOrigin, String aIconURL, String aOriginalOrigin) {
        WebappAllocator allocator = WebappAllocator.getInstance(GeckoAppShell.getContext());
        int index = allocator.getIndexForApp(aOriginalOrigin);

        assert aIconURL != null;

        final int preferredSize = GeckoAppShell.getPreferredIconSize();
        Bitmap icon = FaviconDecoder.getMostSuitableBitmapFromDataURI(aIconURL, preferredSize);

        assert aOrigin != null && index != -1;
        allocator.updateAppAllocation(aOrigin, index, icon);

        GeckoAppShell.createShortcut(aTitle, aURI, aOrigin, icon, "webapp");
    }

    
    public static void postInstallWebapp(String aPackageName, String aOrigin) {
        Allocator allocator = Allocator.getInstance(GeckoAppShell.getContext());
        int index = allocator.findOrAllocatePackage(aPackageName);
        allocator.putOrigin(index, aOrigin);
    }

    public static void uninstallWebapp(final String packageName) {
        
        
        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                int index = Allocator.getInstance(GeckoAppShell.getContext()).releaseIndexForApp(packageName);

                
                if (index == -1)
                    return;

                killWebappSlot(GeckoAppShell.getContext(), index);

                
                GeckoProfile.removeProfile(GeckoAppShell.getContext(), "webapp" + index);
            }
        });
    }

    





    public static void killWebappSlot(Context context, int slot) {
        
        String targetProcessName = context.getPackageName();
        targetProcessName = targetProcessName + ":" + targetProcessName + ".Webapp" + slot;

        ActivityManager am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RunningAppProcessInfo> procs = am.getRunningAppProcesses();
        if (procs != null) {
            for (ActivityManager.RunningAppProcessInfo proc : procs) {
                if (proc.processName.equals(targetProcessName)) {
                    android.os.Process.killProcess(proc.pid);
                    break;
                }
            }
        }
    }

    public static void installApk(final Activity context, NativeJSObject message, final EventCallback callback) {
        final JSONObject messageData;

        
        
        String manifestUrl = null;
        String filePath = null;

        try {
            filePath = message.getString("filePath");
            messageData = new JSONObject(message.getObject("data").toString());
            manifestUrl = messageData.getJSONObject("app").getString("manifestURL");
        } catch (JSONException e) {
            Log.wtf(LOGTAG, "Error getting file path and data", e);
            callback.sendError("Error getting file path and data: " + e.toString());
            return;
        }

        
        
        final InstallListener receiver = new InstallListener(manifestUrl, messageData);

        
        IntentFilter filter = new IntentFilter(Intent.ACTION_PACKAGE_ADDED);
        filter.addDataScheme("package");
        context.registerReceiver(receiver, filter);

        File file = new File(filePath);
        if (!file.exists()) {
            Log.wtf(LOGTAG, "APK file doesn't exist at path " + filePath);
            callback.sendError("APK file doesn't exist at path " + filePath);
            return;
        }

        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setDataAndType(Uri.fromFile(file), "application/vnd.android.package-archive");

        
        ActivityHandlerHelper.startIntentForActivity(context, intent, new ActivityResultHandler() {
            @Override
            public void onActivityResult(int resultCode, Intent data) {
                
                
                if (resultCode == Activity.RESULT_CANCELED) {
                    try {
                        context.unregisterReceiver(receiver);
                        receiver.cleanup();
                    } catch (java.lang.IllegalArgumentException e) {
                        
                        
                        
                        
                        Log.e(LOGTAG, "error unregistering install receiver: ", e);
                    }
                    callback.sendError("APK installation cancelled by user");
                }
            }
        });
    }

    private static final int DEFAULT_VERSION_CODE = -1;

    public static JSONObject getApkVersions(Activity context, String[] packageNames) {
        Set<String> packageNameSet = new HashSet<String>();
        packageNameSet.addAll(Arrays.asList(packageNames));

        final PackageManager pm = context.getPackageManager();
        List<ApplicationInfo> apps = pm.getInstalledApplications(0);

        JSONObject jsonMessage = new JSONObject();

        for (ApplicationInfo app : apps) {
            if (packageNameSet.contains(app.packageName)) {
                int versionCode = DEFAULT_VERSION_CODE;
                try {
                    versionCode = pm.getPackageInfo(app.packageName, 0).versionCode;
                } catch (PackageManager.NameNotFoundException e) {
                    Log.e(LOGTAG, "couldn't get version for app " + app.packageName, e);
                }
                try {
                    jsonMessage.put(app.packageName, versionCode);
                } catch (JSONException e) {
                    Log.e(LOGTAG, "unable to store version code field for app " + app.packageName, e);
                }
            }
        }

        return jsonMessage;
    }
}
