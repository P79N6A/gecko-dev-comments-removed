


"use strict";

const { Cu, Ci, Cc } = require("chrome");
const { Class } = require("sdk/core/heritage");
const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const { defer, resolve } = require("sdk/core/promise");
const { Services } = Cu.import("resource://gre/modules/Services.jsm", {});

Cu.import("resource://gre/modules/Task.jsm");

loader.lazyRequireGetter(this, "HarCollector", "devtools/netmonitor/har/har-collector", true);
loader.lazyRequireGetter(this, "HarExporter", "devtools/netmonitor/har/har-exporter", true);
loader.lazyRequireGetter(this, "HarUtils", "devtools/netmonitor/har/har-utils", true);

const prefDomain = "devtools.netmonitor.har.";


const trace = {
  log: function(...args) {
  }
}















var HarAutomation = Class({
  

  initialize: function(toolbox) {
    this.toolbox = toolbox;

    let target = toolbox.target;
    target.makeRemote().then(() => {
      this.startMonitoring(target.client, target.form);
    });
  },

  destroy: function() {
    if (this.collector) {
      this.collector.stop();
    }

    if (this.tabWatcher) {
      this.tabWatcher.disconnect();
    }
  },

  

  startMonitoring: function(client, tabGrip, callback) {
    if (!client) {
      return;
    }

    if (!tabGrip) {
      return;
    }

    this.debuggerClient = client;
    this.tabClient = this.toolbox.target.activeTab;
    this.webConsoleClient = this.toolbox.target.activeConsole;

    let netPrefs = { "NetworkMonitor.saveRequestAndResponseBodies": true };
    this.webConsoleClient.setPreferences(netPrefs, () => {
      this.tabWatcher = new TabWatcher(this.toolbox, this);
      this.tabWatcher.connect();
    });
  },

  pageLoadBegin: function(aResponse) {
    this.resetCollector();
  },

  resetCollector: function() {
    if (this.collector) {
      this.collector.stop();
    }

    
    
    this.collector = new HarCollector({
      collector: this,
      webConsoleClient: this.webConsoleClient,
      debuggerClient: this.debuggerClient
    });

    this.collector.start();
  },

  









  pageLoadDone: function(aResponse) {
    trace.log("HarAutomation.pageLoadDone; ", aResponse);

    if (this.collector) {
      this.collector.waitForHarLoad().then(collector => {
        return this.autoExport();
      });
    }
  },

  autoExport: function() {
    let autoExport = Services.prefs.getBoolPref(prefDomain +
      "enableAutoExportToFile");

    if (!autoExport) {
      return resolve();
    }

    
    
    let data = {
      fileName: Services.prefs.getCharPref(prefDomain + "defaultFileName"),
    }

    return this.executeExport(data);
  },

  

  


  triggerExport: function(data) {
    if (!data.fileName) {
      data.fileName = Services.prefs.getCharPref(prefDomain +
        "defaultFileName");
    }

    this.executeExport(data);
  },

  


  clear: function() {
    this.resetCollector();
  },

  

  



  executeExport: function(data) {
    let items = this.collector.getItems();
    let form = this.toolbox.target.form;
    let title = form.title || form.url;

    let options = {
      getString: this.getString.bind(this),
      view: this,
      items: items,
    }

    options.defaultFileName = data.fileName;
    options.compress = data.compress;
    options.title = data.title || title;
    options.id = data.id;
    options.jsonp = data.jsonp;
    options.includeResponseBodies = data.includeResponseBodies;
    options.jsonpCallback = data.jsonpCallback;
    options.forceExport = data.forceExport;

    trace.log("HarAutomation.executeExport; " + data.fileName, options);

    return HarExporter.fetchHarData(options).then(jsonString => {
      
      if (jsonString && options.defaultFileName) {
        let file = getDefaultTargetFile(options);
        if (file) {
          HarUtils.saveToFile(file, jsonString, options.compress);
        }
      }

      return jsonString;
    });
  },

  

  










  getString: function(aStringGrip) {
    
    if (typeof aStringGrip != "object" || aStringGrip.type != "longString") {
      return resolve(aStringGrip); 
    }
    
    if (aStringGrip._fullText) {
      return aStringGrip._fullText.promise;
    }

    let deferred = aStringGrip._fullText = defer();
    let { actor, initial, length } = aStringGrip;
    let longStringClient = this.webConsoleClient.longString(aStringGrip);

    longStringClient.substring(initial.length, length, aResponse => {
      if (aResponse.error) {
        Cu.reportError(aResponse.error + ": " + aResponse.message);
        deferred.reject(aResponse);
        return;
      }
      deferred.resolve(initial + aResponse.substring);
    });

    return deferred.promise;
  },

  












  _getFormDataSections: Task.async(function*(aHeaders, aUploadHeaders, aPostData) {
    let formDataSections = [];

    let { headers: requestHeaders } = aHeaders;
    let { headers: payloadHeaders } = aUploadHeaders;
    let allHeaders = [...payloadHeaders, ...requestHeaders];

    let contentTypeHeader = allHeaders.find(e => e.name.toLowerCase() == "content-type");
    let contentTypeLongString = contentTypeHeader ? contentTypeHeader.value : "";
    let contentType = yield this.getString(contentTypeLongString);

    if (contentType.includes("x-www-form-urlencoded")) {
      let postDataLongString = aPostData.postData.text;
      let postData = yield this.getString(postDataLongString);

      for (let section of postData.split(/\r\n|\r|\n/)) {
        
        
        if (payloadHeaders.every(header => !section.startsWith(header.name))) {
          formDataSections.push(section);
        }
      }
    }

    return formDataSections;
  }),
});



function TabWatcher(toolbox, listener) {
  this.target = toolbox.target;
  this.listener = listener;

  this.onTabNavigated = this.onTabNavigated.bind(this);
}

TabWatcher.prototype = {
  

  connect: function() {
    this.target.on("navigate", this.onTabNavigated);
    this.target.on("will-navigate", this.onTabNavigated);
  },

  disconnect: function() {
    if (!this.target) {
      return;
    }

    this.target.off("navigate", this.onTabNavigated);
    this.target.off("will-navigate", this.onTabNavigated);
  },

  

  







  onTabNavigated: function(aType, aPacket) {
    switch (aType) {
      case "will-navigate": {
        this.listener.pageLoadBegin(aPacket);
        break;
      }
      case "navigate": {
        this.listener.pageLoadDone(aPacket);
        break;
      }
    }
  },
};






function getDefaultTargetFile(options) {
  let path = options.defaultLogDir ||
    Services.prefs.getCharPref("devtools.netmonitor.har.defaultLogDir");
  let folder = HarUtils.getLocalDirectory(path);
  let fileName = HarUtils.getHarFileName(options.defaultFileName,
    options.jsonp, options.compress);

  folder.append(fileName);
  folder.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, parseInt("0666", 8));

  return folder;
}


exports.HarAutomation = HarAutomation;
