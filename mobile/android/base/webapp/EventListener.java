




package org.mozilla.gecko.webapp;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.favicons.decoders.FaviconDecoder;
import org.mozilla.gecko.gfx.BitmapUtils;
import org.mozilla.gecko.util.ActivityResultHandler;
import org.mozilla.gecko.util.EventDispatcher;
import org.mozilla.gecko.util.GeckoEventListener;
import org.mozilla.gecko.util.GeckoEventResponder;
import org.mozilla.gecko.util.ThreadUtils;
import org.mozilla.gecko.WebAppAllocator;

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
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class EventListener implements GeckoEventListener, GeckoEventResponder {

    private static final String LOGTAG = "GeckoWebAppEventListener";

    private EventListener() { }

    private static EventListener mEventListener;
    private String mCurrentResponse = "";

    private static EventListener getEventListener() {
        if (mEventListener == null) {
            mEventListener = new EventListener();
        }
        return mEventListener;
    }

    private static void registerEventListener(String event) {
        GeckoAppShell.getEventDispatcher().registerEventListener(event, EventListener.getEventListener());
    }

    private static void unregisterEventListener(String event) {
        GeckoAppShell.getEventDispatcher().unregisterEventListener(event, EventListener.getEventListener());
    }

    public static void registerEvents() {
        registerEventListener("WebApps:PreInstall");
        registerEventListener("WebApps:InstallApk");
        registerEventListener("WebApps:PostInstall");
        registerEventListener("WebApps:Open");
        registerEventListener("WebApps:Uninstall");
        registerEventListener("WebApps:GetApkVersions");
    }

    public static void unregisterEvents() {
        unregisterEventListener("WebApps:PreInstall");
        unregisterEventListener("WebApps:InstallApk");
        unregisterEventListener("WebApps:PostInstall");
        unregisterEventListener("WebApps:Open");
        unregisterEventListener("WebApps:Uninstall");
        unregisterEventListener("WebApps:GetApkVersions");
    }

    @Override
    public void handleMessage(String event, JSONObject message) {
        try {
            if (AppConstants.MOZ_ANDROID_SYNTHAPKS && event.equals("WebApps:InstallApk")) {
                installApk(GeckoAppShell.getGeckoInterface().getActivity(), message.getString("filePath"), message.getString("data"));
            } else if (event.equals("WebApps:PostInstall")) {
                if (AppConstants.MOZ_ANDROID_SYNTHAPKS) {
                    postInstallWebApp(message.getString("apkPackageName"), message.getString("origin"));
                } else {
                    postInstallWebApp(message.getString("name"),
                                      message.getString("manifestURL"),
                                      message.getString("origin"),
                                      message.getString("iconURL"),
                                      message.getString("originalOrigin"));
                }
            } else if (event.equals("WebApps:Open")) {
                Intent intent = GeckoAppShell.getWebAppIntent(message.getString("manifestURL"),
                                                              message.getString("origin"),
                                                              "", null);
                if (intent == null) {
                    return;
                }
                GeckoAppShell.getGeckoInterface().getActivity().startActivity(intent);
            } else if (!AppConstants.MOZ_ANDROID_SYNTHAPKS && event.equals("WebApps:Uninstall")) {
                uninstallWebApp(message.getString("origin"));
            } else if (!AppConstants.MOZ_ANDROID_SYNTHAPKS && event.equals("WebApps:PreInstall")) {
                String name = message.getString("name");
                String manifestURL = message.getString("manifestURL");
                String origin = message.getString("origin");

                
                mCurrentResponse = preInstallWebApp(name, manifestURL, origin).toString();
            } else if (event.equals("WebApps:GetApkVersions")) {
                mCurrentResponse = getApkVersions(GeckoAppShell.getGeckoInterface().getActivity(),
                                                  message.getJSONArray("packageNames")).toString();
            }
        } catch (Exception e) {
            Log.e(LOGTAG, "Exception handling message \"" + event + "\":", e);
        }
    }

    public String getResponse(JSONObject origMessage) {
        String res = mCurrentResponse;
        mCurrentResponse = "";
        return res;
    }

    
    public static File preInstallWebApp(String aTitle, String aURI, String aOrigin) {
        int index = WebAppAllocator.getInstance(GeckoAppShell.getContext()).findAndAllocateIndex(aOrigin, aTitle, (String) null);
        GeckoProfile profile = GeckoProfile.get(GeckoAppShell.getContext(), "webapp" + index);
        return profile.getDir();
    }

    
    public static void postInstallWebApp(String aTitle, String aURI, String aOrigin, String aIconURL, String aOriginalOrigin) {
        WebAppAllocator allocator = WebAppAllocator.getInstance(GeckoAppShell.getContext());
        int index = allocator.getIndexForApp(aOriginalOrigin);

        assert aIconURL != null;

        final int preferredSize = GeckoAppShell.getPreferredIconSize();
        Bitmap icon = FaviconDecoder.getMostSuitableBitmapFromDataURI(aIconURL, preferredSize);

        assert aOrigin != null && index != -1;
        allocator.updateAppAllocation(aOrigin, index, icon);

        GeckoAppShell.createShortcut(aTitle, aURI, aOrigin, icon, "webapp");
    }

    
    public static void postInstallWebApp(String aPackageName, String aOrigin) {
        Allocator allocator = Allocator.getInstance(GeckoAppShell.getContext());
        int index = allocator.findOrAllocatePackage(aPackageName);
        allocator.putOrigin(index, aOrigin);
    }

    public static void uninstallWebApp(final String uniqueURI) {
        
        
        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                int index;
                index = Allocator.getInstance(GeckoAppShell.getContext()).releaseIndexForApp(uniqueURI);

                
                if (index == -1)
                    return;

                
                String targetProcessName = GeckoAppShell.getContext().getPackageName();
                targetProcessName = targetProcessName + ":" + targetProcessName + ".WebApp" + index;

                ActivityManager am = (ActivityManager) GeckoAppShell.getContext().getSystemService(Context.ACTIVITY_SERVICE);
                List<ActivityManager.RunningAppProcessInfo> procs = am.getRunningAppProcesses();
                if (procs != null) {
                    for (ActivityManager.RunningAppProcessInfo proc : procs) {
                        if (proc.processName.equals(targetProcessName)) {
                            android.os.Process.killProcess(proc.pid);
                            break;
                        }
                    }
                }

                
                GeckoProfile.removeProfile(GeckoAppShell.getContext(), "webapp" + index);
            }
        });
    }

    public static void installApk(final Activity context, String filePath, String data) {
        
        JSONObject argsObj = null;

        
        
        String manifestUrl = null;
        try {
            argsObj = new JSONObject(data);
            manifestUrl = argsObj.getJSONObject("app").getString("manifestURL");
        } catch (JSONException e) {
            Log.e(LOGTAG, "can't get manifest URL from JSON data", e);
            
            return;
        }

        
        
        final InstallListener receiver = new InstallListener(manifestUrl, argsObj);

        
        IntentFilter filter = new IntentFilter(Intent.ACTION_PACKAGE_ADDED);
        filter.addDataScheme("package");
        context.registerReceiver(receiver, filter);

        File file = new File(filePath);
        if (!file.exists()) {
            Log.wtf(LOGTAG, "APK file doesn't exist at path " + filePath);
            
            return;
        }

        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setDataAndType(Uri.fromFile(file), "application/vnd.android.package-archive");

        
        GeckoAppShell.sActivityHelper.startIntentForActivity(context, intent, new ActivityResultHandler() {
            @Override
            public void onActivityResult(int resultCode, Intent data) {
                
                
                if (resultCode == Activity.RESULT_CANCELED) {
                    try {
                        context.unregisterReceiver(receiver);
                        receiver.cleanup();
                    } catch (java.lang.IllegalArgumentException e) {
                        
                        
                        
                        
                        Log.e(LOGTAG, "error unregistering install receiver: ", e);
                    }
                }
            }
        });
    }

    private static final int DEFAULT_VERSION_CODE = -1;

    public static JSONObject getApkVersions(Activity context, JSONArray packageNames) {
        Set<String> packageNameSet = new HashSet<String>();
        for (int i = 0; i < packageNames.length(); i++) {
            try {
                packageNameSet.add(packageNames.getString(i));
            } catch (JSONException e) {
                Log.w(LOGTAG, "exception populating settings item", e);
            }
        }

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
