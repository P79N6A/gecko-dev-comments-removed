




































package org.mozilla.gecko;

import android.content.Context;
import android.os.AsyncTask;
import android.os.Build;
import android.util.Log;

import java.io.File;
import java.io.FileFilter;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.Void;
import java.util.Date;
import java.util.Random;
import java.util.Map;
import java.util.HashMap;

abstract public class GeckoDirProvider
{
    private static final String LOGTAG = "GeckoDirProvider";
    private static HashMap<String, File> mProfileDirs = new HashMap<String, File>();

    







    static public File getProfileDir(final Context aContext)
            throws IllegalArgumentException, IOException {
        
        return getProfileDir(aContext, "default");
    }

    











    static public File getProfileDir(final Context aContext, final String aProfileName)
            throws IllegalArgumentException, IOException {

        if (aContext == null)
            throw new IllegalArgumentException("Must provide a valid context");

        if (aProfileName == null || aProfileName.trim().equals(""))
            throw new IllegalArgumentException("Profile name: '" + aProfileName + "' is not valid");

        Log.i(LOGTAG, "Get profile dir for " + aProfileName);
        synchronized (mProfileDirs) {
            File profileDir = mProfileDirs.get(aProfileName);
            if (profileDir != null)
                return profileDir;

            
            
            File mozDir = GeckoDirProvider.ensureMozillaDirectory(aContext);
            profileDir = GeckoDirProvider.getProfileDir(mozDir, aProfileName);

            if (profileDir == null) {
                
                profileDir = GeckoDirProvider.createProfileDir(mozDir, aProfileName);
            }
            mProfileDirs.put(aProfileName, profileDir);
            return profileDir;
        }
    }

    private static File getProfileDir(final File aRoot, final String aProfileName)
            throws IllegalArgumentException {
        if (aRoot == null)
            throw new IllegalArgumentException("Invalid root directory");

        File[] profiles = aRoot.listFiles(new FileFilter() {
            public boolean accept(File pathname) {
                return pathname.getName().endsWith("." + aProfileName);
            }
        });

        if (profiles != null && profiles.length > 0)
            return profiles[0];
        return null;
    }

    private static File ensureMozillaDirectory(final Context aContext)
            throws IOException, IllegalArgumentException {
        if (aContext == null)
            throw new IllegalArgumentException("Must provide a valid context");
        File filesDir = GeckoDirProvider.getFilesDir(aContext);

        File mozDir = new File(filesDir, "mozilla");
        if (!mozDir.exists()) {
            if (!mozDir.mkdir())
                throw new IOException("Unable to create mozilla directory at " + mozDir.getPath());
        }
        return mozDir;
    }

    static final char kTable[] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                   'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
                                   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' };

    private static File createProfileDir(final File aRootDir, final String aProfileName)
            throws IOException, IllegalArgumentException {

        if (aRootDir == null)
            throw new IllegalArgumentException("Must provide a valid root directory");

        if (aProfileName == null || aProfileName.trim().equals(""))
            throw new IllegalArgumentException("Profile name: '" + aProfileName + "' is not valid");

        
        
        
        final File profileIni = new File(aRootDir, "profiles.ini");
        if (profileIni.exists())
            throw new IOException("Can't create new profiles");

        String saltedName = saltProfileName(aProfileName);
        File profile = new File(aRootDir, saltedName);
        while (profile.exists()) {
            saltedName = saltProfileName(aProfileName);
            profile = new File(aRootDir, saltedName);
        }

        if (!profile.mkdir()) 
            throw new IOException("Unable to create profile at " + profile.getPath());

        Log.i(LOGTAG, "Creating new profile at " + profile.getPath());
        final String fSaltedName = saltedName;

        FileWriter outputStream = new FileWriter(profileIni, true);
        outputStream.write("[General]\n" +
                           "StartWithLastProfile=1\n" +
                           "\n" +
                           "[Profile0]\n" +
                           "Name=" + aProfileName + "\n" +
                           "IsRelative=1\n" +
                           "Path=" + fSaltedName + "\n" +
                           "Default=1\n");
        outputStream.close();

        return profile;
    }

    private static String saltProfileName(final String aName) {
        Random randomGenerator = new Random(System.nanoTime());

        StringBuilder salt = new StringBuilder();
        int i;
        for (i = 0; i < 8; ++i)
            salt.append(kTable[randomGenerator.nextInt(kTable.length)]);

        salt.append(".");
        return salt.append(aName).toString();
    }

    public static File getFilesDir(final Context aContext) {
        if (aContext == null)
            throw new IllegalArgumentException("Must provide a valid context");

        if (Build.VERSION.SDK_INT < 8 ||
            aContext.getPackageResourcePath().startsWith("/data") ||
            aContext.getPackageResourcePath().startsWith("/system")) {
            return aContext.getFilesDir();
        }
        return aContext.getExternalFilesDir(null);
    }
}
