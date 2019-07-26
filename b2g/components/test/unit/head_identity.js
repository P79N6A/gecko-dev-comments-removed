


const Ci = Components.interfaces;
const Cu = Components.utils;




Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "MinimalIDService",
                                  "resource://gre/modules/identity/MinimalIdentity.jsm",
                                  "IdentityService");

XPCOMUtils.defineLazyModuleGetter(this,
                                  "Logger",
                                  "resource://gre/modules/identity/LogUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this,
                                   "uuidGenerator",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

const TEST_URL = "https://myfavoriteflan.com";
const TEST_USER = "uumellmahaye1969@hotmail.com";
const TEST_PRIVKEY = "i-am-a-secret";
const TEST_CERT = "i~like~pie";



function log(...aMessageArgs) {
  Logger.log.apply(Logger, ["test"].concat(aMessageArgs));
}

function partial(fn) {
  let args = Array.prototype.slice.call(arguments, 1);
  return function() {
    return fn.apply(this, args.concat(Array.prototype.slice.call(arguments)));
  };
}

function uuid() {
  return uuidGenerator.generateUUID().toString();
}



function mockDoc(aIdentity, aOrigin, aDoFunc) {
  let mockedDoc = {};
  mockedDoc.id = uuid();
  mockedDoc.loggedInEmail = aIdentity;
  mockedDoc.origin = aOrigin;
  mockedDoc['do'] = aDoFunc;
  mockedDoc.doReady = partial(aDoFunc, 'ready');
  mockedDoc.doLogin = partial(aDoFunc, 'login');
  mockedDoc.doLogout = partial(aDoFunc, 'logout');
  mockedDoc.doError = partial(aDoFunc, 'error');
  mockedDoc.doCancel = partial(aDoFunc, 'cancel');
  mockedDoc.doCoffee = partial(aDoFunc, 'coffee');

  return mockedDoc;
}





function mockPipe() {
  let MockedPipe = {
    communicate: function(aRpOptions, aGaiaOptions, aMessageCallback) {
      switch (aGaiaOptions.message) {
        case "identity-delegate-watch":
          aMessageCallback({json: {method: "ready"}});
          break;
        case "identity-delegate-request":
          aMessageCallback({json: {method: "login", assertion: TEST_CERT}});
          break;
        case "identity-delegate-logout":
          aMessageCallback({json: {method: "logout"}});
          break;
        default:
          throw("what the what?? " + aGaiaOptions.message);
          break;
      }
    }
  };
  return MockedPipe;
}




function makeObserver(aObserveTopic, aObserveFunc) {
  let observer = {
    
    
    QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports, Ci.nsIObserver]),

    observe: function (aSubject, aTopic, aData) {
      if (aTopic == aObserveTopic) {
        Services.obs.removeObserver(observer, aObserveTopic);
        aObserveFunc(aSubject, aTopic, aData);
      }
    }
  };

  Services.obs.addObserver(observer, aObserveTopic, false);
}



function setup_test_identity(identity, cert, cb) {
  cb();
}




function call_sequentially() {
  let numCalls = 0;
  let funcs = arguments;

  return function() {
    if (!funcs[numCalls]) {
      let argString = Array.prototype.slice.call(arguments).join(",");
      do_throw("Too many calls: " + argString);
      return;
    }
    funcs[numCalls].apply(funcs[numCalls],arguments);
    numCalls += 1;
  };
}
