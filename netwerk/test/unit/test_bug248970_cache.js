




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;


const kDiskDevice = "disk";
const kMemoryDevice = "memory";
const kOfflineDevice = "offline";


const kPrivateBrowsing = "PrivateBrowsing";

var _PBSvc = null;
function get_privatebrowsing_service() {
  if (_PBSvc)
    return _PBSvc;

  try {
    _PBSvc = Cc["@mozilla.org/privatebrowsing;1"].
             getService(Ci.nsIPrivateBrowsingService);
    return _PBSvc;
  } catch (e) {}
  return null;
}

var _CSvc = null;
function get_cache_service() {
  if (_CSvc)
    return _CSvc;

  return _CSvc = Cc["@mozilla.org/network/cache-service;1"].
                 getService(Ci.nsICacheService);
}

var profileDir;
function setup_profile_dir() {
  var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
               getService(Ci.nsIProperties);
  var leafRandomName = "Cache" + Math.floor(Math.random() * 10000);
  profileDir = dirSvc.get("TmpD", Ci.nsILocalFile);
  profileDir.append(leafRandomName);
  profileDir.createUnique(Ci.nsIFile.DIRECTORY_TYPE, 0700);
  var provider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == "ProfLD" ||
          prop == "ProfD" ||
          prop == "cachePDir")
        return profileDir;
      throw Cr.NS_ERROR_FAILURE;
    },
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsIDirectoryProvider) ||
          iid.equals(Ci.nsISupports)) {
        return this;
      }
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  };
  dirSvc.QueryInterface(Ci.nsIDirectoryService).registerProvider(provider);
}

function remove_profile_dir() {
  
  var obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
  obs.notifyObservers(null, "quit-application", "shutdown");
  obs.notifyObservers(null, "profile-change-net-teardown", "shutdown");
  obs.notifyObservers(null, "profile-change-teardown", "shutdown");
  obs.notifyObservers(null, "profile-before-change", "shutdown-cleanse");
  obs.notifyObservers(null, "xpcom-shutdown", "");

  
  profileDir.remove(true);
}

function check_devices_available(devices) {
  var cs = get_cache_service();
  var found_devices = [];

  var visitor = {
    visitDevice: function (deviceID, deviceInfo) {
      found_devices.push(deviceID);
      return false;
    },
    visitEntry: function (deviceID, entryInfo) {
      do_throw("nsICacheVisitor.visitEntry should not be called " +
        "when checking the availability of devices");
    }
  };

  
  cs.visitEntries(visitor);

  
  if (devices.sort().toString() != found_devices.sort().toString()) {
    do_throw("Expected to find these devices: \"" + devices.sort().toString() +
      "\", but found these instead: \"" + found_devices.sort().toString() + "\"");
  }

  
  if (found_devices.length > devices.length) {
    do_throw("Expected to find these devices: [" + devices.join(", ") +
      "], but instead got: [" + found_devices.join(", ") + "]");
  }
}

function get_device_entry_count(device) {
  var cs = get_cache_service();
  var entry_count = -1;

  var visitor = {
    visitDevice: function (deviceID, deviceInfo) {
      if (device == deviceID)
        entry_count = deviceInfo.entryCount;
      return false;
    },
    visitEntry: function (deviceID, entryInfo) {
      do_throw("nsICacheVisitor.visitEntry should not be called " +
        "when checking the availability of devices");
    }
  };

  
  cs.visitEntries(visitor);

  return entry_count;
}

function store_in_cache(aKey, aContent, aWhere) {
  var storageFlag, streaming = true;
  if (aWhere == kDiskDevice)
    storageFlag = Ci.nsICache.STORE_ON_DISK;
  else if (aWhere == kOfflineDevice)
    storageFlag = Ci.nsICache.STORE_OFFLINE;
  else if (aWhere == kMemoryDevice)
    storageFlag = Ci.nsICache.STORE_IN_MEMORY;

  var cache = get_cache_service();
  var session = cache.createSession(kPrivateBrowsing, storageFlag, streaming);
  var cacheEntry = session.openCacheEntry(aKey, Ci.nsICache.ACCESS_WRITE, true);

  var oStream = cacheEntry.openOutputStream(0);

  var written = oStream.write(aContent, aContent.length);
  if (written != aContent.length) {
    do_throw("oStream.write has not written all data!\n" +
             "  Expected: " + aContent.length  + "\n" +
             "  Actual: " + written + "\n");
  }
  oStream.close();
  cacheEntry.close();
}

function make_input_stream_scriptable(input) {
  var wrapper = Cc["@mozilla.org/scriptableinputstream;1"].
                createInstance(Ci.nsIScriptableInputStream);
  wrapper.init(input);
  return wrapper;
}

function retrieve_from_cache(aKey, aWhere) {
  var storageFlag, streaming = true;
  if (aWhere == kDiskDevice)
    storageFlag = Ci.nsICache.STORE_ANYWHERE;
  else if (aWhere == kOfflineDevice)
    storageFlag = Ci.nsICache.STORE_OFFLINE;
  else if (aWhere == kMemoryDevice)
    storageFlag = Ci.nsICache.STORE_ANYWHERE;

  var cache = get_cache_service();
  var session = cache.createSession(kPrivateBrowsing, storageFlag, streaming);
  try {
    var cacheEntry = session.openCacheEntry(aKey, Ci.nsICache.ACCESS_READ, true);
  } catch (e) {
    if (e.result == Cr.NS_ERROR_CACHE_KEY_NOT_FOUND ||
        e.result == Cr.NS_ERROR_FAILURE)
      
      
      
      return null;
    else
      do_throw(e); 
  }

  var iStream = make_input_stream_scriptable(cacheEntry.openInputStream(0));

  var read = iStream.read(iStream.available());
  iStream.close();
  cacheEntry.close();

  return read;
}

function run_test() {
  var pb = get_privatebrowsing_service();
  if (pb) { 
    var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);
    prefBranch.setBoolPref("browser.privatebrowsing.keep_current_session", true);

    const kCacheA = "cache-A",
          kCacheB = "cache-B",
          kCacheC = "cache-C",
          kTestContent = "test content";

    
    setup_profile_dir();

    var cs = get_cache_service();

    
    cs.evictEntries(Ci.nsICache.STORE_ANYWHERE);

    
    store_in_cache(kCacheA, kTestContent, kMemoryDevice);
    store_in_cache(kCacheB, kTestContent, kDiskDevice);
    store_in_cache(kCacheC, kTestContent, kOfflineDevice);

    
    check_devices_available([kMemoryDevice, kDiskDevice, kOfflineDevice]);

    
    do_check_eq(retrieve_from_cache(kCacheA, kMemoryDevice), kTestContent);
    do_check_eq(retrieve_from_cache(kCacheB, kDiskDevice), kTestContent);
    do_check_eq(retrieve_from_cache(kCacheC, kOfflineDevice), kTestContent);

    
    pb.privateBrowsingEnabled = true;

    
    do_check_eq(retrieve_from_cache(kCacheA, kMemoryDevice), null);
    do_check_eq(retrieve_from_cache(kCacheB, kDiskDevice), null);
    do_check_eq(retrieve_from_cache(kCacheC, kOfflineDevice), null);

    
    check_devices_available([kMemoryDevice]);

    
    do_check_eq(get_device_entry_count(kMemoryDevice), 0);

    
    pb.privateBrowsingEnabled = false;

    
    check_devices_available([kMemoryDevice, kDiskDevice, kOfflineDevice]);

    
    do_check_eq(retrieve_from_cache(kCacheA, kMemoryDevice), null);
    do_check_eq(retrieve_from_cache(kCacheB, kDiskDevice), kTestContent);
    do_check_eq(retrieve_from_cache(kCacheC, kOfflineDevice), kTestContent);

    prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
    remove_profile_dir();
  }
}
