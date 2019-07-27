



package org.mozilla.gecko.tests;

import java.io.File;
import java.util.Enumeration;
import java.util.Hashtable;

import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.GeckoProfileDirectories;
import org.mozilla.gecko.GeckoSharedPrefs;
import org.mozilla.gecko.util.INIParser;
import org.mozilla.gecko.util.INISection;

import android.content.Context;
import android.text.TextUtils;






public class testGeckoProfile extends PixelTest {
    private final String TEST_PROFILE_NAME = "testProfile";
    private File mozDir;
    public void testGeckoProfile() {
        blockForGeckoReady();

        try {
            mozDir = GeckoProfileDirectories.getMozillaDirectory(getActivity());
        } catch(Exception ex) {
            
            mAsserter.ok(false, "Couldn't get moz dir", ex.toString());
            return;
        }

        checkProfileCreationDeletion();
        checkGuestProfile();
    }

    
    private void checkDefaultGetter() {
        mAsserter.info("Test using the default profile", GeckoProfile.DEFAULT_PROFILE);
        GeckoProfile profile = GeckoProfile.get(getActivity());
        
        verifyProfile(profile, GeckoProfile.DEFAULT_PROFILE, ((GeckoApp) getActivity()).getProfile().getDir(), true);

        try {
            profile = GeckoProfile.get(null);
            mAsserter.ok(false, "Passing a null context should throw", profile.toString());
        } catch(Exception ex) {
            mAsserter.ok(true, "Passing a null context should throw", ex.toString());
        }
    }

    
    private void checkNamedGetter(String name) {
        mAsserter.info("Test using a named profile", name);
        GeckoProfile profile = GeckoProfile.get(getActivity(), name);
        if (!TextUtils.isEmpty(name)) {
            verifyProfile(profile, name, findDir(name), false);
            removeProfile(profile, true);
        } else {
            
            File defaultProfile = ((GeckoApp) getActivity()).getProfile().getDir();
            verifyProfile(profile, GeckoProfile.DEFAULT_PROFILE, defaultProfile, true);
        }
    }

    
    private void checkNameAndPathGetter(String name, boolean createBefore) {
        if (TextUtils.isEmpty(name)) {
            checkNameAndPathGetter(name, null, createBefore);
        } else {
            checkNameAndPathGetter(name, name + "_FORCED_DIR", createBefore);
        }
    }

    
    private void checkNameAndPathGetter(String name, String path, boolean createBefore) {
        mAsserter.info("Test using a named profile and path", name + ", " + path);

        File f = null;
        if (!TextUtils.isEmpty(path)) {
            f = new File(mozDir, path);
            
            if (createBefore) {
                f.mkdir();
            }
            path = f.getAbsolutePath();
        }

        try {
            GeckoProfile profile = GeckoProfile.get(getActivity(), name, path);
            if (!TextUtils.isEmpty(name)) {
                verifyProfile(profile, name, f, createBefore);
                removeProfile(profile, !createBefore);
            } else {
                mAsserter.ok(TextUtils.isEmpty(path), "Passing a null name and non-null path should throw", name + ", " + path);
                
                File defaultProfile = ((GeckoApp) getActivity()).getProfile().getDir();
                verifyProfile(profile, GeckoProfile.DEFAULT_PROFILE, defaultProfile, true);
            }
        } catch(Exception ex) {
            mAsserter.ok(TextUtils.isEmpty(name) && !TextUtils.isEmpty(path), "Passing a null name and non null path should throw", name + ", " + path);
        }
    }

    private void checkNameAndFileGetter(String name, boolean createBefore) {
        if (TextUtils.isEmpty(name)) {
            checkNameAndFileGetter(name, null, createBefore);
        } else {
            checkNameAndFileGetter(name, new File(mozDir, name + "_FORCED_DIR"), createBefore);
        }
    }

    private void checkNameAndFileGetter(String name, File f, boolean createBefore) {
        mAsserter.info("Test using a named profile and path", name + ", " + f);
        if (f != null && createBefore) {
            f.mkdir();
        }

        try {
            GeckoProfile profile = GeckoProfile.get(getActivity(), name, f);
            if (!TextUtils.isEmpty(name)) {
                verifyProfile(profile, name, f, createBefore);
                removeProfile(profile, !createBefore);
            } else {
                mAsserter.ok(f == null, "Passing a null name and non-null file should throw", name + ", " + f);
                
                File defaultProfile = ((GeckoApp) getActivity()).getProfile().getDir();
                verifyProfile(profile, GeckoProfile.DEFAULT_PROFILE, defaultProfile, true);
            }
        } catch(Exception ex) {
            mAsserter.ok(TextUtils.isEmpty(name) && f != null, "Passing a null name and non null file should throw", name + ", " + f);
        }
    }

    private void checkProfileCreationDeletion() {
        
        checkDefaultGetter();

        int index = 0;
        checkNamedGetter(TEST_PROFILE_NAME + (index++)); 
        checkNamedGetter("");
        checkNamedGetter(null);

        
        checkNameAndPathGetter(TEST_PROFILE_NAME + (index++), true); 
        checkNameAndPathGetter(TEST_PROFILE_NAME + (index++), false); 
        
        checkNameAndPathGetter(null, TEST_PROFILE_NAME + (index++) + "_FORCED_DIR", true); 
        checkNameAndPathGetter(null, TEST_PROFILE_NAME + (index++) + "_FORCED_DIR", false); 
        checkNameAndPathGetter("", TEST_PROFILE_NAME + (index++) + "_FORCED_DIR", true); 
        checkNameAndPathGetter("", TEST_PROFILE_NAME + (index++) + "_FORCED_DIR", false); 
        
        checkNameAndPathGetter(TEST_PROFILE_NAME + (index++), null, false); 
        checkNameAndPathGetter(TEST_PROFILE_NAME + (index++), "", false); 
        
        checkNameAndPathGetter(null, null, false);
        checkNameAndPathGetter("", null, false);
        checkNameAndPathGetter(null, "", false);
        checkNameAndPathGetter("", "", false);

        
        checkNameAndFileGetter(TEST_PROFILE_NAME + (index++), true); 
        checkNameAndFileGetter(TEST_PROFILE_NAME + (index++), false); 
        
        checkNameAndFileGetter(null, new File(mozDir, TEST_PROFILE_NAME + (index++) + "_FORCED_DIR"), true); 
        checkNameAndFileGetter(null, new File(mozDir, TEST_PROFILE_NAME + (index++) + "_FORCED_DIR"), false); 
        checkNameAndFileGetter("", new File(mozDir, TEST_PROFILE_NAME + (index++) + "_FORCED_DIR"), true); 
        checkNameAndFileGetter("", new File(mozDir, TEST_PROFILE_NAME + (index++) + "_FORCED_DIR"), false); 
        
        checkNameAndFileGetter(TEST_PROFILE_NAME + (index++), null, false); 
        
        checkNameAndFileGetter(null, null, false);
    }

    
    private void checkGuestProfile() {
        mAsserter.info("Test getting a guest profile", "");
        GeckoProfile profile = GeckoProfile.createGuestProfile(getActivity());
        verifyProfile(profile, GeckoProfile.GUEST_PROFILE, getActivity().getFileStreamPath("guest"), true);
        File dir = profile.getDir();

        mAsserter.ok(profile.inGuestMode(), "Profile is in guest mode", profile.getName());

        mAsserter.info("Test deleting a guest profile", "");
        mAsserter.ok(!GeckoProfile.maybeCleanupGuestProfile(getActivity()), "Can't clean up locked guest profile", profile.getName());
        GeckoProfile.leaveGuestSession(getActivity());
        mAsserter.ok(GeckoProfile.maybeCleanupGuestProfile(getActivity()), "Cleaned up unlocked guest profile", profile.getName());
        mAsserter.ok(!dir.exists(), "Guest dir was deleted", dir.toString());
    }

    
    private void verifyProfile(GeckoProfile profile, String name, File requestedDir, boolean shouldHaveFound) {
        mAsserter.is(profile.getName(), name, "Profile name is correct");

        File dir = null;
        if (!shouldHaveFound) {
            mAsserter.is(findDir(name), null, "Dir with name doesn't exist yet");

            dir = profile.getDir();
            mAsserter.isnot(requestedDir, dir, "Profile should not have used expectedDir");

            
            requestedDir = findDir(name);
        } else {
            dir = profile.getDir();
        }

        mAsserter.is(dir, requestedDir, "Profile dir is correct");
        mAsserter.ok(dir.exists(), "Profile dir exists after getting it", dir.toString());
    }

    
    private void findInProfilesIni(GeckoProfile profile, boolean shouldFind) {
        final File mozDir;
        try {
            mozDir = GeckoProfileDirectories.getMozillaDirectory(getActivity());
        } catch(Exception ex) {
            mAsserter.ok(false, "Couldn't get moz dir", ex.toString());
            return;
        }

        final String name = profile.getName();
        final File dir = profile.getDir();

        final INIParser parser = GeckoProfileDirectories.getProfilesINI(mozDir);
        final Hashtable<String, INISection> sections = parser.getSections();

        boolean found = false;
        for (Enumeration<INISection> e = sections.elements(); e.hasMoreElements();) {
            final INISection section = e.nextElement();
            String iniName = section.getStringProperty("Name");
            if (iniName == null || !iniName.equals(name)) {
                continue;
            }

            found = true;

            String iniPath = section.getStringProperty("Path");
            mAsserter.is(name, iniName, "Section with name found");
            mAsserter.is(dir.getName(), iniPath, "Section has correct path");
        }

        mAsserter.is(found, shouldFind, "Found profile where expected");
    }

    
    private void removeProfile(GeckoProfile profile, boolean inProfilesIni) {
        findInProfilesIni(profile, inProfilesIni);
        File dir = profile.getDir();
        mAsserter.ok(dir.exists(), "Profile dir exists before removing", dir.toString());
        mAsserter.is(inProfilesIni, GeckoProfile.removeProfile(getActivity(), profile.getName()), "Remove was successful");
        mAsserter.ok(!dir.exists(), "Profile dir was deleted when it was removed", dir.toString());
        findInProfilesIni(profile, false);
    }

    
    private File findDir(String name) {
        final File root;
        try {
            root = GeckoProfileDirectories.getMozillaDirectory(getActivity());
        } catch(Exception ex) {
            return null;
        }

        File[] dirs = root.listFiles();
        for (File dir : dirs) {
            if (dir.getName().endsWith(name)) {
                return dir;
            }
        }

        return null;
    }

    @Override
    public void tearDown() throws Exception {
        
        final Context context = getInstrumentation().getContext();
        GeckoSharedPrefs.forProfile(context).edit().clear().apply();

        super.tearDown();
    }
}
