const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

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

function write_big_datafile(status, entry)
{
  do_check_eq(status, Cr.NS_OK);
  var os = entry.openOutputStream(0);
  var data = gen_1MiB();

  
  var i;
  for (i=0 ; i<65 ; i++)
    write_and_check(os, data, data.length);

  
  try {
    write_and_check(os, data, data.length);
    do_throw("write should fail");
  } catch (e) {}

  os.close();
  entry.close();

  
  
  
  
  syncWithCacheIOThread(run_test_2);
}

function write_big_metafile(status, entry)
{
  do_check_eq(status, Cr.NS_OK);
  var os = entry.openOutputStream(0);
  var data = gen_1MiB();

  
  var i;
  for (i=0 ; i<65 ; i++)
    entry.setMetaDataElement("metadata_"+i, data);

  os.close();
  entry.close();

  
  
  
  asyncOpenCacheEntry("smalldata",
                      "HTTP",
                      Ci.nsICache.STORE_ON_DISK,
                      Ci.nsICache.ACCESS_WRITE,
                      write_and_doom_small_datafile);
}

function write_and_doom_small_datafile(status, entry)
{
  do_check_eq(status, Cr.NS_OK);
  var os = entry.openOutputStream(0);
  var data = "0123456789";

  write_and_check(os, data, data.length);

  os.close();
  entry.doom();
  entry.close();
  syncWithCacheIOThread(run_test_3);
}

function check_cache_size() {
  var diskDeviceVisited;
  var visitor = {
    visitDevice: function (deviceID, deviceInfo) {
      if (deviceID == "disk") {
        diskDeviceVisited = true;
        do_check_eq(deviceInfo.totalSize, 0)
      }
      return true;
    },
    visitEntry: function (deviceID, entryInfo) {
      do_throw("unexpected call to visitEntry");
    }
  };

  var cs = get_cache_service();
  diskDeviceVisited = false;
  cs.visitEntries(visitor);
  do_check_true(diskDeviceVisited);
}

function run_test() {
  var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);

  
  prefBranch.setIntPref("browser.cache.disk.max_entry_size", 65*1024);
  
  prefBranch.setIntPref("browser.cache.disk.capacity", 8*65*1024);
  
  prefBranch.setBoolPref("browser.cache.disk.smart_size.enabled", false);

  do_get_profile();

  
  evict_cache_entries();

  
  asyncOpenCacheEntry("bigdata",
                      "HTTP",
                      Ci.nsICache.STORE_ON_DISK,
                      Ci.nsICache.ACCESS_WRITE,
                      write_big_datafile);

  do_test_pending();
}

function run_test_2()
{
  check_cache_size();

  var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);

  
  
  prefBranch.setIntPref("browser.cache.disk.capacity", 64*1024);

  
  asyncOpenCacheEntry("bigmetadata",
                      "HTTP",
                      Ci.nsICache.STORE_ON_DISK,
                      Ci.nsICache.ACCESS_WRITE,
                      write_big_metafile);
}

function run_test_3()
{
  check_cache_size();

  do_test_finished();
}
