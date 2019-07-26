



package org.mozilla.gecko.background.healthreport;

import java.util.ArrayList;















public abstract class Environment {
  public static int VERSION = 1;

  protected volatile String hash = null;
  protected volatile int id = -1;

  
  public int profileCreation;

  
  public int cpuCount;
  public int memoryMB;
  public String architecture;
  public String sysName;
  public String sysVersion;      

  
  public String vendor;
  public String appName;
  public String appID;
  public String appVersion;
  public String appBuildID;
  public String platformVersion;
  public String platformBuildID;
  public String os;
  public String xpcomabi;
  public String updateChannel;

  
  public int isBlocklistEnabled;
  public int isTelemetryEnabled;
  

  
  public final ArrayList<String> addons = new ArrayList<String>();

  
  public int extensionCount;
  public int pluginCount;
  public int themeCount;

  public String getHash() {
    
    if (hash != null) {
      return hash;
    }

    StringBuilder b = new StringBuilder();
    b.append(profileCreation);
    b.append(cpuCount);
    b.append(memoryMB);
    b.append(architecture);
    b.append(sysName);
    b.append(sysVersion);
    b.append(vendor);
    b.append(appName);
    b.append(appID);
    b.append(appVersion);
    b.append(appBuildID);
    b.append(platformVersion);
    b.append(platformBuildID);
    b.append(os);
    b.append(xpcomabi);
    b.append(updateChannel);
    b.append(isBlocklistEnabled);
    b.append(isTelemetryEnabled);
    b.append(extensionCount);
    b.append(pluginCount);
    b.append(themeCount);

    for (String addon : addons) {
      b.append(addon);
    }

    return hash = HealthReportUtils.getEnvironmentHash(b.toString());
  }

  








  public abstract int register();
}
