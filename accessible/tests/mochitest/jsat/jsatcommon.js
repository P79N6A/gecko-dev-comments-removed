




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

  _waitForExplicitFinish: false,

  waitForExplicitFinish: function AccessFuTest_waitForExplicitFinish() {
    this._waitForExplicitFinish = true;
  },

  finish: function AccessFuTest_finish() {
    
    Logger.test = false;
    AccessFu.doneCallback = function doneCallback() {
      
      
      AccessFu.detach();
      
      SimpleTest.finish();
    };
    
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
      

      if (AccessFuTest._waitForExplicitFinish) {
        
        AccessFuTest.nextTest();
      } else {
        
        [testFunc() for (testFunc of gTestFuncs)];
        AccessFuTest.finish();
      }
    };

    
    SpecialPowers.setIntPref("accessibility.accessfu.activate", 1);
  }
};
