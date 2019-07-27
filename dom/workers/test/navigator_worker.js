





var supportedProps = [
  "appCodeName",
  "appName",
  "appVersion",
  { name: "getDataStores", b2g: true },
  "platform",
  "product",
  "taintEnabled",
  "userAgent",
  "onLine",
  "language",
  "languages",
];

var isDesktop = !/Mobile|Tablet/.test(navigator.userAgent);
var isB2G = !isDesktop && !navigator.userAgent.contains("Android");



var interfaceMap = {};

for (var prop of supportedProps) {
  if (typeof(prop) === "string") {
    interfaceMap[prop] = true;
    continue;
  }

  if (prop.b2g === !isB2G) {
    interfaceMap[prop.name] = false;
    continue;
  }

  interfaceMap[prop.name] = true;
}

for (var prop in navigator) {
  
  if (!interfaceMap[prop]) {
    throw "Navigator has the '" + prop + "' property that isn't in the list!";
  }
}

var obj;

for (var prop in interfaceMap) {
  
  if (!interfaceMap[prop]) {
    continue;
  }

  if (typeof navigator[prop] == "undefined") {
    throw "Navigator has no '" + prop + "' property!";
  }

  obj = { name:  prop };

  if (prop === "taintEnabled") {
    obj.value = navigator[prop]();
  } else if (prop === "getDataStores") {
    obj.value = typeof navigator[prop];
  } else {
    obj.value = navigator[prop];
  }

  postMessage(JSON.stringify(obj));
}

obj = {
  name: "testFinished"
};

postMessage(JSON.stringify(obj));
