












































let (ios = Cc["@mozilla.org/network/io-service;1"]
           .getService(Ci.nsIIOService2)) {
  ios.manageOfflineStatus = false;
  ios.offline = false;
}

const SERVER_PORT = 8888;
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
      for (var [key,value] in attrs) {
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
  
  for each (var tag in tags) {
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




function runServer()
{
  serverBasePath = __LOCATION__.parent;
  server = createMochitestServer(serverBasePath);
  server.start(SERVER_PORT);

  
  var foStream = Cc["@mozilla.org/network/file-output-stream;1"]
                   .createInstance(Ci.nsIFileOutputStream);
  var serverAlive = Cc["@mozilla.org/file/local;1"]
                      .createInstance(Ci.nsILocalFile);
  serverAlive.initWithFile(serverBasePath);
  serverAlive.append("mochitesttestingprofile");

  
  
  if (serverAlive.exists()) {
    serverAlive.append("server_alive.txt");
    foStream.init(serverAlive,
                  0x02 | 0x08 | 0x20, 0664, 0); 
    data = "It's alive!";
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
  server.registerContentType("sjs", "sjs"); 
  server.registerContentType("jar", "application/x-jar");
  server.registerContentType("ogg", "application/ogg");
  server.registerContentType("ogv", "video/ogg");
  server.registerContentType("oga", "audio/ogg");
  server.setIndexHandler(defaultDirHandler);

  processLocations(server);

  return server;
}






function processLocations(server)
{
  var serverLocations = serverBasePath.clone();
  serverLocations.append("server-locations.txt");

  const PR_RDONLY = 0x01;
  var fis = new FileInputStream(serverLocations, PR_RDONLY, 0444,
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
  
  
  var files = [file for (file in dirIter(dir))
               if (file.exists() && file.path.indexOf("SimpleTest") == -1)];
  
  
  
  function leafNameComparator(first, second) {
    if (first.leafName < second.leafName)
      return -1;
    if (first.leafName > second.leafName)
      return 1;
    return 0;
  }
  files.sort(leafNameComparator);
  
  count = files.length;
  for each (var file in files) {
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
        links[key] = true;
      }
    }
  }

  return [links, count];
}






function isTest(filename, pattern)
{
  if (pattern)
    return pattern.test(filename);

  return filename.indexOf("test_") > -1 &&
         filename.indexOf(".js") == -1 &&
         filename.indexOf(".css") == -1 &&
         !/\^headers\^$/.test(filename);
}




function linksToListItems(links)
{
  var response = "";
  var children = "";
  for (var [link, value] in links) {
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
  for (var [link, value] in links) {
    var classVal = (!isTest(link) && !(value instanceof Object))
      ? "non-test invisible"
      : "";

    spacer = "padding-left: " + (10 * recursionLevel) + "px";

    if (value instanceof Object) {
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
  for (var [link, value] in linkArray) {
    if (value instanceof Object) {
      arrayOfTestFiles(value, fileArray, testPattern);
    } else if (isTest(link, testPattern)) {
      fileArray.push(link)
    }
  }
}



function jsonArrayOfTestFiles(links)
{
  var testFiles = [];
  arrayOfTestFiles(links, testFiles);
  testFiles = ['"' + file + '"' for each(file in testFiles)];
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





function testListing(metadata, response)
{
  var [links, count] = list(metadata.path,
                            metadata.getProperty("directory"),
                            true);
  dumpn("count: " + count);
  var tests = jsonArrayOfTestFiles(links);
  var runtests = "Run Tests";
  if (metadata.queryString.indexOf("autorun=1") != -1) {
    runtests = "Stop Tests";
  }
  response.write(
    HTML(
      HEAD(
        TITLE("MochiTest | ", metadata.path),
        LINK({rel: "stylesheet",
              type: "text/css", href: "/static/harness.css"}
        ),
        SCRIPT({type: "text/javascript", src: "/MochiKit/packed.js"}),
        SCRIPT({type: "text/javascript",
                 src: "/tests/SimpleTest/TestRunner.js"}),
        SCRIPT({type: "text/javascript",
                 src: "/tests/SimpleTest/MozillaFileLogger.js"}),
        SCRIPT({type: "text/javascript",
                 src: "/tests/SimpleTest/quit.js"}),
        SCRIPT({type: "text/javascript",
                 src: "/tests/SimpleTest/setup.js"}),
        SCRIPT({type: "text/javascript"},
               "connect(window, 'onload', hookup); gTestList=" + tests + ";"
        )
      ),
      BODY(
        DIV({class: "container"},
          H2("--> ", A({href: "#", id: "runtests"}, runtests), " <--"),
            P({style: "float: right;"},
            SMALL(
              "Based on the ",
              A({href:"http://www.mochikit.com/"}, "MochiKit"),
              " unit tests."
            )
          ),
          DIV({class: "status"},
            H1({id: "indicator"}, "Status"),
            H2({id: "file"}, "File: ",
               SPAN({id: "progress"}, "0 / " + count + " (0%)")),
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
            IFRAME({scrolling: "no", id: "testframe", width: "500", height: "300"})
          ),
          DIV({class: "clear"}),
          DIV({class: "toggle"},
            A({href: "#", id: "toggleNonTests"}, "Show Non-Tests"),
            BR()
          ),
    
          TABLE({cellpadding: 0, cellspacing: 0, id: "test-table"},
            TR(TD("Passed"), TD("Failed"), TD("Todo"), TD("Test Files")),
            linksToTableRows(links, 0)
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
  response.setHeader("Content-type", "text/html", false);
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
