














BaseClasses = require("study_base_classes.js");


exports.experimentInfo = {

  testName: "JavaScript memory usage",
  testId: 1717,
  testInfoUrl: "https://", 
  summary: "This study aims to explore how well JavaScript manages memory over time. " +
           "It will periodically collect data on the browser's JavaScript memory " +
           "reclamation performance.",
  thumbnail: "http://", 
  
  versionNumber: 1, 
    

  duration: 1, 
  minTPVersion: "1.0", 
    
  minFXVersion: "4.0b1", 
    

  
  recursAutomatically: false,
  recurrenceInterval: 60, 

  
  startDate: null, 
  optInRequired: false 
};

var GCInfoStructFields = [
  {name: 'appTime', kind: 'double', desc: 'Elapsed application time'},
  {name: 'gcTime', kind: 'double', desc: 'Total time to perform garbage collection'},
  {name: 'waitTime', kind: 'double', desc: 'GC start wait time'},
  {name: 'markTime', kind: 'double', desc: 'JS entity traversal time'},
  {name: 'sweepTime', kind: 'double', desc: 'JS entity destruction time'},
  {name: 'sweepObjTime', kind: 'double', desc: 'JS object destruction time'},
  {name: 'sweepStringTime', kind: 'double', desc: 'JS string destruction time'},
  {name: 'sweepShapeTime', kind: 'double', desc: 'JS shape destruction time'},
  {name: 'destroyTime', kind: 'double', desc: 'Heap arena destruction time'},
  {name: 'endTime', kind: 'double', desc: 'GC end wait time'},
  {name: 'isCompartmental', kind: 'bool', desc: 'GC is compartmental'},
];

const GCInfoStructFieldNames = [field.name for each (field in GCInfoStructFields)]

function makeCTypeProps(ctypes, fields) {
  const kindToCType = {
    double: ctypes.float64_t,
    bool: ctypes.bool,
  };
  let attrs = [];
  for each (field in fields) {
    let record = {};
    record[field.name] = kindToCType[field.kind];
    attrs.push(record);
  }
  return attrs;
}

function makeColumns(fields) {
  const kindToType = {
    double: BaseClasses.TYPE_DOUBLE,
    bool: BaseClasses.TYPE_INT_32,
  };
  let columns = [];
  for each (field in fields)
    columns.push({property: field.name, type: kindToType[field.kind], displayName: field.desc});
  return columns;
}

function zipObj(names, values) {
  let o = {};
  for (let i in names)
    o[names[i]] = values[i];
  return o;
}

exports.dataStoreInfo = {
  fileName: "testpilot_jsgc_results.sqlite",
  tableName: "jsgc_info",
  columns: makeColumns(GCInfoStructFields),
};

function GCGlobalObserver() {
  GCGlobalObserver.baseConstructor.call(this);
}

BaseClasses.extend(GCGlobalObserver, BaseClasses.GenericGlobalObserver);

GCGlobalObserver.prototype.onExperimentStartup = function(store) {
  GCGlobalObserver.superClass.onExperimentStartup.call(this, store);

  dump('Installing GC observer\n');
  try {
    this.gcObserver = GCObserver.install(store);
  } catch (e) {
    this.console.debug('Exception during install: ' + e);
  }
  dump('Done installing GC observer\n');
};

GCGlobalObserver.prototype.getStudyMetadata = function() {
    return [{droppedGCInfoCount: this.gcObserver.dropCount}];
};

GCGlobalObserver.prototype.onExperimentShutdown = function(store) {
  GCGlobalObserver.superClass.onExperimentShutdown.call(this, store);
  if (this.gcObserver)
    this.gcObserver.uninstall();
};


GCGlobalObserver.prototype.doExperimentCleanup = function() {
  
};


exports.handlers = new GCGlobalObserver();


function JSGCWebContent()  {
  JSGCWebContent.baseConstructor.call(this, exports.experimentInfo);
}

BaseClasses.extend(JSGCWebContent, BaseClasses.GenericWebContent);

JSGCWebContent.prototype.__defineGetter__("dataCanvas",
  function() {
      return '<div class="dataBox"><h3>View Your Data:</h3>' +
      this.dataViewExplanation +
      this.rawDataLink +
      '<div id="data-plot-div" style="width:480x;height:480px"></div>' +
      this.saveButtons + '</div>';
  });

JSGCWebContent.prototype.__defineGetter__("dataViewExplanation",
  function() {
    return "This plot shows the garbage collection duration during the course of the application's lifetime.";
  });


JSGCWebContent.prototype.onPageLoad = function(experiment,
                                                  document,
                                                  graphUtils) {
  experiment.getDataStoreAsJSON(function(rawData) {

    let compartmental = [];
    let full = [];
    for each (row in rawData) {
      let series = row.isCompartmental ? compartmental : full;
      series.push([row.appTime, row.gcTime]);
    }
    let div = document.getElementById('data-plot-div');
    graphUtils.plot(div,
      [
        {label: 'Compartmental GCs (ms)',
         data: compartmental,
         points: {show: true}},
        {label: 'Full GCs (ms)',
         data: full,
         points: {show: true}},
      ]
    );
  });
};


exports.webContent = new JSGCWebContent();

const GCINFO_READ_INTERVAL_SECONDS = 30;

var GCObserver = {
  alreadyInstalled : false,
  selfPingInterval: GCINFO_READ_INTERVAL_SECONDS * 1000,
  ctypes: null,
  libxul: null,
  runtime: null,
  GCInfoStruct: null,
  getGCInfo: null,
  droppedGCInfo: null,
  setGCInfoEnabled: null,
  wasGCInfoEnabled: null,
  store: null,
  dropCount: 0,

  install: function GCObserver__install(store) {
    this.debug('Installing GCObserver');
    if (this.alreadyInstalled)
      return;
    this.debug('Adding GCObserver');
    try {
      this.createCTypes();
    } catch (e) {
      this.debug('Failed to create ctypes: ' + e);
      return;
    }
    this.store = store;
    this.wasGCInfoEnabled = this.getGCInfoEnabled(this.runtime);
    this.setGCInfoEnabled(this.runtime, true);
    if (!this.getGCInfoEnabled(this.runtime)) {
        throw new Error('GC info could not be enabled');
    }
    
    this.debug('Successfully enabled');
    this.alreadyInstalled = true;
    this.debug('Initiating self-ping');
    this.selfPingTimer = Cc['@mozilla.org/timer;1'].createInstance(Ci.nsITimer);
    this.pingSelf();
    this.debug('Finished installing');
  },

  uninstall: function GCObserver__uninstall() {
    if (!this.alreadyInstalled)
      return;
    if (this.selfPingTimer)
      this.selfPingTimer.cancel();
    this.selfPingTimer = null;
    this.setGCInfoEnabled(this.wasGCInfoEnabled);
    this.alreadyInstalled = false;
  },

  debug: function GCObserver__debug(msg) {
    dump('//// ' + msg + '\n');
  },

  recordGCInfo: function GCObserver__alwaysRecord(fieldContents) {
    this.debug('Fields: ' + fieldContents.join(' '));
    this.store.storeEvent(zipObj(GCInfoStructFieldNames, fieldContents));
  },

  tryFindLibXUL: function GCObserver__tryFindLibXUL(ctypes) {
    try { return ctypes.open(ctypes.libraryName('xul')); } catch (e) {}
    try { return ctypes.open(ctypes.libraryName('mozjs')); } catch (e) {}
    throw new Error('could not find SpiderMonkey dll');
  },

  createCTypes: function GCObserver__createCTypes() {
    Components.utils.import('resource://gre/modules/ctypes.jsm');
    this.ctypes = ctypes;
    let libxul = this.tryFindLibXUL(ctypes);
    this.libxul = libxul;
    this.runtime = ctypes.getRuntime(ctypes.voidptr_t);
    let GCInfo = this.createGCInfoStruct();
    this.getGCInfo = libxul.declare('JS_GCInfoFront', ctypes.default_abi, GCInfo.ptr, ctypes.voidptr_t);
    this.popGCInfo = libxul.declare('JS_GCInfoPopFront', ctypes.default_abi, ctypes.bool, ctypes.voidptr_t);
    this.setGCInfoEnabled = libxul.declare('JS_SetGCInfoEnabled', ctypes.default_abi, ctypes.void_t, ctypes.voidptr_t, ctypes.bool);
    this.getGCInfoEnabled = libxul.declare('JS_GetGCInfoEnabled', ctypes.default_abi, ctypes.bool, ctypes.voidptr_t);
  },

  createGCInfoStruct: function GCObserver__createGCInfoStruct() {
    let ctypes = this.ctypes;
    let GCInfoStruct = this.GCInfoStruct = new ctypes.StructType('GCInfo', makeCTypeProps(ctypes, GCInfoStructFields));
    GCInfoStruct.ptr = new ctypes.PointerType(GCInfoStruct);
    return GCInfoStruct;
  },

  pingSelf: function GCObserver__pingSelf() {
    let self = this;
    const TYPE_ONE_SHOT = 0;
    const TYPE_REPEATING_SLACK = 1;
    this.selfPingTimer.initWithCallback(function dumpAndRepeat() {
      self.dumpGCInfos();
      self.pingSelf();
    }, this.selfPingInterval, TYPE_ONE_SHOT);
  },

  dumpGCInfos: function GCObserver__dumpGCInfos() {
    let self = this;
    let debug = this.debug;

    let rt = this.runtime;

    debug('Dumping GC infos');
    for (var i = 0;; i++) {
      debug('Getting GCInfo');
      let info = this.getGCInfo(rt);
      debug('GCInfo: ' + info);

      if (info.isNull()) {
        debug('Breaking due to null');
        break;
      }

      
      let c = info.contents;
      let data = [c[name] for each (name in GCInfoStructFieldNames)]

      let dropped = this.popGCInfo(rt);
      if (dropped) {
        debug('Dropped!');
        ++this.dropCount;
        break;
      }

      self.recordGCInfo(data);
    }
    debug ('Saw ' + i + ' entries');
  }
};
