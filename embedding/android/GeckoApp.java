




































package org.mozilla.gecko;

import java.io.*;
import java.util.*;
import java.util.zip.*;
import java.nio.*;
import java.nio.channels.FileChannel;

import android.os.*;
import android.app.*;
import android.text.*;
import android.view.*;
import android.view.inputmethod.*;
import android.content.*;
import android.graphics.*;
import android.widget.*;
import android.hardware.*;

import android.util.*;

abstract public class GeckoApp
    extends Activity
{
    public static final String ACTION_ALERT_CLICK = "org.mozilla.gecko.ACTION_ALERT_CLICK";
    public static final String ACTION_ALERT_CLEAR = "org.mozilla.gecko.ACTION_ALERT_CLEAR";

    public static FrameLayout mainLayout;
    public static GeckoSurfaceView surfaceView;
    public static GeckoApp mAppContext;
    ProgressDialog mProgressDialog;

    void showErrorDialog(String message)
    {
        new AlertDialog.Builder(this)
            .setMessage(message)
            .setCancelable(false)
            .setPositiveButton("Exit",
                               new DialogInterface.OnClickListener() {
                                   public void onClick(DialogInterface dialog,
                                                       int id)
                                   {
                                       GeckoApp.this.finish();
                                   }
                               }).show();
    }

    void launch()
    {
        
        try {
            unpackComponents();
        } catch (FileNotFoundException fnfe) {
            showErrorDialog(getString(R.string.error_loading_file));
            return;
        } catch (IOException ie) {
            String msg = ie.getMessage();
            if (msg.equalsIgnoreCase("No space left on device"))
                showErrorDialog(getString(R.string.no_space_to_start_error));
            else
                showErrorDialog(getString(R.string.error_loading_file));
            return;
        }
        
        Intent i = getIntent();
        String env = i.getStringExtra("env0");
        GeckoAppShell.runGecko(getApplication().getPackageResourcePath(),
                               i.getStringExtra("args"),
                               i.getDataString());
    }

    
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        Log.i("GeckoApp", "create");
        super.onCreate(savedInstanceState);

        mAppContext = this;

        
        requestWindowFeature(Window.FEATURE_NO_TITLE);

        checkAndLaunchUpdate();

        surfaceView = new GeckoSurfaceView(this);
        
        mainLayout = new FrameLayout(this);
        mainLayout.addView(surfaceView,
                           new FrameLayout.LayoutParams(FrameLayout.LayoutParams.FILL_PARENT,
                                                        FrameLayout.LayoutParams.FILL_PARENT));

        boolean useLaunchButton = false;

        String intentAction = getIntent().getAction();
        if (intentAction != null && intentAction.equals("org.mozilla.gecko.DEBUG"))
            useLaunchButton = true;

        setContentView(mainLayout,
                       new ViewGroup.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT,
                                                  ViewGroup.LayoutParams.FILL_PARENT));

        if (!GeckoAppShell.sGeckoRunning) {
            try {
                BufferedReader reader =
                    new BufferedReader(new FileReader("/proc/cpuinfo"));
                String line;
                while ((line = reader.readLine()) != null) {
                    int index = line.indexOf("CPU architecture:");
                    if (index == -1)
                        continue;
                    String versionStr = line.substring(18);
                    Log.i("GeckoApp", "cpu version: " + versionStr);
                    int version = Integer.parseInt(versionStr);

                    if (version < getMinCPUVersion()) {
                        showErrorDialog(
                            getString(R.string.incompatable_cpu_error));
                        return;
                    }
                    else {
                        break;
                    }
                }
                
            } catch (Exception ex) {
                
                Log.i("GeckoApp", "exception: " + ex);
            }

            if (!useLaunchButton)
                mProgressDialog = 
                    ProgressDialog.show(GeckoApp.this, "",
                                        getString(R.string.splash_screen_label),
                                        true);
            
            
            
            GeckoAppShell.loadGeckoLibs();

            if (useLaunchButton) {
                final Button b = new Button(this);
                b.setText("Launch"); 
                b.setOnClickListener(new Button.OnClickListener() {
                        public void onClick (View v) {
                            
                            mainLayout.removeView(b);
                            launch();
                        }
                    });
                mainLayout.addView(b, 300, 200);
            } else {
                launch();
            }
        }

        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        final String action = intent.getAction();
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
        if (GeckoAppShell.sGeckoRunning)
            GeckoAppShell.onResume();
        if (surfaceView != null)
            surfaceView.mSurfaceNeedsRedraw = true;
        
        
        super.onResume();
    }

    @Override
    public void onStop()
    {
        Log.i("GeckoApp", "stop");
        
        
        
        
        
        
        
        
        
        

        

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
        
        
        GeckoAppShell.sendEventToGecko(new GeckoEvent(GeckoEvent.ACTIVITY_STOPPING));

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
        Log.i("GeckoApp", "low memory");
        if (GeckoAppShell.sGeckoRunning)
            GeckoAppShell.onLowMemory();
        super.onLowMemory();
    }

    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BACK:
                if (event.getRepeatCount() == 0) {
                    event.startTracking();
                    return true;
                } else {
                    return false;
                }
            case KeyEvent.KEYCODE_VOLUME_UP:
            case KeyEvent.KEYCODE_VOLUME_DOWN:
            case KeyEvent.KEYCODE_SEARCH:
                return false;
            case KeyEvent.KEYCODE_DEL:
                
                if (surfaceView != null &&
                    surfaceView.inputConnection != null &&
                    surfaceView.inputConnection.onKeyDel()) {
                    return true;
                }
                break;
            default:
                break;
        }
        GeckoAppShell.sendEventToGecko(new GeckoEvent(event));
        return true;
    }

    public boolean onKeyUp(int keyCode, KeyEvent event) {
        switch(keyCode) {
            case KeyEvent.KEYCODE_BACK:
                if (!event.isTracking() || event.isCanceled())
                    return false;
                break;
            default:
                break;
        }
        GeckoAppShell.sendEventToGecko(new GeckoEvent(event));
        return true;
    }

    public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event) {
        GeckoAppShell.sendEventToGecko(new GeckoEvent(event));
        return true;
    }

    abstract public String getAppName();
    abstract public String getContentProcessName();
    abstract public int getMinCPUVersion();

    protected void unpackComponents()
        throws IOException, FileNotFoundException
    {
        ZipFile zip;
        InputStream listStream;

        File componentsDir = new File("/data/data/org.mozilla." + getAppName() +
                                      "/components");
        componentsDir.mkdir();
        zip = new ZipFile(getApplication().getPackageResourcePath());

        byte[] buf = new byte[8192];
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

    private void unpackFile(ZipFile zip, byte[] buf, ZipEntry fileEntry,
                            String name)
        throws IOException, FileNotFoundException
    {
        if (fileEntry == null)
            fileEntry = zip.getEntry(name);
        if (fileEntry == null)
            throw new FileNotFoundException("Can't find " + name + " in " +
                                            zip.getName());

        File outFile = new File("/data/data/org.mozilla." + getAppName() +
                                "/" + name);
        if (outFile.exists() &&
            outFile.lastModified() >= fileEntry.getTime() &&
            outFile.length() == fileEntry.getSize())
            return;

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
    }

    public String getEnvString() {
        Map<String,String> envMap = System.getenv();
        Set<Map.Entry<String,String>> envSet = envMap.entrySet();
        Iterator<Map.Entry<String,String>> envIter = envSet.iterator();
        StringBuffer envstr = new StringBuffer();
        int c = 0;
        while (envIter.hasNext()) {
            Map.Entry<String,String> entry = envIter.next();
            
            
            
            if (!entry.getKey().equals("BOOTCLASSPATH") &&
                !entry.getKey().equals("ANDROID_SOCKET_zygote") && 
                !entry.getKey().equals("TMPDIR") &&
                !entry.getKey().equals("ANDROID_BOOTLOGO") &&
                !entry.getKey().equals("EXTERNAL_STORAGE") &&
                !entry.getKey().equals("ANDROID_ASSETS") &&
                !entry.getKey().equals("PATH") &&
                !entry.getKey().equals("TERMINFO") &&
                !entry.getKey().equals("LD_LIBRARY_PATH") &&
                !entry.getKey().equals("ANDROID_DATA") &&
                !entry.getKey().equals("ANDROID_PROPERTY_WORKSPACE") &&
                !entry.getKey().equals("ANDROID_ROOT")) {
                envstr.append(" --es env" + c + " " + entry.getKey() + "=" 
                              + entry.getValue());
                c++;
            }
        }
        return envstr.toString();        
    }

    public void doRestart() {
        try {
            String action = "org.mozilla.gecko.restart" + getAppName();
            String amCmd = "/system/bin/am broadcast -a " + action + getEnvString() + " -n org.mozilla." + getAppName() + "/org.mozilla." + getAppName() + ".Restarter";
            Log.i("GeckoAppJava", amCmd);
            Runtime.getRuntime().exec(amCmd);
        } catch (Exception e) {
            Log.i("GeckoAppJava", e.toString());
        }
        System.exit(0);
    }

    public void handleNotification(String action, String alertName, String alertCookie) {
        GeckoAppShell.handleNotification(action, alertName, alertCookie);
    }

    private void checkAndLaunchUpdate() {
        Log.i("GeckoAppJava", "Checking for an update");

        int statusCode = 8; 

        String updateDir = "/data/data/org.mozilla." + getAppName() + "/updates/0/";
        File updateFile = new File(updateDir + "update.apk");

        if (!updateFile.exists())
            return;

        Log.i("GeckoAppJava", "Update is available!");

        
        File updateFileToRun = new File(updateDir + getAppName() + "-update.apk");
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
            Log.i("GeckoAppJava", e.toString());
        }

        
        String status = statusCode == 0 ? "succeeded\n" : "failed: "+ statusCode + "\n";

        File statusFile = new File(updateDir + "update.status");
        OutputStream outStream;
        try {
            byte[] buf = status.getBytes("UTF-8");
            outStream = new FileOutputStream(statusFile);
            outStream.write(buf, 0, buf.length);
            outStream.close();
        } catch (Exception e) {
            Log.i("GeckoAppJava", e.toString());
        }

        if (statusCode == 0)
            System.exit(0);
    }
}
