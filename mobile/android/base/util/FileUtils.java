



package org.mozilla.gecko.util;

import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.io.FilenameFilter;
import java.util.Scanner;

import org.mozilla.gecko.mozglue.RobocopTarget;

public class FileUtils {
    private static final String LOGTAG= "GeckoFileUtils";
    


    static public class NameAndAgeFilter implements FilenameFilter {
        final private String mName;
        final private double mMaxAge;

        public NameAndAgeFilter(String name, double age) {
            mName = name;
            mMaxAge = age;
        }

        @Override
        public boolean accept(File dir, String filename) {
            if (mName == null || mName.matches(filename)) {
                File f = new File(dir, filename);

                if (mMaxAge < 0 || System.currentTimeMillis() - f.lastModified() > mMaxAge) {
                    return true;
                }
            }

            return false;
        }
    }

    @RobocopTarget
    public static void delTree(File dir, FilenameFilter filter, boolean recurse) {
        String[] files = null;

        if (filter != null) {
          files = dir.list(filter);
        } else {
          files = dir.list();
        }

        if (files == null) {
          return;
        }

        for (String file : files) {
            File f = new File(dir, file);
            delete(f, recurse);
        }
    }

    public static boolean delete(File file) throws IOException {
        return delete(file, true);
    }

    public static boolean delete(File file, boolean recurse) {
        if (file.isDirectory() && recurse) {
            
            String files[] = file.list();
            for (String temp : files) {
                File fileDelete = new File(file, temp);
                try {
                    delete(fileDelete);
                } catch(IOException ex) {
                    Log.i(LOGTAG, "Error deleting " + fileDelete.getPath(), ex);
                }
            }
        }

        
        return file.delete();
    }

    
    public static String getFileContents(File file) throws IOException {
        Scanner scanner = null;
        try {
            scanner = new Scanner(file, "UTF-8");
            return scanner.useDelimiter("\\A").next();
        } finally {
            if (scanner != null) {
                scanner.close();
            }
        }
    }
}
