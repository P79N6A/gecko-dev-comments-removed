




var gTestFuncs = [];



var gIterator;

Components.utils.import('resource://gre/modules/Services.jsm');
Components.utils.import("resource://gre/modules/accessibility/Utils.jsm");
Components.utils.import("resource://gre/modules/accessibility/EventManager.jsm");

var AccessFuTest = {

  addFunc: function AccessFuTest_addFunc(aFunc) {
    if (aFunc) {
      gTestFuncs.push(aFunc);
    }
  },

  _registerListener: function AccessFuTest__registerListener(aWaitForMessage, aListenerFunc) {
    var listener = {
      observe: function observe(aMessage) {
        
        if (!(aMessage instanceof Components.interfaces.nsIConsoleMessage)) {
          return;
        }
        if (aMessage.message.indexOf(aWaitForMessage) < 0) {
          return;
        }
        aListenerFunc.apply(listener);
      }
    };
    Services.console.registerListener(listener);
    return listener;
  },

  on_log: function AccessFuTest_on_log(aWaitForMessage, aListenerFunc) {
    return this._registerListener(aWaitForMessage, aListenerFunc);
  },

  off_log: function AccessFuTest_off_log(aListener) {
    Services.console.unregisterListener(aListener);
  },

  once_log: function AccessFuTest_once_log(aWaitForMessage, aListenerFunc) {
    return this._registerListener(aWaitForMessage,
      function listenAndUnregister() {
        Services.console.unregisterListener(this);
        aListenerFunc();
      });
  },

  _addObserver: function AccessFuTest__addObserver(aWaitForData, aListener) {
    var listener = function listener(aSubject, aTopic, aData) {
      var data = JSON.parse(aData)[1];
      
      if (!data) {
        return;
      }
      isDeeply(data.details.actions, aWaitForData, "Data is correct");
      aListener.apply(listener);
    };
    Services.obs.addObserver(listener, 'accessfu-output', false);
    return listener;
  },

  on: function AccessFuTest_on(aWaitForData, aListener) {
    return this._addObserver(aWaitForData, aListener);
  },

  off: function AccessFuTest_off(aListener) {
    Services.obs.removeObserver(aListener, 'accessfu-output');
  },

  once: function AccessFuTest_once(aWaitForData, aListener) {
    return this._addObserver(aWaitForData, function observerAndRemove() {
      Services.obs.removeObserver(this, 'accessfu-output');
      aListener();
    });
  },

  _waitForExplicitFinish: false,

  waitForExplicitFinish: function AccessFuTest_waitForExplicitFinish() {
    this._waitForExplicitFinish = true;
  },

  finish: function AccessFuTest_finish() {
    
    Logger.test = false;
    Logger.logLevel = Logger.INFO;
    AccessFu.doneCallback = function doneCallback() {
      
      
      AccessFu.detach();
      
      SimpleTest.finish();
    };
    
    SpecialPowers.setIntPref("accessibility.accessfu.notify_output", 0);
    SpecialPowers.setIntPref("accessibility.accessfu.activate", 0);
  },

  nextTest: function AccessFuTest_nextTest() {
    var testFunc;
    try {
      
      
      testFunc = gIterator.next()[1];
    } catch (ex) {
      
      this.finish();
      return;
    }
    testFunc();
  },

  runTests: function AccessFuTest_runTests() {
    if (gTestFuncs.length === 0) {
      ok(false, "No tests specified!");
      simpleTest.finish();
      return;
    }

    
    gIterator = Iterator(gTestFuncs);

    
    Components.utils.import("resource://gre/modules/accessibility/AccessFu.jsm");

    AccessFu.attach(getMainChromeWindow(window));

    AccessFu.readyCallback = function readyCallback() {
      
      Logger.test = true;
      Logger.logLevel = Logger.DEBUG;
      

      if (AccessFuTest._waitForExplicitFinish) {
        
        AccessFuTest.nextTest();
      } else {
        
        [testFunc() for (testFunc of gTestFuncs)];
        AccessFuTest.finish();
      }
    };

    
    SpecialPowers.setIntPref("accessibility.accessfu.activate", 1);
    SpecialPowers.setIntPref("accessibility.accessfu.notify_output", 1);
  }
};
