








package org.mozilla.gecko;

import org.mozilla.gecko.util.GeckoBackgroundThread;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public final class Distribution {
    private static final String LOGTAG = "GeckoDistribution";

    private static final int STATE_UNKNOWN = 0;
    private static final int STATE_NONE = 1;
    private static final int STATE_SET = 2;

    




    public static void init(final Context context, final String packagePath) {
        
        GeckoBackgroundThread.getHandler().post(new Runnable() {
            public void run() {
                
                SharedPreferences settings = context.getSharedPreferences(GeckoApp.PREFS_NAME, Activity.MODE_PRIVATE);
                String keyName = context.getPackageName() + ".distribution_state";
                int state = settings.getInt(keyName, STATE_UNKNOWN);
                if (state == STATE_NONE) {
                    return;
                }

                
                if (state == STATE_SET) {
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Distribution:Set", null));
                    return;
                }

                boolean distributionSet = false;
                try {
                    distributionSet = copyFiles(context, packagePath);
                } catch (IOException e) {
                    Log.e(LOGTAG, "Error copying distribution files", e);
                }

                if (distributionSet) {
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Distribution:Set", null));
                    settings.edit().putInt(keyName, STATE_SET).commit();
                } else {
                    settings.edit().putInt(keyName, STATE_NONE).commit();
                }
            }
        });
    }

    



    private static boolean copyFiles(Context context, String packagePath) throws IOException {
        File applicationPackage = new File(packagePath);
        ZipFile zip = new ZipFile(applicationPackage);

        boolean distributionSet = false;
        Enumeration<? extends ZipEntry> zipEntries = zip.entries();
        while (zipEntries.hasMoreElements()) {
            ZipEntry fileEntry = zipEntries.nextElement();
            String name = fileEntry.getName();

            if (!name.startsWith("distribution/"))
                continue;

            distributionSet = true;

            File dataDir = new File(context.getApplicationInfo().dataDir);
            File outFile = new File(dataDir, name);

            File dir = outFile.getParentFile();
            if (!dir.exists())
                dir.mkdirs();

            InputStream fileStream = zip.getInputStream(fileEntry);
            OutputStream outStream = new FileOutputStream(outFile);

            int b;
            while ((b = fileStream.read()) != -1)
                outStream.write(b);

            fileStream.close();
            outStream.close();
            outFile.setLastModified(fileEntry.getTime());
        }

        zip.close();

        return distributionSet;
    }

    



    public static JSONArray getBookmarks(Context context) {
        SharedPreferences settings = context.getSharedPreferences(GeckoApp.PREFS_NAME, Activity.MODE_PRIVATE);
        String keyName = context.getPackageName() + ".distribution_state";
        int state = settings.getInt(keyName, STATE_UNKNOWN);
        if (state == STATE_NONE) {
            return null;
        }

        ZipFile zip = null;
        InputStream inputStream = null;
        try {
            if (state == STATE_UNKNOWN) {
                
                File applicationPackage = new File(context.getPackageResourcePath());
                zip = new ZipFile(applicationPackage);
                ZipEntry zipEntry = zip.getEntry("distribution/bookmarks.json");
                if (zipEntry == null) {
                    return null;
                }
                inputStream = zip.getInputStream(zipEntry);
            } else {
                
                File dataDir = new File(context.getApplicationInfo().dataDir);
                File file = new File(dataDir, "distribution/bookmarks.json");
                inputStream = new FileInputStream(file);
            }

            
            BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
            StringBuilder stringBuilder = new StringBuilder();
            String s;
            while ((s = reader.readLine()) != null) {
                stringBuilder.append(s);
            }
            return new JSONArray(stringBuilder.toString());
        } catch (IOException e) {
            Log.e(LOGTAG, "Error getting bookmarks", e);
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error parsing bookmarks.json", e);
        } finally {
            try {
                if (zip != null) {
                    zip.close();
                }
                if (inputStream != null) {
                    inputStream.close();
                }
            } catch (IOException e) {
                Log.e(LOGTAG, "Error closing streams", e);
            } 
        }
        return null;
    }
}
