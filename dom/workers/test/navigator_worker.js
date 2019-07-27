





var supportedProps = [
  "appCodeName",
  "appName",
  "appVersion",
  { name: "getDataStores", b2g: true },
  "platform",
  "product",
  "userAgent",
  "onLine",
  "language",
  "languages",
];

self.onmessage = function(event) {
  if (!event || !event.data) {
    return;
  }

  startTest(event.data.isB2G);
};

function startTest(isB2G) {
  
  
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

    if (prop === "getDataStores") {
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
}
