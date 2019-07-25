






































var EXPORTED_SYMBOLS = ["openFile", "saveFile", "saveAsFile", "genBoiler", 
                        "getFile", "Copy", "getChromeWindow", "getWindows", "runEditor",
                        "runFile", "getWindowByTitle", "getWindowByType", "tempfile", 
                        "getMethodInWindows", "getPreference", "setPreference",
                        "sleep"];

var hwindow = Components.classes["@mozilla.org/appshell/appShellService;1"]
              .getService(Components.interfaces.nsIAppShellService)
              .hiddenDOMWindow;




var uuidgen = Components.classes["@mozilla.org/uuid-generator;1"]
    .getService(Components.interfaces.nsIUUIDGenerator);

function Copy (obj) {
  for (n in obj) {
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
































 
 var genBoiler = function(w){
   var value = "var setupModule = function(module) {\n" +
   "  module.controller = mozmill.getBrowserController();\n" +
   "}\n" +
   "\n" +
   "var testFoo = function(){\n" +
   "  controller.open('http://www.google.com');\n" +
   "}\n"
   w.editAreaLoader.setValue('editorInput', value);
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
 
 var runEditor = function(w){
   var data = w.document.getElementById('editorInput').value;
   
   
   
   
 };

 var saveFile = function(w){
   
   var file = Components.classes["@mozilla.org/file/local;1"]
                        .createInstance(Components.interfaces.nsILocalFile);
   
   file.initWithPath(w.openFn);
   
   
   var foStream = Components.classes["@mozilla.org/network/file-output-stream;1"]
                            .createInstance(Components.interfaces.nsIFileOutputStream);

   
   foStream.init(file, 0x02 | 0x08 | 0x20, 0666, 0); 
   
   
   
   var data = w.editAreaLoader.getValue('editorInput');
   
   foStream.write(data, data.length);
   foStream.close();
 };
 
  var saveAsFile = function(w){
     
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
       
       
       
       var data = w.editAreaLoader.getValue('editorInput');
       foStream.write(data, data.length);
       foStream.close();
       w.editAreaLoader.closeFile('editorInput', w.openFn);
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




function sleep (milliseconds) {
  var self = {};

  
  function wait() {
    self.timeup = true;
  }

  
  var interval = hwindow.setInterval(wait, milliseconds);

  var thread = Components.classes["@mozilla.org/thread-manager;1"]
            .getService()
            .currentThread;
  
  
  
  while(!self.timeup)
    thread.processNextEvent(true);
  hwindow.clearInterval(interval);

  return true;
}


 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
