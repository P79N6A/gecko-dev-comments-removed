



"use strict";

function _getPath() {
  return window.location.pathname
               .substring(0, window.location.pathname.lastIndexOf('/'))
               .replace("/priority", "");
}

const browserElementTestHelpers = {
  _getBoolPref: function(pref) {
    try {
      return SpecialPowers.getBoolPref(pref);
    }
    catch (e) {
      return undefined;
    }
  },

  _setPref: function(pref, value) {
    this.lockTestReady();
    if (value !== undefined && value !== null) {
      SpecialPowers.pushPrefEnv({'set': [[pref, value]]}, this.unlockTestReady.bind(this));
    } else {
      SpecialPowers.pushPrefEnv({'clear': [[pref]]}, this.unlockTestReady.bind(this));
    }
  },

  _setPrefs: function() {
    this.lockTestReady();
    SpecialPowers.pushPrefEnv({'set': Array.slice(arguments)}, this.unlockTestReady.bind(this));
  },

  _testReadyLockCount: 0,
  _firedTestReady: false,
  lockTestReady: function() {
    this._testReadyLockCount++;
  },

  unlockTestReady: function() {
    this._testReadyLockCount--;
    if (this._testReadyLockCount == 0 && !this._firedTestReady) {
      this._firedTestReady = true;
      dispatchEvent(new Event("testready"));
    }
  },

  enableProcessPriorityManager: function() {
    this._setPrefs(
      ['dom.ipc.processPriorityManager.BACKGROUND.LRUPoolLevels', 2],
      ['dom.ipc.processPriorityManager.FOREGROUND.LRUPoolLevels', 2],
      ['dom.ipc.processPriorityManager.testMode', true],
      ['dom.ipc.processPriorityManager.enabled', true],
      ['dom.ipc.processCount', 3]
    );
  },

  setEnabledPref: function(value) {
    this._setPref('dom.mozBrowserFramesEnabled', value);
  },

  setSelectionChangeEnabledPref: function(value) {
    this._setPref('selectioncaret.enabled', value);
  },

  getOOPByDefaultPref: function() {
    return this._getBoolPref("dom.ipc.browser_frames.oop_by_default");
  },

  addPermission: function() {
    this.lockTestReady();
    SpecialPowers.pushPermissions(
      [{'type': "browser", 'allow': 1, 'context': document}],
      this.unlockTestReady.bind(this));
  },

  _observers: [],

  
  
  
  
  
  addProcessPriorityObserver: function(processPriorityTopic, observerFn) {
    var topic = "process-priority-manager:TEST-ONLY:" + processPriorityTopic;

    
    
    var observer = {
      observe: observerFn
    };

    SpecialPowers.addObserver(observer, topic,  false);
    this._observers.push([observer, topic]);
  },

  cleanUp: function() {
    for (var i = 0; i < this._observers.length; i++) {
      SpecialPowers.removeObserver(this._observers[i][0],
                                   this._observers[i][1]);
    }
  },

  
  'emptyPage1': 'http://example.com' + _getPath() + '/file_empty.html',
  'emptyPage2': 'http://example.org' + _getPath() + '/file_empty.html',
  'emptyPage3': 'http://test1.example.org' + _getPath() + '/file_empty.html',
  'focusPage': 'http://example.org' + _getPath() + '/file_focus.html',
};



function expectProcessCreated( initialPriority) {
  return new Promise(function(resolve, reject) {
    var observed = false;
    browserElementTestHelpers.addProcessPriorityObserver(
      "process-created",
      function(subject, topic, data) {
        
        
        if (observed) {
          return;
        }
        observed = true;

        var childID = parseInt(data);
        ok(true, 'Got new process, id=' + childID);
        if (initialPriority) {
          expectPriorityChange(childID, initialPriority).then(function() {
            resolve(childID);
          });
        } else {
          resolve(childID);
        }
      }
    );
  });
}



function expectOnlyOneProcessCreated( initialPriority) {
  var p = expectProcessCreated(initialPriority);
  p.then(function() {
    expectProcessCreated().then(function(childID) {
      ok(false, 'Got unexpected process creation, childID=' + childID);
    });
  });
  return p;
}





function expectPriorityChange(childID, expectedPriority) {
  return new Promise(function(resolve, reject) {
    var observed = false;
    browserElementTestHelpers.addProcessPriorityObserver(
      'process-priority-set',
      function(subject, topic, data) {
        if (observed) {
          return;
        }

        var [id, priority] = data.split(":");
        if (id != childID) {
          return;
        }

        
        
        observed = true;

        is(priority, expectedPriority,
           'Expected priority of childID ' + childID +
           ' to change to ' + expectedPriority);

        if (priority == expectedPriority) {
          resolve();
        } else {
          reject();
        }
      }
    );
  });
}






function expectPriorityWithLRUSet(childID, expectedPriority, expectedLRU) {
  return new Promise(function(resolve, reject) {
    var observed = false;
    browserElementTestHelpers.addProcessPriorityObserver(
      'process-priority-with-LRU-set',
      function(subject, topic, data) {
        if (observed) {
          return;
        }

        var [id, priority, lru] = data.split(":");
        if (id != childID) {
          return;
        }

        
        
        
        observed = true;

        is(lru, expectedLRU,
           'Expected LRU ' + lru +
           ' of childID ' + childID +
           ' to change to ' + expectedLRU);

        if ((priority == expectedPriority) && (lru == expectedLRU)) {
          resolve();
        } else {
          reject();
        }
      }
    );
  });
}





function expectPriorityDelay(childID, expectedPriority) {
  return new Promise(function(resolve, reject) {
    var observed = false;
    browserElementTestHelpers.addProcessPriorityObserver(
      'process-priority-delayed',
      function(subject, topic, data) {
        if (observed) {
          return;
        }

        var [id, priority] = data.split(":");
        if (id != childID) {
          return;
        }

        
        
        observed = true;

        is(priority, expectedPriority,
           'Expected delayed priority change of childID ' + childID +
           ' to ' + expectedPriority);

        if (priority == expectedPriority) {
          resolve();
        } else {
          reject();
        }
      }
    );
  });
}



function expectMozbrowserEvent(iframe, eventName) {
  return new Promise(function(resolve, reject) {
    iframe.addEventListener('mozbrowser' + eventName, function handler(e) {
      iframe.removeEventListener('mozbrowser' + eventName, handler);
      resolve(e);
    });
  });
}




































(function() {
  var oop = location.pathname.indexOf('_inproc_') == -1;

  browserElementTestHelpers.lockTestReady();
  SpecialPowers.setBoolPref("network.disable.ipc.security", true);
  SpecialPowers.pushPrefEnv({set: [["browser.pagethumbnails.capturing_disabled", true],
                                   ["dom.ipc.browser_frames.oop_by_default", oop],
                                   ["dom.ipc.tabs.disabled", false],
                                   ["security.mixed_content.block_active_content", false]]},
                            browserElementTestHelpers.unlockTestReady.bind(browserElementTestHelpers));
})();

addEventListener('unload', function() {
  browserElementTestHelpers.cleanUp();
});


browserElementTestHelpers.lockTestReady();
addEventListener('load', function() {
  SimpleTest.executeSoon(browserElementTestHelpers.unlockTestReady.bind(browserElementTestHelpers));
});
