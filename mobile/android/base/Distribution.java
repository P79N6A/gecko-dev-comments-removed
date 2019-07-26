




package org.mozilla.gecko;

import org.mozilla.gecko.util.ThreadUtils;

import org.json.JSONArray;
import org.json.JSONException;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Scanner;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public final class Distribution {
    private static final String LOGTAG = "GeckoDistribution";

    private static final int STATE_UNKNOWN = 0;
    private static final int STATE_NONE = 1;
    private static final int STATE_SET = 2;

    





    public static void init(final Context context, final String packagePath) {
        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                Distribution dist = new Distribution(context, packagePath);
                boolean distributionSet = dist.doInit();
                if (distributionSet) {
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Distribution:Set", ""));
                }
            }
        });
    }

    



    public static void init(final Context context) {
        Distribution.init(context, context.getPackageResourcePath());
    }

    



    public static JSONArray getBookmarks(final Context context) {
        Distribution dist = new Distribution(context);
        return dist.getBookmarks();
    }

    private final String packagePath;
    private final Context context;

    private int state = STATE_UNKNOWN;
    private File distributionDir = null;

    


    public Distribution(final Context context, final String packagePath) {
        this.context = context;
        this.packagePath = packagePath;
    }

    public Distribution(final Context context) {
        this(context, context.getPackageResourcePath());
    }

    




    private boolean doInit() {
        
        
        SharedPreferences settings = context.getSharedPreferences(GeckoApp.PREFS_NAME, Activity.MODE_PRIVATE);
        String keyName = context.getPackageName() + ".distribution_state";
        this.state = settings.getInt(keyName, STATE_UNKNOWN);
        if (this.state == STATE_NONE) {
            return false;
        }

        
        if (this.state == STATE_SET) {
            
            
            return true;
        }

        boolean distributionSet = false;
        try {
            
            distributionSet = copyFiles();
            if (distributionSet) {
                
                
                
                this.distributionDir = new File(getDataDir(), "distribution/");
            }
        } catch (IOException e) {
            Log.e(LOGTAG, "Error copying distribution files", e);
        }

        if (!distributionSet) {
            
            File distDir = getSystemDistributionDir();
            if (distDir.exists()) {
                distributionSet = true;
                this.distributionDir = distDir;
            }
        }

        this.state = distributionSet ? STATE_SET : STATE_NONE;
        settings.edit().putInt(keyName, this.state).commit();
        return distributionSet;
    }

    



    private boolean copyFiles() throws IOException {
        File applicationPackage = new File(packagePath);
        ZipFile zip = new ZipFile(applicationPackage);

        boolean distributionSet = false;
        Enumeration<? extends ZipEntry> zipEntries = zip.entries();

        byte[] buffer = new byte[1024];
        while (zipEntries.hasMoreElements()) {
            ZipEntry fileEntry = zipEntries.nextElement();
            String name = fileEntry.getName();

            if (!name.startsWith("distribution/")) {
                continue;
            }

            distributionSet = true;

            File outFile = new File(getDataDir(), name);
            File dir = outFile.getParentFile();

            if (!dir.exists()) {
                if (!dir.mkdirs()) {
                    Log.e(LOGTAG, "Unable to create directories: " + dir.getAbsolutePath());
                    continue;
                }
            }

            InputStream fileStream = zip.getInputStream(fileEntry);
            OutputStream outStream = new FileOutputStream(outFile);

            int count;
            while ((count = fileStream.read(buffer)) != -1) {
                outStream.write(buffer, 0, count);
            }

            fileStream.close();
            outStream.close();
            outFile.setLastModified(fileEntry.getTime());
        }

        zip.close();

        return distributionSet;
    }

    





    private File ensureDistributionDir() {
        if (this.distributionDir != null) {
            return this.distributionDir;
        }

        if (this.state != STATE_SET) {
            return null;
        }

        
        
        
        
        File copied = new File(getDataDir(), "distribution/");
        if (copied.exists()) {
            return this.distributionDir = copied;
        }
        File system = getSystemDistributionDir();
        if (system.exists()) {
            return this.distributionDir = system;
        }
        return null;
    }

    public JSONArray getBookmarks() {
        if (this.state == STATE_UNKNOWN) {
            this.doInit();
        }

        File dist = ensureDistributionDir();
        if (dist == null) {
            return null;
        }

        File bookmarks = new File(dist, "bookmarks.json");
        if (!bookmarks.exists()) {
            return null;
        }

        
        try {
            Scanner scanner = null;
            try {
                scanner = new Scanner(bookmarks, "UTF-8");
                final String contents = scanner.useDelimiter("\\A").next();
                return new JSONArray(contents);
            } finally {
                if (scanner != null) {
                    scanner.close();
                }
            }

        } catch (IOException e) {
            Log.e(LOGTAG, "Error getting bookmarks", e);
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error parsing bookmarks.json", e);
        }

        return null;
    }

    private String getDataDir() {
        return context.getApplicationInfo().dataDir;
    }

    private File getSystemDistributionDir() {
        return new File("/system/" + context.getPackageName() + "/distribution");
    }
}
