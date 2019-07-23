










































var Ci = Components.interfaces;
var Cc = Components.classes;
var Cr = Components.results;

var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
var ds = Cc[NS_DIRECTORY_SERVICE_CONTRACTID].getService(Ci.nsIProperties);
const WM = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);






var Stack = {
  _stack: [],
  _time: 250,
  _timer: Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer),
  notify: function _notify(aTimer) {
    this.pop();
  },
  push: function _push(aItem) {
    this._stack.push(aItem);
    if (this._stack.length == 1) {
      this._timer.initWithCallback({notify:function (aTimer) {Stack.pop();}},
                                   this._time, 1);
    }
  },
  pop: function _pop() {
    var obj = this._stack.pop();
    try {
      obj.method.apply(obj, obj.args);
    } catch (e) {
      Components.utils.reportError(e);
    }
    if (!this._stack.length) {
      this._timer.cancel();
    }
  }
};





var toRun = [];





function SimpleGenerator(enumr, interface) {
  while (enumr.hasMoreElements()) {
    yield enumr.getNext().QueryInterface(interface);
  }
}

var wins = [w for (w in SimpleGenerator(WM.getEnumerator(null), Ci.nsIDOMWindow))];




function MsgBase() {
}
MsgBase.prototype = {
  args: [],
  method: function() {
    Components.utils.reportError(this._msg);
  }
};
function TestStart(aCat) {
  this._msg = 'TESTING:START:COVERAGE:' + aCat;
}
TestStart.prototype = new MsgBase;
function TestDone(aCat) {
  this._msg = 'TESTING:DONE:COVERAGE:' + aCat;
}
TestDone.prototype = new MsgBase;





function onLoad() {
  for each (var obj in toRun) {
    Stack.push(obj);
  }
}
