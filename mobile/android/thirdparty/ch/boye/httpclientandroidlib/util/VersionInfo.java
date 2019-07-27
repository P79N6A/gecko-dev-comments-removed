


























package ch.boye.httpclientandroidlib.util;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Properties;













public class VersionInfo {

    
    public final static String UNAVAILABLE = "UNAVAILABLE";

    
    public final static String VERSION_PROPERTY_FILE = "version.properties";

    
    public final static String PROPERTY_MODULE    = "info.module";
    public final static String PROPERTY_RELEASE   = "info.release";
    public final static String PROPERTY_TIMESTAMP = "info.timestamp";


    
    private final String infoPackage;

    
    private final String infoModule;

    
    private final String infoRelease;

    
    private final String infoTimestamp;

    
    private final String infoClassloader;


    








    protected VersionInfo(final String pckg, final String module,
                          final String release, final String time, final String clsldr) {
        Args.notNull(pckg, "Package identifier");
        infoPackage     = pckg;
        infoModule      = (module  != null) ? module  : UNAVAILABLE;
        infoRelease     = (release != null) ? release : UNAVAILABLE;
        infoTimestamp   = (time    != null) ? time    : UNAVAILABLE;
        infoClassloader = (clsldr  != null) ? clsldr  : UNAVAILABLE;
    }


    





    public final String getPackage() {
        return infoPackage;
    }

    





    public final String getModule() {
        return infoModule;
    }

    





    public final String getRelease() {
        return infoRelease;
    }

    





    public final String getTimestamp() {
        return infoTimestamp;
    }

    







    public final String getClassloader() {
        return infoClassloader;
    }


    




    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder
            (20 + infoPackage.length() + infoModule.length() +
             infoRelease.length() + infoTimestamp.length() +
             infoClassloader.length());

        sb.append("VersionInfo(")
            .append(infoPackage).append(':').append(infoModule);

        
        
        if (!UNAVAILABLE.equals(infoRelease)) {
            sb.append(':').append(infoRelease);
        }
        if (!UNAVAILABLE.equals(infoTimestamp)) {
            sb.append(':').append(infoTimestamp);
        }

        sb.append(')');

        if (!UNAVAILABLE.equals(infoClassloader)) {
            sb.append('@').append(infoClassloader);
        }

        return sb.toString();
    }


    









    public static VersionInfo[] loadVersionInfo(final String[] pckgs,
                                                      final ClassLoader clsldr) {
        Args.notNull(pckgs, "Package identifier array");
        final List<VersionInfo> vil = new ArrayList<VersionInfo>(pckgs.length);
        for (final String pckg : pckgs) {
            final VersionInfo vi = loadVersionInfo(pckg, clsldr);
            if (vi != null) {
                vil.add(vi);
            }
        }

        return vil.toArray(new VersionInfo[vil.size()]);
    }


    











    public static VersionInfo loadVersionInfo(final String pckg,
                                              final ClassLoader clsldr) {
        Args.notNull(pckg, "Package identifier");
        final ClassLoader cl = clsldr != null ? clsldr : Thread.currentThread().getContextClassLoader();

        Properties vip = null; 
        try {
            
            
            final InputStream is = cl.getResourceAsStream
                (pckg.replace('.', '/') + "/" + VERSION_PROPERTY_FILE);
            if (is != null) {
                try {
                    final Properties props = new Properties();
                    props.load(is);
                    vip = props;
                } finally {
                    is.close();
                }
            }
        } catch (final IOException ex) {
            
        }

        VersionInfo result = null;
        if (vip != null) {
            result = fromMap(pckg, vip, cl);
        }

        return result;
    }


    









    protected static VersionInfo fromMap(final String pckg, final Map<?, ?> info,
                                               final ClassLoader clsldr) {
        Args.notNull(pckg, "Package identifier");
        String module = null;
        String release = null;
        String timestamp = null;

        if (info != null) {
            module = (String) info.get(PROPERTY_MODULE);
            if ((module != null) && (module.length() < 1)) {
                module = null;
            }

            release = (String) info.get(PROPERTY_RELEASE);
            if ((release != null) && ((release.length() < 1) ||
                                      (release.equals("${pom.version}")))) {
                release = null;
            }

            timestamp = (String) info.get(PROPERTY_TIMESTAMP);
            if ((timestamp != null) &&
                ((timestamp.length() < 1) ||
                 (timestamp.equals("${mvn.timestamp}")))
                ) {
                timestamp = null;
            }
        } 

        String clsldrstr = null;
        if (clsldr != null) {
            clsldrstr = clsldr.toString();
        }

        return new VersionInfo(pckg, module, release, timestamp, clsldrstr);
    }

    













    public static String getUserAgent(final String name, final String pkg, final Class<?> cls) {
        
        final VersionInfo vi = VersionInfo.loadVersionInfo(pkg, cls.getClassLoader());
        final String release = (vi != null) ? vi.getRelease() : VersionInfo.UNAVAILABLE;
        final String javaVersion = System.getProperty("java.version");
        return name + "/" + release + " (Java 1.5 minimum; Java/" + javaVersion + ")";
    }

} 
