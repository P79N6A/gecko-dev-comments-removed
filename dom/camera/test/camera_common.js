function isDefinedObj(obj) {
  return typeof(obj) !== 'undefined' && obj != null;
}

function isDefined(obj) {
  return typeof(obj) !== 'undefined';
}
































function CameraTestSuite() {
  SimpleTest.waitForExplicitFinish();

  this._window = window;
  this._document = document;
  this.viewfinder = document.getElementById('viewfinder');
  this._tests = [];
  this.hwType = '';

  









  this.setup = this._setup.bind(this);
  this.teardown = this._teardown.bind(this);
  this.test = this._test.bind(this);
  this.run = this._run.bind(this);
  this.waitPreviewStarted = this._waitPreviewStarted.bind(this);
  this.waitParameterPush = this._waitParameterPush.bind(this);
  this.initJsHw = this._initJsHw.bind(this);
  this.getCamera = this._getCamera.bind(this);
  this.setLowMemoryPlatform = this._setLowMemoryPlatform.bind(this);
  this.logError = this._logError.bind(this);
  this.expectedError = this._expectedError.bind(this);
  this.expectedRejectGetCamera = this._expectedRejectGetCamera.bind(this);
  this.expectedRejectConfigure = this._expectedRejectConfigure.bind(this);
  this.expectedRejectAutoFocus = this._expectedRejectAutoFocus.bind(this);
  this.expectedRejectTakePicture = this._expectedRejectTakePicture.bind(this);
  this.expectedRejectStartRecording = this._expectedRejectStartRecording.bind(this);
  this.expectedRejectStopRecording = this._expectedRejectStopRecording.bind(this);
  this.rejectGetCamera = this._rejectGetCamera.bind(this);
  this.rejectConfigure = this._rejectConfigure.bind(this);
  this.rejectRelease = this._rejectRelease.bind(this);
  this.rejectAutoFocus = this._rejectAutoFocus.bind(this);
  this.rejectTakePicture = this._rejectTakePicture.bind(this);
  this.rejectStartRecording = this._rejectStartRecording.bind(this);
  this.rejectStopRecording = this._rejectStopRecording.bind(this);
  this.rejectPreviewStarted = this._rejectPreviewStarted.bind(this);

  var self = this;
  this._window.addEventListener('beforeunload', function() {
    if (isDefinedObj(self.viewfinder)) {
      self.viewfinder.mozSrcObject = null;
    }

    self.hw = null;
    if (isDefinedObj(self.camera)) {
      ok(false, 'window unload triggered camera release instead of test completion');
      self.camera.release();
      self.camera = null;
    }
  });
}

CameraTestSuite.prototype = {
  camera: null,
  hw: null,
  _lowMemSet: false,
  _reloading: false,

  _setupPermission: function(permission) {
    if (!SpecialPowers.hasPermission(permission, document)) {
      info("requesting " + permission + " permission");
      SpecialPowers.addPermission(permission, true, document);
      this._reloading = true;
    }
  },

  



  _setup: function(hwType) {
    


    this._setupPermission("camera");
    this._setupPermission("device-storage:videos");
    this._setupPermission("device-storage:videos-create");
    this._setupPermission("device-storage:videos-write");

    if (this._reloading) {
      window.location.reload();
      return Promise.reject();
    }

    info("has necessary permissions");
    if (!isDefined(hwType)) {
      hwType = 'hardware';
    }

    this._hwType = hwType;
    return new Promise(function(resolve, reject) {
      SpecialPowers.pushPrefEnv({'set': [['device.storage.prompt.testing', true]]}, function() {
        SpecialPowers.pushPrefEnv({'set': [['camera.control.test.permission', true]]}, function() {
          SpecialPowers.pushPrefEnv({'set': [['camera.control.test.enabled', hwType]]}, function() {
            resolve();
          });
        });
      });
    });
  },

  


  _teardown: function() {
    return new Promise(function(resolve, reject) {
      SpecialPowers.flushPrefEnv(function() {
        resolve();
      });
    });
  },

  



  _setLowMemoryPlatform: function(val) {
    if (typeof(val) === 'undefined') {
      val = true;
    }

    if (this._lowMemSet === val) {
      return Promise.resolve();
    }

    var self = this;
    return new Promise(function(resolve, reject) {
      SpecialPowers.pushPrefEnv({'set': [['camera.control.test.is_low_memory', val]]}, function() {
        self._lowMemSet = val;
        resolve();
      });
    }).catch(function(e) {
      return self.logError('set low memory ' + val + ' failed', e);
    });
  },

  
  _test: function(aName, aCb) {
    this._tests.push({
      name: aName,
      cb: aCb
    });
  },

  
  _run: function() {
    if (this._reloading) {
      return;
    }

    var test = this._tests.shift();
    var self = this;
    if (test) {
      info(test.name + ' started');

      function runNextTest() {
        self.run();
      }

      function resetLowMem() {
        return self.setLowMemoryPlatform(false);
      }

      function postTest(pass) {
        ok(pass, test.name + ' finished');
        var camera = self.camera;
        self.viewfinder.mozSrcObject = null;
        self.camera = null;

        if (!isDefinedObj(camera)) {
          return Promise.resolve();
        }

        function handler(e) {
          ok(typeof(e) === 'undefined', 'camera released');
          return Promise.resolve();
        }

        return camera.release().then(handler).catch(handler);
      }

      this.initJsHw();

      var testPromise;
      try {
        testPromise = test.cb();
        if (!isDefinedObj(testPromise)) {
          testPromise = Promise.resolve();
        }
      } catch(e) {
        ok(false, 'caught exception while running test: ' + e);
        testPromise = Promise.reject(e);
      }

      testPromise
        .then(function(p) {
          return postTest(true);
        }, function(e) {
          self.logError('unhandled error', e);
          return postTest(false);
        })
        .then(resetLowMem, resetLowMem)
        .then(runNextTest, runNextTest);
    } else {
      ok(true, 'all tests completed');
      var finish = SimpleTest.finish.bind(SimpleTest);
      this.teardown().then(finish, finish);
    }
  },

  







  _initJsHw: function() {
    if (this._hwType === 'hardware') {
      this.hw = SpecialPowers.Cc['@mozilla.org/cameratesthardware;1']
                .getService(SpecialPowers.Ci.nsICameraTestHardware);
      this.hw.reset(this._window);

      
      this.hw.params['preview-size'] = '320x240';
      this.hw.params['preview-size-values'] = '320x240';
      this.hw.params['picture-size-values'] = '320x240';
    } else {
      this.hw = null;
    }
  },

  



  _getCamera: function(name, config) {
    var cameraManager = navigator.mozCameras;
    if (!isDefined(name)) {
      name = cameraManager.getListOfCameras()[0];
    }

    var self = this;
    return cameraManager.getCamera(name, config).then(
      function(p) {
        ok(isDefinedObj(p) && isDefinedObj(p.camera), 'got camera');
        self.camera = p.camera;
        

        return Promise.resolve(p);
      }
    );
  },

  



  _waitPreviewStarted: function() {
    var self = this;

    return new Promise(function(resolve, reject) {
      function onPreviewStateChange(e) {
        try {
          if (e.newState === 'started') {
            ok(true, 'viewfinder is ready and playing');
            self.camera.removeEventListener('previewstatechange', onPreviewStateChange);
            resolve();
          }
        } catch(e) {
          reject(e);
        }
      }

      if (!isDefinedObj(self.viewfinder)) {
        reject(new Error('no viewfinder object'));
        return;
      }

      self.viewfinder.mozSrcObject = self.camera;
      self.viewfinder.play();
      self.camera.addEventListener('previewstatechange', onPreviewStateChange);
    });
  },

  




  _waitParameterPush: function() {
    var self = this;

    return new Promise(function(resolve, reject) {
      self.hw.attach({
        'pushParameters': function() {
          self._window.setTimeout(resolve);
        }
      });
    });
  },

  



















  _logError: function(msg, e) {
    if (isDefined(e)) {
      ok(false, msg + ': ' + e);
    }
    
    return Promise.reject();
  },

  




  _rejectGetCamera: function(e) {
    return this.logError('get camera failed', e);
  },

  _rejectConfigure: function(e) {
    return this.logError('set configuration failed', e);
  },

  _rejectRelease: function(e) {
    return this.logError('release camera failed', e);
  },

  _rejectAutoFocus: function(e) {
    return this.logError('auto focus failed', e);
  },

  _rejectTakePicture: function(e) {
    return this.logError('take picture failed', e);
  },

  _rejectStartRecording: function(e) {
    return this.logError('start recording failed', e);
  },

  _rejectStopRecording: function(e) {
    return this.logError('stop recording failed', e);
  },

  _rejectPreviewStarted: function(e) {
    return this.logError('preview start failed', e);
  },

  




  _expectedError: function(msg) {
    ok(false, msg);
    


    return Promise.reject();
  },

  _expectedRejectGetCamera: function(p) {
    

    self.camera = p.camera;
    return this.expectedError('expected get camera to fail');
  },

  _expectedRejectConfigure: function(p) {
    return this.expectedError('expected set configuration to fail');
  },

  _expectedRejectAutoFocus: function(p) {
    return this.expectedError('expected auto focus to fail');
  },

  _expectedRejectTakePicture: function(p) {
    return this.expectedError('expected take picture to fail');
  },

  _expectedRejectStartRecording: function(p) {
    return this.expectedError('expected start recording to fail');
  },

  _expectedRejectStopRecording: function(p) {
    return this.expectedError('expected stop recording to fail');
  },
};

ise(SpecialPowers.sanityCheck(), "foo", "SpecialPowers passed sanity check");
