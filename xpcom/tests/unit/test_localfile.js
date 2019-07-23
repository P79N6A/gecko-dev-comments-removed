





































const Cr = Components.results;
const CC = Components.Constructor;
const Ci = Components.interfaces;

const MAX_TIME_DIFFERENCE = 2500;
const MILLIS_PER_DAY      = 1000 * 60 * 60 * 24;

var LocalFile = CC("@mozilla.org/file/local;1", "nsILocalFile", "initWithPath");

function run_test()
{
  test_toplevel_parent_is_null();
  test_normalize_crash_if_media_missing();
  test_file_modification_time();
  test_directory_modification_time();
}

function test_toplevel_parent_is_null()
{
  try
  {
    var lf = new LocalFile("C:\\");

    
    
    do_check_true(lf.path.length == 2);

    do_check_true(lf.parent === null);
  }
  catch (e)
  {
    
    do_check_eq(e.result, Cr.NS_ERROR_FILE_UNRECOGNIZED_PATH);
  }
}

function test_normalize_crash_if_media_missing()
{
  const a="a".charCodeAt(0);
  const z="z".charCodeAt(0);
  for (var i = a; i <= z; ++i)
  {
    try
    {
      LocalFile(String.fromCharCode(i)+":.\\test").normalize();
    }
    catch (e)
    {
    }
  }
}


function test_file_modification_time()
{
  var file = do_get_profile();
  file.append("testfile");

  
  if (file.exists())
    file.remove(true);

  var now = Date.now();
  file.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0644);
  do_check_true(file.exists());

  
  
  var diff = Math.abs(file.lastModifiedTime - now);
  do_check_true(diff < MAX_TIME_DIFFERENCE);

  var yesterday = now - MILLIS_PER_DAY;
  file.lastModifiedTime = yesterday;

  diff = Math.abs(file.lastModifiedTime - yesterday);
  do_check_true(diff < MAX_TIME_DIFFERENCE);

  var tomorrow = now - MILLIS_PER_DAY;
  file.lastModifiedTime = tomorrow;

  diff = Math.abs(file.lastModifiedTime - tomorrow);
  do_check_true(diff < MAX_TIME_DIFFERENCE);

  file.remove(true);
}


function test_directory_modification_time()
{
  var dir = do_get_profile();
  dir.append("testdir");

  
  if (dir.exists())
    dir.remove(true);

  var now = Date.now();
  dir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  do_check_true(dir.exists());

  
  
  var diff = Math.abs(dir.lastModifiedTime - now);
  do_check_true(diff < MAX_TIME_DIFFERENCE);

  var yesterday = now - MILLIS_PER_DAY;
  dir.lastModifiedTime = yesterday;

  diff = Math.abs(dir.lastModifiedTime - yesterday);
  do_check_true(diff < MAX_TIME_DIFFERENCE);

  var tomorrow = now - MILLIS_PER_DAY;
  dir.lastModifiedTime = tomorrow;

  diff = Math.abs(dir.lastModifiedTime - tomorrow);
  do_check_true(diff < MAX_TIME_DIFFERENCE);

  dir.remove(true);
}

