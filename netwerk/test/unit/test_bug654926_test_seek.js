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

function write_datafile(status, entry)
{
  do_check_eq(status, Cr.NS_OK);
  var os = entry.openOutputStream(0);
  var data = gen_1MiB();

  write_and_check(os, data, data.length);

  os.close();
  entry.close();

  
  asyncOpenCacheEntry("data",
                      "HTTP",
                      Ci.nsICache.STORE_ON_DISK,
                      Ci.nsICache.ACCESS_READ_WRITE,
                      open_for_readwrite);
}

function open_for_readwrite(status, entry)
{
  do_check_eq(status, Cr.NS_OK);
  var os = entry.openOutputStream(entry.dataSize);

  
  
  

  os.close();
  entry.close();

  do_test_finished();
}

function run_test() {
  do_get_profile();

  
  evict_cache_entries();

  asyncOpenCacheEntry("data",
                      "HTTP",
                      Ci.nsICache.STORE_ON_DISK,
                      Ci.nsICache.ACCESS_WRITE,
                      write_datafile);

  do_test_pending();
}
