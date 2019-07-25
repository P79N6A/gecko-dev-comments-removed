








package org.mozilla.gecko;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.channels.FileChannel;
import java.util.HashMap;
import java.util.Enumeration;
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

    
    private static final long SESSION_TIMEOUT = 30 * 1000; 

    static private INIParser getProfilesINI(Context context) {
      File filesDir = context.getFilesDir();
      File mozillaDir = new File(filesDir, "mozilla");
      File profilesIni = new File(mozillaDir, "profiles.ini");
      return new INIParser(profilesIni);
    }

    public static GeckoProfile get(Context context) {
        return get(context, "");
    }

    public static GeckoProfile get(Context context, String profileName) {
        if (context == null) {
            throw new IllegalArgumentException("context must be non-null");
        }

        
        
        if (TextUtils.isEmpty(profileName)) {
            profileName = "default";

            INIParser parser = getProfilesINI(context);

            String profile = "";
            boolean foundDefault = false;
            for (Enumeration<INISection> e = parser.getSections().elements(); e.hasMoreElements();) {
                INISection section = e.nextElement();
                if (section.getIntProperty("Default") == 1) {
                    profile = section.getStringProperty("Name");
                    foundDefault = true;
                }
            }

            if (foundDefault)
                profileName = profile;
        }

        
        synchronized (sProfileCache) {
            Log.i(LOGTAG, "Get profile " + profileName);
            GeckoProfile profile = sProfileCache.get(profileName);
            if (profile == null) {
                profile = new GeckoProfile(context, profileName);
                sProfileCache.put(profileName, profile);
            }
            return profile;
        }
    }

    public static File ensureMozillaDirectory(Context context) throws IOException {
        synchronized (context) {
            File filesDir = context.getFilesDir();
            File mozDir = new File(filesDir, "mozilla");
            if (! mozDir.exists()) {
                if (! mozDir.mkdirs()) {
                    throw new IOException("Unable to create mozilla directory at " + mozDir.getAbsolutePath());
                }
            }
            return mozDir;
        }
    }

    private GeckoProfile(Context context, String profileName) {
        mContext = context;
        mName = profileName;
    }

    public String getName() {
        return mName;
    }

    public synchronized File getDir() {
        if (mDir != null) {
            return mDir;
        }

        try {
            
            ProfileMigrator profileMigrator = new ProfileMigrator(mContext);
            if (!profileMigrator.isProfileMoved()) {
                Log.i(LOGTAG, "New installation or update, checking for old profiles.");
                profileMigrator.launchMoveProfile();
            }

            
            File mozillaDir = ensureMozillaDirectory(mContext);
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

    public File getFilesDir() {
        return mContext.getFilesDir();
    }

    public boolean shouldRestoreSession() {
        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - start check sessionstore.js exists");
        File dir = getDir();
        if (dir == null)
            return false;

        File sessionFile = new File(dir, "sessionstore.js");
        if (!sessionFile.exists())
            return false;

        boolean shouldRestore = (System.currentTimeMillis() - sessionFile.lastModified() < SESSION_TIMEOUT);
        Log.w(LOGTAG, "zerdatime " + SystemClock.uptimeMillis() + " - finish check sessionstore.js exists");
        return shouldRestore;
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

    private File findProfileDir(File mozillaDir) {
        
        INIParser parser = getProfilesINI(mContext);

        for (Enumeration<INISection> e = parser.getSections().elements(); e.hasMoreElements();) {
            INISection section = e.nextElement();
            String name = section.getStringProperty("Name");
            if (name != null && name.equals(mName)) {
                if (section.getIntProperty("IsRelative") == 1) {
                    return new File(mozillaDir, section.getStringProperty("Path"));
                }
                return new File(section.getStringProperty("Path"));
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
        INIParser parser = getProfilesINI(mContext);

        
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

        
        
        
        int profileNum = 0;
        while (parser.getSection("Profile" + profileNum) != null) {
            profileNum++;
        }

        INISection profileSection = new INISection("Profile" + profileNum);
        profileSection.setProperty("Name", mName);
        profileSection.setProperty("IsRelative", 1);
        profileSection.setProperty("Path", saltedName);

        if (parser.getSection("General") == null) {
            INISection generalSection = new INISection("General");
            generalSection.setProperty("StartWithLastProfile", 1);
            parser.addSection(generalSection);

            
            Log.i(LOGTAG, "WESJ - SET DEFAULT");
            profileSection.setProperty("Default", 1);
        }

        parser.addSection(profileSection);
        parser.write();

        return profileDir;
    }
}
