


MARIONETTE_TIMEOUT = 20000;

SpecialPowers.addPermission("mobileconnection", true, document);



let ifr = document.createElement("iframe");
let mobileConnection;
ifr.onload = function() {
  mobileConnection = ifr.contentWindow.navigator.mozMobileConnection;

  tasks.run();
};
document.body.appendChild(ifr);

let tasks = {
  
  
  _tasks: [],
  _nextTaskIndex: 0,

  push: function push(func) {
    this._tasks.push(func);
  },

  next: function next() {
    let index = this._nextTaskIndex++;
    let task = this._tasks[index];
    try {
      task();
    } catch (ex) {
      ok(false, "test task[" + index + "] throws: " + ex);
      
      if (index != this._tasks.length - 1) {
        this.abort();
      }
    }
  },

  abort: function abort() {
    this._tasks[this._tasks.length - 1]();
  },

  run: function run() {
    this.next();
  }
};

tasks.push(function verifyInitialState() {
  log("Verifying initial state.");

  ok(mobileConnection instanceof ifr.contentWindow.MozMobileConnection,
      "mobileConnection is instanceof " + mobileConnection.constructor);

  tasks.next();
});

tasks.push(function testGettingIMEI() {
  log("Test *#06# ...");

  let request = mobileConnection.sendMMI("*#06#");
  ok(request instanceof DOMRequest,
     "request is instanceof " + request.constructor);

  request.onsuccess = function onsuccess(event) {
    ok(true, "request success");
    is(typeof event.target.result, "object", "typeof result object");
    ok(event.target.result instanceof Object, "result instanceof Object");
    is(event.target.result.statusMessage, "000000000000000", "Emulator IMEI");
    is(event.target.result.serviceCode, "scImei", "Service code IMEI");
    is(event.target.result.additionalInformation, undefined,
       "No additional information");
    tasks.next();
  }
  request.onerror = function onerror() {
    ok(false, "request success");
    tasks.abort();
  };
});


tasks.push(function cleanUp() {
  SpecialPowers.removePermission("mobileconnection", document);
  finish();
});
