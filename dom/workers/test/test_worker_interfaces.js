























var ecmaGlobals =
  [
    "Array",
    "ArrayBuffer",
    "Boolean",
    "DataView",
    "Date",
    "Error",
    "EvalError",
    "Float32Array",
    "Float64Array",
    "Function",
    "Infinity",
    "Int16Array",
    "Int32Array",
    "Int8Array",
    "InternalError",
    {name: "Intl", android: false},
    "Iterator",
    "JSON",
    "Map",
    "Math",
    "NaN",
    "Number",
    "Object",
    "Proxy",
    "RangeError",
    "ReferenceError",
    "Reflect",
    "RegExp",
    "Set",
    {name: "SharedArrayBuffer", nightly: true},
    {name: "SharedInt8Array", nightly: true},
    {name: "SharedUint8Array", nightly: true},
    {name: "SharedUint8ClampedArray", nightly: true},
    {name: "SharedInt16Array", nightly: true},
    {name: "SharedUint16Array", nightly: true},
    {name: "SharedInt32Array", nightly: true},
    {name: "SharedUint32Array", nightly: true},
    {name: "SharedFloat32Array", nightly: true},
    {name: "SharedFloat64Array", nightly: true},
    {name: "SIMD", nightly: true},
    {name: "Atomics", nightly: true},
    "StopIteration",
    "String",
    "Symbol",
    "SyntaxError",
    {name: "TypedObject", nightly: true},
    "TypeError",
    "Uint16Array",
    "Uint32Array",
    "Uint8Array",
    "Uint8ClampedArray",
    "URIError",
    "WeakMap",
    "WeakSet",
  ];




var interfaceNamesInGlobalScope =
  [

    "Blob",

    "BroadcastChannel",

    { name: "Cache", release: false},

    { name: "CacheStorage", release: false},

    "DedicatedWorkerGlobalScope",

    { name: "DataStore", b2g: true },

    { name: "DataStoreCursor", b2g: true },

    "DOMCursor",

    "DOMError",

    "DOMException",

    "DOMRequest",

    "DOMStringList",

    "Event",

    "EventTarget",

    "File",

    "FileReaderSync",

    "FormData",

    "Headers",

    "IDBCursor",

    "IDBDatabase",

    "IDBFactory",

    "IDBIndex",

    "IDBKeyRange",

    "IDBObjectStore",

    "IDBOpenDBRequest",

    "IDBRequest",

    "IDBTransaction",

    "IDBVersionChangeEvent",

    "ImageData",

    "MessageChannel",

    "MessageEvent",

    "MessagePort",

    "Notification",

    "Performance",

    "PerformanceEntry",

    "PerformanceMark",

    "PerformanceMeasure",

    "Promise",

    "Request",

    "Response",

    { name: "ServiceWorkerRegistration", release: false, b2g: false },

    "TextDecoder",

    "TextEncoder",

    "XMLHttpRequest",

    "XMLHttpRequestEventTarget",

    "XMLHttpRequestUpload",

    "URL",

    "URLSearchParams",

    "WebSocket",

    "Worker",

    "WorkerGlobalScope",

    "WorkerLocation",

    "WorkerNavigator",

  ];


function createInterfaceMap(permissionMap, version, userAgent, isB2G) {
  var isNightly = version.endsWith("a1");
  var isRelease = !version.includes("a");
  var isDesktop = !/Mobile|Tablet/.test(userAgent);
  var isAndroid = !!navigator.userAgent.includes("Android");

  var interfaceMap = {};

  function addInterfaces(interfaces)
  {
    for (var entry of interfaces) {
      if (typeof(entry) === "string") {
        interfaceMap[entry] = true;
      } else {
        ok(!("pref" in entry), "Bogus pref annotation for " + entry.name);
        if ((entry.nightly === !isNightly) ||
            (entry.desktop === !isDesktop) ||
            (entry.android === !isAndroid) ||
            (entry.b2g === !isB2G) ||
            (entry.release === !isRelease) ||
            (entry.permission && !permissionMap[entry.permission]) ||
            entry.disabled) {
          interfaceMap[entry.name] = false;
        } else {
          interfaceMap[entry.name] = true;
        }
      }
    }
  }

  addInterfaces(ecmaGlobals);
  addInterfaces(interfaceNamesInGlobalScope);

  return interfaceMap;
}

function runTest(permissionMap, version, userAgent, isB2G) {
  var interfaceMap = createInterfaceMap(permissionMap, version, userAgent, isB2G);
  for (var name of Object.getOwnPropertyNames(self)) {
    
    if (!/^[A-Z]/.test(name)) {
      continue;
    }
    ok(interfaceMap[name],
       "If this is failing: DANGER, are you sure you want to expose the new interface " + name +
       " to all webpages as a property on the worker? Do not make a change to this file without a " +
       " review from a DOM peer for that specific change!!! (or a JS peer for changes to ecmaGlobals)");
    delete interfaceMap[name];
  }
  for (var name of Object.keys(interfaceMap)) {
    ok(name in self === interfaceMap[name],
       name + " should " + (interfaceMap[name] ? "" : " NOT") + " be defined on the global scope");
    if (!interfaceMap[name]) {
      delete interfaceMap[name];
    }
  }
  is(Object.keys(interfaceMap).length, 0,
     "The following interface(s) are not enumerated: " + Object.keys(interfaceMap).join(", "));
}

function appendPermissions(permissions, interfaces) {
  for (var entry of interfaces) {
    if (entry.permission !== undefined &&
        permissions.indexOf(entry.permission) === -1) {
      permissions.push(entry.permission);
    }
  }
}

var permissions = [];
appendPermissions(permissions, ecmaGlobals);
appendPermissions(permissions, interfaceNamesInGlobalScope);

workerTestGetPermissions(permissions, function(permissionMap) {
  workerTestGetVersion(function(version) {
    workerTestGetUserAgent(function(userAgent) {
      workerTestGetIsB2G(function(isB2G) {
        runTest(permissionMap, version, userAgent, isB2G);
        workerTestDone();
      });
    });
  });
});
