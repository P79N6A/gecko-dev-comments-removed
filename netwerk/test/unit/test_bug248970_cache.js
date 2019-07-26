




const kDiskDevice = "disk";
const kMemoryDevice = "memory";
const kOfflineDevice = "appcache";

const kCacheA = "http://cache/A";
const kCacheA2 = "http://cache/A2";
const kCacheB = "http://cache/B";
const kCacheC = "http://cache/C";
const kTestContent = "test content";

function make_input_stream_scriptable(input) {
  var wrapper = Cc["@mozilla.org/scriptableinputstream;1"].
                createInstance(Ci.nsIScriptableInputStream);
  wrapper.init(input);
  return wrapper;
}

const entries = [

  [kCacheA,  kTestContent, kMemoryDevice,  true],
  [kCacheA2, kTestContent, kDiskDevice,    false],
  [kCacheB,  kTestContent, kDiskDevice,    true],
  [kCacheC,  kTestContent, kOfflineDevice, true]
]

var store_idx;
var store_cb = null;
var appCache = null;

function store_entries(cb)
{
  if (cb) {
    store_cb = cb;
    store_idx = 0;
  }

  if (store_idx == entries.length) {
    do_execute_soon(store_cb);
    return;
  }

  asyncOpenCacheEntry(entries[store_idx][0],
                      entries[store_idx][2],
                      Ci.nsICacheStorage.OPEN_TRUNCATE,
                      LoadContextInfo.custom(!entries[store_idx][3]),
                      store_data,
                      appCache);
}

var store_data = function(status, entry) {
  do_check_eq(status, Cr.NS_OK);
  var os = entry.openOutputStream(0);

  var written = os.write(entries[store_idx][1], entries[store_idx][1].length);
  if (written != entries[store_idx][1].length) {
    do_throw("os.write has not written all data!\n" +
             "  Expected: " + entries[store_idx][1].length  + "\n" +
             "  Actual: " + written + "\n");
  }
  os.close();
  entry.close();
  store_idx++;
  do_execute_soon(store_entries);
};

var check_idx;
var check_cb = null;
var check_pb_exited;
function check_entries(cb, pbExited)
{
  if (cb) {
    check_cb = cb;
    check_idx = 0;
    check_pb_exited = pbExited;
  }

  if (check_idx == entries.length) {
    do_execute_soon(check_cb);
    return;
  }

  asyncOpenCacheEntry(entries[check_idx][0],
                      entries[check_idx][2],
                      Ci.nsICacheStorage.OPEN_READONLY,
                      LoadContextInfo.custom(!entries[check_idx][3]),
                      check_data,
                      appCache);
}

var check_data = function (status, entry) {
  var cont = function() {
    check_idx++;
    do_execute_soon(check_entries);
  }

  if (!check_pb_exited || entries[check_idx][3]) {
    do_check_eq(status, Cr.NS_OK);
    var is = entry.openInputStream(0);
    pumpReadStream(is, function(read) {
      entry.close();
      do_check_eq(read, entries[check_idx][1]);
      cont();
    });
  } else {
    do_check_eq(status, Cr.NS_ERROR_CACHE_KEY_NOT_FOUND);
    cont();
  }
};

function run_test() {
  
  do_get_profile();

  appCache = Cc["@mozilla.org/network/application-cache-service;1"].
             getService(Ci.nsIApplicationCacheService).
             getApplicationCache("fake-client-id|fake-group-id");

  
  evict_cache_entries();

  
  store_entries(run_test2);

  do_test_pending();
}

function run_test2() {
  
  check_entries(run_test3, false);
}

function run_test3() {
  
  var obsvc = Cc["@mozilla.org/observer-service;1"].
    getService(Ci.nsIObserverService);
  obsvc.notifyObservers(null, "last-pb-context-exited", null);

  
  get_device_entry_count(kMemoryDevice, null, function(count) {
    do_check_eq(count, 1);
    
    check_entries(do_test_finished, true);
  });
}
