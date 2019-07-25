









































var srv, dir, dirEntries;

function run_test()
{
  createTestDirectory();

  srv = createServer();
  srv.registerDirectory("/", dir);

  var nameDir = do_get_file("data/name-scheme/");
  srv.registerDirectory("/bar/", nameDir);

  srv.start(4444);

  function done()
  {
    do_test_pending();
    destroyTestDirectory();
    srv.stop(function() { do_test_finished(); });
  }

  runHttpTests(tests, done);
}

function createTestDirectory()
{
  dir = Cc["@mozilla.org/file/directory_service;1"]
          .getService(Ci.nsIProperties)
          .get("TmpD", Ci.nsIFile);
  dir.append("index_handler_test_" + Math.random());
  dir.createUnique(Ci.nsIFile.DIRECTORY_TYPE, 0744);

  
  

  var files = [];

  makeFile("aa_directory", true, dir, files);
  makeFile("Ba_directory", true, dir, files);
  makeFile("bb_directory", true, dir, files);
  makeFile("foo", true, dir, files);
  makeFile("a_file", false, dir, files);
  makeFile("B_file", false, dir, files);
  makeFile("za'z", false, dir, files);
  makeFile("zb&z", false, dir, files);
  makeFile("zc<q", false, dir, files);
  makeFile('zd"q', false, dir, files);
  makeFile("ze%g", false, dir, files);
  makeFile("zf%200h", false, dir, files);
  makeFile("zg>m", false, dir, files);

  dirEntries = [files];

  var subdir = dir.clone();
  subdir.append("foo");

  files = [];

  makeFile("aa_dir", true, subdir, files);
  makeFile("b_dir", true, subdir, files);
  makeFile("AA_file.txt", false, subdir, files);
  makeFile("test.txt", false, subdir, files);

  dirEntries.push(files);
}

function destroyTestDirectory()
{
  dir.remove(true);
}







function hiddenDataCheck(bytes, uri, path)
{
  var data = String.fromCharCode.apply(null, bytes);

  var parser = Cc["@mozilla.org/xmlextras/domparser;1"]
                 .createInstance(Ci.nsIDOMParser);

  
  
  
  try
  {
    var doc = parser.parseFromString(data, "application/xml");
  }
  catch (e)
  {
    do_throw("document failed to parse as XML");
  }

  
  
  

  var body = doc.documentElement.getElementsByTagName("body");
  do_check_eq(body.length, 1);
  body = body.item(0);

  
  var header = body.QueryInterface(Ci.nsIDOMElement)
                   .getElementsByTagName("h1");
  do_check_eq(header.length, 1);

  do_check_eq(header.item(0).QueryInterface(Ci.nsIDOM3Node).textContent, path);

  
  var lst = body.getElementsByTagName("ol");
  do_check_eq(lst.length, 1);
  var items = lst.item(0).QueryInterface(Ci.nsIDOMElement)
                         .getElementsByTagName("li");

  var ios = Cc["@mozilla.org/network/io-service;1"]
              .getService(Ci.nsIIOService);

  var top = ios.newURI(uri, null, null);

  
  var dirEntries = [{name: "file.txt", isDirectory: false},
                    {name: "SHOULD_SEE_THIS.txt^", isDirectory: false}];

  for (var i = 0; i < items.length; i++)
  {
    var link = items.item(i)
                    .childNodes
                    .item(0)
                    .QueryInterface(Ci.nsIDOM3Node)
                    .QueryInterface(Ci.nsIDOMElement);
    var f = dirEntries[i];

    var sep = f.isDirectory ? "/" : "";

    do_check_eq(link.textContent, f.name + sep);

    uri = ios.newURI(link.getAttribute("href"), null, top);
    do_check_eq(decodeURIComponent(uri.path), path + f.name + sep);
  }
}


















function dataCheck(bytes, uri, path, dirEntries)
{
  var data = String.fromCharCode.apply(null, bytes);

  var parser = Cc["@mozilla.org/xmlextras/domparser;1"]
                 .createInstance(Ci.nsIDOMParser);

  
  
  
  try
  {
    var doc = parser.parseFromString(data, "application/xml");
  }
  catch (e)
  {
    do_throw("document failed to parse as XML");
  }

  
  
  

  var body = doc.documentElement.getElementsByTagName("body");
  do_check_eq(body.length, 1);
  body = body.item(0);

  
  var header = body.QueryInterface(Ci.nsIDOMElement)
                   .getElementsByTagName("h1");
  do_check_eq(header.length, 1);

  do_check_eq(header.item(0).QueryInterface(Ci.nsIDOM3Node).textContent, path);

  
  var lst = body.getElementsByTagName("ol");
  do_check_eq(lst.length, 1);
  var items = lst.item(0).QueryInterface(Ci.nsIDOMElement)
                         .getElementsByTagName("li");

  var ios = Cc["@mozilla.org/network/io-service;1"]
              .getService(Ci.nsIIOService);

  var dirURI = ios.newURI(uri, null, null);

  for (var i = 0; i < items.length; i++)
  {
    var link = items.item(i)
                    .childNodes
                    .item(0)
                    .QueryInterface(Ci.nsIDOM3Node)
                    .QueryInterface(Ci.nsIDOMElement);
    var f = dirEntries[i];

    var sep = f.isDirectory ? "/" : "";

    do_check_eq(link.textContent, f.name + sep);

    uri = ios.newURI(link.getAttribute("href"), null, top);
    do_check_eq(decodeURIComponent(uri.path), path + f.name + sep);
  }
}






function makeFile(name, isDirectory, parentDir, lst)
{
  var type = Ci.nsIFile[isDirectory ? "DIRECTORY_TYPE" : "NORMAL_FILE_TYPE"];
  var file = parentDir.clone();

  try
  {
    file.append(name);
    file.create(type, 0755);
    lst.push({name: name, isDirectory: isDirectory});
  }
  catch (e) {  }
}





var tests = [];
var test;


test = new Test("http://localhost:4444/",
                null, start, stopRootDirectory),
tests.push(test);
function start(ch)
{
  do_check_eq(ch.getResponseHeader("Content-Type"), "text/html");
}
function stopRootDirectory(ch, cx, status, data)
{
  dataCheck(data, "http://localhost:4444/", "/", dirEntries[0]);
}



test = new Test("http://localhost:4444/foo/",
                null, start, stopFooDirectory),
tests.push(test);
function stopFooDirectory(ch, cx, status, data)
{
  dataCheck(data, "http://localhost:4444/foo/", "/foo/", dirEntries[1]);
}



test = new Test("http://localhost:4444/bar/folder^/",
                null, start, stopTrailingCaretDirectory),
tests.push(test);
function stopTrailingCaretDirectory(ch, cx, status, data)
{
  hiddenDataCheck(data, "http://localhost:4444/bar/folder^/", "/bar/folder^/");
}
