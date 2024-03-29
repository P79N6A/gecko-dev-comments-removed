











let ios = Cc["@mozilla.org/network/io-service;1"]
          .getService(Ci.nsIIOService2);
ios.manageOfflineStatus = false;
ios.offline = false;

var server; 




var tags = ['A', 'ABBR', 'ACRONYM', 'ADDRESS', 'APPLET', 'AREA', 'B', 'BASE',
            'BASEFONT', 'BDO', 'BIG', 'BLOCKQUOTE', 'BODY', 'BR', 'BUTTON',
            'CAPTION', 'CENTER', 'CITE', 'CODE', 'COL', 'COLGROUP', 'DD',
            'DEL', 'DFN', 'DIR', 'DIV', 'DL', 'DT', 'EM', 'FIELDSET', 'FONT',
            'FORM', 'FRAME', 'FRAMESET', 'H1', 'H2', 'H3', 'H4', 'H5', 'H6',
            'HEAD', 'HR', 'HTML', 'I', 'IFRAME', 'IMG', 'INPUT', 'INS',
            'ISINDEX', 'KBD', 'LABEL', 'LEGEND', 'LI', 'LINK', 'MAP', 'MENU',
            'META', 'NOFRAMES', 'NOSCRIPT', 'OBJECT', 'OL', 'OPTGROUP',
            'OPTION', 'P', 'PARAM', 'PRE', 'Q', 'S', 'SAMP', 'SCRIPT',
            'SELECT', 'SMALL', 'SPAN', 'STRIKE', 'STRONG', 'STYLE', 'SUB',
            'SUP', 'TABLE', 'TBODY', 'TD', 'TEXTAREA', 'TFOOT', 'TH', 'THEAD',
            'TITLE', 'TR', 'TT', 'U', 'UL', 'VAR'];






function makeTagFunc(tagName)
{
  return function (attrs )
  {
    var startChildren = 0;
    var response = "";

    
    response += "<" + tagName;
    
    if (attrs && typeof attrs == 'object') {
      startChildren = 1;

      for (let key in attrs) {
        const value = attrs[key];
        var val = "" + value;
        response += " " + key + '="' + val.replace('"','&quot;') + '"';
      }
    }
    response += ">";

    
    for (var i = startChildren; i < arguments.length; i++) {
      if (typeof arguments[i] == 'function') {
        response += arguments[i]();
      } else {
        response += arguments[i];
      }
    }

    
    response += "</" + tagName + ">\n";
    return response;
  }
}

function makeTags() {
  
  for (let tag of tags) {
      this[tag] = makeTagFunc(tag.toLowerCase());
  }
}

var _quitting = false;


function serverStopped()
{
  _quitting = true;
}


if (this["nsHttpServer"]) {
  
  
  
  runServer();

  
  if (_quitting)
  {
    dumpn("HTTP server stopped, all pending requests complete");
    quit(0);
  }

  
  dumpn("TEST-UNEXPECTED-FAIL | failure to correctly shut down HTTP server");
  quit(1);
}

var serverBasePath;
var displayResults = true;

var gServerAddress;
var SERVER_PORT;




function runServer()
{
  serverBasePath = __LOCATION__.parent;
  server = createMochitestServer(serverBasePath);

  
  
  if (typeof(_SERVER_ADDR) != "undefined") {
    if (_SERVER_ADDR == "localhost") {
      gServerAddress = _SERVER_ADDR;      
    } else {
      var quads = _SERVER_ADDR.split('.');
      if (quads.length == 4) {
        var invalid = false;
        for (var i=0; i < 4; i++) {
          if (quads[i] < 0 || quads[i] > 255)
            invalid = true;
        }
        if (!invalid)
          gServerAddress = _SERVER_ADDR;
        else
          throw "invalid _SERVER_ADDR, please specify a valid IP Address";
      }
    }
  } else {
    throw "please defined _SERVER_ADDR (as an ip address) before running server.js";
  }

  if (typeof(_SERVER_PORT) != "undefined") {
    if (parseInt(_SERVER_PORT) > 0 && parseInt(_SERVER_PORT) < 65536)
      SERVER_PORT = _SERVER_PORT;
  } else {
    throw "please define _SERVER_PORT (as a port number) before running server.js";
  }

  
  if (typeof(_DISPLAY_RESULTS) != "undefined") {
    displayResults = _DISPLAY_RESULTS;
  }

  server._start(SERVER_PORT, gServerAddress);

  
  var foStream = Cc["@mozilla.org/network/file-output-stream;1"]
                   .createInstance(Ci.nsIFileOutputStream);
  var serverAlive = Cc["@mozilla.org/file/local;1"]
                      .createInstance(Ci.nsILocalFile);

  if (typeof(_PROFILE_PATH) == "undefined") {
    serverAlive.initWithFile(serverBasePath);
    serverAlive.append("mochitesttestingprofile");
  } else {
    serverAlive.initWithPath(_PROFILE_PATH);
  }

  
  
  if (serverAlive.exists()) {
    serverAlive.append("server_alive.txt");
    foStream.init(serverAlive,
                  0x02 | 0x08 | 0x20, 436, 0); 
    var data = "It's alive!";
    foStream.write(data, data.length);
    foStream.close();
  }

  makeTags();

  
  
  
  
  var thread = Cc["@mozilla.org/thread-manager;1"]
                 .getService()
                 .currentThread;
  while (!server.isStopped())
    thread.processNextEvent(true);

  
  

  
  while (thread.hasPendingEvents())
    thread.processNextEvent(true);
}


function createMochitestServer(serverBasePath)
{
  var server = new nsHttpServer();

  server.registerDirectory("/", serverBasePath);
  server.registerPathHandler("/server/shutdown", serverShutdown);
  server.registerPathHandler("/server/debug", serverDebug);
  server.registerPathHandler("/nested_oop", nestedTest);
  server.registerContentType("sjs", "sjs"); 
  server.registerContentType("jar", "application/x-jar");
  server.registerContentType("ogg", "application/ogg");
  server.registerContentType("pdf", "application/pdf");
  server.registerContentType("ogv", "video/ogg");
  server.registerContentType("oga", "audio/ogg");
  server.registerContentType("opus", "audio/ogg; codecs=opus");
  server.registerContentType("dat", "text/plain; charset=utf-8");
  server.registerContentType("frag", "text/plain"); 
  server.registerContentType("vert", "text/plain"); 
  server.setIndexHandler(defaultDirHandler);

  var serverRoot =
    {
      getFile: function getFile(path)
      {
        var file = serverBasePath.clone().QueryInterface(Ci.nsILocalFile);
        path.split("/").forEach(function(p) {
          file.appendRelativePath(p);
        });
        return file;
      },
      QueryInterface: function(aIID) { return this; }
    };

  server.setObjectState("SERVER_ROOT", serverRoot);

  processLocations(server);

  return server;
}






function processLocations(server)
{
  var serverLocations = serverBasePath.clone();
  serverLocations.append("server-locations.txt");

  const PR_RDONLY = 0x01;
  var fis = new FileInputStream(serverLocations, PR_RDONLY, 292 ,
                                Ci.nsIFileInputStream.CLOSE_ON_EOF);

  var lis = new ConverterInputStream(fis, "UTF-8", 1024, 0x0);
  lis.QueryInterface(Ci.nsIUnicharLineInputStream);

  const LINE_REGEXP =
    new RegExp("^([a-z][-a-z0-9+.]*)" +
               "://" +
               "(" +
                 "\\d+\\.\\d+\\.\\d+\\.\\d+" +
                 "|" +
                 "(?:[a-z0-9](?:[-a-z0-9]*[a-z0-9])?\\.)*" +
                 "[a-z](?:[-a-z0-9]*[a-z0-9])?" +
               ")" +
               ":" +
               "(\\d+)" +
               "(?:" +
               "\\s+" +
               "(\\S+(?:,\\S+)*)" +
               ")?$");

  var line = {};
  var lineno = 0;
  var seenPrimary = false;
  do
  {
    var more = lis.readLine(line);
    lineno++;

    var lineValue = line.value;
    if (lineValue.charAt(0) == "#" || lineValue == "")
      continue;

    var match = LINE_REGEXP.exec(lineValue);
    if (!match)
      throw "Syntax error in server-locations.txt, line " + lineno;

    var [, scheme, host, port, options] = match;
    if (options)
    {
      if (options.split(",").indexOf("primary") >= 0)
      {
        if (seenPrimary)
        {
          throw "Multiple primary locations in server-locations.txt, " +
                "line " + lineno;
        }
  
        server.identity.setPrimary(scheme, host, port);
        seenPrimary = true;
        continue;
      }
    }

    server.identity.add(scheme, host, port);
  }
  while (more);
}




function serverShutdown(metadata, response)
{
  response.setStatusLine("1.1", 200, "OK");
  response.setHeader("Content-type", "text/plain", false);

  var body = "Server shut down.";
  response.bodyOutputStream.write(body, body.length);

  dumpn("Server shutting down now...");
  server.stop(serverStopped);
}


function serverDebug(metadata, response)
{
  response.setStatusLine(metadata.httpVersion, 400, "Bad debugging level");
  if (metadata.queryString.length !== 1)
    return;

  var mode;
  if (metadata.queryString === "0") {
    
    dumpn("Server debug logs disabled.");
    DEBUG = false;
    DEBUG_TIMESTAMP = false;
    mode = "disabled";
  } else if (metadata.queryString === "1") {
    DEBUG = true;
    DEBUG_TIMESTAMP = false;
    mode = "enabled";
  } else if (metadata.queryString === "2") {
    DEBUG = true;
    DEBUG_TIMESTAMP = true;
    mode = "enabled, with timestamps";
  } else {
    return;
  }

  response.setStatusLine(metadata.httpVersion, 200, "OK");
  response.setHeader("Content-type", "text/plain", false);
  var body = "Server debug logs " + mode + ".";
  response.bodyOutputStream.write(body, body.length);
  dumpn(body);
}









function dirIter(dir)
{
  var en = dir.directoryEntries;
  while (en.hasMoreElements()) {
    var file = en.getNext();
    yield file.QueryInterface(Ci.nsILocalFile);
  }
}





function list(requestPath, directory, recurse)
{
  var count = 0;
  var path = requestPath;
  if (path.charAt(path.length - 1) != "/") {
    path += "/";
  }

  var dir = directory.QueryInterface(Ci.nsIFile);
  var links = {};

  
  let files = [];
  for (let file of dirIter(dir)) {
    if (file.exists() && file.path.indexOf("SimpleTest") == -1) {
      files.push(file);
    }
  }

  
  
  function leafNameComparator(first, second) {
    if (first.leafName < second.leafName)
      return -1;
    if (first.leafName > second.leafName)
      return 1;
    return 0;
  }
  files.sort(leafNameComparator);

  count = files.length;
  for (let file of files) {
    var key = path + file.leafName;
    var childCount = 0;
    if (file.isDirectory()) {
      key += "/";
    }
    if (recurse && file.isDirectory()) {
      [links[key], childCount] = list(key, file, recurse);
      count += childCount;
    } else {
      if (file.leafName.charAt(0) != '.') {
        links[key] = {'test': {'url': key, 'expected': 'pass'}};
      }
    }
  }

  return [links, count];
}






function isTest(filename, pattern)
{
  if (pattern)
    return pattern.test(filename);

  
  
  var testPrefix = typeof(_TEST_PREFIX) == "string" ? _TEST_PREFIX : "test_";
  var testPattern = new RegExp("^" + testPrefix);

  var pathPieces = filename.split('/');
    
  return testPattern.test(pathPieces[pathPieces.length - 1]) &&
         filename.indexOf(".js") == -1 &&
         filename.indexOf(".css") == -1 &&
         !/\^headers\^$/.test(filename);
}




function linksToListItems(links)
{
  var response = "";
  var children = "";
  for (let link in links) {
    const value = links[link];
    var classVal = (!isTest(link) && !(value instanceof Object))
      ? "non-test invisible"
      : "test";
    if (value instanceof Object) {
      children = UL({class: "testdir"}, linksToListItems(value)); 
    } else {
      children = "";
    }

    var bug_title = link.match(/test_bug\S+/);
    var bug_num = null;
    if (bug_title != null) {
        bug_num = bug_title[0].match(/\d+/);
    }

    if ((bug_title == null) || (bug_num == null)) {
      response += LI({class: classVal}, A({href: link}, link), children);
    } else {
      var bug_url = "https://bugzilla.mozilla.org/show_bug.cgi?id="+bug_num;
      response += LI({class: classVal}, A({href: link}, link), " - ", A({href: bug_url}, "Bug "+bug_num), children);
    }

  }
  return response;
}




function linksToTableRows(links, recursionLevel)
{
  var response = "";
  for (let link in links) {
    const value = links[link];
    var classVal = (!isTest(link) && ((value instanceof Object) && ('test' in value)))
      ? "non-test invisible"
      : "";

    var spacer = "padding-left: " + (10 * recursionLevel) + "px";

    if ((value instanceof Object) && !('test' in value)) {
      response += TR({class: "dir", id: "tr-" + link },
                     TD({colspan: "3"}, "&#160;"),
                     TD({style: spacer},
                        A({href: link}, link)));
      response += linksToTableRows(value, recursionLevel + 1);
    } else {
      var bug_title = link.match(/test_bug\S+/);
      var bug_num = null;
      if (bug_title != null) {
          bug_num = bug_title[0].match(/\d+/);
      }
      if ((bug_title == null) || (bug_num == null)) {
        response += TR({class: classVal, id: "tr-" + link },
                       TD("0"),
                       TD("0"),
                       TD("0"),
                       TD({style: spacer},
                          A({href: link}, link)));
      } else {
        var bug_url = "https://bugzilla.mozilla.org/show_bug.cgi?id=" + bug_num;
        response += TR({class: classVal, id: "tr-" + link },
                       TD("0"),
                       TD("0"),
                       TD("0"),
                       TD({style: spacer},
                          A({href: link}, link), " - ",
                          A({href: bug_url}, "Bug " + bug_num)));
      }
    }
  }
  return response;
}

function arrayOfTestFiles(linkArray, fileArray, testPattern) {
  for (let link in linkArray) {
    const value = linkArray[link];
    if ((value instanceof Object) && !('test' in value)) {
      arrayOfTestFiles(value, fileArray, testPattern);
    } else if (isTest(link, testPattern) && (value instanceof Object)) {
      fileArray.push(value['test'])
    }
  }
}



function jsonArrayOfTestFiles(links)
{
  var testFiles = [];
  arrayOfTestFiles(links, testFiles);
  testFiles = testFiles.map(function(file) { return '"' + file['url'] + '"'; });

  return "[" + testFiles.join(",\n") + "]";
}




function regularListing(metadata, response)
{
  var [links, count] = list(metadata.path,
                            metadata.getProperty("directory"),
                            false);
  response.write(
    HTML(
      HEAD(
        TITLE("mochitest index ", metadata.path)
      ),
      BODY(
        BR(),
        A({href: ".."}, "Up a level"),
        UL(linksToListItems(links))
      )
    )
  );
}





function convertManifestToTestLinks(root, manifest)
{
  Cu.import("resource://gre/modules/NetUtil.jsm");

  var manifestFile = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
  manifestFile.initWithFile(serverBasePath);
  manifestFile.append(manifest);

  var manifestStream = Cc["@mozilla.org/network/file-input-stream;1"].createInstance(Ci.nsIFileInputStream);
  manifestStream.init(manifestFile, -1, 0, 0);

  var manifestObj = JSON.parse(NetUtil.readInputStreamToString(manifestStream,
                                                               manifestStream.available()));
  var paths = manifestObj.tests;
  var pathPrefix = '/' + root + '/'
  return [paths.reduce(function(t, p) { t[pathPrefix + p.path] = true; return t; }, {}),
          paths.length];
}




function nestedTest(metadata, response)
{
  response.setStatusLine("1.1", 200, "OK");
  response.setHeader("Content-type", "text/html;charset=utf-8", false);
  response.write(
    HTML(
      HEAD(
        TITLE("Mochitest | ", metadata.path),
        LINK({rel: "stylesheet",
              type: "text/css", href: "/static/harness.css"}),
        SCRIPT({type: "text/javascript",
                src: "/nested_setup.js"}),
        SCRIPT({type: "text/javascript"},
               "window.onload = addPermissions; gTestURL = '/tests?" + metadata.queryString + "';")
        ),
      BODY(
        DIV({class: "container"},
          DIV({class: "frameholder", id: "holder-div"})
        )
        )));
}





function testListing(metadata, response)
{
  var links = {};
  var count = 0;
  if (metadata.queryString.indexOf('manifestFile') == -1) {
    [links, count] = list(metadata.path,
                          metadata.getProperty("directory"),
                          true);
  } else if (typeof(Components) != undefined) {
    var manifest = metadata.queryString.match(/manifestFile=([^&]+)/)[1];

    [links, count] = convertManifestToTestLinks(metadata.path.split('/')[1],
                                                manifest);
  }

  var table_class = metadata.queryString.indexOf("hideResultsTable=1") > -1 ? "invisible": "";

  let testname = (metadata.queryString.indexOf("testname=") > -1)
                 ? metadata.queryString.match(/testname=([^&]+)/)[1]
                 : "";

  dumpn("count: " + count);
  var tests = testname
              ? "['/" + testname + "']"
              : jsonArrayOfTestFiles(links);
  response.write(
    HTML(
      HEAD(
        TITLE("MochiTest | ", metadata.path),
        LINK({rel: "stylesheet",
              type: "text/css", href: "/static/harness.css"}
        ),
        SCRIPT({type: "text/javascript",
                 src: "/tests/SimpleTest/LogController.js"}),
        SCRIPT({type: "text/javascript",
                 src: "/tests/SimpleTest/MemoryStats.js"}),
        SCRIPT({type: "text/javascript",
                 src: "/tests/SimpleTest/TestRunner.js"}),
        SCRIPT({type: "text/javascript",
                 src: "/tests/SimpleTest/MozillaLogger.js"}),
        SCRIPT({type: "text/javascript",
                 src: "/chunkifyTests.js"}),
        SCRIPT({type: "text/javascript",
                 src: "/manifestLibrary.js"}),
        SCRIPT({type: "text/javascript",
                 src: "/tests/SimpleTest/setup.js"}),
        SCRIPT({type: "text/javascript"},
               "window.onload =  hookup; gTestList=" + tests + ";"
        )
      ),
      BODY(
        DIV({class: "container"},
          H2("--> ", A({href: "#", id: "runtests"}, "Run Tests"), " <--"),
            P({style: "float: right;"},
            SMALL(
              "Based on the ",
              A({href:"http://www.mochikit.com/"}, "MochiKit"),
              " unit tests."
            )
          ),
          DIV({class: "status"},
            H1({id: "indicator"}, "Status"),
            H2({id: "pass"}, "Passed: ", SPAN({id: "pass-count"},"0")),
            H2({id: "fail"}, "Failed: ", SPAN({id: "fail-count"},"0")),
            H2({id: "fail"}, "Todo: ", SPAN({id: "todo-count"},"0"))
          ),
          DIV({class: "clear"}),
          DIV({id: "current-test"},
            B("Currently Executing: ",
              SPAN({id: "current-test-path"}, "_")
            )
          ),
          DIV({class: "clear"}),
          DIV({class: "frameholder"},
            IFRAME({scrolling: "no", id: "testframe"})
          ),
          DIV({class: "clear"}),
          DIV({class: "toggle"},
            A({href: "#", id: "toggleNonTests"}, "Show Non-Tests"),
            BR()
          ),

          (
           displayResults ?
            TABLE({cellpadding: 0, cellspacing: 0, class: table_class, id: "test-table"},
              TR(TD("Passed"), TD("Failed"), TD("Todo"), TD("Test Files")),
              linksToTableRows(links, 0)
            ) : ""
          ),

          BR(),
          TABLE({cellpadding: 0, cellspacing: 0, border: 1, bordercolor: "red", id: "fail-table"}
          ),

          DIV({class: "clear"})
        )
      )
    )
  );
}





function defaultDirHandler(metadata, response)
{
  response.setStatusLine("1.1", 200, "OK");
  response.setHeader("Content-type", "text/html;charset=utf-8", false);
  try {
    if (metadata.path.indexOf("/tests") != 0) {
      regularListing(metadata, response);
    } else {
      testListing(metadata, response);
    }
  } catch (ex) {
    response.write(ex);
  }  
}
