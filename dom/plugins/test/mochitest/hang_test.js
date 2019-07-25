
Components.utils.import("resource://gre/modules/KeyValueParser.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;

var success = false;
var observerFired = false;

var testObserver = {
  idleHang: true,

  observe: function(subject, topic, data) {
    observerFired = true;
    ok(true, "Observer fired");
    is(topic, "plugin-crashed", "Checking correct topic");
    is(data,  null, "Checking null data");
    ok((subject instanceof Ci.nsIPropertyBag2), "got Propbag");
    ok((subject instanceof Ci.nsIWritablePropertyBag2), "got writable Propbag");

    var pluginId = subject.getPropertyAsAString("pluginDumpID");
    isnot(pluginId, "", "got a non-empty plugin crash id");

    
    let directoryService =
      Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
    let profD = directoryService.get("ProfD", Ci.nsIFile);
    profD.append("minidumps");
    let pluginDumpFile = profD.clone();
    pluginDumpFile.append(pluginId + ".dmp");
    ok(pluginDumpFile.exists(), "plugin minidump exists");

    let pluginExtraFile = profD.clone();
    pluginExtraFile.append(pluginId + ".extra");
    ok(pluginExtraFile.exists(), "plugin extra file exists");

    let extraData = parseKeyValuePairsFromFile(pluginExtraFile);

    

    ok("additional_minidumps" in extraData, "got field for additional minidumps");
    let additionalDumps = extraData.additional_minidumps.split(',');
    ok(additionalDumps.indexOf('browser') >= 0, "browser in additional_minidumps");

    let additionalDumpFiles = [];
    for (let name of additionalDumps) {
      let file = profD.clone();
      file.append(pluginId + "-" + name + ".dmp");
      ok(file.exists(), "additional dump '"+name+"' exists");
      if (file.exists()) {
        additionalDumpFiles.push(file);
      }
    }

    

    ok("PluginCpuUsage" in extraData, "got extra field for plugin cpu usage");
    let cpuUsage = parseFloat(extraData["PluginCpuUsage"]);
    if (this.idleHang) {
      ok(cpuUsage == 0, "plugin cpu usage is 0%");
    } else {
      ok(cpuUsage > 0, "plugin cpu usage is >0%");
    }

    
    ok("NumberOfProcessors" in extraData, "got extra field for processor count");
    ok(parseInt(extraData["NumberOfProcessors"]) > 0, "number of processors is >0");

    
    pluginDumpFile.remove(false);
    pluginExtraFile.remove(false);
    for (let file of additionalDumpFiles) {
      file.remove(false);
    }
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIObserver) ||
        iid.equals(Ci.nsISupportsWeakReference) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Components.results.NS_NOINTERFACE;
  }
};


function onPluginCrashed(aEvent) {
  ok(true, "Plugin crashed notification received");
  ok(observerFired, "Observer should have fired first");
  is(aEvent.type, "PluginCrashed", "event is correct type");

  var pluginElement = document.getElementById("plugin1");
  is (pluginElement, aEvent.target, "Plugin crashed event target is plugin element");

  ok(aEvent instanceof Ci.nsIDOMDataContainerEvent,
     "plugin crashed event has the right interface");

  var minidumpID = aEvent.getData("minidumpID");
  isnot(minidumpID, "", "got a non-empty dump ID");
  var pluginName = aEvent.getData("pluginName");
  is(pluginName, "Test Plug-in", "got correct plugin name");
  var pluginFilename = aEvent.getData("pluginFilename");
  isnot(pluginFilename, "", "got a non-empty filename");
  var didReport = aEvent.getData("submittedCrashReport");
  
  
  ok((didReport == true || didReport == false), "event said crash report was submitted");

  var os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.removeObserver(testObserver, "plugin-crashed");

  SimpleTest.finish();
}
