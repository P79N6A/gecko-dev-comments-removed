





































const Cr = Components.results;
const CC = Components.Constructor;

var LocalFile = CC("@mozilla.org/file/local;1", "nsILocalFile", "initWithPath");

function run_test()
{
  test_toplevel_parent_is_null();
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
