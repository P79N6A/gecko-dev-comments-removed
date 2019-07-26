




package org.mozilla.gecko;

import org.mozilla.gecko.util.INIParser;
import org.mozilla.gecko.util.INISection;
import org.mozilla.gecko.util.ThreadUtils;

import android.content.Context;
import android.text.TextUtils;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.nio.charset.Charset;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Hashtable;

public final class GeckoProfile {
    private static final String LOGTAG = "GeckoProfile";
    
    private static final String LOCK_FILE_NAME = ".active_lock";

    private static HashMap<String, GeckoProfile> sProfileCache = new HashMap<String, GeckoProfile>();
    private static String sDefaultProfileName = null;
    private static File mMozDir;

    private final Context mContext;
    private final String mName;
    private File mDir;

    
    private enum LockState {
        LOCKED,
        UNLOCKED,
        UNDEFINED
    };
    
    
    private LockState mLocked = LockState.UNDEFINED;

    
    private static File mGuestDir = null;

    private boolean mInGuestMode = false;
    private static GeckoProfile mGuestProfile = null;

    private static native int createSymLink(String filePath, String linkPath);
    private static native int removeSymLink(String linkPath);

    public static GeckoProfile get(Context context) {
        if (context instanceof GeckoApp) {
            
            
            if (((GeckoApp)context).mProfile != null) {
                return ((GeckoApp)context).mProfile;
            }

            GeckoProfile guest = GeckoProfile.getGuestProfile(context);
            
            if (guest != null && guest.locked()) {
                return guest;
            }

            
            return get(context, ((GeckoApp)context).getDefaultProfileName());
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

    public static GeckoProfile get(Context context, String profileName, String profilePath) {
        File dir = null;
        if (!TextUtils.isEmpty(profilePath))
            dir = new File(profilePath);
        return get(context, profileName, dir);
    }

    public static GeckoProfile get(Context context, String profileName, File profileDir) {
        if (context == null) {
            throw new IllegalArgumentException("context must be non-null");
        }

        
        
        if (TextUtils.isEmpty(profileName) && profileDir == null) {
            profileName = GeckoProfile.findDefaultProfile(context);
            if (profileName == null)
                profileName = "default";
        }

        
        synchronized (sProfileCache) {
            GeckoProfile profile = sProfileCache.get(profileName);
            if (profile == null) {
                profile = new GeckoProfile(context, profileName, profileDir);
                sProfileCache.put(profileName, profile);
            } else {
                profile.setDir(profileDir);
            }
            return profile;
        }
    }

    public static File ensureMozillaDirectory(Context context) throws IOException {
        if (mMozDir != null) {
            return mMozDir;
        }

        synchronized (context) {
            File filesDir = context.getFilesDir();
            File mozDir = new File(filesDir, "mozilla");
            if (! mozDir.exists()) {
                if (! mozDir.mkdirs()) {
                    throw new IOException("Unable to create mozilla directory at " + mozDir.getAbsolutePath());
                }
            }
            mMozDir = mozDir;
            return mozDir;
        }
    }

    public static boolean removeProfile(Context context, String profileName) {
        return new GeckoProfile(context, profileName).remove();
    }

    public static GeckoProfile createGuestProfile(Context context) {
        try {
            removeGuestProfile(context);
            
            
            getGuestDir(context).mkdir();
            GeckoProfile profile = getGuestProfile(context);
            profile.lock();
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
        if (mGuestDir == null) {
            mGuestDir = context.getFileStreamPath("guest");
        }
        return mGuestDir;
    }

    private static GeckoProfile getGuestProfile(Context context) {
        if (mGuestProfile == null) {
            File guestDir = getGuestDir(context);
            if (guestDir.exists()) {
                mGuestProfile = get(context, "guest", guestDir);
                mGuestProfile.mInGuestMode = true;
            }
        }

        return mGuestProfile;
    }

    public static boolean maybeCleanupGuestProfile(final Context context) {
        final GeckoProfile profile = getGuestProfile(context);

        if (profile == null) {
            return false;
        } else if (!profile.locked()) {
            profile.mInGuestMode = false;

            
            removeGuestProfile(context);

            return true;
        }
        return false;
    }

    private static void removeGuestProfile(Context context) {
        try {
            File guestDir = getGuestDir(context);
            if (guestDir.exists()) {
                delete(guestDir);
            }
        } catch (Exception ex) {
            Log.e(LOGTAG, "Error removing guest profile", ex);
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

    
    public boolean locked() {
        if (mLocked != LockState.UNDEFINED) {
            return mLocked == LockState.LOCKED;
        }

        File lockFile = new File(getDir(), LOCK_FILE_NAME);
        boolean res = lockFile.exists();
        mLocked = res ? LockState.LOCKED : LockState.UNLOCKED;
        return res;
    }

    public boolean lock() {
        try {
            File lockFile = new File(getDir(), LOCK_FILE_NAME);
            boolean result = lockFile.createNewFile();
            if (result) {
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
        try {
            File lockFile = new File(getDir(), LOCK_FILE_NAME);
            boolean result = delete(lockFile);
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

    private GeckoProfile(Context context, String profileName) {
        mContext = context;
        mName = profileName;
    }

    private GeckoProfile(Context context, String profileName, File profileDir) {
        mContext = context;
        mName = profileName;
        setDir(profileDir);
    }

    public boolean inGuestMode() {
        return mInGuestMode;
    }

    private void setDir(File dir) {
        if (dir != null && dir.exists() && dir.isDirectory()) {
            mDir = dir;
        } else {
            Log.w(LOGTAG, "requested profile directory missing: " + dir);
        }
    }

    public String getName() {
        return mName;
    }

    public synchronized File getDir() {
        if (mDir != null) {
            return mDir;
        }

        try {
            
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

    public File getFile(String aFile) {
        File f = getDir();
        if (f == null)
            return null;

        return new File(f, aFile);
    }

    public File getFilesDir() {
        return mContext.getFilesDir();
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
            File dir = getDir();
            if (dir.exists())
                delete(dir);

            File mozillaDir = ensureMozillaDirectory(mContext);
            mDir = findProfileDir(mozillaDir);
            if (mDir == null) {
                return false;
            } else {
                delete(mDir);
                return true;
            }
        } catch (IOException ioe) {
            Log.e(LOGTAG, "Error getting profile dir", ioe);
        }
        return false;
    }

    public static String findDefaultProfile(Context context) {
        
        
        
        if (sDefaultProfileName != null) {
            return sDefaultProfileName;
        }

        try {
            File activeDir = new File(ensureMozillaDirectory(context), "active-profile");
            if (activeDir.exists()) {
                sDefaultProfileName = activeDir.getCanonicalFile().getName();
            }
        } catch (IOException ioe) {
            Log.e(LOGTAG, "Error getting currDir", ioe);
        }
        return sDefaultProfileName;
    }

    public static void setAsDefault(Context context, File defaultFile) {
        try {
            String symlinkPath = ensureMozillaDirectory(context).getAbsolutePath() + "/active-profile";
            File activeFile = new File(symlinkPath);
            
            if (activeFile.exists()) {
                removeSymLink(symlinkPath);
            }

            
            createSymLink(defaultFile.getAbsolutePath(), symlinkPath);
        } catch (IOException ioe) {
            Log.e(LOGTAG, "Error setting default profile", ioe);
        }
    }

    private File findProfileDir(File mozillaDir) {
        File nameDir = new File(mozillaDir, mName);
        if (! nameDir.exists()) {
            return null;
        }

        return nameDir;
    }

    private File createProfileDir(File mozillaDir) throws IOException {
        File profileDir = new File(mozillaDir, mName);

        
        if (! profileDir.mkdirs()) {
            throw new IOException("Unable to create profile at " + profileDir.getAbsolutePath());
        }
        Log.d(LOGTAG, "Created new profile dir at " + profileDir.getAbsolutePath());

        
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

        return profileDir;
    }
}
