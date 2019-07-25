






































package org.mozilla.gecko;

import java.io.*;
import java.util.*;
import java.util.zip.*;
import java.nio.*;
import java.nio.channels.FileChannel;
import java.util.concurrent.*;

import android.os.*;
import android.app.*;
import android.text.*;
import android.view.*;
import android.view.inputmethod.*;
import android.content.*;
import android.content.res.*;
import android.graphics.*;
import android.widget.*;
import android.hardware.*;

import android.util.*;
import android.net.*;

abstract public class GeckoApp
    extends Activity
{
    public static final String ACTION_ALERT_CLICK = "org.mozilla.gecko.ACTION_ALERT_CLICK";
    public static final String ACTION_ALERT_CLEAR = "org.mozilla.gecko.ACTION_ALERT_CLEAR";

    public static FrameLayout mainLayout;
    public static GeckoSurfaceView surfaceView;
    public static GeckoApp mAppContext;
    public static boolean mFullscreen = false;
    public static boolean mStartedEarly = false;
    public static File sGREDir = null;
    static Thread mLibLoadThread = null;
    public Handler mMainHandler;

    enum LaunchState {PreLaunch, Launching, WaitButton,
                      Launched, GeckoRunning, GeckoExiting};
    private static LaunchState sLaunchState = LaunchState.PreLaunch;


    static boolean checkLaunchState(LaunchState checkState) {
        synchronized(sLaunchState) {
            return sLaunchState == checkState;
        }
    }

    static void setLaunchState(LaunchState setState) {
        synchronized(sLaunchState) {
            sLaunchState = setState;
        }
    }

    
    
    static boolean checkAndSetLaunchState(LaunchState checkState, LaunchState setState) {
        synchronized(sLaunchState) {
            if (sLaunchState != checkState)
                return false;
            sLaunchState = setState;
            return true;
        }
    }

    void showErrorDialog(String message)
    {
        new AlertDialog.Builder(this)
            .setMessage(message)
            .setCancelable(false)
            .setPositiveButton(R.string.exit_label,
                               new DialogInterface.OnClickListener() {
                                   public void onClick(DialogInterface dialog,
                                                       int id)
                                   {
                                       GeckoApp.this.finish();
                                       System.exit(0);
                                   }
                               }).show();
    }

    
    boolean launch(Intent intent)
    {
        if (!checkAndSetLaunchState(LaunchState.Launching, LaunchState.Launched))
            return false;

        if (intent == null)
            intent = getIntent();
        final Intent i = intent;
        new Thread() {
            public void run() {
                long startup_time = System.currentTimeMillis();
                try {
                    if (mLibLoadThread != null)
                        mLibLoadThread.join();
                } catch (InterruptedException ie) {}
                surfaceView.mSplashStatusMsg =
                    getResources().getString(R.string.splash_screen_label);
                surfaceView.drawSplashScreen();
                
                try {
                    unpackComponents();
                } catch (FileNotFoundException fnfe) {
                    Log.e("GeckoApp", "error unpacking components", fnfe);
                    Looper.prepare();
                    showErrorDialog(getString(R.string.error_loading_file));
                    Looper.loop();
                    return;
                } catch (IOException ie) {
                    Log.e("GeckoApp", "error unpacking components", ie);
                    String msg = ie.getMessage();
                    Looper.prepare();
                    if (msg != null && msg.equalsIgnoreCase("No space left on device"))
                        showErrorDialog(getString(R.string.no_space_to_start_error));
                    else
                        showErrorDialog(getString(R.string.error_loading_file));
                    Looper.loop();
                    return;
                }

                
                String env = i.getStringExtra("env0");
                if (GeckoApp.mStartedEarly) {
                    GeckoAppShell.putenv("MOZ_APP_RESTART=" + startup_time);
                }
                GeckoAppShell.runGecko(getApplication().getPackageResourcePath(),
                                       i.getStringExtra("args"),
                                       i.getDataString());
            }
        }.start();
        return true;
    }

    
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        mAppContext = this;
        mMainHandler = new Handler();

        SharedPreferences settings = getPreferences(Activity.MODE_PRIVATE);
        String localeCode = settings.getString(getPackageName() + ".locale", "");
        if (localeCode != null && localeCode.length() > 0)
            GeckoAppShell.setSelectedLocale(localeCode);

        Log.i("GeckoApp", "create");
        super.onCreate(savedInstanceState);

        if (sGREDir == null)
            sGREDir = new File(this.getApplicationInfo().dataDir);

        getWindow().setFlags(mFullscreen ?
                             WindowManager.LayoutParams.FLAG_FULLSCREEN : 0,
                             WindowManager.LayoutParams.FLAG_FULLSCREEN);

        if (surfaceView == null)
            surfaceView = new GeckoSurfaceView(this);
        else
            mainLayout.removeView(surfaceView);

        mainLayout = new FrameLayout(this);
        mainLayout.addView(surfaceView,
                           new FrameLayout.LayoutParams(FrameLayout.LayoutParams.FILL_PARENT,
                                                        FrameLayout.LayoutParams.FILL_PARENT));

        setContentView(mainLayout,
                       new ViewGroup.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT,
                                                  ViewGroup.LayoutParams.FILL_PARENT));

        if (!checkAndSetLaunchState(LaunchState.PreLaunch,
                                    LaunchState.Launching))
            return;

        checkAndLaunchUpdate();
        mLibLoadThread = new Thread(new Runnable() {
            public void run() {
                
                
                Locale locale = Locale.getDefault();
                GeckoAppShell.loadGeckoLibs(
                    getApplication().getPackageResourcePath());
                Locale.setDefault(locale);
                Resources res = getBaseContext().getResources();
                Configuration config = res.getConfiguration();
                config.locale = locale;
                res.updateConfiguration(config, res.getDisplayMetrics());


            }});
        File cacheFile = GeckoAppShell.getCacheDir();
        File libxulFile = new File(cacheFile, "libxul.so");

        if (GeckoAppShell.getFreeSpace() > GeckoAppShell.kFreeSpaceThreshold &&
            (!libxulFile.exists() ||
             new File(getApplication().getPackageResourcePath()).lastModified()
             >= libxulFile.lastModified()))
            surfaceView.mSplashStatusMsg =
                getResources().getString(R.string.splash_screen_installing);
        else
            surfaceView.mSplashStatusMsg =
                getResources().getString(R.string.splash_screen_label);
        mLibLoadThread.start();
        if (IsNewInstall() && IsUnsupportedDevice()) {
            new AlertDialog.Builder(this)
                .setMessage(R.string.incompatable_device)
                .setCancelable(false)
                .setPositiveButton(R.string.continue_label, null)
                .setNegativeButton(R.string.exit_label,
                                   new DialogInterface.OnClickListener() {
                                       public void onClick(DialogInterface dialog,
                                                           int id)
                                       {
                                           GeckoApp.this.finish();
                                           System.exit(0);
                                       }
                                   })
                .show();
        }
    }

    boolean IsNewInstall() {
        File appIni = new File(sGREDir, "application.ini");
        return !appIni.exists();
    }

    boolean IsUnsupportedDevice() {
        
        File meminfo = new File("/proc/meminfo");
        try {
            BufferedReader br = new BufferedReader(new FileReader(meminfo));
            String totalMem = "";
            while(!totalMem.contains("MemTotal:") && totalMem != null)
                totalMem = br.readLine();
            StringTokenizer st = new StringTokenizer(totalMem, " ");
            st.nextToken(); 
            totalMem = st.nextToken();

            Log.i("GeckoMemory", "MemTotal: " + Integer.parseInt(totalMem));
            return Integer.parseInt(totalMem) <= 524288L;
        } catch (Exception ex) {
            
            
            
            
            
            Log.w("GeckoMemTest", "Exception when finding total memory", ex);
        }
        return false;
    }

    @Override
    protected void onNewIntent(Intent intent) {
        if (checkLaunchState(LaunchState.GeckoExiting)) {
            
            
            System.exit(0);
            return;
        }
        final String action = intent.getAction();
        if ("org.mozilla.gecko.DEBUG".equals(action) &&
            checkAndSetLaunchState(LaunchState.Launching, LaunchState.WaitButton)) {
            final Button launchButton = new Button(this);
            launchButton.setText("Launch"); 
            launchButton.setOnClickListener(new Button.OnClickListener() {
                public void onClick (View v) {
                    
                    mainLayout.removeView(launchButton);
                    setLaunchState(LaunchState.Launching);
                    launch(null);
                }
            });
            mainLayout.addView(launchButton, 300, 200);
            return;
        }
        if (checkLaunchState(LaunchState.WaitButton) || launch(intent))
            return;

        if (Intent.ACTION_VIEW.equals(action)) {
            String uri = intent.getDataString();
            GeckoAppShell.sendEventToGecko(new GeckoEvent(uri));
            Log.i("GeckoApp","onNewIntent: "+uri);
        }
        else if (Intent.ACTION_MAIN.equals(action)) {
            Log.i("GeckoApp", "Intent : ACTION_MAIN");
            GeckoAppShell.sendEventToGecko(new GeckoEvent(""));
        }
        else if (action.equals("org.mozilla.fennec.WEBAPP")) {
            String uri = intent.getStringExtra("args");
            GeckoAppShell.sendEventToGecko(new GeckoEvent(uri));
            Log.i("GeckoApp","Intent : WEBAPP - " + uri);
        }
    }

    @Override
    public void onPause()
    {
        Log.i("GeckoApp", "pause");
        GeckoAppShell.sendEventToGecko(new GeckoEvent(GeckoEvent.ACTIVITY_PAUSING));
        
        
        

        
        

        
        super.onPause();
    }

    @Override
    public void onResume()
    {
        Log.i("GeckoApp", "resume");
        if (checkLaunchState(LaunchState.GeckoRunning))
            GeckoAppShell.onResume();
        
        
        super.onResume();

        
        if (checkLaunchState(LaunchState.PreLaunch) ||
            checkLaunchState(LaunchState.Launching))
            onNewIntent(getIntent());
    }

    @Override
    public void onStop()
    {
        Log.i("GeckoApp", "stop");
        
        
        
        
        
        
        
        
        
        

        GeckoAppShell.sendEventToGecko(new GeckoEvent(GeckoEvent.ACTIVITY_STOPPING));
        super.onStop();
    }

    @Override
    public void onRestart()
    {
        Log.i("GeckoApp", "restart");
        super.onRestart();
    }

    @Override
    public void onStart()
    {
        Log.i("GeckoApp", "start");
        super.onStart();
    }

    @Override
    public void onDestroy()
    {
        Log.i("GeckoApp", "destroy");
        
        
        if (isFinishing())
            GeckoAppShell.sendEventToGecko(new GeckoEvent(GeckoEvent.ACTIVITY_SHUTDOWN));

        super.onDestroy();
    }

    @Override
    public void onConfigurationChanged(android.content.res.Configuration newConfig)
    {
        Log.i("GeckoApp", "configuration changed");
        
        super.onConfigurationChanged(newConfig);
    }

    @Override
    public void onLowMemory()
    {
        Log.e("GeckoApp", "low memory");
        if (checkLaunchState(LaunchState.GeckoRunning))
            GeckoAppShell.onLowMemory();
        super.onLowMemory();
    }

    abstract public String getPackageName();
    abstract public String getContentProcessName();

    protected void unpackComponents()
        throws IOException, FileNotFoundException
    {
        ZipFile zip;
        InputStream listStream;

        File componentsDir = new File(sGREDir, "components");
        componentsDir.mkdir();
        zip = new ZipFile(getApplication().getPackageResourcePath());

        byte[] buf = new byte[8192];
        try {
            if (unpackFile(zip, buf, null, "removed-files"))
                removeFiles();
        } catch (Exception ex) {
            
            Log.w("GeckoApp", "error removing files", ex);
        }
        unpackFile(zip, buf, null, "application.ini");
        unpackFile(zip, buf, null, getContentProcessName());
        try {
            unpackFile(zip, buf, null, "update.locale");
        } catch (Exception e) {}

        
        Enumeration<? extends ZipEntry> zipEntries = zip.entries();
        while (zipEntries.hasMoreElements()) {
          ZipEntry entry = zipEntries.nextElement();
          if (entry.getName().startsWith("extensions/") && entry.getName().endsWith(".xpi")) {
            Log.i("GeckoAppJava", "installing extension : " + entry.getName());
            unpackFile(zip, buf, entry, entry.getName());
          }
        }
    }

    void removeFiles() throws IOException {
        BufferedReader reader = new BufferedReader(
            new FileReader(new File(sGREDir, "removed-files")));
        try {
            for (String removedFileName = reader.readLine(); 
                 removedFileName != null; removedFileName = reader.readLine()) {
                File removedFile = new File(sGREDir, removedFileName);
                if (removedFile.exists())
                    removedFile.delete();
            }
        } finally {
            reader.close();
        }
        
    }

    boolean haveKilledZombies = false;

    private boolean unpackFile(ZipFile zip, byte[] buf, ZipEntry fileEntry,
                            String name)
        throws IOException, FileNotFoundException
    {
        if (fileEntry == null)
            fileEntry = zip.getEntry(name);
        if (fileEntry == null)
            throw new FileNotFoundException("Can't find " + name + " in " +
                                            zip.getName());

        File outFile = new File(sGREDir, name);
        if (outFile.exists() &&
            outFile.lastModified() == fileEntry.getTime() &&
            outFile.length() == fileEntry.getSize())
            return false;

        if (!haveKilledZombies) {
            haveKilledZombies = true;
            GeckoAppShell.killAnyZombies();
        }

        File dir = outFile.getParentFile();
        if (!outFile.exists())
            dir.mkdirs();

        InputStream fileStream;
        fileStream = zip.getInputStream(fileEntry);

        OutputStream outStream = new FileOutputStream(outFile);

        while (fileStream.available() > 0) {
            int read = fileStream.read(buf, 0, buf.length);
            outStream.write(buf, 0, read);
        }

        fileStream.close();
        outStream.close();
        outFile.setLastModified(fileEntry.getTime());
        return true;
    }

    public void addEnvToIntent(Intent intent) {
        Map<String,String> envMap = System.getenv();
        Set<Map.Entry<String,String>> envSet = envMap.entrySet();
        Iterator<Map.Entry<String,String>> envIter = envSet.iterator();
        StringBuffer envstr = new StringBuffer();
        int c = 0;
        while (envIter.hasNext()) {
            Map.Entry<String,String> entry = envIter.next();
            intent.putExtra("env" + c, entry.getKey() + "="
                            + entry.getValue());
            c++;
        }
    }

    public void doRestart() {
        try {
            String action = "org.mozilla.gecko.restart";
            Intent intent = new Intent(action);
            intent.setClassName(getPackageName(),
                                getPackageName() + ".Restarter");
            addEnvToIntent(intent);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK |
                            Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
            Log.i("GeckoAppJava", intent.toString());
            GeckoAppShell.killAnyZombies();
            startActivity(intent);
        } catch (Exception e) {
            Log.i("GeckoAppJava", "error doing restart", e);
        }
        finish();
        
        GeckoAppShell.waitForAnotherGeckoProc();
    }

    public void handleNotification(String action, String alertName, String alertCookie) {
        GeckoAppShell.handleNotification(action, alertName, alertCookie);
    }

    private void checkAndLaunchUpdate() {
        Log.i("GeckoAppJava", "Checking for an update");

        int statusCode = 8; 
        File baseUpdateDir = null;
        if (Build.VERSION.SDK_INT >= 8)
            baseUpdateDir = getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
        else
            baseUpdateDir = new File(Environment.getExternalStorageDirectory().getPath(), "download");

        File updateDir = new File(new File(baseUpdateDir, "updates"),"0");

        File updateFile = new File(updateDir, "update.apk");
        File statusFile = new File(updateDir, "update.status");

        if (!statusFile.exists() || !readUpdateStatus(statusFile).equals("pending"))
            return;

        if (!updateFile.exists())
            return;

        Log.i("GeckoAppJava", "Update is available!");

        
        File updateFileToRun = new File(updateDir + getPackageName() + "-update.apk");
        try {
            if (updateFile.renameTo(updateFileToRun)) {
                String amCmd = "/system/bin/am start -a android.intent.action.VIEW " +
                               "-n com.android.packageinstaller/.PackageInstallerActivity -d file://" +
                               updateFileToRun.getPath();
                Log.i("GeckoAppJava", amCmd);
                Runtime.getRuntime().exec(amCmd);
                statusCode = 0; 
            } else {
                Log.i("GeckoAppJava", "Cannot rename the update file!");
                statusCode = 7; 
            }
        } catch (Exception e) {
            Log.i("GeckoAppJava", "error launching installer to update", e);
        }

        
        String status = statusCode == 0 ? "succeeded\n" : "failed: "+ statusCode + "\n";

        OutputStream outStream;
        try {
            byte[] buf = status.getBytes("UTF-8");
            outStream = new FileOutputStream(statusFile);
            outStream.write(buf, 0, buf.length);
            outStream.close();
        } catch (Exception e) {
            Log.i("GeckoAppJava", "error writing status file", e);
        }

        if (statusCode == 0)
            System.exit(0);
    }

    private String readUpdateStatus(File statusFile) {
        String status = "";
        try {
            BufferedReader reader = new BufferedReader(new FileReader(statusFile));
            status = reader.readLine();
            reader.close();
        } catch (Exception e) {
            Log.i("GeckoAppJava", "error reading update status", e);
        }
        return status;
    }

    static final int FILE_PICKER_REQUEST = 1;

    private SynchronousQueue<String> mFilePickerResult = new SynchronousQueue();
    public String showFilePicker(String aMimeType) {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType(aMimeType);
        GeckoApp.this.
            startActivityForResult(
                Intent.createChooser(intent,"choose a file"),
                FILE_PICKER_REQUEST);
        String filePickerResult = "";
        try {
            filePickerResult = mFilePickerResult.take();
        } catch (InterruptedException e) {
            Log.i("GeckoApp", "showing file picker ",  e);
        }

        return filePickerResult;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode,
                                    Intent data) {
        String filePickerResult = "";
        if (data != null && resultCode == RESULT_OK) {
            try {
                ContentResolver cr = getContentResolver();
                Uri uri = data.getData();
                String mimeType = cr.getType(uri);
                String fileExt = "." +
                    GeckoAppShell.getExtensionFromMimeType(mimeType);
                File file =
                    File.createTempFile("tmp_" +
                                        (int)Math.floor(1000 * Math.random()),
                                        fileExt, sGREDir);

                FileOutputStream fos = new FileOutputStream(file);
                InputStream is = cr.openInputStream(uri);
                byte[] buf = new byte[4096];
                int len = is.read(buf);
                while (len != -1) {
                    fos.write(buf, 0, len);
                    len = is.read(buf);
                }
                fos.close();
                filePickerResult =  file.getAbsolutePath();
            }catch (Exception e) {
                Log.e("GeckoApp", "showing file picker", e);
            }
        }
        try {
            mFilePickerResult.put(filePickerResult);
        } catch (InterruptedException e) {
            Log.i("GeckoApp", "error returning file picker result", e);
        }
    }
}
