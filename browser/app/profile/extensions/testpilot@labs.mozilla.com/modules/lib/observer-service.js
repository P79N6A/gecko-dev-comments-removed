





































var jsm = {}; Cu.import("resource://gre/modules/XPCOMUtils.jsm", jsm);
var XPCOMUtils = jsm.XPCOMUtils;








var service = Cc["@mozilla.org/observer-service;1"].
              getService(Ci.nsIObserverService);






var cache = [];
















var add = exports.add = function add(topic, callback, thisObject) {
  var observer = new Observer(topic, callback, thisObject);
  service.addObserver(observer, topic, true);
  cache.push(observer);

  return observer;
};













var remove = exports.remove = function remove(topic, callback, thisObject) {
  
  
  
  
  var [observer] = cache.filter(function(v) {
                                  return (v.topic      == topic    &&
                                          v.callback   == callback &&
                                          v.thisObject == thisObject);
                                });
  if (observer) {
    service.removeObserver(observer, topic);
    cache.splice(cache.indexOf(observer), 1);
  }
};

















var notify = exports.notify = function notify(topic, subject, data) {
  subject = (typeof subject == "undefined") ? null : new Subject(subject);
  data = (typeof    data == "undefined") ? null : data;
  service.notifyObservers(subject, topic, data);
};

function Observer(topic, callback, thisObject) {
  this.topic = topic;
  this.callback = callback;
  this.thisObject = thisObject;
}

Observer.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),
  observe: function(subject, topic, data) {
    
    
    
    
    if (subject && typeof subject == "object" &&
        ("wrappedJSObject" in subject) &&
        ("observersModuleSubjectWrapper" in subject.wrappedJSObject))
      subject = subject.wrappedJSObject.object;

    try {
      if (typeof this.callback == "function") {
        if (this.thisObject)
          this.callback.call(this.thisObject, subject, data);
        else
          this.callback(subject, data);
      } else 
        this.callback.observe(subject, topic, data);
    } catch (e) {
      console.exception(e);
    }
  }
};

function Subject(object) {
  
  
  
  
  
  this.wrappedJSObject = {
    observersModuleSubjectWrapper: true,
    object: object
  };
}

Subject.prototype = {
  QueryInterface: XPCOMUtils.generateQI([]),
  getHelperForLanguage: function() {},
  getInterfaces: function() {}
};

require("unload").when(
  function removeAllObservers() {
    
    
    cache.slice().forEach(
      function(observer) {
        remove(observer.topic, observer.callback, observer.thisObject);
      });
  });
