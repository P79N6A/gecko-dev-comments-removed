















var EXPORTED_SYMBOLS = ['ShumwayCom'];

Components.utils.import('resource://gre/modules/XPCOMUtils.jsm');
Components.utils.import('resource://gre/modules/Services.jsm');
Components.utils.import('resource://gre/modules/Promise.jsm');

Components.utils.import('chrome://shumway/content/SpecialInflate.jsm');
Components.utils.import('chrome://shumway/content/SpecialStorage.jsm');
Components.utils.import('chrome://shumway/content/RtmpUtils.jsm');
Components.utils.import('chrome://shumway/content/ExternalInterface.jsm');
Components.utils.import('chrome://shumway/content/FileLoader.jsm');

XPCOMUtils.defineLazyModuleGetter(this, 'ShumwayTelemetry',
  'resource://shumway/ShumwayTelemetry.jsm');

const MAX_USER_INPUT_TIMEOUT = 250; 

function getBoolPref(pref, def) {
  try {
    return Services.prefs.getBoolPref(pref);
  } catch (ex) {
    return def;
  }
}

function log(aMsg) {
  let msg = 'ShumwayCom.js: ' + (aMsg.join ? aMsg.join('') : aMsg);
  Services.console.logStringMessage(msg);
  dump(msg + '\n');
}

var ShumwayCom = {
  createAdapter: function (content, callbacks, hooks) {
    
    
    var wrapped = {
      enableDebug: function enableDebug() {
        callbacks.enableDebug()
      },

      setFullscreen: function setFullscreen(value) {
        callbacks.sendMessage('setFullscreen', value, false);
      },

      endActivation: function endActivation() {
        callbacks.sendMessage('endActivation', null, false);
      },

      fallback: function fallback() {
        callbacks.sendMessage('fallback', null, false);
      },

      getSettings: function getSettings() {
        return Components.utils.cloneInto(
          callbacks.sendMessage('getSettings', null, true), content);
      },

      getPluginParams: function getPluginParams() {
        return Components.utils.cloneInto(
          callbacks.sendMessage('getPluginParams', null, true), content);
      },

      reportIssue: function reportIssue() {
        callbacks.sendMessage('reportIssue', null, false);
      },

      reportTelemetry: function reportTelemetry(args) {
        callbacks.sendMessage('reportTelemetry', args, false);
      },

      userInput: function userInput() {
        callbacks.sendMessage('userInput', null, true);
      },

      setupComBridge: function setupComBridge(playerWindow) {
        
        
        function postSyncMessage(msg) {
          if (onSyncMessageCallback) {
            
            var reclonedMsg = Components.utils.cloneInto(Components.utils.waiveXrays(msg), content);
            var result = onSyncMessageCallback(reclonedMsg);
            
            var waivedResult = Components.utils.waiveXrays(result);
            return waivedResult;
          }
        }

        
        var playerContent = playerWindow.contentWindow.wrappedJSObject;
        ShumwayCom.createPlayerAdapter(playerContent, postSyncMessage, callbacks, hooks);
      },

      setSyncMessageCallback: function (callback) {
        onSyncMessageCallback = callback;
      }
    };

    var onSyncMessageCallback;

    var shumwayComAdapter = Components.utils.cloneInto(wrapped, content, {cloneFunctions:true});
    content.ShumwayCom = shumwayComAdapter;
  },

  createPlayerAdapter: function (content, postSyncMessage, callbacks, hooks) {
    
    
    var wrapped = {
      externalCom: function externalCom(args) {
        var result = String(callbacks.sendMessage('externalCom', args, true));
        return Components.utils.cloneInto(result, content);
      },

      loadFile: function loadFile(args) {
        callbacks.sendMessage('loadFile', args, false);
      },

      reportTelemetry: function reportTelemetry(args) {
        callbacks.sendMessage('reportTelemetry', args, false);
      },

      setClipboard: function setClipboard(args) {
        callbacks.sendMessage('setClipboard', args, false);
      },

      navigateTo: function navigateTo(args) {
        callbacks.sendMessage('navigateTo', args, false);
      },

      loadSystemResource: function loadSystemResource(id) {
        loadShumwaySystemResource(id).then(function (data) {
          if (onSystemResourceCallback) {
            onSystemResourceCallback(id, Components.utils.cloneInto(data, content));
          }
        });
      },

      postSyncMessage: function (msg) {
        var result = postSyncMessage(msg);
        return Components.utils.cloneInto(result, content)
      },

      createSpecialStorage: function () {
        var environment = callbacks.getEnvironment();
        return SpecialStorageUtils.createWrappedSpecialStorage(content,
          environment.swfUrl, environment.privateBrowsing);
      },

      getWeakMapKeys: function (weakMap) {
        var keys = Components.utils.nondeterministicGetWeakMapKeys(weakMap);
        var result = new content.Array();
        keys.forEach(function (key) {
          result.push(key);
        });
        return result;
      },

      setLoadFileCallback: function (callback) {
        onLoadFileCallback = callback;
      },
      setExternalCallback: function (callback) {
        onExternalCallback = callback;
      },
      setSystemResourceCallback: function (callback) {
        onSystemResourceCallback = callback;
      }
    };

    
    
    if (SpecialInflateUtils.isSpecialInflateEnabled) {
      wrapped.createSpecialInflate = function () {
        return SpecialInflateUtils.createWrappedSpecialInflate(content);
      };
    }

    
    
    if (RtmpUtils.isRtmpEnabled) {
      wrapped.createRtmpSocket = function (params) {
        return RtmpUtils.createSocket(content, params);
      };
      wrapped.createRtmpXHR = function () {
        return RtmpUtils.createXHR(content);
      };
    }

    var onSystemResourceCallback;
    var onExternalCallback;
    var onLoadFileCallback;

    hooks.onLoadFileCallback = function (arg) {
      if (onLoadFileCallback) {
        onLoadFileCallback(Components.utils.cloneInto(arg, content));
      }
    };
    hooks.onExternalCallback = function (call) {
      if (onExternalCallback) {
        return onExternalCallback(Components.utils.cloneInto(call, content));
      }
    };

    var shumwayComAdapter = Components.utils.cloneInto(wrapped, content, {cloneFunctions:true});
    content.ShumwayCom = shumwayComAdapter;
  },

  createActions: function (startupInfo, window, document) {
    return new ShumwayChromeActions(startupInfo, window, document);
  }
};

function loadShumwaySystemResource(id) {
  var url, type;
  switch (id) {
    case 0: 
      url = 'resource://shumway/libs/builtin.abc';
      type = 'arraybuffer';
      break;
    case 1: 
      url = 'resource://shumway/playerglobal/playerglobal.abcs';
      type = 'arraybuffer';
      break;
    case 2: 
      url = 'resource://shumway/playerglobal/playerglobal.json';
      type = 'json';
      break;
    default:
      return Promise.reject('Unsupported system resource id');
  }

  var deferred = Promise.defer();

  var xhr = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"]
                              .createInstance(Components.interfaces.nsIXMLHttpRequest);
  xhr.open('GET', url, true);
  xhr.responseType = type;
  xhr.onload = function () {
    deferred.resolve(xhr.response);
  };
  xhr.send(null);

  return deferred.promise;
}


function ShumwayChromeActions(startupInfo, window, document) {
  this.url = startupInfo.url;
  this.objectParams = startupInfo.objectParams;
  this.movieParams = startupInfo.movieParams;
  this.baseUrl = startupInfo.baseUrl;
  this.isOverlay = startupInfo.isOverlay;
  this.embedTag = startupInfo.embedTag;
  this.isPausedAtStart = startupInfo.isPausedAtStart;
  this.window = window;
  this.document = document;
  this.allowScriptAccess = startupInfo.allowScriptAccess;
  this.lastUserInput = 0;
  this.telemetry = {
    startTime: Date.now(),
    features: [],
    errors: []
  };

  this.fileLoader = new FileLoader(startupInfo.url, startupInfo.baseUrl, function (args) {
    this.onLoadFileCallback(args);
  }.bind(this));
  this.onLoadFileCallback = null;

  this.externalInterface = null;
  this.onExternalCallback = null;
}

ShumwayChromeActions.prototype = {
  
  
  
  
  invoke: function (name, args) {
    return this[name].call(this, args);
  },

  getBoolPref: function (data) {
    if (!/^shumway\./.test(data.pref)) {
      return null;
    }
    return getBoolPref(data.pref, data.def);
  },

  getSettings: function getSettings() {
    return {
      compilerSettings: {
        appCompiler: getBoolPref('shumway.appCompiler', true),
        sysCompiler: getBoolPref('shumway.sysCompiler', false),
        verifier: getBoolPref('shumway.verifier', true)
      },
      playerSettings: {
        turboMode: getBoolPref('shumway.turboMode', false),
        hud: getBoolPref('shumway.hud', false),
        forceHidpi: getBoolPref('shumway.force_hidpi', false)
      }
    }
  },

  getPluginParams: function getPluginParams() {
    return {
      url: this.url,
      baseUrl : this.baseUrl,
      movieParams: this.movieParams,
      objectParams: this.objectParams,
      isOverlay: this.isOverlay,
      isPausedAtStart: this.isPausedAtStart,
      isDebuggerEnabled: getBoolPref('shumway.debug.enabled', false)
    };
  },

  loadFile: function loadFile(data) {
    this.fileLoader.load(data);
  },

  navigateTo: function (data) {
    
    
    
    var url = data.url || 'about:blank';
    var target = data.target || '_self';
    var isWhitelistedProtocol = /^(http|https):\/\//i.test(url);
    if (!isWhitelistedProtocol || !this.allowScriptAccess) {
      return;
    }
    
    if (!this.isUserInputInProgress()) {
      return;
    }
    log('!!navigateTo: ' + url + ' ... ' + target);
    var embedTag = this.embedTag.wrappedJSObject;
    var window = embedTag ? embedTag.ownerDocument.defaultView : this.window;
    window.open(url, target);
  },

  fallback: function(automatic) {
    automatic = !!automatic;
    var event = this.document.createEvent('CustomEvent');
    event.initCustomEvent('shumwayFallback', true, true, {
      automatic: automatic
    });
    this.window.dispatchEvent(event);
  },

  userInput: function() {
    var win = this.window;
    var winUtils = win.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                      .getInterface(Components.interfaces.nsIDOMWindowUtils);
    if (winUtils.isHandlingUserInput) {
      this.lastUserInput = Date.now();
    }
  },

  isUserInputInProgress: function () {
    
    if (!getBoolPref('shumway.userInputSecurity', true)) {
      return true;
    }

    
    
    if ((Date.now() - this.lastUserInput) > MAX_USER_INPUT_TIMEOUT) {
      return false;
    }
    
    return true;
  },

  setClipboard: function (data) {
    if (typeof data !== 'string' || !this.isUserInputInProgress()) {
      return;
    }

    let clipboard = Components.classes["@mozilla.org/widget/clipboardhelper;1"]
                                      .getService(Components.interfaces.nsIClipboardHelper);
    clipboard.copyString(data);
  },

  setFullscreen: function (enabled) {
    enabled = !!enabled;

    if (!this.isUserInputInProgress()) {
      return;
    }

    var target = this.embedTag || this.document.body;
    if (enabled) {
      target.mozRequestFullScreen();
    } else {
      target.ownerDocument.mozCancelFullScreen();
    }
  },

  endActivation: function () {
    var event = this.document.createEvent('CustomEvent');
    event.initCustomEvent('shumwayActivated', true, true, null);
    this.window.dispatchEvent(event);
  },

  reportTelemetry: function (data) {
    var topic = data.topic;
    switch (topic) {
      case 'firstFrame':
        var time = Date.now() - this.telemetry.startTime;
        ShumwayTelemetry.onFirstFrame(time);
        break;
      case 'parseInfo':
        ShumwayTelemetry.onParseInfo({
          parseTime: +data.parseTime,
          size: +data.bytesTotal,
          swfVersion: data.swfVersion|0,
          frameRate: +data.frameRate,
          width: data.width|0,
          height: data.height|0,
          bannerType: data.bannerType|0,
          isAvm2: !!data.isAvm2
        });
        break;
      case 'feature':
        var featureType = data.feature|0;
        var MIN_FEATURE_TYPE = 0, MAX_FEATURE_TYPE = 999;
        if (featureType >= MIN_FEATURE_TYPE && featureType <= MAX_FEATURE_TYPE &&
          !this.telemetry.features[featureType]) {
          this.telemetry.features[featureType] = true; 
          ShumwayTelemetry.onFeature(featureType);
        }
        break;
      case 'error':
        var errorType = data.error|0;
        var MIN_ERROR_TYPE = 0, MAX_ERROR_TYPE = 2;
        if (errorType >= MIN_ERROR_TYPE && errorType <= MAX_ERROR_TYPE &&
          !this.telemetry.errors[errorType]) {
          this.telemetry.errors[errorType] = true; 
          ShumwayTelemetry.onError(errorType);
        }
        break;
    }
  },

  reportIssue: function (exceptions) {
    var urlTemplate = "https://bugzilla.mozilla.org/enter_bug.cgi?op_sys=All&priority=--" +
      "&rep_platform=All&target_milestone=---&version=Trunk&product=Firefox" +
      "&component=Shumway&short_desc=&comment={comment}" +
      "&bug_file_loc={url}";
    var windowUrl = this.window.parent.wrappedJSObject.location + '';
    var url = urlTemplate.split('{url}').join(encodeURIComponent(windowUrl));
    var params = {
      swf: encodeURIComponent(this.url)
    };
    getVersionInfo().then(function (versions) {
      params.versions = versions;
    }).then(function () {
      var ffbuild = params.versions.geckoVersion + ' (' + params.versions.geckoBuildID + ')';
      
      var comment = '+++ Initially filed via the problem reporting functionality in Shumway +++\n' +
        'Please add any further information that you deem helpful here:\n\n\n\n' +
        '----------------------\n\n' +
        'Technical Information:\n' +
        'Firefox version: ' + ffbuild + '\n' +
        'Shumway version: ' + params.versions.shumwayVersion;
      url = url.split('{comment}').join(encodeURIComponent(comment));
      this.window.open(url);
    }.bind(this));
  },

  externalCom: function (data) {
    if (!this.allowScriptAccess)
      return;

    
    if (!this.externalInterface) {
      var parentWindow = this.window.parent.wrappedJSObject;
      var embedTag = this.embedTag.wrappedJSObject;
      this.externalInterface = new ExternalInterface(parentWindow, embedTag, function (call) {
        this.onExternalCallback(call);
      }.bind(this));
    }

    return this.externalInterface.processAction(data);
  }
};

function getVersionInfo() {
  var deferred = Promise.defer();
  var versionInfo = {
    geckoVersion: 'unknown',
    geckoBuildID: 'unknown',
    shumwayVersion: 'unknown'
  };
  try {
    var appInfo = Components.classes["@mozilla.org/xre/app-info;1"]
                                    .getService(Components.interfaces.nsIXULAppInfo);
    versionInfo.geckoVersion = appInfo.version;
    versionInfo.geckoBuildID = appInfo.appBuildID;
  } catch (e) {
    log('Error encountered while getting platform version info: ' + e);
  }
  var xhr = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"]
                              .createInstance(Components.interfaces.nsIXMLHttpRequest);
  xhr.open('GET', 'resource://shumway/version.txt', true);
  xhr.overrideMimeType('text/plain');
  xhr.onload = function () {
    try {
      
      
      var lines = xhr.responseText.split(/\n/g);
      lines[1] = '(' + lines[1] + ')';
      versionInfo.shumwayVersion = lines.join(' ');
    } catch (e) {
      log('Error while parsing version info: ' + e);
    }
    deferred.resolve(versionInfo);
  };
  xhr.onerror = function () {
    log('Error while reading version info: ' + xhr.error);
    deferred.resolve(versionInfo);
  };
  xhr.send();

  return deferred.promise;
}
