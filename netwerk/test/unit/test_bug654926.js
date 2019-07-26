var _PSvc;
function get_pref_service() {
  if (_PSvc)
    return _PSvc;

  return _PSvc = Cc["@mozilla.org/preferences-service;1"].
                 getService(Ci.nsIPrefBranch);
}

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

function write_datafile(status, entry)
{
  do_check_eq(status, Cr.NS_OK);
  var os = entry.openOutputStream(0);
  var data = gen_1MiB();

  
  var i;
  for (i=0 ; i<2 ; i++)
    write_and_check(os, data, data.length);

  os.close();
  entry.close();

  
  get_pref_service().setIntPref("browser.cache.disk.max_entry_size", 1024);

  
  asyncOpenCacheEntry("http://data/",
                      "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
                      append_datafile);
}

function append_datafile(status, entry)
{
  do_check_eq(status, Cr.NS_OK);
  var os = entry.openOutputStream(entry.dataSize);
  var data = gen_1MiB();

  
  try {
    write_and_check(os, data, data.length);
    do_throw();
  }
  catch (ex) { }

  
  try {
    os.close();
    do_throw();
  }
  catch (ex) { }

  entry.close();

  do_test_finished();
}

function run_test() {
  if (newCacheBackEndUsed()) {
    
    do_check_true(true, "This test doesn't run with the new cache backend, the test or the cache needs to be fixed");
    return;
  }

  do_get_profile();

  
  evict_cache_entries();

  asyncOpenCacheEntry("http://data/",
                      "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
                      write_datafile);

  do_test_pending();
}
