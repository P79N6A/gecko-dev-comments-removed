








function run_test() {
  if (!IS_AUTHENTICODE_CHECK_ENABLED) {
    return;
  }

  let binDir = getGREBinDir();
  let maintenanceServiceBin = binDir.clone();
  maintenanceServiceBin.append(FILE_MAINTENANCE_SERVICE_BIN);

  let updaterBin = binDir.clone();
  updaterBin.append(FILE_UPDATER_BIN);

  debugDump("Launching maintenance service bin: " +
            maintenanceServiceBin.path + " to check updater: " +
            updaterBin.path + " signature.");

  
  let env = Cc["@mozilla.org/process/environment;1"].
            getService(Ci.nsIEnvironment);
  env.set("__COMPAT_LAYER", "RunAsInvoker");

  let dummyInstallPath = "---";
  let maintenanceServiceBinArgs = ["check-cert", dummyInstallPath,
                                   updaterBin.path];
  let maintenanceServiceBinProcess = Cc["@mozilla.org/process/util;1"].
                                     createInstance(Ci.nsIProcess);
  maintenanceServiceBinProcess.init(maintenanceServiceBin);
  maintenanceServiceBinProcess.run(true, maintenanceServiceBinArgs,
                                   maintenanceServiceBinArgs.length);
  Assert.equal(maintenanceServiceBinProcess.exitValue, 0,
               "the maintenance service exit value should be 0");
}
