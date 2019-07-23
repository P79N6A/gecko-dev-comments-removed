






































const Cc = Components.classes;
const Ci = Components.interfaces;

var NUM_CYCLES = 5;

var pageFilterRegexp = null;
var reportFormat = "js";
var useBrowser = true;
var winWidth = 1024;
var winHeight = 768;
var urlPrefix = null;

var doRenderTest = false;

var pages;
var pageIndex;
var results;
var start_time;
var cycle;
var report;
var renderReport;
var running = false;

var content;

var browserWindow = null;

function plInit() {
  if (running) {
    return;
  }
  running = true;

  cycle = 0;
  results = {};

  try {
    var args = window.arguments[0].wrappedJSObject;

    var manifestURI = args.manifest;
    var startIndex = 0;
    var endIndex = -1;
    if (args.startIndex) startIndex = parseInt(args.startIndex);
    if (args.endIndex) endIndex = parseInt(args.endIndex);
    if (args.numCycles) NUM_CYCLES = parseInt(args.numCycles);
    if (args.format) reportFormat = args.format;
    if (args.width) winWidth = parseInt(args.width);
    if (args.height) winHeight = parseInt(args.height);
    if (args.filter) pageFilterRegexp = new RegExp(args.filter);
    if (args.prefix) urlPrefix = args.prefix;
    doRenderTest = args.doRender;

    var ios = Cc["@mozilla.org/network/io-service;1"]
      .getService(Ci.nsIIOService);
    if (args.offline)
      ios.offline = true;
    var fileURI = ios.newURI(manifestURI, null, null);
    pages = plLoadURLsFromURI(fileURI);

    if (!pages) {
      dumpLine('tp: could not load URLs, quitting');
      plStop(true);
    }

    if (pages.length == 0) {
      dumpLine('tp: no pages to test, quitting');
      plStop(true);
    }

    if (startIndex < 0)
      startIndex = 0;
    if (endIndex == -1 || endIndex >= pages.length)
      endIndex = pages.length-1;
    if (startIndex > endIndex) {
      dumpLine("tp: error: startIndex >= endIndex");
      plStop(true);
    }

    pages = pages.slice(startIndex,endIndex+1);
    report = new Report(pages);

    if (doRenderTest)
      renderReport = new Report(pages);

    pageIndex = 0;

    if (args.useBrowserChrome) {
      var wwatch = Cc["@mozilla.org/embedcomp/window-watcher;1"]
	.getService(Ci.nsIWindowWatcher);
      var blank = Cc["@mozilla.org/supports-string;1"]
	.createInstance(Ci.nsISupportsString);
      blank.data = "about:blank";
      browserWindow = wwatch.openWindow
        (null, "chrome://browser/content/", "_blank",
         "chrome,dialog=no,width=" + winWidth + ",height=" + winHeight, blank);

      
      window.resizeTo(10,10);

      var browserLoadFunc = function (ev) {
        browserWindow.removeEventListener('load', browserLoadFunc, true);

        
        
        
        setTimeout(function () {
                     browserWindow.resizeTo(winWidth, winHeight);
                     browserWindow.moveTo(0, 0);
                     browserWindow.focus();

                     content = browserWindow.getBrowser();
                     content.addEventListener('load', plLoadHandler, true);
                     setTimeout(plLoadPage, 100);
                   }, 500);
      };

      browserWindow.addEventListener('load', browserLoadFunc, true);
    } else {
      window.resizeTo(winWidth, winHeight);

      content = document.getElementById('contentPageloader');
      content.addEventListener('load', plLoadHandler, true);

      setTimeout(plLoadPage, 0);
    }
  } catch(e) {
    dumpLine(e);
    plStop(true);
  }
}

function plLoadPage() {
  start_time = Date.now();
  content.loadURI(pages[pageIndex]);
}

function plLoadHandler(evt) {
  
  if (evt.type != 'load' ||
      (!evt.originalTarget instanceof Ci.nsIDOMHTMLDocument ||
       evt.originalTarget.defaultView.frameElement))
      return;

  var end_time = Date.now();
  var time = (end_time - start_time);

  var pageName = pages[pageIndex];

  results[pageName] = time;
  report.recordTime(pageIndex, time);

  if (doRenderTest)
    runRenderTest();

  if (pageIndex < pages.length-1) {
    pageIndex++;
    setTimeout(plLoadPage, 0);
  } else {
    plStop(false);
  }
}

function runRenderTest() {
  const redrawsPerSample = 5;
  const renderCycles = 10;

  if (!Ci.nsIDOMWindowUtils)
    return;

  var win;

  if (browserWindow)
    win = content.contentWindow;
  else 
    win = window;
  var wu = win.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);

  for (var i = 0; i < renderCycles; i++) {
    var start = Date.now();
    for (var j = 0; j < redrawsPerSample; j++)
      wu.redraw();
    var end = Date.now();

    renderReport.recordTime(pageIndex, end - start);
  }
}

function plStop(force) {
  try {
    if (force == false) {
      pageIndex = 0;
      results = {};
      if (cycle < NUM_CYCLES-1) {
	cycle++;
	setTimeout(plLoadPage, 0);
	return;
      }

      var formats = reportFormat.split(",");

      for each (var fmt in formats)
	dumpLine(report.getReport(fmt));

      if (renderReport) {
	dumpLine ("*************** Render report *******************");
	for each (var fmt in formats)
	  dumpLine(renderReport.getReport(fmt));
      }
    }
  } catch (e) {
    dumpLine(e);
  }

  if (content)
    content.removeEventListener('load', plLoadHandler, true);

  goQuitApplication();
}


function plLoadURLsFromURI(uri) {
  var data = "";
  var fstream = Cc["@mozilla.org/network/file-input-stream;1"]
    .createInstance(Ci.nsIFileInputStream);
  var sstream = Cc["@mozilla.org/scriptableinputstream;1"]
    .createInstance(Ci.nsIScriptableInputStream);
  var uriFile = uri.QueryInterface(Ci.nsIFileURL);
  fstream.init(uriFile.file, -1, 0, 0);
  sstream.init(fstream); 
    
  var str = sstream.read(4096);
  while (str.length > 0) {
    data += str;
    str = sstream.read(4096);
  }
    
  sstream.close();
  fstream.close();
  var p = data.split("\n");

  
  
  p = p.filter(function(s) {
		 if (s == "" || s.indexOf("#") == 0)
		   return false;
		 if (pageFilterRegexp && !pageFilterRegexp.test(s))
		   return false;
		 return true;
	       });

  
  if (urlPrefix)
    p = p.map(function(s) { return urlPrefix + s; });

  return p;
}

function dumpLine(str) {
  dump(str);
  dump("\n");
}
