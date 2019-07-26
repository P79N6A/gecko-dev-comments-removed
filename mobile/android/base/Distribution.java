








package org.mozilla.gecko;

import org.mozilla.gecko.util.GeckoBackgroundThread;

import android.app.Activity;
import android.content.SharedPreferences;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import java.lang.Exception;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public final class Distribution {
    private static final String LOGTAG = "Distribution";

    


    public static void init(final Activity activity) {
        
        GeckoBackgroundThread.getHandler().post(new Runnable() {
            public void run() {
                
                SharedPreferences settings = activity.getPreferences(Activity.MODE_PRIVATE);
                String keyName = activity.getPackageName() + ".distribution_initialized";
                if (settings.getBoolean(keyName, false))
                    return;

                settings.edit().putBoolean(keyName, true).commit();

                try {
                    copyFiles(activity);
                } catch (IOException e) {
                    Log.e(LOGTAG, "Error copying distribution files", e);
                }
            }
        });
    }

    


    private static void copyFiles(Activity activity) throws IOException {
        File applicationPackage = new File(activity.getPackageResourcePath());
        ZipFile zip = new ZipFile(applicationPackage);

        Enumeration<? extends ZipEntry> zipEntries = zip.entries();
        while (zipEntries.hasMoreElements()) {
            ZipEntry fileEntry = zipEntries.nextElement();
            String name = fileEntry.getName();

            if (!name.startsWith("distribution/"))
                continue;

            File dataDir = new File(activity.getApplicationInfo().dataDir);
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
    }
}
