



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
      ['dom.ipc.processPriorityManager.testMode', true],
      ['dom.ipc.processPriorityManager.enabled', true],
      ['dom.ipc.processPriorityManager.backgroundLRUPoolLevels', 2]
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
    SpecialPowers.addPermission("browser", true, document);
    this.tempPermissions.push(location.href)
  },

  'tempPermissions': [],
  addPermissionForUrl: function(url) {
    SpecialPowers.addPermission("browser", true, url);
    this.tempPermissions.push(url);
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
    for (var i = 0; i < this.tempPermissions.length; i++) {
      SpecialPowers.removePermission("browser", this.tempPermissions[i]);
    }

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



function expectProcessCreated() {
  var deferred = Promise.defer();

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
      deferred.resolve(childID);
    }
  );

  return deferred.promise;
}



function expectOnlyOneProcessCreated() {
  var p = expectProcessCreated();
  p.then(function() {
    expectProcessCreated().then(function(childID) {
      ok(false, 'Got unexpected process creation, childID=' + childID);
    });
  });
  return p;
}









function expectPriorityChange(childID, expectedPriority,
                               expectedCPUPriority) {
  var deferred = Promise.defer();

  var observed = false;
  browserElementTestHelpers.addProcessPriorityObserver(
    'process-priority-set',
    function(subject, topic, data) {
      if (observed) {
        return;
      }

      var [id, priority, cpuPriority] = data.split(":");
      if (id != childID) {
        return;
      }

      
      
      observed = true;

      is(priority, expectedPriority,
         'Expected priority of childID ' + childID +
         ' to change to ' + expectedPriority);

      if (expectedCPUPriority) {
        is(cpuPriority, expectedCPUPriority,
           'Expected CPU priority of childID ' + childID +
           ' to change to ' + expectedCPUPriority);
      }

      if (priority == expectedPriority &&
          (!expectedCPUPriority || expectedCPUPriority == cpuPriority)) {
        deferred.resolve();
      } else {
        deferred.reject();
      }
    }
  );

  return deferred.promise;
}





function expectPriorityWithBackgroundLRUSet(childID, expectedBackgroundLRU) {
  var deferred = Promise.defer();

  browserElementTestHelpers.addProcessPriorityObserver(
    'process-priority-with-background-LRU-set',
    function(subject, topic, data) {

      var [id, priority, cpuPriority, backgroundLRU] = data.split(":");
      if (id != childID) {
        return;
      }

      is(backgroundLRU, expectedBackgroundLRU,
         'Expected backgroundLRU ' + backgroundLRU + ' of childID ' + childID +
         ' to change to ' + expectedBackgroundLRU);

      if (backgroundLRU == expectedBackgroundLRU) {
        deferred.resolve();
      } else {
        deferred.reject();
      }
    }
  );

  return deferred.promise;
}



function expectMozbrowserEvent(iframe, eventName) {
  var deferred = Promise.defer();
  iframe.addEventListener('mozbrowser' + eventName, function handler(e) {
    iframe.removeEventListener('mozbrowser' + eventName, handler);
    deferred.resolve(e);
  });
  return deferred.promise;
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
