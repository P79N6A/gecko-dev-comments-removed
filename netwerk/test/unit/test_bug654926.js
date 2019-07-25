const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

var _CSvc;
function get_cache_service() {
  if (_CSvc)
    return _CSvc;

  return _CSvc = Cc["@mozilla.org/network/cache-service;1"].
                 getService(Ci.nsICacheService);
}

function get_ostream_for_entry(key, asFile, append, entryRef)
{
  var cache = get_cache_service();
  var session = cache.createSession(
                  "HTTP",
                  asFile ? Ci.nsICache.STORE_ON_DISK_AS_FILE
                         : Ci.nsICache.STORE_ON_DISK,
                  Ci.nsICache.STREAM_BASED);
  var cacheEntry = session.openCacheEntry(
                     key,
                     append ? Ci.nsICache.ACCESS_READ_WRITE
                            : Ci.nsICache.ACCESS_WRITE,
                     true);
  var oStream = cacheEntry.openOutputStream(append ? cacheEntry.dataSize : 0);
  entryRef.value = cacheEntry;
  return oStream;
}

function sync_with_cache_IO_thread(aFunc)
{
  do_check_eq(sync_with_cache_IO_thread_cb.listener, null);
  sync_with_cache_IO_thread_cb.listener = aFunc;
  var cache = get_cache_service();
  var session = cache.createSession(
                  "HTTP",
                  Ci.nsICache.STORE_ON_DISK,
                  Ci.nsICache.STREAM_BASED);
  var cacheEntry = session.asyncOpenCacheEntry(
                     "nonexistententry",
                     Ci.nsICache.ACCESS_READ,
                     sync_with_cache_IO_thread_cb);
}
var sync_with_cache_IO_thread_cb = {
  listener: null,

  onCacheEntryAvailable: function oCEA(descriptor, accessGranted, status) {
    do_check_eq(status, Cr.NS_ERROR_CACHE_KEY_NOT_FOUND);
    cb = this.listener;
    this.listener = null;
    do_timeout(0, cb);
  }
};

function gen_1MiB()
{
  var i;
  var data="x";
  for (i=0 ; i < 20 ; i++)
    data+=data;
  return data;
}

function write_and_check(str, data, len)
{
  var written = str.write(data, len);
  if (written != len) {
    do_throw("str.write has not written all data!\n" +
             "  Expected: " + len  + "\n" +
             "  Actual: " + written + "\n");
  }
}

function write_datafile()
{
  var entry = {};
  var oStr = get_ostream_for_entry("data", true, false, entry);
  var data = gen_1MiB();

  
  var i;
  for (i=0 ; i<6 ; i++)
    write_and_check(oStr, data, data.length);

  oStr.close();
  entry.value.close();

  
  sync_with_cache_IO_thread(append_datafile);
}

function append_datafile()
{
  var entry = {};
  var oStr = get_ostream_for_entry("data", false, true, entry);
  var data = gen_1MiB();


  
  try {
    write_and_check(oStr, data, data.length);
    do_throw();
  }
  catch (ex) { }

  
  try {
    oStr.close();
    do_throw();
  }
  catch (ex) { }

  entry.value.close();
  
  
  sync_with_cache_IO_thread(do_test_finished);
}

function run_test() {
  do_get_profile();

  
  get_cache_service().evictEntries(Ci.nsICache.STORE_ANYWHERE);

  
  sync_with_cache_IO_thread(write_datafile);

  do_test_pending();
}
