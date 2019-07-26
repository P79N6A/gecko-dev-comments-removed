




package org.mozilla.gecko.util;

import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.os.Build;
import android.util.Log;
import android.view.ViewConfiguration;

import java.io.FileNotFoundException;
import java.io.RandomAccessFile;
import java.io.IOException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public final class HardwareUtils {
    private static final String LOGTAG = "GeckoHardwareUtils";

    
    
    
    
    
    
    private static final int LOW_MEMORY_THRESHOLD_MB = 384;
    private static volatile int sTotalRAM = -1;

    private static Context sContext;

    private static Boolean sIsLargeTablet;
    private static Boolean sIsSmallTablet;
    private static Boolean sIsTelevision;
    private static Boolean sHasMenuButton;

    private HardwareUtils() {
    }

    public static void init(Context context) {
        if (sContext != null) {
            Log.w(LOGTAG, "HardwareUtils.init called twice!");
        }
        sContext = context;
    }

    public static boolean isTablet() {
        return isLargeTablet() || isSmallTablet();
    }

    public static boolean isLargeTablet() {
        if (sIsLargeTablet == null) {
            int screenLayout = sContext.getResources().getConfiguration().screenLayout;
            sIsLargeTablet = (Build.VERSION.SDK_INT >= 11 &&
                              ((screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK) == Configuration.SCREENLAYOUT_SIZE_XLARGE));
        }
        return sIsLargeTablet;
    }

    public static boolean isSmallTablet() {
        if (sIsSmallTablet == null) {
            int screenLayout = sContext.getResources().getConfiguration().screenLayout;
            sIsSmallTablet = (Build.VERSION.SDK_INT >= 11 &&
                              ((screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK) == Configuration.SCREENLAYOUT_SIZE_LARGE));
        }
        return sIsSmallTablet;
    }

    public static boolean isTelevision() {
        if (sIsTelevision == null) {
            sIsTelevision = sContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_TELEVISION);
        }
        return sIsTelevision;
    }

    public static boolean hasMenuButton() {
        if (sHasMenuButton == null) {
            sHasMenuButton = Boolean.TRUE;
            if (Build.VERSION.SDK_INT >= 11) {
                sHasMenuButton = Boolean.FALSE;
            }
            if (Build.VERSION.SDK_INT >= 14) {
                sHasMenuButton = ViewConfiguration.get(sContext).hasPermanentMenuKey();
            }
        }
        return sHasMenuButton;
    }

    







    public static int getMemSize() {
        if (sTotalRAM >= 0) {
            return sTotalRAM;
        }

        try {
            RandomAccessFile reader = new RandomAccessFile("/proc/meminfo", "r");
            try {
                
                int i = 0;
                String memTotal = null;
                while (i++ < 3) {
                    memTotal = reader.readLine();
                    if (memTotal == null ||
                        memTotal.startsWith("MemTotal: ")) {
                        break;
                    }
                    memTotal = null;
                }

                if (memTotal == null) {
                    return sTotalRAM = 0;
                }

                
                
                Matcher m = Pattern.compile("^MemTotal:\\s+([0-9]+) kB\\s*$")
                                   .matcher(memTotal);
                if (m.matches()) {
                    String kb = m.group(1);
                    if (kb != null) {
                        sTotalRAM = (Integer.parseInt(kb) / 1024);
                        Log.d(LOGTAG, "System memory: " + sTotalRAM + "MB.");
                        return sTotalRAM;
                    }
                }

                Log.w(LOGTAG, "Got unexpected MemTotal line: " + memTotal);
                return sTotalRAM = 0;
              } finally {
                  reader.close();
              }
          } catch (FileNotFoundException f) {
              return sTotalRAM = 0;
          } catch (IOException e) {
              return sTotalRAM = 0;
          }
    }

    public static boolean isLowMemoryPlatform() {
        final int memSize = getMemSize();

        
        
        if (memSize == 0) {
            Log.w(LOGTAG, "Could not compute system memory. Falling back to isLowMemoryPlatform = false.");
            return false;
        }

        return memSize < LOW_MEMORY_THRESHOLD_MB;
    }
}
