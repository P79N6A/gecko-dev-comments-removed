



"use strict";








this.EXPORTED_SYMBOLS = [ "Node", "HTMLElement", "setTimeout", "clearTimeout" ];





this.Node = Components.interfaces.nsIDOMNode;
this.HTMLElement = Components.interfaces.nsIDOMHTMLElement;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");




let nextID = 1;




const timers = {};




function TimerCallback(callback) {
  this._callback = callback;
  const interfaces = [ Components.interfaces.nsITimerCallback ];
  this.QueryInterface = XPCOMUtils.generateQI(interfaces);
}

TimerCallback.prototype.notify = function(timer) {
  try {
    for (let timerID in timers) {
      if (timers[timerID] === timer) {
        delete timers[timerID];
        break;
      }
    }
    this._callback.apply(null, []);
  }
  catch (ex) {
    dump(ex +  '\n');
  }
};











this.setTimeout = function setTimeout(callback, delay) {
  const timer = Components.classes["@mozilla.org/timer;1"]
                        .createInstance(Components.interfaces.nsITimer);

  let timerID = nextID++;
  timers[timerID] = timer;

  timer.initWithCallback(new TimerCallback(callback), delay, timer.TYPE_ONE_SHOT);
  return timerID;
};







this.clearTimeout = function clearTimeout(timerID) {
  let timer = timers[timerID];
  if (timer) {
    timer.cancel();
    delete timers[timerID];
  }
};
