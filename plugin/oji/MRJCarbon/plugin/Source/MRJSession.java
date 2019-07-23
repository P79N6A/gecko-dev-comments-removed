




































package netscape.oji;

import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.util.Date;
import java.util.Properties;
import java.net.URL;







public class MRJSession {
    
    private static PrintStream out;
    private static PrintStream err;
    private static PrintStream console;

    private static Properties loadProperties(String pluginHome) {
        Properties props = new Properties();
        try {
            InputStream propsStream = new FileInputStream(pluginHome + "/MRJPlugin.properties");
            props.load(propsStream);
            propsStream.close();
        } catch (IOException ex) {
        }
        return props;
    }

    public static void open(String consolePath) throws IOException {
        String pluginHome = System.getProperty("netscape.oji.plugin.home");
        Properties props = loadProperties(pluginHome);
        boolean append = Boolean.valueOf(props.getProperty("netscape.oji.plugin.console.append")).booleanValue();
    
        
        File consoleFile = new File(consolePath);
        File parentFile = consoleFile.getParentFile();
        if (!parentFile.exists()) {
            parentFile.mkdirs();
        }
    
        
        MRJSession.out = System.out;
        MRJSession.err = System.err;
        console = new PrintStream(new FileOutputStream(consolePath, append));
        System.setOut(console);
        System.setErr(console);

        Date date = new Date();
        String version = props.getProperty("netscape.oji.plugin.version");
        System.out.println("MRJ Plugin for Mac OS X v" + version);
        System.out.println("[starting up Java Applet Security @ " + date + "]");

        
        if (System.getSecurityManager() == null) {
            try {
                
                
                System.setProperty("java.security.policy", "file:" + pluginHome + "/MRJPlugin.policy");
                String name = props.getProperty("netscape.oji.plugin.security");
                SecurityManager securityManager = (SecurityManager) Class.forName(name).newInstance();
                System.setSecurityManager(securityManager);
            } catch (Exception ex) {
            }
        }
    }

    public static void close() throws IOException {
        System.setOut(MRJSession.out);
        System.setErr(MRJSession.err);
        console.close();
    }
}
