




package org.mozilla.gecko;

import android.os.StrictMode;
import android.util.Log;

import java.io.File;
import java.io.FileFilter;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

import java.util.regex.Pattern;









public final class SysInfo {
    private static final String LOG_TAG = "GeckoSysInfo";

    
    private static final int MEMINFO_BUFFER_SIZE_BYTES = 256;

    
    
    
    private static volatile int cpuCount = -1;

    private static volatile int totalRAM = -1;

    












    public static int getCPUCount() {
        if (cpuCount > 0) {
            return cpuCount;
        }

        
        StrictMode.ThreadPolicy savedPolicy = StrictMode.allowThreadDiskReads();
        try {
            return readCPUCount();
        } finally {
            StrictMode.setThreadPolicy(savedPolicy);
        }
    }

    private static int readCPUCount() {
        class CpuFilter implements FileFilter {
            @Override
            public boolean accept(File pathname) {
                return Pattern.matches("cpu[0-9]+", pathname.getName());
            }
        }
        try {
            final File dir = new File("/sys/devices/system/cpu/");
            return cpuCount = dir.listFiles(new CpuFilter()).length;
        } catch (Exception e) {
            Log.w(LOG_TAG, "Assuming 1 CPU; got exception.", e);
            return cpuCount = 1;
        }
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
        if (totalRAM >= 0) {
            return totalRAM;
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
                        totalRAM = extractMemValue(buffer, i, length) / 1024;
                        Log.d(LOG_TAG, "System memory: " + totalRAM + "MB.");
                        return totalRAM;
                    }
                }
            } finally {
                is.close();
            }

            Log.w(LOG_TAG, "Did not find MemTotal line in /proc/meminfo.");
            return totalRAM = 0;
        } catch (FileNotFoundException f) {
            return totalRAM = 0;
        } catch (IOException e) {
            return totalRAM = 0;
        }
    }

    


    public static int getVersion() {
        return android.os.Build.VERSION.SDK_INT;
    }

    


    public static String getReleaseVersion() {
        return android.os.Build.VERSION.RELEASE;
    }

    


    public static String getKernelVersion() {
        return System.getProperty("os.version", "");
    }

    


    public static String getManufacturer() {
        return android.os.Build.MANUFACTURER;
    }

    


    public static String getDevice() {
        
        return android.os.Build.MODEL;
    }

    


    public static String getHardware() {
        return android.os.Build.HARDWARE;
    }

    


    public static String getName() {
        
        return "Android";
    }

    


    public static String getArchABI() {
        
        
        return android.os.Build.CPU_ABI;
    }
}
