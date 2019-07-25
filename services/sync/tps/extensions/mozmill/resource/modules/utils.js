






































var EXPORTED_SYMBOLS = ["openFile", "saveFile", "saveAsFile", "genBoiler", 
                        "getFile", "Copy", "getChromeWindow", "getWindows", "runEditor",
                        "runFile", "getWindowByTitle", "getWindowByType", "tempfile", 
                        "getMethodInWindows", "getPreference", "setPreference",
                        "sleep", "assert", "unwrapNode", "TimeoutError", "waitFor",
                        "takeScreenshot",
                       ];

var hwindow = Components.classes["@mozilla.org/appshell/appShellService;1"]
              .getService(Components.interfaces.nsIAppShellService)
              .hiddenDOMWindow;

var uuidgen = Components.classes["@mozilla.org/uuid-generator;1"]
    .getService(Components.interfaces.nsIUUIDGenerator);

function Copy (obj) {
  for (var n in obj) {
    this[n] = obj[n];
  }
}

function getChromeWindow(aWindow) {
  var chromeWin = aWindow
           .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
           .getInterface(Components.interfaces.nsIWebNavigation)
           .QueryInterface(Components.interfaces.nsIDocShellTreeItem)
           .rootTreeItem
           .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
           .getInterface(Components.interfaces.nsIDOMWindow)
           .QueryInterface(Components.interfaces.nsIDOMChromeWindow);
  return chromeWin;
}

function getWindows(type) {
  if (type == undefined) {
      type = "";
  }
  var windows = []
  var enumerator = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator)
                     .getEnumerator(type);
  while(enumerator.hasMoreElements()) {
    windows.push(enumerator.getNext());
  }
  if (type == "") {
    windows.push(hwindow);
  }
  return windows;
}

function getMethodInWindows (methodName) {
  for each(w in getWindows()) {
    if (w[methodName] != undefined) {
      return w[methodName];
    }
  }
  throw new Error("Method with name: '" + methodName + "' is not in any open window.");
}

function getWindowByTitle(title) {
  for each(w in getWindows()) {
    if (w.document.title && w.document.title == title) {
      return w;
    }
  }
}

function getWindowByType(type) {
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
           .getService(Components.interfaces.nsIWindowMediator);
  return wm.getMostRecentWindow(type);
}

function tempfile(appention) {
  if (appention == undefined) {
    var appention = "mozmill.utils.tempfile"
  }
	var tempfile = Components.classes["@mozilla.org/file/directory_service;1"].getService(Components.interfaces.nsIProperties).get("TmpD", Components.interfaces.nsIFile);
	tempfile.append(uuidgen.generateUUID().toString().replace('-', '').replace('{', '').replace('}',''))
	tempfile.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0777);
	tempfile.append(appention);
	tempfile.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, 0666);
	
	return tempfile.clone()
}

var checkChrome = function() {
   var loc = window.document.location.href;
   try {
       loc = window.top.document.location.href;
   } catch (e) {}

   if (/^chrome:\/\//.test(loc)) { return true; } 
   else { return false; }
}

 
 var runFile = function(w){
   
   var nsIFilePicker = Components.interfaces.nsIFilePicker;
   var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
   
   fp.init(w, "Select a File", nsIFilePicker.modeOpen);
   fp.appendFilter("JavaScript Files","*.js");
   
   var res = fp.show();
   
   if (res == nsIFilePicker.returnOK){
     var thefile = fp.file;
     
     var paramObj = {};
     paramObj.files = [];
     paramObj.files.push(thefile.path);
   }
 };
 
 var saveFile = function(w, content, filename){
   
   var file = Components.classes["@mozilla.org/file/local;1"]
                        .createInstance(Components.interfaces.nsILocalFile);
   
   file.initWithPath(filename);
   
   
   var foStream = Components.classes["@mozilla.org/network/file-output-stream;1"]
                            .createInstance(Components.interfaces.nsIFileOutputStream);

   
   foStream.init(file, 0x02 | 0x08 | 0x20, 0666, 0); 
   
   
   
   
   foStream.write(content, content.length);
   foStream.close();
 };
 
  var saveAsFile = function(w, content){
     
     var nsIFilePicker = Components.interfaces.nsIFilePicker;
     var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
     
     fp.init(w, "Select a File", nsIFilePicker.modeSave);
     fp.appendFilter("JavaScript Files","*.js");
     
     var res = fp.show();
     
     if ((res == nsIFilePicker.returnOK) || (res == nsIFilePicker.returnReplace)){
       var thefile = fp.file;
              
       
       if (thefile.path.indexOf(".js") == -1){
         
         var file = Components.classes["@mozilla.org/file/local;1"]
                              .createInstance(Components.interfaces.nsILocalFile);
         
         file.initWithPath(thefile.path+".js");
         var thefile = file;
       }
       
       
       var foStream = Components.classes["@mozilla.org/network/file-output-stream;1"]
                               .createInstance(Components.interfaces.nsIFileOutputStream);

       
       foStream.init(thefile, 0x02 | 0x08 | 0x20, 0666, 0); 
       
       
       
       foStream.write(content, content.length);
       foStream.close();
       return thefile.path;
     }
  };
  
 var openFile = function(w){
    
    var nsIFilePicker = Components.interfaces.nsIFilePicker;
    var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
    
    fp.init(w, "Select a File", nsIFilePicker.modeOpen);
    fp.appendFilter("JavaScript Files","*.js");
    
    var res = fp.show();
    
    if (res == nsIFilePicker.returnOK){
      var thefile = fp.file;
      
      var data = getFile(thefile.path);

      return {path:thefile.path, data:data};
    }
  };
  
 var getFile = function(path){
   
   var file = Components.classes["@mozilla.org/file/local;1"]
                        .createInstance(Components.interfaces.nsILocalFile);
   
   file.initWithPath(path);
   
   var data = "";
   var fstream = Components.classes["@mozilla.org/network/file-input-stream;1"]
                           .createInstance(Components.interfaces.nsIFileInputStream);
   var sstream = Components.classes["@mozilla.org/scriptableinputstream;1"]
                           .createInstance(Components.interfaces.nsIScriptableInputStream);
   fstream.init(file, -1, 0, 0);
   sstream.init(fstream); 

   
   var str = sstream.read(4096);
   while (str.length > 0) {
     data += str;
     str = sstream.read(4096);
   }

   sstream.close();
   fstream.close();

   
   return data;
 };
 











function getPreference(aPrefName, aDefaultValue) {
  try {
    var branch = Components.classes["@mozilla.org/preferences-service;1"].
                 getService(Components.interfaces.nsIPrefBranch);
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
  } catch(e) {
    return aDefaultValue;
  }
}












function setPreference(aName, aValue) {
  try {
    var branch = Components.classes["@mozilla.org/preferences-service;1"].
                 getService(Components.interfaces.nsIPrefBranch);
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
  } catch(e) {
    return false;
  }

  return true;
}







function sleep(milliseconds) {
  
  var timeup = false;
  function wait() { timeup = true; }
  hwindow.setTimeout(wait, milliseconds);

  var thread = Components.classes["@mozilla.org/thread-manager;1"].
               getService().currentThread;
  while(!timeup) {
    thread.processNextEvent(true);
  }
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






function TimeoutError(message, fileName, lineNumber) {
  var err = new Error();
  if (err.stack) {
    this.stack = err.stack;
  }
  this.message = message === undefined ? err.message : message;
  this.fileName = fileName === undefined ? err.fileName : fileName;
  this.lineNumber = lineNumber === undefined ? err.lineNumber : lineNumber;
};
TimeoutError.prototype = new Error();
TimeoutError.prototype.constructor = TimeoutError;
TimeoutError.prototype.name = 'TimeoutError';




function waitFor(callback, message, timeout, interval, thisObject) {
  timeout = timeout || 5000;
  interval = interval || 100;

  var self = {counter: 0, result: callback.call(thisObject)};

  function wait() {
    self.counter += interval;
    self.result = callback.call(thisObject);
  }

  var timeoutInterval = hwindow.setInterval(wait, interval);
  var thread = Components.classes["@mozilla.org/thread-manager;1"].
               getService().currentThread;

  while((self.result != true) && (self.counter < timeout))  {
    thread.processNextEvent(true);
  }

  hwindow.clearInterval(timeoutInterval);

  if (self.counter >= timeout) {
    message = message || arguments.callee.name + ": Timeout exceeded for '" + callback + "'";
    throw new TimeoutError(message);
  }

  return true;
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




function takeScreenshot(node, name, highlights) {
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

  
  if (name) {
    return saveCanvas(canvas, name);
  } 
  return canvas.toDataURL("image/png","");
}





function saveCanvas(canvas, name) {
  var file = Components.classes["@mozilla.org/file/directory_service;1"]
                                .getService(Components.interfaces.nsIProperties)
                                .get("TmpD", Components.interfaces.nsIFile);
  file.append("mozmill_screens");
  file.append(name + ".png");
  file.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, 0666);

  
  var io = Components.classes["@mozilla.org/network/io-service;1"]
                              .getService(Components.interfaces.nsIIOService);
  var source = io.newURI(canvas.toDataURL("image/png", ""), "UTF8", null);
  var target = io.newFileURI(file)
 
  
  var persist = Components.classes["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"]
                                   .createInstance(Components.interfaces.nsIWebBrowserPersist);

  persist.persistFlags = Components.interfaces.nsIWebBrowserPersist.PERSIST_FLAGS_REPLACE_EXISTING_FILES;
  persist.persistFlags |= Components.interfaces.nsIWebBrowserPersist.PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION;
 
  
  persist.saveURI(source, null, null, null, null, file);

  return file.path;
}
