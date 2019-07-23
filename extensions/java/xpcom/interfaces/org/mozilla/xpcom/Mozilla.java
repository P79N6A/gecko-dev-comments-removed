



































package org.mozilla.xpcom;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLClassLoader;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.Properties;

import org.mozilla.interfaces.nsIComponentManager;
import org.mozilla.interfaces.nsIComponentRegistrar;
import org.mozilla.interfaces.nsILocalFile;
import org.mozilla.interfaces.nsIServiceManager;
import org.mozilla.interfaces.nsISupports;






























public class Mozilla implements IMozilla, IGRE, IXPCOM, IJavaXPCOMUtils,
IXPCOMError {

  private static Mozilla mozillaInstance = new Mozilla();

  private static final String JAVAXPCOM_JAR = "javaxpcom.jar";

  private IMozilla mozilla = null;
  private IGRE gre = null;
  private IXPCOM xpcom = null;
  private IJavaXPCOMUtils jxutils = null;

  


  public static Mozilla getInstance() {
    return mozillaInstance;
  }

  


  private Mozilla() {
  }

  





















  public static File getGREPathWithProperties(GREVersionRange[] aVersions,
          Properties aProperties) throws FileNotFoundException {
    File grePath = null;

    
    String env = System.getProperty("GRE_HOME");
    if (env != null) {
      try {
        grePath = new File(env).getCanonicalFile();
      } catch (IOException e) {
        throw new FileNotFoundException("cannot access GRE_HOME");
      }
      if (!grePath.exists()) {
        throw new FileNotFoundException("GRE_HOME doesn't exist");
      }
      return grePath;
    }

    
    env = System.getProperty("USE_LOCAL_GRE");
    if (env != null) {
      return null;
    }

    
    
    if (aProperties == null) {
      aProperties = new Properties();
    }
    aProperties.setProperty("javaxpcom", "1");

    String osName = System.getProperty("os.name").toLowerCase();
    if (osName.startsWith("mac os x")) {
      grePath = getGREPathMacOSX(aVersions);
    } else if (osName.startsWith("windows")) {
      grePath = getGREPathWindows(aVersions, aProperties);
    } else {
      
      grePath = getGREPathUnix(aVersions, aProperties);
    }

    if (grePath == null) {
      throw new FileNotFoundException("GRE not found");
    }

    return grePath;
  }

  



  private static File getGREPathMacOSX(GREVersionRange[] aVersions) {
    



    File grePath = findGREBundleFramework();
    if (grePath != null) {
      return grePath;
    }

    
    String home = System.getProperty("user.home");
    if (home != null) {
      grePath = findGREFramework(home, aVersions);
      if (grePath != null) {
        return grePath;
      }
    }

    
    return findGREFramework("", aVersions);
  }

  


  private static File findGREBundleFramework() {
    



    try {
      URL[] urls = new URL[1];
      urls[0] = new File("/System/Library/Java/").toURL();
      ClassLoader loader = new URLClassLoader(urls);
      Class bundleClass = Class.forName("com.apple.cocoa.foundation.NSBundle",
                                        true, loader);

      
      
      Method mainBundleMethod = bundleClass.getMethod("mainBundle", null);
      Object bundle = mainBundleMethod.invoke(null, null);

      if (bundle != null) {
        
        Method fwPathMethod = bundleClass.getMethod("privateFrameworksPath",
                                                    null);
        String path = (String) fwPathMethod.invoke(bundle, null);

        
        if (path.length() != 0) {
          File xulDir = new File(path, "XUL.framework");
          if (xulDir.isDirectory()) {
            File xpcomLib = new File(xulDir, "libxpcom.dylib");
            if (xpcomLib.canRead()) {
              File grePath = xpcomLib.getCanonicalFile().getParentFile();

              
              
              File jar = new File(grePath, JAVAXPCOM_JAR);
              if (jar.canRead()) {
                
                return grePath;
              }
            }
          }
        }
      }
    } catch (Exception e) { }

    return null;
  }

  




  private static File findGREFramework(String aRootPath,
                                       GREVersionRange[] aVersions) {
    File frameworkDir = new File(aRootPath +
                                 "/Library/Frameworks/XUL.framework/Versions");
    if (!frameworkDir.exists())
      return null;

    File[] files = frameworkDir.listFiles();
    for (int i = 0; i < files.length; i++) {
      if (checkVersion(files[i].getName(), aVersions)) {
        File xpcomLib = new File(files[i], "libxpcom.dylib");

        
        
        File jar = new File(files[i], JAVAXPCOM_JAR);
        if (xpcomLib.canRead() && jar.canRead()) {
          return files[i];
        }
      }
    }

    return null;
  }

  




  private static File getGREPathWindows(GREVersionRange[] aVersions,
                                        Properties aProperties) {
    








    final String greKey = "Software\\mozilla.org\\GRE";

    
    
    String key = "HKEY_CURRENT_USER" + "\\" + greKey;
    File grePath = getGREPathFromRegKey(key, aVersions, aProperties);
    if (grePath == null) {
      key = "HKEY_LOCAL_MACHINE" + "\\" + greKey;
      grePath = getGREPathFromRegKey(key, aVersions, aProperties);
    }

    return grePath;
  }

  





  private static File getGREPathFromRegKey(String aRegKey,
          GREVersionRange[] aVersions, Properties aProperties) {
    
    File tempFile;
    try {
      tempFile = File.createTempFile("jx_registry", null);
    } catch (IOException e) {
      
      return null;
    }

    Process proc;
    try {
      proc = Runtime.getRuntime().exec("regedit /e " + "\"" + tempFile.getPath()
              + "\" \"" + aRegKey + "\"");
      proc.waitFor();
    } catch (Exception e) {
      
      
    }

    
    
    File grePath = null;
    if (tempFile.length() != 0) {
      grePath = getGREPathFromRegistryFile(tempFile.getPath(),
              aRegKey, aVersions, aProperties);
    }

    tempFile.delete();
    return grePath;
  }

  







  private static File getGREPathFromRegistryFile(String aFileName,
          String aKeyName, GREVersionRange[] aVersions,
          Properties aProperties) {
    INIParser parser;
    try {
      parser = new INIParser(aFileName, Charset.forName("UTF-16"));
    } catch (Exception e) {
      
      return null;
    }

    Iterator sectionsIter = parser.getSections();
    while (sectionsIter.hasNext()) {
      
      String section = (String) sectionsIter.next();

      
      int gre_len = aKeyName.length();
      if (section.length() <= gre_len) {
        continue;
      }

      
      
      String subkeyName = section.substring(gre_len + 1);

      
      
      
      if (subkeyName.indexOf('\\') != -1) {
        continue;
      }

      
      
      String version = parser.getString(section, "\"Version\"");
      if (version == null) {
        continue;
      }
      
      version = version.substring(1, version.length() - 1);
      if (!checkVersion(version, aVersions)) {
        continue;
      }

      
      
      if (aProperties != null) {
        boolean ok = true;
        Enumeration e = aProperties.propertyNames();
        while (ok && e.hasMoreElements()) {
          String prop = (String) e.nextElement();
          String greValue = parser.getString(section, "\"" + prop + "\"");
          if (greValue == null) {
            
            ok = false;
          } else  {
            
            
            String value = aProperties.getProperty(prop);
            if (!greValue.equals("\"" + value + "\"")) {
              ok = false;
            }
          }
        }
        if (!ok) {
          continue;
        }
      }

      String pathStr = parser.getString(section, "\"GreHome\"");
      if (pathStr != null) {
        
        pathStr = pathStr.substring(1, pathStr.length() - 1);
        File grePath = new File(pathStr);
        if (grePath.exists()) {
          File xpcomLib = new File(grePath, "xpcom.dll");
          if (xpcomLib.canRead()) {
            
            return grePath;
          }
        }
      }
    }

    return null;
  }

  




  private static File getGREPathUnix(GREVersionRange[] aVersions,
                                     Properties aProperties) {
    File grePath = null;

    String env = System.getProperty("MOZ_GRE_CONF");
    if (env != null) {
      grePath = getPathFromConfigFile(env, aVersions, aProperties);
      if (grePath != null) {
        return grePath;
      }
    }

    final String greUserConfFile = ".gre.config";
    final String greUserConfDir = ".gre.d";
    final String greConfPath = "/etc/gre.conf";
    final String greConfDir = "/etc/gre.d";

    env = System.getProperty("user.home");
    if (env != null) {
      
      grePath = getPathFromConfigFile(env + File.separator + greUserConfFile,
                                      aVersions, aProperties);
      if (grePath != null) {
        return grePath;
      }

      
      grePath = getPathFromConfigDir(env + File.separator + greUserConfDir,
                                     aVersions, aProperties);
      if (grePath != null) {
        return grePath;
      }
    }

    
    grePath = getPathFromConfigFile(greConfPath, aVersions, aProperties);
    if (grePath != null) {
      return grePath;
    }

    
    grePath = getPathFromConfigDir(greConfDir, aVersions, aProperties);
    return grePath;
  }

  





  private static File getPathFromConfigFile(String aFileName,
          GREVersionRange[] aVersions, Properties aProperties) {
    INIParser parser;
    try {
      parser = new INIParser(aFileName);
    } catch (Exception e) {
      
      return null;
    }

    Iterator sectionsIter = parser.getSections();
    while (sectionsIter.hasNext()) {
      
      String section = (String) sectionsIter.next();

      
      
      if (!checkVersion(section, aVersions)) {
        continue;
      }

      
      if (aProperties != null) {
        boolean ok = true;
        Enumeration e = aProperties.propertyNames();
        while (ok && e.hasMoreElements()) {
          String prop = (String) e.nextElement();
          String greValue = parser.getString(section, prop);
          if (greValue == null) {
            
            ok = false;
          } else  {
            
            
            if (!greValue.equals(aProperties.getProperty(prop))) {
              ok = false;
            }
          }
        }
        if (!ok) {
          continue;
        }
      }

      String pathStr = parser.getString(section, "GRE_PATH");
      if (pathStr != null) {
        File grePath = new File(pathStr);
        if (grePath.exists()) {
          File xpcomLib = new File(grePath, "libxpcom.so");
          if (xpcomLib.canRead()) {
            
            return grePath;
          }
        }
      }
    }

    return null;
  }

  





  private static File getPathFromConfigDir(String aDirName,
          GREVersionRange[] aVersions, Properties aProperties) {
    





    File dir = new File(aDirName);
    if (!dir.isDirectory()) {
      return null;
    }

    File grePath = null;
    File[] files = dir.listFiles();
    for (int i = 0; i < files.length && grePath == null; i++) {
      
      if (!files[i].getName().endsWith(".conf")) {
        continue;
      }

      grePath = getPathFromConfigFile(files[i].getPath(), aVersions,
                                      aProperties);
    }

    return grePath;
  }

  




  private static boolean checkVersion(String aVersionToCheck,
                                      GREVersionRange[] aVersions) {
    for (int i = 0; i < aVersions.length; i++) {
      if (aVersions[i].check(aVersionToCheck)) {
        return true;
      }
    }
    return false;
  }

  









  public void initialize(File aLibXULDirectory)
  throws XPCOMInitializationException {
    File jar = new File(aLibXULDirectory, JAVAXPCOM_JAR);
    if (!jar.exists()) {
      throw new XPCOMInitializationException("Could not find " + JAVAXPCOM_JAR +
          " in " + aLibXULDirectory);
    }

    URL[] urls = new URL[1];
    try {
      urls[0] = jar.toURL();
    } catch (MalformedURLException e) {
      throw new XPCOMInitializationException(e);
    }
    ClassLoader loader = new URLClassLoader(urls,
            this.getClass().getClassLoader());

    try {
      Class mozillaClass = Class.forName("org.mozilla.xpcom.internal.MozillaImpl",
          true, loader);
      mozilla  = (IMozilla) mozillaClass.newInstance();

      Class greClass = Class.forName("org.mozilla.xpcom.internal.GREImpl",
          true, loader);
      gre = (IGRE) greClass.newInstance();

      Class xpcomClass = Class.forName("org.mozilla.xpcom.internal.XPCOMImpl",
                                       true, loader);
      xpcom = (IXPCOM) xpcomClass.newInstance();

      Class javaXPCOMClass =
    	  Class.forName("org.mozilla.xpcom.internal.JavaXPCOMMethods",
    			  true, loader);
      jxutils  = (IJavaXPCOMUtils) javaXPCOMClass.newInstance();
    } catch (Exception e) {
      throw new XPCOMInitializationException("Could not load " +
          "org.mozilla.xpcom.internal.* classes", e);
    }
    
    mozilla.initialize(aLibXULDirectory);
  }

  





















  public void initEmbedding(File aLibXULDirectory, File aAppDirectory,
          IAppFileLocProvider aAppDirProvider) throws XPCOMException {
    try {
      gre.initEmbedding(aLibXULDirectory, aAppDirectory, aAppDirProvider);
    } catch (NullPointerException e) {
      throw new XPCOMInitializationException("Must call " +
          "Mozilla.getInstance().initialize() before using this method", e);
    }
  }

  








  public void termEmbedding() {
    try {
      gre.termEmbedding();
    } catch (NullPointerException e) {
      throw new XPCOMInitializationException("Must call " +
          "Mozilla.getInstance().initialize() before using this method", e);
    } finally {
      mozilla = null;
      gre = null;
      xpcom = null;
    }
  }

  














  public ProfileLock lockProfileDirectory(File aDirectory)
  throws XPCOMException {
	  try {
     return gre.lockProfileDirectory(aDirectory);
    } catch (NullPointerException e) {
      throw new XPCOMInitializationException("Must call " +
          "Mozilla.getInstance().initialize() before using this method", e);      
    }
  }

  





































  public void notifyProfile() {
    try {
      gre.notifyProfile();
    } catch (NullPointerException e) {
      throw new XPCOMInitializationException("Must call " +
          "Mozilla.getInstance().initialize() before using this method", e);
    }
  }

  
























  public nsIServiceManager initXPCOM(File aMozBinDirectory,
          IAppFileLocProvider aAppFileLocProvider) throws XPCOMException {
    try {
      return xpcom.initXPCOM(aMozBinDirectory, aAppFileLocProvider);
    } catch (NullPointerException e) {
      throw new XPCOMInitializationException("Must call " +
          "Mozilla.getInstance().initialize() before using this method", e);
    }
  }

  










  public void shutdownXPCOM(nsIServiceManager aServMgr) throws XPCOMException {
    try {
      xpcom.shutdownXPCOM(aServMgr);
    } catch (NullPointerException e) {
      throw new XPCOMInitializationException("Must call " +
          "Mozilla.getInstance().initialize() before using this method", e);
    } finally {
      mozilla = null;
      gre = null;
      xpcom = null;
    }
  }

  








  public nsIServiceManager getServiceManager() throws XPCOMException {
    try {
      return xpcom.getServiceManager();
    } catch (NullPointerException e) {
      throw new XPCOMInitializationException("Must call " +
          "Mozilla.getInstance().initialize() before using this method", e);
    }
  }

  








  public nsIComponentManager getComponentManager() throws XPCOMException {
    try {
      return xpcom.getComponentManager();
    } catch (NullPointerException e) {
      throw new XPCOMInitializationException("Must call " +
          "Mozilla.getInstance().initialize() before using this method", e);
    }
  }

  








  public nsIComponentRegistrar getComponentRegistrar() throws XPCOMException {
    try {
      return xpcom.getComponentRegistrar();
    } catch (NullPointerException e) {
      throw new XPCOMInitializationException("Must call " +
          "Mozilla.getInstance().initialize() before using this method", e);
    }
  }

  



















  public nsILocalFile newLocalFile(String aPath, boolean aFollowLinks)
          throws XPCOMException {
    try {
      return xpcom.newLocalFile(aPath, aFollowLinks);
    } catch (NullPointerException e) {
      throw new XPCOMInitializationException("Must call " +
          "Mozilla.getInstance().initialize() before using this method", e);
    }
  }

  


















  public static nsISupports queryInterface(nsISupports aObject, String aIID) {
    ArrayList classes = new ArrayList();
    classes.add(aObject.getClass());

    while (!classes.isEmpty()) {
      Class clazz = (Class) classes.remove(0);

      
      String className = clazz.getName();
      if (className.startsWith("java.") || className.startsWith("javax.")) {
        continue;
      }

      
      
      if (clazz.isInterface() && className.startsWith("org.mozilla")) {
        String iid = Mozilla.getInterfaceIID(clazz);
        if (iid != null && aIID.equals(iid)) {
          return aObject;
        }
      }

      
      Class[] interfaces = clazz.getInterfaces();
      for (int i = 0; i < interfaces.length; i++ ) {
        classes.add(interfaces[i]);
      }

      
      Class superclass = clazz.getSuperclass();
      if (superclass != null) {
        classes.add(superclass);
      }
    }

    return null;
  }

  







  public static String getInterfaceIID(Class aInterface) {
    
    StringBuffer iidName = new StringBuffer();
    String fullClassName = aInterface.getName();
    int index = fullClassName.lastIndexOf(".");
    String className = index > 0 ? fullClassName.substring(index + 1)
                                 : fullClassName;

    
    if (className.startsWith("ns")) {
      iidName.append("NS_");
      iidName.append(className.substring(2).toUpperCase());
    } else {
      iidName.append(className.toUpperCase());
    }
    iidName.append("_IID");

    String iid;
    try {
      Field iidField = aInterface.getDeclaredField(iidName.toString());
      iid = (String) iidField.get(null);
    } catch (NoSuchFieldException e) {
      
      
      iid = null;
    } catch (IllegalAccessException e) {
      
      
      System.err.println("ERROR: Could not get field " + iidName.toString());
      iid = null;
    }

    return iid;
  }

  public long getNativeHandleFromAWT(Object widget) {
    try {
      return mozilla.getNativeHandleFromAWT(widget);
    } catch (NullPointerException e) {
      throw new XPCOMInitializationException("Must call " +
          "Mozilla.getInstance().initialize() before using this method", e);
    }
  }

	public long wrapJavaObject(Object aJavaObject, String aIID) {
		try {
			return jxutils.wrapJavaObject(aJavaObject, aIID);
		} catch (NullPointerException e) {
			throw new XPCOMInitializationException("Must call " +
					"Mozilla.getInstance().initialize() before using this method", e);
		}
	}

	public Object wrapXPCOMObject(long aXPCOMObject, String aIID) {
		try {
			return jxutils.wrapXPCOMObject(aXPCOMObject, aIID);
		} catch (NullPointerException e) {
			throw new XPCOMInitializationException("Must call " +
					"Mozilla.getInstance().initialize() before using this method", e);
		}
	}

}
