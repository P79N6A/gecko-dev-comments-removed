



package org.mozilla.gecko;

import java.io.File;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;

import org.mozilla.gecko.mozglue.RobocopTarget;
import org.mozilla.gecko.util.INIParser;
import org.mozilla.gecko.util.INISection;

import android.content.Context;








public class GeckoProfileDirectories {
    @SuppressWarnings("serial")
    public static class NoMozillaDirectoryException extends Exception {
        public NoMozillaDirectoryException(Throwable cause) {
            super(cause);
        }

        public NoMozillaDirectoryException(String reason) {
            super(reason);
        }

        public NoMozillaDirectoryException(String reason, Throwable cause) {
            super(reason, cause);
        }
    }

    @SuppressWarnings("serial")
    public static class NoSuchProfileException extends Exception {
        public NoSuchProfileException(String detailMessage, Throwable cause) {
            super(detailMessage, cause);
        }

        public NoSuchProfileException(String detailMessage) {
            super(detailMessage);
        }
    }

    private interface INISectionPredicate {
        public boolean matches(INISection section);
    }

    private static final String MOZILLA_DIR_NAME = "mozilla";

    


    private static INISectionPredicate sectionIsDefault = new INISectionPredicate() {
        @Override
        public boolean matches(INISection section) {
            return section.getIntProperty("Default") == 1;
        }
    };

    


    private static INISectionPredicate sectionHasName = new INISectionPredicate() {
        @Override
        public boolean matches(INISection section) {
            final String name = section.getStringProperty("Name");
            return name != null;
        }
    };

    @RobocopTarget
    public static INIParser getProfilesINI(File mozillaDir) {
        return new INIParser(new File(mozillaDir, "profiles.ini"));
    }

    



    public static String saltProfileName(final String name) {
        if (name == null) {
            throw new IllegalArgumentException("Cannot salt null profile name.");
        }

        final String allowedChars = "abcdefghijklmnopqrstuvwxyz0123456789";
        final int scale = allowedChars.length();
        final int saltSize = 8;

        final StringBuilder saltBuilder = new StringBuilder(saltSize + 1 + name.length());
        for (int i = 0; i < saltSize; i++) {
            saltBuilder.append(allowedChars.charAt((int)(Math.random() * scale)));
        }
        saltBuilder.append('.');
        saltBuilder.append(name);
        return saltBuilder.toString();
    }

    










    @RobocopTarget
    public static File getMozillaDirectory(Context context) throws NoMozillaDirectoryException {
        final File mozillaDir = new File(context.getFilesDir(), MOZILLA_DIR_NAME);
        if (mozillaDir.exists() || mozillaDir.mkdirs()) {
            return mozillaDir;
        }

        
        
        throw new NoMozillaDirectoryException("Unable to create mozilla directory at " + mozillaDir.getAbsolutePath());
    }

    









    static String findDefaultProfileName(final Context context) throws NoMozillaDirectoryException {
      final INIParser parser = GeckoProfileDirectories.getProfilesINI(getMozillaDirectory(context));

      for (Enumeration<INISection> e = parser.getSections().elements(); e.hasMoreElements();) {
          final INISection section = e.nextElement();
          if (section.getIntProperty("Default") == 1) {
              return section.getStringProperty("Name");
          }
      }

      return null;
    }

    static Map<String, String> getDefaultProfile(final File mozillaDir) {
        return getMatchingProfiles(mozillaDir, sectionIsDefault, true);
    }

    static Map<String, String> getProfilesNamed(final File mozillaDir, final String name) {
        final INISectionPredicate predicate = new INISectionPredicate() {
            @Override
            public boolean matches(final INISection section) {
                return name.equals(section.getStringProperty("Name"));
            }
        };
        return getMatchingProfiles(mozillaDir, predicate, true);
    }

    



    static Map<String, String> getAllProfiles(final File mozillaDir) {
        return getMatchingProfiles(mozillaDir, sectionHasName, false);
    }

    















    public static Map<String, String> getMatchingProfiles(final File mozillaDir, INISectionPredicate predicate, boolean stopOnSuccess) {
        final HashMap<String, String> result = new HashMap<String, String>();
        final INIParser parser = GeckoProfileDirectories.getProfilesINI(mozillaDir);

        for (Enumeration<INISection> e = parser.getSections().elements(); e.hasMoreElements();) {
            final INISection section = e.nextElement();
            if (predicate == null || predicate.matches(section)) {
                final String name = section.getStringProperty("Name");
                final String pathString = section.getStringProperty("Path");
                final boolean isRelative = section.getIntProperty("IsRelative") == 1;
                final File path = isRelative ? new File(mozillaDir, pathString) : new File(pathString);
                result.put(name, path.getAbsolutePath());

                if (stopOnSuccess) {
                    return result;
                }
            }
        }
        return result;
    }

    public static File findProfileDir(final File mozillaDir, final String profileName) throws NoSuchProfileException {
        
        final INIParser parser = GeckoProfileDirectories.getProfilesINI(mozillaDir);

        for (Enumeration<INISection> e = parser.getSections().elements(); e.hasMoreElements();) {
            final INISection section = e.nextElement();
            final String name = section.getStringProperty("Name");
            if (name != null && name.equals(profileName)) {
                if (section.getIntProperty("IsRelative") == 1) {
                    return new File(mozillaDir, section.getStringProperty("Path"));
                }
                return new File(section.getStringProperty("Path"));
            }
        }

        throw new NoSuchProfileException("No profile " + profileName);
    }
}
