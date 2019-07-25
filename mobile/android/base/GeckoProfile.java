








package org.mozilla.gecko;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.util.HashMap;
import android.content.Context;
import android.os.Build;
import android.os.SystemClock;
import android.text.TextUtils;
import android.util.Log;

public final class GeckoProfile {
    private static final String LOGTAG = "GeckoProfile";

    private static HashMap<String, GeckoProfile> sProfileCache = new HashMap<String, GeckoProfile>();

    private final Context mContext;
    private final String mName;
    private File mMozDir;
    private File mDir;

    public static GeckoProfile get(Context context) {
        return get(context, null);
    }

    public static GeckoProfile get(Context context, String profileName) {
        if (context == null) {
            throw new IllegalArgumentException("context must be non-null");
        }
        if (TextUtils.isEmpty(profileName)) {
            
            profileName = "default";
        }

        synchronized (sProfileCache) {
            GeckoProfile profile = sProfileCache.get(profileName);
            if (profile == null) {
                profile = new GeckoProfile(context, profileName);
                sProfileCache.put(profileName, profile);
            }
            return profile;
        }
    }

    private GeckoProfile(Context context, String profileName) {
        mContext = context;
        mName = profileName;
    }

    public File getDir() {
        if (mDir != null) {
            return mDir;
        }

        try {
            File mozillaDir = ensureMozillaDirectory();
            mDir = findProfileDir(mozillaDir);
            if (mDir == null) {
                mDir = createProfileDir(mozillaDir);
            } else {
                Log.d(LOGTAG, "Found profile dir: " + mDir.getAbsolutePath());
            }
        } catch (IOException ioe) {
            Log.e(LOGTAG, "Error getting profile dir", ioe);
        }
        return mDir;
    }

    public boolean hasSession() {
        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - start check sessionstore.js exists");
        File dir = getDir();
        boolean hasSession = (dir != null && new File(dir, "sessionstore.js").exists());
        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - finish check sessionstore.js exists");
        return hasSession;
    }

    public String readSessionFile(boolean geckoReady) {
        File dir = getDir();
        if (dir == null) {
            return null;
        }

        File sessionFile = null;
        if (! geckoReady) {
            
            sessionFile = new File(dir, "sessionstore.js");
            if (! sessionFile.exists()) {
                sessionFile = null;
            }
        }
        if (sessionFile == null) {
            
            
            
            sessionFile = new File(dir, "sessionstore.bak");
            
            
        }

        try {
            return readFile(sessionFile);
        } catch (IOException ioe) {
            Log.i(LOGTAG, "Unable to read session file " + sessionFile.getAbsolutePath());
            return null;
        }
    }

    public String readFile(String filename) throws IOException {
        File dir = getDir();
        if (dir == null) {
            throw new IOException("No profile directory found");
        }
        File target = new File(dir, filename);
        return readFile(target);
    }

    private String readFile(File target) throws IOException {
        FileReader fr = new FileReader(target);
        try {
            StringBuffer sb = new StringBuffer();
            char[] buf = new char[8192];
            int read = fr.read(buf);
            while (read >= 0) {
                sb.append(buf, 0, read);
                read = fr.read(buf);
            }
            return sb.toString();
        } finally {
            fr.close();
        }
    }

    public File getFilesDir() {
        if (isOnInternalStorage()) {
            return mContext.getFilesDir();
        } else {
            return mContext.getExternalFilesDir(null);
        }
    }

    private boolean isOnInternalStorage() {
        
        if (Build.VERSION.SDK_INT < 8) {
            return true;
        }
        
        File externalDir = mContext.getExternalFilesDir(null);
        if (externalDir == null) {
            return true;
        }
        
        String resourcePath = mContext.getPackageResourcePath();
        if (resourcePath.startsWith("/data") || resourcePath.startsWith("/system")) {
            return true;
        }

        
        return false;
    }

    public void moveProfilesToAppInstallLocation() {
        
        moveProfilesFrom(new File("/data/data/" + mContext.getPackageName()));

        if (isOnInternalStorage()) {
            if (Build.VERSION.SDK_INT >= 8) {
                
                
                moveProfilesFrom(mContext.getExternalFilesDir(null));
            }
        } else {
            
            
            moveProfilesFrom(mContext.getFilesDir());
        }
    }

    private void moveProfilesFrom(File oldFilesDir) {
        if (oldFilesDir == null) {
            return;
        }
        File oldMozDir = new File(oldFilesDir, "mozilla");
        if (! (oldMozDir.exists() && oldMozDir.isDirectory())) {
            return;
        }

        

        File currentMozDir;
        try {
            currentMozDir = ensureMozillaDirectory();
            if (currentMozDir.equals(oldMozDir)) {
                return;
            }
        } catch (IOException ioe) {
            Log.e(LOGTAG, "Unable to create a profile directory!", ioe);
            return;
        }

        Log.d(LOGTAG, "Moving old profile directories from " + oldMozDir.getAbsolutePath());

        
        moveDirContents(oldMozDir, currentMozDir);
    }

    private void moveDirContents(File src, File dst) {
        File[] files = src.listFiles();
        if (files == null) {
            src.delete();
            return;
        }
        for (File f : files) {
            File target = new File(dst, f.getName());
            try {
                if (f.renameTo(target)) {
                    continue;
                }
            } catch (SecurityException se) {
                Log.e(LOGTAG, "Unable to rename file to " + target.getAbsolutePath() + " while moving profiles", se);
            }
            
            if (f.isDirectory()) {
                if (target.mkdirs()) {
                    moveDirContents(f, target);
                } else {
                    Log.e(LOGTAG, "Unable to create folder " + target.getAbsolutePath() + " while moving profiles");
                }
            } else {
                if (! moveFile(f, target)) {
                    Log.e(LOGTAG, "Unable to move file " + target.getAbsolutePath() + " while moving profiles");
                }
            }
        }
        src.delete();
    }

    private boolean moveFile(File src, File dst) {
        boolean success = false;
        long lastModified = src.lastModified();
        try {
            FileInputStream fis = new FileInputStream(src);
            try {
                FileOutputStream fos = new FileOutputStream(dst);
                try {
                    FileChannel inChannel = fis.getChannel();
                    long size = inChannel.size();
                    if (size == inChannel.transferTo(0, size, fos.getChannel())) {
                        success = true;
                    }
                } finally {
                    fos.close();
                }
            } finally {
                fis.close();
            }
        } catch (IOException ioe) {
            Log.e(LOGTAG, "Exception while attempting to move file to " + dst.getAbsolutePath(), ioe);
        }

        if (success) {
            dst.setLastModified(lastModified);
            src.delete();
        } else {
            dst.delete();
        }
        return success;
    }

    private File ensureMozillaDirectory() throws IOException {
        if (mMozDir != null) {
            return mMozDir;
        }

        File filesDir = getFilesDir();
        File mozDir = new File(filesDir, "mozilla");
        if (! mozDir.exists()) {
            if (! mozDir.mkdirs()) {
                throw new IOException("Unable to create mozilla directory at " + mozDir.getAbsolutePath());
            }
        }
        mMozDir = mozDir;
        return mMozDir;
    }

    private File findProfileDir(File mozillaDir) {
        String suffix = '.' + mName;
        File[] candidates = mozillaDir.listFiles();
        if (candidates == null) {
            return null;
        }
        for (File f : candidates) {
            if (f.isDirectory() && f.getName().endsWith(suffix)) {
                return f;
            }
        }
        return null;
    }

    private static String saltProfileName(String name) {
        String allowedChars = "abcdefghijklmnopqrstuvwxyz0123456789";
        StringBuffer salt = new StringBuffer(16);
        for (int i = 0; i < 8; i++) {
            salt.append(allowedChars.charAt((int)(Math.random() * allowedChars.length())));
        }
        salt.append('.');
        salt.append(name);
        return salt.toString();
    }

    private File createProfileDir(File mozillaDir) throws IOException {
        
        
        
        File profileIniFile = new File(mozillaDir, "profiles.ini");
        if (profileIniFile.exists()) {
            throw new IOException("Can't create new profiles");
        }

        String saltedName = saltProfileName(mName);
        File profileDir = new File(mozillaDir, saltedName);
        while (profileDir.exists()) {
            saltedName = saltProfileName(mName);
            profileDir = new File(mozillaDir, saltedName);
        }

        if (! profileDir.mkdirs()) {
            throw new IOException("Unable to create profile at " + profileDir.getAbsolutePath());
        }
        Log.d(LOGTAG, "Created new profile dir at " + profileDir.getAbsolutePath());

        FileWriter out = new FileWriter(profileIniFile, true);
        try {
            out.write("[General]\n" +
                      "StartWithLastProfile=1\n" +
                      "\n" +
                      "[Profile0]\n" +
                      "Name=" + mName + "\n" +
                      "IsRelative=1\n" +
                      "Path=" + saltedName + "\n" +
                      "Default=1\n");
        } finally {
            out.close();
        }

        return profileDir;
    }
}
