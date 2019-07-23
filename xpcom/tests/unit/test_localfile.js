





































const Cr = Components.results;
const CC = Components.Constructor;

var LocalFile = CC("@mozilla.org/file/local;1", "nsILocalFile", "initWithPath");

function run_test()
{
  test_toplevel_parent_is_null();
  test_normalize_crash_if_media_missing();
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

