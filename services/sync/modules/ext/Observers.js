




































let EXPORTED_SYMBOLS = ["Observers"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let Observers = {
  add: function(callback, topic) {
    let observer = new Observer(callback);
    if (!(topic in Observers._observers))
      Observers._observers[topic] = {};
    Observers._observers[topic][callback] = observer;
    Observers._service.addObserver(observer, topic, true);
    return observer;
  },

  remove: function(callback, topic) {
    let observer = Observers._observers[topic][callback];
    if (observer) {
      Observers._service.removeObserver(observer, topic);
      delete this._observers[topic][callback];
    }
  },

  notify: function(subject, topic, data) {
    Observers._service.notifyObservers(new Subject(subject), topic, data);
  },

  _service: Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService),

  
  
  _observers: {}
};


function Observer(callback) {
  this._callback = callback;
}

Observer.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupportsWeakReference]),
  observe: function(subject, topic, data) {
    
    
    
    let unwrappedSubject = subject.wrappedJSObject || subject;

    if (typeof this._callback == "function")
      this._callback(unwrappedSubject, topic, data);
    else
      this._callback.observe(unwrappedSubject, topic, data);
  }
}


function Subject(object) {
  this.wrappedJSObject = object;
}

Subject.prototype = {
  QueryInterface: XPCOMUtils.generateQI([]),
  getHelperForLanguage: function() {},
  getInterfaces: function() {}
};
