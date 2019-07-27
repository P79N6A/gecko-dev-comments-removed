




package org.mozilla.gecko;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.nio.charset.Charset;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Hashtable;

import org.mozilla.gecko.GeckoProfileDirectories.NoMozillaDirectoryException;
import org.mozilla.gecko.GeckoProfileDirectories.NoSuchProfileException;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.distribution.Distribution;
import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.util.INIParser;
import org.mozilla.gecko.util.INISection;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.support.v4.content.LocalBroadcastManager;
import android.text.TextUtils;
import android.util.Log;

public final class GeckoProfile {
    private static final String LOGTAG = "GeckoProfile";

    
    private static final String LOCK_FILE_NAME = ".active_lock";
    public static final String DEFAULT_PROFILE = "default";
    public static final String GUEST_PROFILE = "guest";

    private static HashMap<String, GeckoProfile> sProfileCache = new HashMap<String, GeckoProfile>();
    private static String sDefaultProfileName;

    
    private static File sGuestDir;
    private static GeckoProfile sGuestProfile;

    public static boolean sIsUsingCustomProfile;

    private final String mName;
    private final File mMozillaDir;
    private final boolean mIsWebAppProfile;
    private final Context mApplicationContext;

    





    private File mProfileDir;

    
    
    
    
    
    private volatile LockState mLocked = LockState.UNDEFINED;
    private volatile boolean mInGuestMode;

    
    private enum LockState {
        LOCKED,
        UNLOCKED,
        UNDEFINED
    };

    public static GeckoProfile get(Context context) {
        boolean isGeckoApp = false;
        try {
            isGeckoApp = context instanceof GeckoApp;
        } catch (NoClassDefFoundError ex) {}

        if (isGeckoApp) {
            
            
            final GeckoApp geckoApp = (GeckoApp) context;
            if (geckoApp.mProfile != null) {
                return geckoApp.mProfile;
            }
        }

        
        if (GuestSession.shouldUse(context, "")) {
            return GeckoProfile.getGuestProfile(context);
        }

        if (isGeckoApp) {
            final GeckoApp geckoApp = (GeckoApp) context;
            String defaultProfileName;
            try {
                defaultProfileName = geckoApp.getDefaultProfileName();
            } catch (NoMozillaDirectoryException e) {
                
                
                Log.wtf(LOGTAG, "Unable to get default profile name.", e);
                throw new RuntimeException(e);
            }
            
            return get(context, defaultProfileName);
        }

        return get(context, "");
    }

    public static GeckoProfile get(Context context, String profileName) {
        synchronized (sProfileCache) {
            GeckoProfile profile = sProfileCache.get(profileName);
            if (profile != null)
                return profile;
        }
        return get(context, profileName, (File)null);
    }

    @RobocopTarget
    public static GeckoProfile get(Context context, String profileName, String profilePath) {
        File dir = null;
        if (!TextUtils.isEmpty(profilePath)) {
            dir = new File(profilePath);
            if (!dir.exists() || !dir.isDirectory()) {
                Log.w(LOGTAG, "requested profile directory missing: " + profilePath);
            }
        }
        return get(context, profileName, dir);
    }

    @RobocopTarget
    public static GeckoProfile get(Context context, String profileName, File profileDir) {
        if (context == null) {
            throw new IllegalArgumentException("context must be non-null");
        }

        
        
        if (TextUtils.isEmpty(profileName) && profileDir == null) {
            try {
                profileName = GeckoProfile.getDefaultProfileName(context);
            } catch (NoMozillaDirectoryException e) {
                
                throw new RuntimeException(e);
            }
        }

        
        synchronized (sProfileCache) {
            GeckoProfile profile = sProfileCache.get(profileName);
            if (profile == null) {
                try {
                    profile = new GeckoProfile(context, profileName);
                } catch (NoMozillaDirectoryException e) {
                    
                    throw new RuntimeException(e);
                }
                profile.setDir(profileDir);
                sProfileCache.put(profileName, profile);
            } else {
                profile.setDir(profileDir);
            }
            return profile;
        }
    }

    public static boolean removeProfile(Context context, String profileName) {
        if (profileName == null) {
            Log.w(LOGTAG, "Unable to remove profile: null profile name.");
            return false;
        }

        final GeckoProfile profile = get(context, profileName);
        if (profile == null) {
            return false;
        }
        final boolean success = profile.remove();

        if (success) {
            
            GeckoSharedPrefs.forProfileName(context, profileName)
                            .edit().clear().apply();
        }

        return success;
    }

    public static GeckoProfile createGuestProfile(Context context) {
        try {
            
            
            getGuestDir(context).mkdir();
            GeckoProfile profile = getGuestProfile(context);

            
            
            
            if (!GuestSession.isSecureKeyguardLocked(context)) {
                profile.lock();
            }

            



            profile.enqueueInitialization();

            return profile;
        } catch (Exception ex) {
            Log.e(LOGTAG, "Error creating guest profile", ex);
        }
        return null;
    }

    public static void leaveGuestSession(Context context) {
        GeckoProfile profile = getGuestProfile(context);
        if (profile != null) {
            profile.unlock();
        }
    }

    private static File getGuestDir(Context context) {
        if (sGuestDir == null) {
            sGuestDir = context.getFileStreamPath("guest");
        }
        return sGuestDir;
    }

    public static GeckoProfile getGuestProfile(Context context) {
        if (sGuestProfile == null) {
            File guestDir = getGuestDir(context);
            if (guestDir.exists()) {
                sGuestProfile = get(context, GUEST_PROFILE, guestDir);
                sGuestProfile.mInGuestMode = true;
            }
        }

        return sGuestProfile;
    }

    public static boolean maybeCleanupGuestProfile(final Context context) {
        final GeckoProfile profile = getGuestProfile(context);
        if (profile == null) {
            return false;
        }

        if (!profile.locked()) {
            profile.mInGuestMode = false;

            
            removeGuestProfile(context);

            return true;
        }
        return false;
    }

    private static void removeGuestProfile(Context context) {
        boolean success = false;
        try {
            File guestDir = getGuestDir(context);
            if (guestDir.exists()) {
                success = delete(guestDir);
            }
        } catch (Exception ex) {
            Log.e(LOGTAG, "Error removing guest profile", ex);
        }

        if (success) {
            
            GeckoSharedPrefs.forProfileName(context, GUEST_PROFILE)
                            .edit().clear().apply();
        }
    }

    public static boolean delete(File file) throws IOException {
        
        if (file.delete())
            return true;

        if (file.isDirectory()) {
            
            String files[] = file.list();
            for (String temp : files) {
                File fileDelete = new File(file, temp);
                delete(fileDelete);
            }
        }

        
        return file.delete();
    }

    private GeckoProfile(Context context, String profileName) throws NoMozillaDirectoryException {
        if (TextUtils.isEmpty(profileName)) {
            throw new IllegalArgumentException("Unable to create GeckoProfile for empty profile name.");
        }

        mApplicationContext = context.getApplicationContext();
        mName = profileName;
        mIsWebAppProfile = profileName.startsWith("webapp");
        mMozillaDir = GeckoProfileDirectories.getMozillaDirectory(context);
    }

    
    public boolean locked() {
        if (mLocked != LockState.UNDEFINED) {
            return mLocked == LockState.LOCKED;
        }

        boolean profileExists;
        synchronized (this) {
            profileExists = mProfileDir != null && mProfileDir.exists();
        }

        
        if (profileExists) {
            File lockFile = new File(mProfileDir, LOCK_FILE_NAME);
            boolean res = lockFile.exists();
            mLocked = res ? LockState.LOCKED : LockState.UNLOCKED;
        } else {
            mLocked = LockState.UNLOCKED;
        }

        return mLocked == LockState.LOCKED;
    }

    public boolean lock() {
        try {
            
            final File lockFile = new File(getDir(), LOCK_FILE_NAME);
            final boolean result = lockFile.createNewFile();
            if (lockFile.exists()) {
                mLocked = LockState.LOCKED;
            } else {
                mLocked = LockState.UNLOCKED;
            }
            return result;
        } catch(IOException ex) {
            Log.e(LOGTAG, "Error locking profile", ex);
        }
        mLocked = LockState.UNLOCKED;
        return false;
    }

    public boolean unlock() {
        final File profileDir;
        synchronized (this) {
            
            profileDir = mProfileDir;
        }

        if (profileDir == null || !profileDir.exists()) {
            return true;
        }

        try {
            final File lockFile = new File(profileDir, LOCK_FILE_NAME);
            if (!lockFile.exists()) {
                mLocked = LockState.UNLOCKED;
                return true;
            }

            final boolean result = delete(lockFile);
            if (result) {
                mLocked = LockState.UNLOCKED;
            } else {
                mLocked = LockState.LOCKED;
            }
            return result;
        } catch(IOException ex) {
            Log.e(LOGTAG, "Error unlocking profile", ex);
        }

        mLocked = LockState.LOCKED;
        return false;
    }

    public boolean inGuestMode() {
        return mInGuestMode;
    }

    private void setDir(File dir) {
        if (dir != null && dir.exists() && dir.isDirectory()) {
            synchronized (this) {
                mProfileDir = dir;
            }
        }
    }

    public String getName() {
        return mName;
    }

    public synchronized File getDir() {
        forceCreate();
        return mProfileDir;
    }

    public synchronized GeckoProfile forceCreate() {
        if (mProfileDir != null) {
            return this;
        }

        try {
            
            try {
                mProfileDir = findProfileDir();
                Log.d(LOGTAG, "Found profile dir.");
            } catch (NoSuchProfileException noSuchProfile) {
                
                mProfileDir = createProfileDir();
            }
        } catch (IOException ioe) {
            Log.e(LOGTAG, "Error getting profile dir", ioe);
        }
        return this;
    }

    public File getFile(String aFile) {
        File f = getDir();
        if (f == null)
            return null;

        return new File(f, aFile);
    }

    









    public void moveSessionFile() {
        File sessionFile = getFile("sessionstore.js");
        if (sessionFile != null && sessionFile.exists()) {
            File sessionFileBackup = getFile("sessionstore.bak");
            sessionFile.renameTo(sessionFileBackup);
        }
    }

    











    public String readSessionFile(boolean readBackup) {
        File sessionFile = getFile(readBackup ? "sessionstore.bak" : "sessionstore.js");

        try {
            if (sessionFile != null && sessionFile.exists()) {
                return readFile(sessionFile);
            }
        } catch (IOException ioe) {
            Log.e(LOGTAG, "Unable to read session file", ioe);
        }
        return null;
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
            StringBuilder sb = new StringBuilder();
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

    private boolean remove() {
        try {
            synchronized (this) {
                final File dir = getDir();
                if (dir.exists()) {
                    delete(dir);
                }

                try {
                    mProfileDir = findProfileDir();
                } catch (NoSuchProfileException noSuchProfile) {
                    
                    return false;
                }
            }

            final INIParser parser = GeckoProfileDirectories.getProfilesINI(mMozillaDir);
            final Hashtable<String, INISection> sections = parser.getSections();
            for (Enumeration<INISection> e = sections.elements(); e.hasMoreElements();) {
                final INISection section = e.nextElement();
                String name = section.getStringProperty("Name");

                if (name == null || !name.equals(mName)) {
                    continue;
                }

                if (section.getName().startsWith("Profile")) {
                    
                    try {
                        int sectionNumber = Integer.parseInt(section.getName().substring("Profile".length()));
                        String curSection = "Profile" + sectionNumber;
                        String nextSection = "Profile" + (sectionNumber+1);

                        sections.remove(curSection);

                        while (sections.containsKey(nextSection)) {
                            parser.renameSection(nextSection, curSection);
                            sectionNumber++;

                            curSection = nextSection;
                            nextSection = "Profile" + (sectionNumber+1);
                        }
                    } catch (NumberFormatException nex) {
                        
                        Log.e(LOGTAG, "Malformed section name in profiles.ini: " + section.getName());
                        return false;
                    }
                } else {
                    
                    parser.removeSection(mName);
                }

                break;
            }

            parser.write();
            return true;
        } catch (IOException ex) {
            Log.w(LOGTAG, "Failed to remove profile.", ex);
            return false;
        }
    }

    







    public static String getDefaultProfileName(final Context context) throws NoMozillaDirectoryException {
        
        
        
        if (sDefaultProfileName != null) {
            return sDefaultProfileName;
        }

        final String profileName = GeckoProfileDirectories.findDefaultProfileName(context);
        if (profileName == null) {
            
            sDefaultProfileName = DEFAULT_PROFILE;
            return DEFAULT_PROFILE;
        }

        sDefaultProfileName = profileName;
        return sDefaultProfileName;
    }

    private File findProfileDir() throws NoSuchProfileException {
        return GeckoProfileDirectories.findProfileDir(mMozillaDir, mName);
    }

    private File createProfileDir() throws IOException {
        INIParser parser = GeckoProfileDirectories.getProfilesINI(mMozillaDir);

        
        String saltedName = GeckoProfileDirectories.saltProfileName(mName);
        File profileDir = new File(mMozillaDir, saltedName);
        while (profileDir.exists()) {
            saltedName = GeckoProfileDirectories.saltProfileName(mName);
            profileDir = new File(mMozillaDir, saltedName);
        }

        
        if (!profileDir.mkdirs()) {
            throw new IOException("Unable to create profile.");
        }
        Log.d(LOGTAG, "Created new profile dir.");

        
        
        
        int profileNum = 0;
        boolean isDefaultSet = false;
        INISection profileSection;
        while ((profileSection = parser.getSection("Profile" + profileNum)) != null) {
            profileNum++;
            if (profileSection.getProperty("Default") != null) {
                isDefaultSet = true;
            }
        }

        profileSection = new INISection("Profile" + profileNum);
        profileSection.setProperty("Name", mName);
        profileSection.setProperty("IsRelative", 1);
        profileSection.setProperty("Path", saltedName);

        if (parser.getSection("General") == null) {
            INISection generalSection = new INISection("General");
            generalSection.setProperty("StartWithLastProfile", 1);
            parser.addSection(generalSection);
        }

        if (!isDefaultSet && !mIsWebAppProfile) {
            
            
            profileSection.setProperty("Default", 1);

            
            
            
            Telemetry.startUISession(TelemetryContract.Session.FIRSTRUN);
        }

        parser.addSection(profileSection);
        parser.write();

        
        if (!mIsWebAppProfile) {
            enqueueInitialization();
        }

        
        try {
            FileOutputStream stream = new FileOutputStream(profileDir.getAbsolutePath() + File.separator + "times.json");
            OutputStreamWriter writer = new OutputStreamWriter(stream, Charset.forName("UTF-8"));
            try {
                writer.append("{\"created\": " + System.currentTimeMillis() + "}\n");
            } finally {
                writer.close();
            }
        } catch (Exception e) {
            
            Log.w(LOGTAG, "Couldn't write times.json.", e);
        }

        LocalBroadcastManager lbm = LocalBroadcastManager.getInstance(mApplicationContext);
        final Intent intent = new Intent(BrowserApp.ACTION_NEW_PROFILE);
        lbm.sendBroadcast(intent);

        return profileDir;
    }

    








    @RobocopTarget
    public void enqueueInitialization() {
        Log.i(LOGTAG, "Enqueuing profile init.");
        final Context context = mApplicationContext;

        
        final Distribution distribution = Distribution.getInstance(context);
        distribution.addOnDistributionReadyCallback(new Runnable() {
            @Override
            public void run() {
                Log.d(LOGTAG, "Running post-distribution task: bookmarks.");

                final ContentResolver cr = context.getContentResolver();

                
                
                
                
                
                final int offset = BrowserDB.addDistributionBookmarks(cr, distribution, 0);
                BrowserDB.addDefaultBookmarks(context, cr, offset);
            }
        });
    }
}
