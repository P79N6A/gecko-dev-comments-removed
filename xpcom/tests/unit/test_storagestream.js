





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

function run_test()
{
  test1();
  test2();
  test3();
  test4();
}





function test1()
{
  var ss = Cc["@mozilla.org/storagestream;1"]
             .createInstance(Ci.nsIStorageStream);
  ss.init(1024, 1024, null);

  var out = ss.getOutputStream(0);
  try
  {
    var threw = false;
    var inp2 = ss.newInputStream(0);
  }
  catch (e)
  {
    threw = e.result == Cr.NS_ERROR_NOT_INITIALIZED;
  }
  if (!threw)
    do_throw("must write to a storagestream before creating an " +
             "inputstream for it, even if just .write('', 0)");
}





function test2()
{
  var ss = Cc["@mozilla.org/storagestream;1"]
             .createInstance(Ci.nsIStorageStream);
  ss.init(1024, 1024, null);

  var out = ss.getOutputStream(0);
  out.write("", 0);
  try
  {
    var inp2 = ss.newInputStream(0);
  }
  catch (e)
  {
    do_throw("shouldn't throw exception when new input stream created");
  }
}





function test3()
{
  var ss = Cc["@mozilla.org/storagestream;1"]
             .createInstance(Ci.nsIStorageStream);
  ss.init(1024, 1024, null);

  var out = ss.getOutputStream(0);
  out.write("", 0);
  try
  {
    var inp = ss.newInputStream(0);
  }
  catch (e)
  {
    do_throw("newInputStream(0) shouldn't throw if write() is called: " + e);
  }

  do_check_true(inp.isNonBlocking(), "next test expects a non-blocking stream");

  try
  {
    var threw = false;
    var bis = BIS(inp);
    var dummy = bis.readByteArray(5);
  }
  catch (e)
  {
    if (e.result != Cr.NS_BASE_STREAM_WOULD_BLOCK)
      do_throw("wrong error thrown: " + e);
    threw = true;
  }
  do_check_true(threw,
                "should have thrown (nsStorageInputStream is nonblocking)");
}





function test4()
{
  var bytes = [65, 66, 67, 68, 69, 70, 71, 72, 73, 74];

  var ss = Cc["@mozilla.org/storagestream;1"]
             .createInstance(Ci.nsIStorageStream);
  ss.init(1024, 1024, null);

  var outStream = ss.getOutputStream(0);

  var bos = Cc["@mozilla.org/binaryoutputstream;1"]
              .createInstance(Ci.nsIBinaryOutputStream);
  bos.setOutputStream(outStream);

  bos.writeByteArray(bytes, bytes.length);
  bos.close();

  var inp = ss.newInputStream(0);
  var bis = BIS(inp);

  var count = 0;
  while (count < bytes.length)
  {
    var data = bis.read8(1);
    do_check_eq(data, bytes[count++]);
  }

  var threw = false;
  try
  {
    data = bis.read8(1);
  }
  catch (e)
  {
    if (e.result != Cr.NS_ERROR_FAILURE)
      do_throw("wrong error thrown: " + e);
    threw = true;
  }
  if (!threw)
    do_throw("should have thrown but instead returned: '" + data + "'");
}


function BIS(input)
{
  var bis = Cc["@mozilla.org/binaryinputstream;1"]
              .createInstance(Ci.nsIBinaryInputStream);
  bis.setInputStream(input);
  return bis;
}
