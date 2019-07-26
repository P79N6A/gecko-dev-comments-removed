




package org.mozilla.gecko.util;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.os.Build;
import android.util.Log;
import android.view.ViewConfiguration;

public final class HardwareUtils {
    private static final String LOGTAG = "GeckoHardwareUtils";

    
    
    
    
    
    
    private static final int LOW_MEMORY_THRESHOLD_MB = 384;

    
    private static final int MEMINFO_BUFFER_SIZE_BYTES = 256;

    private static volatile int sTotalRAM = -1;

    private static volatile boolean sInited;

    
    private static volatile boolean sIsLargeTablet;
    private static volatile boolean sIsSmallTablet;
    private static volatile boolean sIsTelevision;
    private static volatile boolean sHasMenuButton;

    private HardwareUtils() {
    }

    public static void init(Context context) {
        if (sInited) {
            
            Log.d(LOGTAG, "HardwareUtils already inited.");
            return;
        }

        
        final int screenLayoutSize = context.getResources().getConfiguration().screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK;
        if (Build.VERSION.SDK_INT >= 11) {
            sHasMenuButton = false;
            if (screenLayoutSize == Configuration.SCREENLAYOUT_SIZE_XLARGE) {
                sIsLargeTablet = true;
            } else if (screenLayoutSize == Configuration.SCREENLAYOUT_SIZE_LARGE) {
                sIsSmallTablet = true;
            }
            if (Build.VERSION.SDK_INT >= 14) {
                sHasMenuButton = ViewConfiguration.get(context).hasPermanentMenuKey();

                if (Build.VERSION.SDK_INT >= 16) {
                    if (context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_TELEVISION)) {
                        sIsTelevision = true;
                    }
                }
            }
        } else {
            sHasMenuButton = true;
        }

        sInited = true;
    }

    public static boolean isTablet() {
        return sIsLargeTablet || sIsSmallTablet;
    }

    public static boolean isLargeTablet() {
        return sIsLargeTablet;
    }

    public static boolean isSmallTablet() {
        return sIsSmallTablet;
    }

    public static boolean isTelevision() {
        return sIsTelevision;
    }

    public static boolean hasMenuButton() {
        return sHasMenuButton;
    }

    





    private static boolean matchMemText(byte[] buffer, int index, int bufferLength, byte[] text) {
        final int N = text.length;
        if ((index + N) >= bufferLength) {
            return false;
        }
        for (int i = 0; i < N; i++) {
            if (buffer[index + i] != text[i]) {
                return false;
            }
        }
        return true;
    }

    









    private static int extractMemValue(byte[] buffer, int offset, int length) {
        if (offset >= length) {
            return 0;
        }

        while (offset < length && buffer[offset] != '\n') {
            if (buffer[offset] >= '0' && buffer[offset] <= '9') {
                int start = offset++;
                while (offset < length &&
                       buffer[offset] >= '0' &&
                       buffer[offset] <= '9') {
                    ++offset;
                }
                return Integer.parseInt(new String(buffer, start, offset - start), 10);
            }
            ++offset;
        }
        return 0;
    }

    







    public static int getMemSize() {
        if (sTotalRAM >= 0) {
            return sTotalRAM;
        }

        
        final byte[] MEMTOTAL = {'M', 'e', 'm', 'T', 'o', 't', 'a', 'l'};
        try {
            final byte[] buffer = new byte[MEMINFO_BUFFER_SIZE_BYTES];
            final FileInputStream is = new FileInputStream("/proc/meminfo");
            try {
                final int length = is.read(buffer);

                for (int i = 0; i < length; i++) {
                    if (matchMemText(buffer, i, length, MEMTOTAL)) {
                        i += 8;
                        sTotalRAM = extractMemValue(buffer, i, length) / 1024;
                        Log.d(LOGTAG, "System memory: " + sTotalRAM + "MB.");
                        return sTotalRAM;
                    }
                }
            } finally {
                is.close();
            }

            Log.w(LOGTAG, "Did not find MemTotal line in /proc/meminfo.");
            return sTotalRAM = 0;
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
