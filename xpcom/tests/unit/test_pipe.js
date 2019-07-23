





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

function run_test()
{
  test_not_initialized();
}

function test_not_initialized()
{
  var p = Cc["@mozilla.org/pipe;1"]
            .createInstance(Ci.nsIPipe);
  try
  {
    var dummy = p.outputStream;
    throw Cr.NS_ERROR_FAILURE;
  }
  catch (e)
  {
    if (e.result != Cr.NS_ERROR_NOT_INITIALIZED)
      do_throw("using a pipe before initializing it should throw NS_ERROR_NOT_INITIALIZED");
  }
}
