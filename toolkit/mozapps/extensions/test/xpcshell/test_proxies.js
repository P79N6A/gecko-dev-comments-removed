





var ADDONS = [
  {
    id: "proxy1@tests.mozilla.org",
    dirId: "proxy1@tests.mozilla.com",
    type: "proxy"
  },
  {
    id: "proxy2@tests.mozilla.org",
    type: "proxy"
  },
  {
    id: "symlink1@tests.mozilla.org",
    dirId: "symlink1@tests.mozilla.com",
    type: "symlink"
  },
  {
    id: "symlink2@tests.mozilla.org",
    type: "symlink"
  }
];

var METADATA = {
  version: "2.0",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "2",
    maxVersion: "2"
  }]
}

const ios = AM_Cc["@mozilla.org/network/io-service;1"].getService(AM_Ci.nsIIOService);

const LocalFile = Components.Constructor("@mozilla.org/file/local;1",
                                         "nsILocalFile", "initWithPath");
const Process = Components.Constructor("@mozilla.org/process/util;1",
                                       "nsIProcess", "init");

const gHaveSymlinks = !("nsIWindowsRegKey" in AM_Ci);
if (gHaveSymlinks)
  var gLink = findExecutable("ln");




function createSymlink(aSource, aDest) {
  if (aSource instanceof AM_Ci.nsIFile)
    aSource = aSource.path;

  let process = Process(gLink);
  process.run(true, ["-s", aSource, aDest.path], 3);
  do_check_eq(process.exitValue, 0, Components.stack.caller);
  return process.exitValue;
}

function findExecutable(aName) {
  if (environment.PATH) {
    for each (let path in environment.PATH.split(":")) {
      try {
        let file = LocalFile(path);
        file.append(aName);

        if (file.exists() && file.isFile() && file.isExecutable())
          return file;
      }
      catch (e) {}
    }
  }
  return null;
}

function writeFile(aData, aFile) {
  if (!aFile.parent.exists())
    aFile.parent.create(AM_Ci.nsIFile.DIRECTORY_TYPE, 0755);

  var fos = AM_Cc["@mozilla.org/network/file-output-stream;1"].
            createInstance(AM_Ci.nsIFileOutputStream);
  fos.init(aFile,
           FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE | FileUtils.MODE_TRUNCATE,
           FileUtils.PERMS_FILE, 0);
  fos.write(aData, aData.length);
  fos.close();
}

function checkAddonsExist() {
  for each (let addon in ADDONS) {
    let file = addon.directory.clone();
    file.append("install.rdf");
    do_check_true(file.exists(), Components.stack.caller);
  }
}


const profileDir = gProfD.clone();
profileDir.append("extensions");


function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2", "2");

  
  do_check_true(!gHaveSymlinks || gLink != null);

  add_test(run_proxy_tests);

  if (gHaveSymlinks)
    add_test(run_symlink_tests);

  run_next_test();
}

function run_proxy_tests() {

  for each (let addon in ADDONS) {
    addon.directory = gTmpD.clone();
    addon.directory.append(addon.id);

    addon.proxyFile = profileDir.clone();
    addon.proxyFile.append(addon.dirId || addon.id);

    METADATA.id = addon.id;
    METADATA.name = addon.id;
    writeInstallRDFToDir(METADATA, addon.directory);

    if (addon.type == "proxy") {
      writeFile(addon.directory.path, addon.proxyFile)
    }
    else if (addon.type == "symlink" && gHaveSymlinks) {
      createSymlink(addon.directory, addon.proxyFile)
    }
  }

  startupManager();

  
  
  checkAddonsExist();

  AddonManager.getAddonsByIDs(ADDONS.map(function(addon) addon.id),
                              function(addons) {
    try {
      for (let [i, addon] in Iterator(addons)) {
        
        
        print(ADDONS[i].id, 
              ADDONS[i].dirId,
              ADDONS[i].dirId != null,
              ADDONS[i].type == "symlink" && !gHaveSymlinks);
        do_check_eq(addon == null,
                    ADDONS[i].dirId != null
                        || ADDONS[i].type == "symlink" && !gHaveSymlinks);

        if (addon != null) {
          
          do_check_eq(addon.permissions & AddonManager.PERM_CAN_UPGRADE, 0);

          
          do_check_eq(ios.newFileURI(ADDONS[i].directory).spec,
                      addon.getResourceURI().spec);

          let file = ADDONS[i].directory.clone();
          file.append("install.rdf");
          do_check_eq(ios.newFileURI(file).spec,
                      addon.getResourceURI("install.rdf").spec);

          addon.uninstall();
        }
      }

      
      restartManager();
      checkAddonsExist();

      shutdownManager();

      
      
      for each (let addon in ADDONS) {
        do_check_false(addon.proxyFile.exists());
        addon.directory.remove(true);
      }
    }
    catch (e) {
      do_throw(e);
    }

    run_next_test();
  });
}

function run_symlink_tests() {
  
  

  METADATA.id = "unpacked@test.mozilla.org";
  METADATA.name = METADATA.id;
  METADATA.unpack = "true";

  let tempDirectory = gTmpD.clone();
  tempDirectory.append(METADATA.id);

  let tempFile = tempDirectory.clone();
  tempFile.append("test.txt");
  tempFile.create(AM_Ci.nsIFile.NORMAL_FILE_TYPE, 0644);

  let addonDirectory = profileDir.clone();
  addonDirectory.append(METADATA.id);

  let symlink = addonDirectory.clone();
  symlink.append(tempDirectory.leafname);
  createSymlink(tempDirectory, symlink);

  
  let file = symlink.clone();
  file.append(tempFile.leafName);
  file.normalize();
  do_check_true(file.equals(tempFile));

  writeInstallRDFToDir(METADATA, addonDirectory);

  startupManager();

  AddonManager.getAddonByID(METADATA.id,
                            function(addon) {
    do_check_neq(addon, null);

    addon.uninstall();

    restartManager();
    shutdownManager();

    
    do_check_false(addonDirectory.exists());

    
    do_check_true(tempFile.exists());

    tempDirectory.remove(true);

    run_next_test();
  });
}

