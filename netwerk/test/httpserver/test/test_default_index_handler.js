









































var paths =
  [
   "http://localhost:4444/",     
   "http://localhost:4444/foo/"  
  ];
var currPathIndex = 0;


















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

  var top = ios.newURI(uri, null, null);

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

    var uri = ios.newURI(link.getAttribute("href"), null, top);
    var fn = encodeURIComponent(f.name) + sep;
    do_check_eq(uri.path, path + fn);
  }
}

var listener =
  {
    
    _data: [],

    onStartRequest: function(request, cx)
    {
      var ch = request.QueryInterface(Ci.nsIHttpChannel);
      do_check_eq(ch.getResponseHeader("Content-Type"), "text/html");

      this._data.length = 0;
      this._uri = paths[currPathIndex];
      this._path = this._uri.substring(paths[0].length - 1);
      this._dirEntries = dirEntries[currPathIndex];
    },
    onDataAvailable: function(request, cx, inputStream, offset, count)
    {
      var bytes = makeBIS(inputStream).readByteArray(count);
      Array.prototype.push.apply(this._data, bytes);
    },
    onStopRequest: function(request, cx, status)
    {
      dataCheck(this._data, this._uri, this._path, this._dirEntries);

      if (++currPathIndex == paths.length)
      {
        srv.stop();
        destroyTestDirectory();
      }
      else
      {
        performNextTest();
      }
      do_test_finished();
    },
    QueryInterface: function(aIID)
    {
      if (aIID.equals(Ci.nsIStreamListener) ||
          aIID.equals(Ci.nsIRequestObserver) ||
          aIID.equals(Ci.nsISupports))
        return this;
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  };


var srv, dir, dirEntries;

function run_test()
{
  createTestDirectory();

  srv = createServer();
  srv.registerDirectory("/", dir);
  srv.start(4444);

  performNextTest();
}

function performNextTest()
{
  do_test_pending();

  var ch = makeChannel(paths[currPathIndex]);
  ch.asyncOpen(listener, null);
}

function createTestDirectory()
{
  dir = Cc["@mozilla.org/file/directory_service;1"]
          .getService(Ci.nsIProperties)
          .get("TmpD", Ci.nsIFile);
  dir.append("index_handler_test_" + Math.random());
  dir.createUnique(Ci.nsIFile.DIRECTORY_TYPE, 0744);

  
  

  files = [];

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









function makeFile(name, isDirectory, parentDir, lst)
{
  var type = isDirectory
           ? Ci.nsIFile.DIRECTORY_TYPE
           : Ci.nsIFile.NORMAL_FILE_TYPE;

  var file = parentDir.clone();

  try
  {
    file.append(name);
    file.create(type, 0755);
    lst.push({name: name, isDirectory: isDirectory});
  }
  catch (e) {  }
}
