




































const EXPORTED_SYMBOLS = ["ThreadedCrypto"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://services-sync/ext/Sync.js");
Cu.import("resource://services-crypto/WeaveCrypto.js");




function Runner(func, thisObj, returnval, error) {
  this.func = func;
  this.thisObj = thisObj;
  this.returnval = returnval;
  this.error = error;
}
Runner.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRunnable]),

  run: function run() {
    let ex = this.error;
    if (ex) {
      this.func.throw(ex);
    } else {
      this.func.call(this.thisObj, this.returnval);
    }
  }
};





function CallbackRunner(func, thisObj, args, callback, cbThread) {
  this.func = func;
  this.thisObj = thisObj;
  this.args = args;
  this.callback = callback;
  this.cbThread = cbThread;
}
CallbackRunner.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRunnable]),

  run: function run() {
    let returnval, error;
    try {
      returnval = this.func.apply(this.thisObj, this.args);
    } catch(ex) {
      error = ex;
    }
    this.cbThread.dispatch(new Runner(this.callback, this.thisObj,
                                      returnval, error),
                           Ci.nsIThread.DISPATCH_NORMAL);
  }
};





function ThreadedCrypto() {
  this.backgroundThread = Services.tm.newThread(0);
  this.crypto = new WeaveCrypto();

  
  this.crypto.makeException = function makeException(message, result) {
    return result;
  };

  
  Services.obs.addObserver(this, "profile-before-change", true);
}
ThreadedCrypto.deferToThread = function deferToThread(methodname) {
  return function threadMethod() {
    
    let args = Array.slice(arguments);
    return Sync(function(callback) {
      let runner = new CallbackRunner(this.crypto[methodname], this.crypto,
                                      args, callback, Services.tm.mainThread);
      this.backgroundThread.dispatch(runner, Ci.nsIThread.DISPATCH_NORMAL);
    }, this)();
  };
};
ThreadedCrypto.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.IWeaveCrypto,
                                         Ci.nsISupportsWeakReference]),

  observe: function observe() {
    this.backgroundThread.shutdown();
  },

  get algorithm() this.crypto.algorithm,
  set algorithm(value) this.crypto.algorithm = value,

  get keypairBits() this.crypto.keypairBits,
  set keypairBits(value) this.crypto.keypairBits = value,

  encrypt: ThreadedCrypto.deferToThread("encrypt"),
  decrypt: ThreadedCrypto.deferToThread("decrypt"),
  generateKeypair: ThreadedCrypto.deferToThread("generateKeypair"),
  generateRandomKey: ThreadedCrypto.deferToThread("generateRandomKey"),
  generateRandomIV: ThreadedCrypto.deferToThread("generateRandomIV"),
  generateRandomBytes: ThreadedCrypto.deferToThread("generateRandomBytes"),
  wrapSymmetricKey: ThreadedCrypto.deferToThread("wrapSymmetricKey"),
  unwrapSymmetricKey: ThreadedCrypto.deferToThread("unwrapSymmetricKey"),
  rewrapPrivateKey: ThreadedCrypto.deferToThread("rewrapPrivateKey"),
  verifyPassphrase: ThreadedCrypto.deferToThread("verifyPassphrase")
};
