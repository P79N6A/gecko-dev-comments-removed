




var inChild = false;
var filePrefix = "";
try {
  inChild = Components.classes["@mozilla.org/xre/runtime;1"].
              getService(Components.interfaces.nsIXULRuntime).processType
              != Components.interfaces.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
  if (inChild) {
    
    filePrefix = "remoteopen";
  }
} 
catch (e) { }
