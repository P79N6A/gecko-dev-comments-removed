# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:






























this.G_ObserverWrapper = function G_ObserverWrapper(topic, observeFunction) {
  this.debugZone = "observer";
  this.topic_ = topic;
  this.observeFunction_ = observeFunction;
}




G_ObserverWrapper.prototype.QueryInterface = function(iid) {
  if (iid.equals(Ci.nsISupports) || iid.equals(Ci.nsIObserver))
    return this;
  throw Components.results.NS_ERROR_NO_INTERFACE;
}




G_ObserverWrapper.prototype.observe = function(subject, topic, data) {
  if (topic == this.topic_)
    this.observeFunction_(subject, topic, data);
}

















this.G_ObserverServiceObserver =
function G_ObserverServiceObserver(topic, observeFunction, opt_onlyOnce) {
  this.debugZone = "observerserviceobserver";
  this.topic_ = topic;
  this.observeFunction_ = observeFunction;
  this.onlyOnce_ = !!opt_onlyOnce;
  
  this.observer_ = new G_ObserverWrapper(this.topic_, 
                                         BindToObject(this.observe_, this));
  this.observerService_ = Cc["@mozilla.org/observer-service;1"]
                          .getService(Ci.nsIObserverService);
  this.observerService_.addObserver(this.observer_, this.topic_, false);
}




G_ObserverServiceObserver.prototype.unregister = function() {
  this.observerService_.removeObserver(this.observer_, this.topic_);
  this.observerService_ = null;
}




G_ObserverServiceObserver.prototype.observe_ = function(subject, topic, data) {
  this.observeFunction_(subject, topic, data);
  if (this.onlyOnce_)
    this.unregister();
}

#ifdef DEBUG
this.TEST_G_Observer = function TEST_G_Observer() {
  if (G_GDEBUG) {

    var z = "observer UNITTEST";
    G_debugService.enableZone(z);

    G_Debug(z, "Starting");

    var regularObserverRan = 0;
    var observerServiceObserverRan = 0;

    let regularObserver = function () {
      regularObserverRan++;
    };

    let observerServiceObserver = function () {
      observerServiceObserverRan++;
    };

    var service = Cc["@mozilla.org/observer-service;1"]
                  .getService(Ci.nsIObserverService);
    var topic = "google-observer-test";

    var o1 = new G_ObserverWrapper(topic, regularObserver);
    service.addObserver(o1, topic, false);

    new G_ObserverServiceObserver(topic, 
                                  observerServiceObserver, true );

    
    service.notifyObservers(null, topic, null);
    service.notifyObservers(null, topic, null);

    G_Assert(z, regularObserverRan == 2, "Regular observer broken");
    G_Assert(z, observerServiceObserverRan == 1, "ObsServObs broken");

    service.removeObserver(o1, topic);
    G_Debug(z, "PASSED");
  }
}
#endif
