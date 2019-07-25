



































package org.mozilla.gecko;

import java.io.*;
import java.util.*;
import java.util.zip.*;
import java.nio.*;

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
    public static FrameLayout mainLayout;
    public static GeckoSurfaceView surfaceView;
    public static GeckoApp mAppContext;

    void launch()
    {
        
        unpackComponents();
        
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

    protected void unpackComponents()
    {
        ZipFile zip;
        InputStream listStream;

        try {
            File componentsDir = new File("/data/data/org.mozilla." + getAppName() +"/components");
            componentsDir.mkdir();
            zip = new ZipFile(getApplication().getPackageResourcePath());

            ZipEntry componentsList = zip.getEntry("components/components.manifest");
            if (componentsList == null) {
                Log.i("GeckoAppJava", "Can't find components.list !");
                return;
            }

            listStream = new BufferedInputStream(zip.getInputStream(componentsList));
        } catch (Exception e) {
            Log.i("GeckoAppJava", e.toString());
            return;
        }

        byte[] buf = new byte[8192];

        StreamTokenizer tkn = new StreamTokenizer(new InputStreamReader(listStream));
        String line = "components/";
        int status;
        boolean addnext = false;
        tkn.eolIsSignificant(true);
        do {
            try {
                status = tkn.nextToken();
            } catch (IOException e) {
                Log.i("GeckoAppJava", e.toString());
                return;
            }
            switch (status) {
            case StreamTokenizer.TT_WORD:
                if (tkn.sval.equals("binary-component"))
                    addnext = true;
                else if (addnext) {
                    line += tkn.sval;
                    addnext = false;
                }
                break;
            case StreamTokenizer.TT_NUMBER:
                break;
            case StreamTokenizer.TT_EOF:
            case StreamTokenizer.TT_EOL:
                unpackFile(zip, buf, null, line);
                line = "components/";
                break;
            }
        } while (status != StreamTokenizer.TT_EOF);

        unpackFile(zip, buf, null, "application.ini");
        unpackFile(zip, buf, null, getContentProcessName());
    }

    private void unpackFile(ZipFile zip, byte[] buf, ZipEntry fileEntry, String name)
    {
        if (fileEntry == null)
            fileEntry = zip.getEntry(name);
        if (fileEntry == null) {
            Log.i("GeckoAppJava", "Can't find " + name + " in " + zip.getName());
            return;
        }

        File outFile = new File("/data/data/org.mozilla." + getAppName() + "/" + name);
        if (outFile.exists() &&
            outFile.lastModified() >= fileEntry.getTime() &&
            outFile.length() == fileEntry.getSize())
            return;

        try {
            File dir = outFile.getParentFile();
            if (!outFile.exists())
                dir.mkdirs();
        } catch (Exception e) {
            Log.i("GeckoAppJava", e.toString());
            return;
        }

        InputStream fileStream;
        try {
            fileStream = zip.getInputStream(fileEntry);
        } catch (Exception e) {
            Log.i("GeckoAppJava", e.toString());
            return;
        }

        OutputStream outStream;
        try {
            outStream = new FileOutputStream(outFile);

            while (fileStream.available() > 0) {
                int read = fileStream.read(buf, 0, buf.length);
                outStream.write(buf, 0, read);
            }

            fileStream.close();
            outStream.close();
        } catch (Exception e) {
            Log.i("GeckoAppJava", e.toString());
            return;
        }
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
}
