




package org.mozilla.gecko.distribution;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Queue;
import java.util.Scanner;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;





public final class Distribution {
    private static final String LOGTAG = "GeckoDistribution";

    private static final int STATE_UNKNOWN = 0;
    private static final int STATE_NONE = 1;
    private static final int STATE_SET = 2;

    private static Distribution instance;

    private final Context context;
    private final String packagePath;
    private final String prefsBranch;

    private volatile int state = STATE_UNKNOWN;
    private File distributionDir = null;

    private final Queue<Runnable> onDistributionReady = new ConcurrentLinkedQueue<Runnable>();

    




    public static synchronized Distribution getInstance(Context context) {
        if (instance == null) {
            instance = new Distribution(context);
        }
        return instance;
    }

    public static class DistributionDescriptor {
        public final boolean valid;
        public final String id;
        public final String version;    

        
        public final String about;

        
        
        
        
        public final Map<String, String> localizedAbout;

        @SuppressWarnings("unchecked")
        public DistributionDescriptor(JSONObject obj) {
            this.id = obj.optString("id");
            this.version = obj.optString("version");
            this.about = obj.optString("about");
            Map<String, String> loc = new HashMap<String, String>();
            try {
                Iterator<String> keys = obj.keys();
                while (keys.hasNext()) {
                    String key = keys.next();
                    if (key.startsWith("about.")) {
                        String locale = key.substring(6);
                        if (!obj.isNull(locale)) {
                            loc.put(locale, obj.getString(key));
                        }
                    }
                }
            } catch (JSONException ex) {
                Log.w(LOGTAG, "Unable to completely process distribution JSON.", ex);
            }

            this.localizedAbout = Collections.unmodifiableMap(loc);
            this.valid = (null != this.id) &&
                         (null != this.version) &&
                         (null != this.about);
        }
    }

    private static void init(final Distribution distribution) {
        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                boolean distributionSet = distribution.doInit();
                if (distributionSet) {
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Distribution:Set", ""));
                }
            }
        });
    }

    





    @RobocopTarget
    public static void init(final Context context, final String packagePath, final String prefsPath) {
        init(new Distribution(context, packagePath, prefsPath));
    }

    



    public static void init(final Context context) {
        Distribution.init(Distribution.getInstance(context));
    }

    



    public static JSONArray getBookmarks(final Context context) {
        Distribution dist = new Distribution(context);
        return dist.getBookmarks();
    }

    


    public Distribution(final Context context, final String packagePath, final String prefsBranch) {
        this.context = context;
        this.packagePath = packagePath;
        this.prefsBranch = prefsBranch;
    }

    public Distribution(final Context context) {
        this(context, context.getPackageResourcePath(), null);
    }

    





    public File getDistributionFile(String name) {
        Log.d(LOGTAG, "Getting file from distribution.");

        if (this.state == STATE_UNKNOWN) {
            if (!this.doInit()) {
                return null;
            }
        }

        File dist = ensureDistributionDir();
        if (dist == null) {
            return null;
        }

        File descFile = new File(dist, name);
        if (!descFile.exists()) {
            Log.e(LOGTAG, "Distribution directory exists, but no file named " + name);
            return null;
        }

        return descFile;
    }

    public DistributionDescriptor getDescriptor() {
        File descFile = getDistributionFile("preferences.json");
        if (descFile == null) {
            
            return null;
        }

        try {
            JSONObject all = new JSONObject(getFileContents(descFile));

            if (!all.has("Global")) {
                Log.e(LOGTAG, "Distribution preferences.json has no Global entry!");
                return null;
            }

            return new DistributionDescriptor(all.getJSONObject("Global"));

        } catch (IOException e) {
            Log.e(LOGTAG, "Error getting distribution descriptor file.", e);
            return null;
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error parsing preferences.json", e);
            return null;
        }
    }

    public JSONArray getBookmarks() {
        File bookmarks = getDistributionFile("bookmarks.json");
        if (bookmarks == null) {
            
            return null;
        }

        try {
            return new JSONArray(getFileContents(bookmarks));
        } catch (IOException e) {
            Log.e(LOGTAG, "Error getting bookmarks", e);
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error parsing bookmarks.json", e);
        }

        return null;
    }

    







    private boolean doInit() {
        ThreadUtils.assertNotOnUiThread();

        
        
        final SharedPreferences settings;
        if (prefsBranch == null) {
            settings = GeckoSharedPrefs.forApp(context);
        } else {
            settings = context.getSharedPreferences(prefsBranch, Activity.MODE_PRIVATE);
        }

        String keyName = context.getPackageName() + ".distribution_state";
        this.state = settings.getInt(keyName, STATE_UNKNOWN);
        if (this.state == STATE_NONE) {
            runReadyQueue();
            return false;
        }

        
        if (this.state == STATE_SET) {
            
            
            runReadyQueue();
            return true;
        }

        
        final boolean distributionSet =
                checkAPKDistribution() ||
                checkSystemDistribution();

        this.state = distributionSet ? STATE_SET : STATE_NONE;
        settings.edit().putInt(keyName, this.state).commit();

        runReadyQueue();
        return distributionSet;
    }

    




    private void runReadyQueue() {
        Runnable task;
        while ((task = onDistributionReady.poll()) != null) {
            ThreadUtils.postToBackgroundThread(task);
        }
    }

    


    private boolean checkAPKDistribution() {
        try {
            
            if (copyFiles()) {
                
                
                
                this.distributionDir = new File(getDataDir(), "distribution/");
                return true;
            }
        } catch (IOException e) {
            Log.e(LOGTAG, "Error copying distribution files from APK.", e);
        }
        return false;
    }

    


    private boolean checkSystemDistribution() {
        
        final File distDir = getSystemDistributionDir();
        if (distDir.exists()) {
            this.distributionDir = distDir;
            return true;
        }
        return false;
    }

    



    private boolean copyFiles() throws IOException {
        final File applicationPackage = new File(packagePath);
        final ZipFile zip = new ZipFile(applicationPackage);

        boolean distributionSet = false;
        try {
            final byte[] buffer = new byte[1024];

            final Enumeration<? extends ZipEntry> zipEntries = zip.entries();
            while (zipEntries.hasMoreElements()) {
                final ZipEntry fileEntry = zipEntries.nextElement();
                final String name = fileEntry.getName();

                if (fileEntry.isDirectory()) {
                    
                    continue;
                }

                if (!name.startsWith("distribution/")) {
                    continue;
                }

                final File outFile = getDataFile(name);
                if (outFile == null) {
                    continue;
                }

                distributionSet = true;

                final InputStream fileStream = zip.getInputStream(fileEntry);
                try {
                    writeStream(fileStream, outFile, fileEntry.getTime(), buffer);
                } finally {
                    fileStream.close();
                }
            }
        } finally {
            zip.close();
        }

        return distributionSet;
    }

    private void writeStream(InputStream fileStream, File outFile, final long modifiedTime, byte[] buffer)
            throws FileNotFoundException, IOException {
        final OutputStream outStream = new FileOutputStream(outFile);
        try {
            int count;
            while ((count = fileStream.read(buffer)) > 0) {
                outStream.write(buffer, 0, count);
            }

            outFile.setLastModified(modifiedTime);
        } finally {
            outStream.close();
        }
    }

    





    private File getDataFile(final String name) {
        File outFile = new File(getDataDir(), name);
        File dir = outFile.getParentFile();

        if (!dir.exists()) {
            Log.d(LOGTAG, "Creating " + dir.getAbsolutePath());
            if (!dir.mkdirs()) {
                Log.e(LOGTAG, "Unable to create directories: " + dir.getAbsolutePath());
                return null;
            }
        }

        return outFile;
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

    
    private String getFileContents(File file) throws IOException {
        Scanner scanner = null;
        try {
            scanner = new Scanner(file, "UTF-8");
            return scanner.useDelimiter("\\A").next();
        } finally {
            if (scanner != null) {
                scanner.close();
            }
        }
    }

    private String getDataDir() {
        return context.getApplicationInfo().dataDir;
    }

    private File getSystemDistributionDir() {
        return new File("/system/" + context.getPackageName() + "/distribution");
    }

    






    public void addOnDistributionReadyCallback(Runnable runnable) {
        if (state == STATE_UNKNOWN) {
            this.onDistributionReady.add(runnable);
        } else {
            
            ThreadUtils.postToBackgroundThread(runnable);
        }
    }

    



    public boolean exists() {
        return state == STATE_SET;
    }
}
