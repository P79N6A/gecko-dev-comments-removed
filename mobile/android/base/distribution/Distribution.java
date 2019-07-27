




package org.mozilla.gecko.distribution;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.ProtocolException;
import java.net.SocketException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.UnknownHostException;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Queue;
import java.util.Scanner;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.jar.JarEntry;
import java.util.jar.JarInputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import javax.net.ssl.SSLException;

import org.apache.http.protocol.HTTP;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.Telemetry;
import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.util.ThreadUtils;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.SystemClock;
import android.util.Log;





@RobocopTarget
public class Distribution {
    private static final String LOGTAG = "GeckoDistribution";

    private static final int STATE_UNKNOWN = 0;
    private static final int STATE_NONE = 1;
    private static final int STATE_SET = 2;

    private static final String FETCH_PROTOCOL = "https";
    private static final String FETCH_HOSTNAME = "distro-download.cdn.mozilla.net";
    private static final String FETCH_PATH = "/android/1/";
    private static final String FETCH_EXTENSION = ".jar";

    private static final String EXPECTED_CONTENT_TYPE = "application/java-archive";

    private static final String DISTRIBUTION_PATH = "distribution/";

    


    private static final String HISTOGRAM_REFERRER_INVALID = "FENNEC_DISTRIBUTION_REFERRER_INVALID";
    private static final String HISTOGRAM_DOWNLOAD_TIME_MS = "FENNEC_DISTRIBUTION_DOWNLOAD_TIME_MS";
    private static final String HISTOGRAM_CODE_CATEGORY = "FENNEC_DISTRIBUTION_CODE_CATEGORY";

    


    private static final int CODE_CATEGORY_STATUS_OUT_OF_RANGE = 0;
    
    private static final int CODE_CATEGORY_OFFLINE = 6;
    private static final int CODE_CATEGORY_FETCH_EXCEPTION = 7;

    
    
    private static final int CODE_CATEGORY_POST_FETCH_EXCEPTION = 8;
    private static final int CODE_CATEGORY_POST_FETCH_SECURITY_EXCEPTION = 9;

    
    
    private static final int CODE_CATEGORY_MALFORMED_DISTRIBUTION = 10;

    
    private static final int CODE_CATEGORY_FETCH_SOCKET_ERROR = 11;
    private static final int CODE_CATEGORY_FETCH_SSL_ERROR = 12;
    private static final int CODE_CATEGORY_FETCH_NON_SUCCESS_RESPONSE = 13;
    private static final int CODE_CATEGORY_FETCH_INVALID_CONTENT_TYPE = 14;

    
    private static final long MAX_DOWNLOAD_TIME_MSEC = 40000;    



    





    @RobocopTarget
    protected static volatile ReferrerDescriptor referrer;

    private static Distribution instance;

    private final Context context;
    private final String packagePath;
    private final String prefsBranch;

    private volatile int state = STATE_UNKNOWN;
    private File distributionDir;

    private final Queue<Runnable> onDistributionReady = new ConcurrentLinkedQueue<Runnable>();

    




    public static synchronized Distribution getInstance(Context context) {
        if (instance == null) {
            instance = new Distribution(context);
        }
        return instance;
    }

    @RobocopTarget
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

    private static Distribution init(final Distribution distribution) {
        
        ThreadUtils.postToBackgroundThread(new Runnable() {
            @Override
            public void run() {
                boolean distributionSet = distribution.doInit();
                if (distributionSet) {
                    GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent("Distribution:Set", ""));
                }
            }
        });

        return distribution;
    }

    





    @RobocopTarget
    public static Distribution init(final Context context, final String packagePath, final String prefsPath) {
        return init(new Distribution(context, packagePath, prefsPath));
    }

    



    @RobocopTarget
    public static Distribution init(final Context context) {
        return init(Distribution.getInstance(context));
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

    





    public static void onReceivedReferrer(ReferrerDescriptor ref) {
        
        referrer = ref;
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
            Telemetry.HistogramAdd(HISTOGRAM_CODE_CATEGORY, CODE_CATEGORY_MALFORMED_DISTRIBUTION);
            return null;
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error parsing preferences.json", e);
            Telemetry.HistogramAdd(HISTOGRAM_CODE_CATEGORY, CODE_CATEGORY_MALFORMED_DISTRIBUTION);
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
            Telemetry.HistogramAdd(HISTOGRAM_CODE_CATEGORY, CODE_CATEGORY_MALFORMED_DISTRIBUTION);
            return null;
        } catch (JSONException e) {
            Log.e(LOGTAG, "Error parsing bookmarks.json", e);
            Telemetry.HistogramAdd(HISTOGRAM_CODE_CATEGORY, CODE_CATEGORY_MALFORMED_DISTRIBUTION);
            return null;
        }
    }

    









    @RobocopTarget
    protected boolean doInit() {
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
                checkIntentDistribution() ||
                checkAPKDistribution() ||
                checkSystemDistribution();

        this.state = distributionSet ? STATE_SET : STATE_NONE;
        settings.edit().putInt(keyName, this.state).apply();

        runReadyQueue();
        return distributionSet;
    }

    





    private boolean checkIntentDistribution() {
        if (referrer == null) {
            return false;
        }

        URI uri = getReferredDistribution(referrer);
        if (uri == null) {
            return false;
        }

        long start = SystemClock.uptimeMillis();
        Log.v(LOGTAG, "Downloading referred distribution: " + uri);

        try {
            HttpURLConnection connection = (HttpURLConnection) uri.toURL().openConnection();

            connection.setRequestProperty(HTTP.USER_AGENT, GeckoAppShell.getGeckoInterface().getDefaultUAString());
            connection.setRequestProperty("Accept", EXPECTED_CONTENT_TYPE);

            try {
                final JarInputStream distro;
                try {
                    distro = fetchDistribution(uri, connection);
                } catch (Exception e) {
                    Log.e(LOGTAG, "Error fetching distribution from network.", e);
                    recordFetchTelemetry(e);
                    return false;
                }

                long end = SystemClock.uptimeMillis();
                final long duration = end - start;
                Log.d(LOGTAG, "Distro fetch took " + duration + "ms; result? " + (distro != null));
                Telemetry.HistogramAdd(HISTOGRAM_DOWNLOAD_TIME_MS, clamp(MAX_DOWNLOAD_TIME_MSEC, duration));

                if (distro == null) {
                    
                    return false;
                }

                
                try {
                    Log.d(LOGTAG, "Copying files from fetched zip.");
                    if (copyFilesFromStream(distro)) {
                        
                        
                        
                        this.distributionDir = new File(getDataDir(), DISTRIBUTION_PATH);
                        return true;
                    }
                } catch (SecurityException e) {
                    Log.e(LOGTAG, "Security exception copying files. Corrupt or malicious?", e);
                    Telemetry.HistogramAdd(HISTOGRAM_CODE_CATEGORY, CODE_CATEGORY_POST_FETCH_SECURITY_EXCEPTION);
                } catch (Exception e) {
                    Log.e(LOGTAG, "Error copying files from distribution.", e);
                    Telemetry.HistogramAdd(HISTOGRAM_CODE_CATEGORY, CODE_CATEGORY_POST_FETCH_EXCEPTION);
                } finally {
                    distro.close();
                }
            } finally {
                connection.disconnect();
            }
        } catch (IOException e) {
            Log.e(LOGTAG, "Error copying distribution files from network.", e);
            recordFetchTelemetry(e);
        }

        return false;
    }

    private static final int clamp(long v, long c) {
        return (int) Math.min(c, v);
    }

    







    @SuppressWarnings("static-method")
    @RobocopTarget
    protected JarInputStream fetchDistribution(URI uri, HttpURLConnection connection) throws IOException {
        final int status = connection.getResponseCode();

        Log.d(LOGTAG, "Distribution fetch: " + status);
        
        final int value;
        if (status > 599 || status < 100) {
            Log.wtf(LOGTAG, "Unexpected HTTP status code: " + status);
            value = CODE_CATEGORY_STATUS_OUT_OF_RANGE;
        } else {
            value = status / 100;
        }
        
        Telemetry.HistogramAdd(HISTOGRAM_CODE_CATEGORY, value);

        if (status != 200) {
            Log.w(LOGTAG, "Got status " + status + " fetching distribution.");
            Telemetry.HistogramAdd(HISTOGRAM_CODE_CATEGORY, CODE_CATEGORY_FETCH_NON_SUCCESS_RESPONSE);
            return null;
        }

        final String contentType = connection.getContentType();
        if (contentType == null || !contentType.startsWith(EXPECTED_CONTENT_TYPE)) {
            Log.w(LOGTAG, "Malformed response: invalid Content-Type.");
            Telemetry.HistogramAdd(HISTOGRAM_CODE_CATEGORY, CODE_CATEGORY_FETCH_INVALID_CONTENT_TYPE);
            return null;
        }

        return new JarInputStream(new BufferedInputStream(connection.getInputStream()), true);
    }

    private static void recordFetchTelemetry(final Exception exception) {
        if (exception == null) {
            
            Telemetry.HistogramAdd(HISTOGRAM_CODE_CATEGORY, CODE_CATEGORY_FETCH_EXCEPTION);
            return;
        }

        if (exception instanceof UnknownHostException) {
            
            Telemetry.HistogramAdd(HISTOGRAM_CODE_CATEGORY, CODE_CATEGORY_OFFLINE);
            return;
        }

        if (exception instanceof SSLException) {
            Telemetry.HistogramAdd(HISTOGRAM_CODE_CATEGORY, CODE_CATEGORY_FETCH_SSL_ERROR);
            return;
        }

        if (exception instanceof ProtocolException ||
            exception instanceof SocketException) {
            Telemetry.HistogramAdd(HISTOGRAM_CODE_CATEGORY, CODE_CATEGORY_FETCH_SOCKET_ERROR);
            return;
        }

        Telemetry.HistogramAdd(HISTOGRAM_CODE_CATEGORY, CODE_CATEGORY_FETCH_EXCEPTION);
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
                
                
                
                this.distributionDir = new File(getDataDir(), DISTRIBUTION_PATH);
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

    




    private boolean copyFilesFromStream(JarInputStream jar) throws FileNotFoundException, IOException {
        final byte[] buffer = new byte[1024];
        boolean distributionSet = false;
        JarEntry entry;
        while ((entry = jar.getNextJarEntry()) != null) {
            final String name = entry.getName();

            if (entry.isDirectory()) {
                
                
                continue;
            }

            if (!name.startsWith(DISTRIBUTION_PATH)) {
                continue;
            }

            File outFile = getDataFile(name);
            if (outFile == null) {
                continue;
            }

            distributionSet = true;

            writeStream(jar, outFile, entry.getTime(), buffer);
        }

        return distributionSet;
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

                if (!name.startsWith(DISTRIBUTION_PATH)) {
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

    private URI getReferredDistribution(ReferrerDescriptor descriptor) {
        final String content = descriptor.content;
        if (content == null) {
            return null;
        }

        
        
        if (!content.matches("^[a-zA-Z0-9]+$")) {
            Log.e(LOGTAG, "Invalid referrer content: " + content);
            Telemetry.HistogramAdd(HISTOGRAM_REFERRER_INVALID, 1);
            return null;
        }

        try {
            return new URI(FETCH_PROTOCOL, FETCH_HOSTNAME, FETCH_PATH + content + FETCH_EXTENSION, null);
        } catch (URISyntaxException e) {
            
            Log.wtf(LOGTAG, "Invalid URI with content " + content + "!");
            return null;
        }
    }

    





    private File ensureDistributionDir() {
        if (this.distributionDir != null) {
            return this.distributionDir;
        }

        if (this.state != STATE_SET) {
            return null;
        }

        
        
        
        
        File copied = new File(getDataDir(), DISTRIBUTION_PATH);
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
