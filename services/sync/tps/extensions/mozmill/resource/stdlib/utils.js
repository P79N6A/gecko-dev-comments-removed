



var EXPORTED_SYMBOLS = ["applicationName", "assert", "Copy", "getBrowserObject",
                        "getChromeWindow", "getWindows", "getWindowByTitle",
                        "getWindowByType", "getWindowId", "getMethodInWindows",
                        "getPreference", "saveDataURL", "setPreference",
                        "sleep", "startTimer", "stopTimer", "takeScreenshot",
                        "unwrapNode", "waitFor"
                       ];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;


Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const applicationIdMap = {
  '{ec8030f7-c20a-464f-9b0e-13a3a9e97384}': 'Firefox'
}
const applicationName = applicationIdMap[Services.appinfo.ID] || Services.appinfo.name;

var assertions = {}; Cu.import('resource://mozmill/modules/assertions.js', assertions);
var broker = {}; Cu.import('resource://mozmill/driver/msgbroker.js', broker);
var errors = {}; Cu.import('resource://mozmill/modules/errors.js', errors);

var assert = new assertions.Assert();

var hwindow = Services.appShell.hiddenDOMWindow;

var uuidgen = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator);

function Copy (obj) {
  for (var n in obj) {
    this[n] = obj[n];
  }
}









function getBrowserObject(aWindow) {
  return aWindow.gBrowser;
}

function getChromeWindow(aWindow) {
  var chromeWin = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIWebNavigation)
                  .QueryInterface(Ci.nsIDocShellTreeItem)
                  .rootTreeItem
                  .QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIDOMWindow)
                  .QueryInterface(Ci.nsIDOMChromeWindow);

  return chromeWin;
}

function getWindows(type) {
  if (type == undefined) {
    type = "";
  }

  var windows = [];
  var enumerator = Services.wm.getEnumerator(type);

  while (enumerator.hasMoreElements()) {
    windows.push(enumerator.getNext());
  }

  if (type == "") {
    windows.push(hwindow);
  }

  return windows;
}

function getMethodInWindows(methodName) {
  for each (var w in getWindows()) {
    if (w[methodName] != undefined) {
      return w[methodName];
    }
  }

  throw new Error("Method with name: '" + methodName + "' is not in any open window.");
}

function getWindowByTitle(title) {
  for each (var w in getWindows()) {
    if (w.document.title && w.document.title == title) {
      return w;
    }
  }

  throw new Error("Window with title: '" + title + "' not found.");
}

function getWindowByType(type) {
  return Services.wm.getMostRecentWindow(type);
}








function getWindowId(aWindow) {
  try {
    
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor).
                   getInterface(Ci.nsIDOMWindowUtils).
                   outerWindowID;
  } catch (e) {
    
    return aWindow.QueryInterface(Ci.nsISupportsPRUint64).data;
  }
}

var checkChrome = function () {
  var loc = window.document.location.href;
  try {
    loc = window.top.document.location.href;
  } catch (e) {
  }

  return /^chrome:\/\//.test(loc);
}












function getPreference(aPrefName, aDefaultValue) {
  try {
    var branch = Services.prefs;

    switch (typeof aDefaultValue) {
      case ('boolean'):
        return branch.getBoolPref(aPrefName);
      case ('string'):
        return branch.getCharPref(aPrefName);
      case ('number'):
        return branch.getIntPref(aPrefName);
      default:
        return branch.getComplexValue(aPrefName);
    }
  } catch (e) {
    return aDefaultValue;
  }
}












function setPreference(aName, aValue) {
  try {
    var branch = Services.prefs;

    switch (typeof aValue) {
      case ('boolean'):
        branch.setBoolPref(aName, aValue);
        break;
      case ('string'):
        branch.setCharPref(aName, aValue);
        break;
      case ('number'):
        branch.setIntPref(aName, aValue);
        break;
      default:
        branch.setComplexValue(aName, aValue);
    }
  } catch (e) {
    return false;
  }

  return true;
}







function sleep(milliseconds) {
  var timeup = false;

  hwindow.setTimeout(function () { timeup = true; }, milliseconds);
  var thread = Services.tm.currentThread;

  while (!timeup) {
    thread.processNextEvent(true);
  }

  broker.pass({'function':'utils.sleep()'});
}




function assert(callback, message, thisObject) {
  var result = callback.call(thisObject);

  if (!result) {
    throw new Error(message || arguments.callee.name + ": Failed for '" + callback + "'");
  }

  return true;
}







function unwrapNode(aNode) {
  var node = aNode;
  if (node) {
    
    if ("unwrap" in XPCNativeWrapper) {
      node = XPCNativeWrapper.unwrap(node);
    }
    else if (node.wrappedJSObject != null) {
      node = node.wrappedJSObject;
    }
  }

  return node;
}




function waitFor(callback, message, timeout, interval, thisObject) {
  broker.log({'function': 'utils.waitFor() - DEPRECATED',
              'message': 'utils.waitFor() is deprecated. Use assert.waitFor() instead'});
  assert.waitFor(callback, message, timeout, interval, thisObject);
}







function getChromeOffset(elem) {
  var win = elem.ownerDocument.defaultView;
  
  var chromeWidth = 0;

  if (win["name"] != "sidebar") {
    chromeWidth = win.outerWidth - win.innerWidth;
  }

  
  var chromeHeight = win.outerHeight - win.innerHeight;
  
  if (chromeHeight > 0) {
    
    var addonbar = win.document.getElementById("addon-bar");
    if (addonbar) {
      chromeHeight -= addonbar.scrollHeight;
    }

    var findbar = win.document.getElementById("FindToolbar");
    if (findbar) {
      chromeHeight -= findbar.scrollHeight;
    }
  }

  return {'x':chromeWidth, 'y':chromeHeight};
}




function takeScreenshot(node, highlights) {
  var rect, win, width, height, left, top, needsOffset;
  
  try {
    
    win = node.ownerDocument.defaultView;
    rect = node.getBoundingClientRect();
    width = rect.width;
    height = rect.height;
    top = rect.top;
    left = rect.left;
    
    needsOffset = false;
  } catch (e) {
    
    win = node;
    width = win.innerWidth;
    height = win.innerHeight;
    top = 0;
    left = 0;
    
    needsOffset = true;
  }

  var canvas = win.document.createElementNS("http://www.w3.org/1999/xhtml", "canvas");
  canvas.width = width;
  canvas.height = height;

  var ctx = canvas.getContext("2d");
  
  ctx.drawWindow(win, left, top, width, height, "rgb(255,255,255)");

  
  if (highlights) {
    ctx.lineWidth = "2";
    ctx.strokeStyle = "red";
    ctx.save();

    for (var i = 0; i < highlights.length; ++i) {
      var elem = highlights[i];
      rect = elem.getBoundingClientRect();

      var offsetY = 0, offsetX = 0;
      if (needsOffset) {
        var offset = getChromeOffset(elem);
        offsetX = offset.x;
        offsetY = offset.y;
      } else {
        
        offsetY = -top;
        offsetX = -left;
      }

      
      ctx.strokeRect(rect.left + offsetX, rect.top + offsetY, rect.width, rect.height);
    }
  }

  return canvas.toDataURL("image/jpeg", 0.5);
}











function saveDataURL(aDataURL, aFilename) {
  var frame = {}; Cu.import('resource://mozmill/modules/frame.js', frame);
  const FILE_PERMISSIONS = parseInt("0644", 8);

  var file;
  file = Cc['@mozilla.org/file/local;1']
         .createInstance(Ci.nsILocalFile);
  file.initWithPath(frame.persisted['screenshots']['path']);
  file.append(aFilename + ".jpg");
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, FILE_PERMISSIONS);

  
  let foStream = Cc["@mozilla.org/network/file-output-stream;1"]
                 .createInstance(Ci.nsIFileOutputStream);
  foStream.init(file, 0x02 | 0x08 | 0x10, FILE_PERMISSIONS, foStream.DEFER_OPEN);

  let dataURI = NetUtil.newURI(aDataURL, "UTF8", null);
  if (!dataURI.schemeIs("data")) {
    throw TypeError("aDataURL parameter has to have 'data'" +
                    " scheme instead of '" + dataURI.scheme + "'");
  }

  
  

  let ready = false;
  let failure = false;

  function sync(aStatus) {
    if (!Components.isSuccessCode(aStatus)) {
      failure = true;
    }
    ready = true;
  }

  NetUtil.asyncFetch(dataURI, function (aInputStream, aAsyncFetchResult) {
    if (!Components.isSuccessCode(aAsyncFetchResult)) {
        
        sync(aAsyncFetchResult);
    } else {
      
      NetUtil.asyncCopy(aInputStream, foStream, function (aAsyncCopyResult) {
        sync(aAsyncCopyResult);
      });
    }
  });

  assert.waitFor(function () {
    return ready;
  }, "DataURL has been saved to '" + file.path + "'");

  return {filename: file.path, failure: failure};
}






var gutility_mzmltimer = 0;





function startTimer(){
  dump("TIMERCHECK:: starting now: " + Date.now() + "\n");
  gutility_mzmltimer = Date.now();
}










function checkTimer(aMsg){
  var end = Date.now();
  dump("TIMERCHECK:: at " + aMsg + " is: " + (end - gutility_mzmltimer) + "\n");
}
