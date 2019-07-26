





Cu.import('resource://gre/modules/ContactService.jsm');
Cu.import('resource://gre/modules/SettingsChangeNotifier.jsm');
#ifdef MOZ_B2G_FM
Cu.import('resource://gre/modules/DOMFMRadioParent.jsm');
#endif
Cu.import('resource://gre/modules/AlarmService.jsm');
Cu.import('resource://gre/modules/ActivitiesService.jsm');
Cu.import('resource://gre/modules/PermissionPromptHelper.jsm');
Cu.import('resource://gre/modules/ObjectWrapper.jsm');
Cu.import('resource://gre/modules/accessibility/AccessFu.jsm');
Cu.import('resource://gre/modules/Payment.jsm');
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import('resource://gre/modules/UserAgentOverrides.jsm');
Cu.import('resource://gre/modules/Keyboard.jsm');
#ifdef MOZ_B2G_RIL
Cu.import('resource://gre/modules/NetworkStatsService.jsm');
#endif


Cu.import('resource://gre/modules/SignInToWebsite.jsm');
SignInToWebsiteController.init();

XPCOMUtils.defineLazyServiceGetter(Services, 'env',
                                   '@mozilla.org/process/environment;1',
                                   'nsIEnvironment');

XPCOMUtils.defineLazyServiceGetter(Services, 'ss',
                                   '@mozilla.org/content/style-sheet-service;1',
                                   'nsIStyleSheetService');

XPCOMUtils.defineLazyServiceGetter(this, 'gSystemMessenger',
                                   '@mozilla.org/system-message-internal;1',
                                   'nsISystemMessagesInternal');

#ifdef MOZ_WIDGET_GONK
XPCOMUtils.defineLazyServiceGetter(Services, 'audioManager',
                                   '@mozilla.org/telephony/audiomanager;1',
                                   'nsIAudioManager');
#else
Services.audioManager = {
  'masterVolume': 0
};
#endif

XPCOMUtils.defineLazyServiceGetter(Services, 'fm',
                                   '@mozilla.org/focus-manager;1',
                                   'nsIFocusManager');

XPCOMUtils.defineLazyGetter(this, 'DebuggerServer', function() {
  Cu.import('resource://gre/modules/devtools/dbg-server.jsm');
  return DebuggerServer;
});

XPCOMUtils.defineLazyGetter(this, "ppmm", function() {
  return Cc["@mozilla.org/parentprocessmessagemanager;1"]
         .getService(Ci.nsIMessageListenerManager);
});

#ifdef MOZ_WIDGET_GONK
XPCOMUtils.defineLazyGetter(this, "libcutils", function () {
  Cu.import("resource://gre/modules/systemlibs.js");
  return libcutils;
});
#endif

function getContentWindow() {
  return shell.contentBrowser.contentWindow;
}

var shell = {

  get CrashSubmit() {
    delete this.CrashSubmit;
    Cu.import("resource://gre/modules/CrashSubmit.jsm", this);
    return this.CrashSubmit;
  },

  onlineForCrashReport: function shell_onlineForCrashReport() {
    let wifiManager = navigator.mozWifiManager;
    let onWifi = (wifiManager &&
                  (wifiManager.connection.status == 'connected'));
    return !Services.io.offline && onWifi;
  },

  reportCrash: function shell_reportCrash(isChrome, aCrashID) {
    let crashID = aCrashID;
    try {
      
      if (isChrome) {
        crashID = Cc["@mozilla.org/xre/app-info;1"]
                    .getService(Ci.nsIXULRuntime).lastRunCrashID;
      }
    } catch(e) { }

    
    if (!crashID) {
      return;
    }

    try {
      
      if (Services.prefs.getBoolPref("app.reportCrashes")) {
        this.submitCrash(crashID);
      }
    } catch (e) { }

    
    this.sendChromeEvent({
      type: "handle-crash",
      crashID: crashID,
      chrome: isChrome
    });
  },

  
  submitCrash: function shell_submitCrash(aCrashID) {
    if (this.onlineForCrashReport()) {
      this.CrashSubmit.submit(aCrashID);
      return;
    }

    Services.obs.addObserver(function observer(subject, topic, state) {
      let network = subject.QueryInterface(Ci.nsINetworkInterface);
      if (network.state == Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED
          && network.type == Ci.nsINetworkInterface.NETWORK_TYPE_WIFI) {
        shell.CrashSubmit.submit(aCrashID);
        Services.obs.removeObserver(observer, topic);
      }
    }, "network-interface-state-changed", false);
  },

  get contentBrowser() {
    delete this.contentBrowser;
    return this.contentBrowser = document.getElementById('homescreen');
  },

  get homeURL() {
    try {
      let homeSrc = Services.env.get('B2G_HOMESCREEN');
      if (homeSrc)
        return homeSrc;
    } catch (e) {}

    return Services.prefs.getCharPref('browser.homescreenURL');
  },

  get manifestURL() {
    return Services.prefs.getCharPref('browser.manifestURL');
  },

  _started: false,
  hasStarted: function shell_hasStarted() {
    return this._started;
  },

  start: function shell_start() {
    this._started = true;

    
    
    
    let cookies = Cc["@mozilla.org/cookieService;1"];

    try {
      let cr = Cc["@mozilla.org/xre/app-info;1"]
                 .getService(Ci.nsICrashReporter);
      
      
      try {
        let dogfoodId = Services.prefs.getCharPref('prerelease.dogfood.id');
        if (dogfoodId != "") {
          cr.annotateCrashReport("Email", dogfoodId);
        }
      }
      catch (e) { }

#ifdef MOZ_WIDGET_GONK
      
      let annotations = [ [ "Android_Hardware",     "ro.hardware" ],
                          [ "Android_Device",       "ro.product.device" ],
                          [ "Android_CPU_ABI2",     "ro.product.cpu.abi2" ],
                          [ "Android_CPU_ABI",      "ro.product.cpu.abi" ],
                          [ "Android_Manufacturer", "ro.product.manufacturer" ],
                          [ "Android_Brand",        "ro.product.brand" ],
                          [ "Android_Model",        "ro.product.model" ],
                          [ "Android_Board",        "ro.product.board" ],
        ];

      annotations.forEach(function (element) {
          cr.annotateCrashReport(element[0], libcutils.property_get(element[1]));
        });

      let androidVersion = libcutils.property_get("ro.build.version.sdk") +
                           "(" + libcutils.property_get("ro.build.version.codename") + ")";
      cr.annotateCrashReport("Android_Version", androidVersion);
#endif
    } catch(e) { }

    let homeURL = this.homeURL;
    if (!homeURL) {
      let msg = 'Fatal error during startup: No homescreen found: try setting B2G_HOMESCREEN';
      alert(msg);
      return;
    }

    let manifestURL = this.manifestURL;
    
    
    
    
    let browserFrame =
      document.createElementNS('http://www.w3.org/1999/xhtml', 'html:iframe');
    browserFrame.setAttribute('id', 'homescreen');
    browserFrame.setAttribute('mozbrowser', 'true');
    browserFrame.setAttribute('mozapp', manifestURL);
    browserFrame.setAttribute('allowfullscreen', 'true');
    browserFrame.setAttribute('style', "overflow: hidden; -moz-box-flex: 1; border: none;");
    browserFrame.setAttribute('src', "data:text/html;charset=utf-8,%3C!DOCTYPE html>%3Cbody style='background:black;");
    document.getElementById('shell').appendChild(browserFrame);

    browserFrame.contentWindow
                .QueryInterface(Ci.nsIInterfaceRequestor)
                .getInterface(Ci.nsIWebNavigation)
                .sessionHistory = Cc["@mozilla.org/browser/shistory;1"]
                                    .createInstance(Ci.nsISHistory);

    
    
    
    
    
    window.addEventListener('keydown', this, true);
    window.addEventListener('keypress', this, true);
    window.addEventListener('keyup', this, true);
    window.addEventListener('MozApplicationManifest', this);
    window.addEventListener('mozfullscreenchange', this);
    window.addEventListener('sizemodechange', this);
    this.contentBrowser.addEventListener('mozbrowserloadstart', this, true);

    CustomEventManager.init();
    WebappsHelper.init();
    AccessFu.attach(window);
    UserAgentOverrides.init();

    
    SettingsListener.observe("debug.fps.enabled", false, function(value) {
      Services.prefs.setBoolPref("layers.acceleration.draw-fps", value);
    });
    SettingsListener.observe("debug.paint-flashing.enabled", false, function(value) {
      Services.prefs.setBoolPref("nglayout.debug.paint_flashing", value);
    });

    this.contentBrowser.src = homeURL;
    this.isHomeLoaded = false;

    ppmm.addMessageListener("content-handler", this);
    ppmm.addMessageListener("dial-handler", this);
    ppmm.addMessageListener("sms-handler", this);
    ppmm.addMessageListener("mail-handler", this);
    ppmm.addMessageListener("app-notification-send", AlertsHelper);
  },

  stop: function shell_stop() {
    window.removeEventListener('keydown', this, true);
    window.removeEventListener('keypress', this, true);
    window.removeEventListener('keyup', this, true);
    window.removeEventListener('MozApplicationManifest', this);
    window.removeEventListener('mozfullscreenchange', this);
    window.removeEventListener('sizemodechange', this);
    this.contentBrowser.removeEventListener('mozbrowserloadstart', this, true);
    ppmm.removeMessageListener("content-handler", this);
    if (this.timer) {
      this.timer.cancel();
      this.timer = null;
    }

#ifndef MOZ_WIDGET_GONK
    delete Services.audioManager;
#endif
    UserAgentOverrides.uninit();
  },

  
  
  
  filterHardwareKeys: function shell_filterHardwareKeys(evt) {
    var type;
    switch (evt.keyCode) {
      case evt.DOM_VK_HOME:         
        type = 'home-button';
        break;
      case evt.DOM_VK_SLEEP:        
      case evt.DOM_VK_END:          
        type = 'sleep-button';
        break;
      case evt.DOM_VK_PAGE_UP:      
        type = 'volume-up-button';
        break;
      case evt.DOM_VK_PAGE_DOWN:    
        type = 'volume-down-button';
        break;
      case evt.DOM_VK_ESCAPE:       
        type = 'back-button';
        break;
      case evt.DOM_VK_CONTEXT_MENU: 
        type = 'menu-button';
        break;
      case evt.DOM_VK_F1: 
        type = 'headset-button';
        break;
      default:                      
        return;  
    }

    
    
    evt.stopImmediatePropagation();
    evt.preventDefault(); 

    
    
    switch (evt.type) {
      case 'keydown':
        type = type + '-press';
        break;
      case 'keyup':
        type = type + '-release';
        break;
      case 'keypress':
        return;
    }

    
    if (evt.keyCode == evt.DOM_VK_F1 && type !== this.lastHardwareButtonEventType) {
      this.lastHardwareButtonEventType = type;
      gSystemMessenger.broadcastMessage('headset-button', type);
      return;
    }

    
    
    
    
    
    
    
    if (type !== this.lastHardwareButtonEventType) {
      this.lastHardwareButtonEventType = type;
      this.sendChromeEvent({type: type});
    }
  },

  lastHardwareButtonEventType: null, 
  needBufferSysMsgs: true,
  bufferedSysMsgs: [],
  timer: null,

  handleEvent: function shell_handleEvent(evt) {
    let content = this.contentBrowser.contentWindow;
    switch (evt.type) {
      case 'keydown':
      case 'keyup':
      case 'keypress':
        this.filterHardwareKeys(evt);
        break;
      case 'mozfullscreenchange':
        
        
        
        if (document.mozFullScreen)
          Services.fm.focusedWindow = window;
        break;
      case 'sizemodechange':
        if (window.windowState == window.STATE_MINIMIZED) {
          this.contentBrowser.setVisible(false);
        } else {
          this.contentBrowser.setVisible(true);
        }
        break;
      case 'mozbrowserloadstart':
        if (content.document.location == 'about:blank')
          return;

        this.contentBrowser.removeEventListener('mozbrowserloadstart', this, true);

        this.reportCrash(true);

        let chromeWindow = window.QueryInterface(Ci.nsIDOMChromeWindow);
        chromeWindow.browserDOMWindow = new nsBrowserAccess();

        Cu.import('resource://gre/modules/Webapps.jsm');
        DOMApplicationRegistry.allAppsLaunchable = true;

        this.sendEvent(window, 'ContentStart');

        content.addEventListener('load', function shell_homeLoaded() {
          content.removeEventListener('load', shell_homeLoaded);
          shell.isHomeLoaded = true;

          if ('pendingChromeEvents' in shell) {
            shell.pendingChromeEvents.forEach((shell.sendChromeEvent).bind(shell));
          }
          delete shell.pendingChromeEvents;
        });

        break;
      case 'MozApplicationManifest':
        try {
          if (!Services.prefs.getBoolPref('browser.cache.offline.enable'))
            return;

          let contentWindow = evt.originalTarget.defaultView;
          let documentElement = contentWindow.document.documentElement;
          if (!documentElement)
            return;

          let manifest = documentElement.getAttribute('manifest');
          if (!manifest)
            return;

          let principal = contentWindow.document.nodePrincipal;
          if (Services.perms.testPermissionFromPrincipal(principal, 'offline-app') == Ci.nsIPermissionManager.UNKNOWN_ACTION) {
            if (Services.prefs.getBoolPref('browser.offline-apps.notify')) {
              
              return;
            }
            return;
          }

          Services.perms.addFromPrincipal(principal, 'offline-app',
                                          Ci.nsIPermissionManager.ALLOW_ACTION);

          let manifestURI = Services.io.newURI(manifest, null, documentURI);
          let updateService = Cc['@mozilla.org/offlinecacheupdate-service;1']
                              .getService(Ci.nsIOfflineCacheUpdateService);
          updateService.scheduleUpdate(manifestURI, documentURI, window);
        } catch (e) {
          dump('Error while creating offline cache: ' + e + '\n');
        }
        break;
    }
  },

  sendEvent: function shell_sendEvent(content, type, details) {
    let event = content.document.createEvent('CustomEvent');
    event.initCustomEvent(type, true, true, details ? details : {});
    content.dispatchEvent(event);
  },

  sendChromeEvent: function shell_sendChromeEvent(details) {
    if (!this.isHomeLoaded) {
      if (!('pendingChromeEvents' in this)) {
        this.pendingChromeEvents = [];
      }

      this.pendingChromeEvents.push(details);
      return;
    }

    this.sendEvent(getContentWindow(), "mozChromeEvent",
                   ObjectWrapper.wrap(details, getContentWindow()));
  },

  sendSystemMessage: function shell_sendSystemMessage(msg) {
    let origin = Services.io.newURI(msg.manifest, null, null).prePath;
    this.sendChromeEvent({
      type: 'open-app',
      url: msg.uri,
      manifestURL: msg.manifest,
      isActivity: (msg.type == 'activity'),
      target: msg.target
    });
  },

  receiveMessage: function shell_receiveMessage(message) {
    var names = { 'content-handler': 'view',
                  'dial-handler'   : 'dial',
                  'mail-handler'   : 'new',
                  'sms-handler'    : 'new' }

    if (!(message.name in names))
      return;

    let data = message.data;
    new MozActivity({
      name: names[message.name],
      data: data
    });
  }
};

function nsBrowserAccess() {
}

nsBrowserAccess.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIBrowserDOMWindow]),

  openURI: function openURI(uri, opener, where, context) {
    
    let content = shell.contentBrowser.contentWindow;
    let contentWindow = content.wrappedJSObject;
    if (!('getApplicationManager' in contentWindow))
      return null;

    let applicationManager = contentWindow.getApplicationManager();
    if (!applicationManager)
      return null;

    let url = uri ? uri.spec : 'about:blank';
    let window = applicationManager.launch(url, where);
    return window.contentWindow;
  },

  openURIInFrame: function openURIInFrame(uri, opener, where, context) {
    throw new Error('Not Implemented');
  },

  isTabContentWindow: function isTabContentWindow(contentWindow) {
    return contentWindow == window;
  }
};


Services.obs.addObserver(function onSystemMessage(subject, topic, data) {
  let msg = JSON.parse(data);
  
  
  if (shell.needBufferSysMsgs && msg.type !== 'activity') {
    shell.bufferedSysMsgs.push(msg);
    return;
  }
  shell.sendSystemMessage(msg);
}, 'system-messages-open-app', false);

Services.obs.addObserver(function(aSubject, aTopic, aData) {
  shell.sendChromeEvent({ type: "fullscreenoriginchange",
                          fullscreenorigin: aData });
}, "fullscreen-origin-change", false);

Services.obs.addObserver(function onWebappsStart(subject, topic, data) {
  shell.sendChromeEvent({ type: 'webapps-registry-start' });
}, 'webapps-registry-start', false);

Services.obs.addObserver(function onWebappsReady(subject, topic, data) {
  shell.sendChromeEvent({ type: 'webapps-registry-ready' });
}, 'webapps-registry-ready', false);

Services.obs.addObserver(function onBluetoothVolumeChange(subject, topic, data) {
  shell.sendChromeEvent({
    type: "bluetooth-volumeset",
    value: data
  });
}, 'bluetooth-volume-change', false);

var CustomEventManager = {
  init: function custevt_init() {
    window.addEventListener("ContentStart", (function(evt) {
      let content = shell.contentBrowser.contentWindow;
      content.addEventListener("mozContentEvent", this, false, true);

      
      
      shell.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      shell.timer.initWithCallback(function timerCallback() {
        shell.bufferedSysMsgs.forEach(function sendSysMsg(msg) {
          shell.sendSystemMessage(msg);
        });
        shell.bufferedSysMsgs.length = 0;
        shell.needBufferSysMsgs = false;
        shell.timer = null;
      }, 10000, Ci.nsITimer.TYPE_ONE_SHOT);
    }).bind(this), false);
  },

  handleEvent: function custevt_handleEvent(evt) {
    let detail = evt.detail;
    dump('XXX FIXME : Got a mozContentEvent: ' + detail.type + "\n");

    switch(detail.type) {
      case 'desktop-notification-click':
      case 'desktop-notification-close':
        AlertsHelper.handleEvent(detail);
        break;
      case 'webapps-install-granted':
      case 'webapps-install-denied':
        WebappsHelper.handleEvent(detail);
        break;
      case 'select-choicechange':
        FormsHelper.handleEvent(detail);
        break;
      case 'system-message-listener-ready':
        Services.obs.notifyObservers(null, 'system-message-listener-ready', null);
        break;
    }
  }
}

var AlertsHelper = {
  _listeners: {},
  _count: 0,

  handleEvent: function alert_handleEvent(detail) {
    if (!detail || !detail.id)
      return;

    let listener = this._listeners[detail.id];
    if (!listener)
     return;

    let topic = detail.type == "desktop-notification-click" ? "alertclickcallback"
                                                            : "alertfinished";

    if (detail.id.startsWith("alert")) {
      listener.observer.observe(null, topic, listener.cookie);
    } else {
      listener.mm.sendAsyncMessage("app-notification-return",
                                   { id: detail.id,
                                     type: detail.type });
    }

    
    if (topic === "alertfinished")
      delete this._listeners[detail.id];
  },

  registerListener: function alert_registerListener(cookie, alertListener) {
    let id = "alert" + this._count++;
    this._listeners[id] = { observer: alertListener, cookie: cookie };
    return id;
  },

  registerAppListener: function alertRegisterAppListener(id, mm, title, text,
                                                         manifestURL, imageURL) {
    this._listeners[id] = {
      mm: mm,
      title: title,
      text: text,
      manifestURL: manifestURL,
      imageURL: imageURL
    };
  },

  showNotification: function alert_showNotification(imageUrl,
                                                    title,
                                                    text,
                                                    textClickable,
                                                    cookie,
                                                    id,
                                                    name,
                                                    manifestUrl) {
    function send(appName, appIcon) {
      shell.sendChromeEvent({
        type: "desktop-notification",
        id: id,
        icon: imageUrl,
        title: title,
        text: text,
        appName: appName,
        appIcon: appIcon
      });
    }

    
    
    if (manifestUrl && manifestUrl.length) {
      let app = DOMApplicationRegistry.getAppByManifestURL(manifestUrl);
      DOMApplicationRegistry.getManifestFor(app.origin, function(aManifest) {
        let helper = new ManifestHelper(aManifest, app.origin);
        send(helper.name, helper.iconURLForSize(128));
      });
    } else {
      send(null, null);
    }
  },

  showAlertNotification: function alert_showAlertNotification(imageUrl,
                                                              title,
                                                              text,
                                                              textClickable,
                                                              cookie,
                                                              alertListener,
                                                              name) {
    let id = this.registerListener(null, alertListener);
    this.showNotification(imageUrl, title, text, textClickable, cookie,
                          id, name, null);
  },

  receiveMessage: function alert_receiveMessage(message) {
    let data = message.data;

    this.registerAppListener(data.id, message.target, data.title, data.text,
                             data.manifestURL, data.imageURL);
    this.showNotification(data.imageURL, data.title, data.text,
                          data.textClickable, null,
                          data.id, null, data.manifestURL);
  },
}

var WebappsHelper = {
  _installers: {},
  _count: 0,

  init: function webapps_init() {
    Services.obs.addObserver(this, "webapps-launch", false);
    Services.obs.addObserver(this, "webapps-ask-install", false);
  },

  registerInstaller: function webapps_registerInstaller(data) {
    let id = "installer" + this._count++;
    this._installers[id] = data;
    return id;
  },

  handleEvent: function webapps_handleEvent(detail) {
    if (!detail || !detail.id)
      return;

    let installer = this._installers[detail.id];
    switch (detail.type) {
      case "webapps-install-granted":
        DOMApplicationRegistry.confirmInstall(installer);
        break;
      case "webapps-install-denied":
        DOMApplicationRegistry.denyInstall(installer);
        break;
    }
  },

  observe: function webapps_observe(subject, topic, data) {
    let json = JSON.parse(data);
    json.mm = subject;

    switch(topic) {
      case "webapps-launch":
        DOMApplicationRegistry.getManifestFor(json.origin, function(aManifest) {
          if (!aManifest)
            return;

          let manifest = new ManifestHelper(aManifest, json.origin);
          shell.sendChromeEvent({
            "type": "webapps-launch",
            "url": manifest.fullLaunchPath(json.startPoint),
            "manifestURL": json.manifestURL
          });
        });
        break;
      case "webapps-ask-install":
        let id = this.registerInstaller(json);
        shell.sendChromeEvent({
          type: "webapps-ask-install",
          id: id,
          app: json.app
        });
        break;
    }
  }
}


function startDebugger() {
  if (!DebuggerServer.initialized) {
    
    DebuggerServer.init(function () { return true; });
    DebuggerServer.addBrowserActors();
    DebuggerServer.addActors('chrome://browser/content/dbg-browser-actors.js');
  }

  let port = Services.prefs.getIntPref('devtools.debugger.remote-port') || 6000;
  try {
    DebuggerServer.openListener(port);
  } catch (e) {
    dump('Unable to start debugger server: ' + e + '\n');
  }
}

function stopDebugger() {
  if (!DebuggerServer.initialized) {
    return;
  }

  try {
    DebuggerServer.closeListener();
  } catch (e) {
    dump('Unable to stop debugger server: ' + e + '\n');
  }
}






window.addEventListener('ContentStart', function ss_onContentStart() {
  let content = shell.contentBrowser.contentWindow;
  content.addEventListener('mozContentEvent', function ss_onMozContentEvent(e) {
    if (e.detail.type !== 'take-screenshot')
      return;

    try {
      var canvas = document.createElementNS('http://www.w3.org/1999/xhtml',
                                            'canvas');
      var width = window.innerWidth;
      var height = window.innerHeight;
      canvas.setAttribute('width', width);
      canvas.setAttribute('height', height);

      var context = canvas.getContext('2d');
      var flags =
        context.DRAWWINDOW_DRAW_CARET |
        context.DRAWWINDOW_DRAW_VIEW |
        context.DRAWWINDOW_USE_WIDGET_LAYERS;
      context.drawWindow(window, 0, 0, width, height,
                         'rgb(255,255,255)', flags);

      
      
      
      shell.sendEvent(getContentWindow(), 'mozChromeEvent', {
        __exposedProps__: { type: 'r', file: 'r' },
        type: 'take-screenshot-success',
        file: canvas.mozGetAsFile('screenshot', 'image/png')
      });
    } catch (e) {
      dump('exception while creating screenshot: ' + e + '\n');
      shell.sendChromeEvent({
        type: 'take-screenshot-error',
        error: String(e)
      });
    }
  });
});

(function contentCrashTracker() {
  Services.obs.addObserver(function(aSubject, aTopic, aData) {
      let props = aSubject.QueryInterface(Ci.nsIPropertyBag2);
      if (props.hasKey("abnormal") && props.hasKey("dumpID")) {
        shell.reportCrash(false, props.getProperty("dumpID"));
      }
    },
    "ipc:content-shutdown", false);
})();


window.addEventListener('ContentStart', function cr_onContentStart() {
  let content = shell.contentBrowser.contentWindow;
  content.addEventListener("mozContentEvent", function cr_onMozContentEvent(e) {
    if (e.detail.type == "submit-crash" && e.detail.crashID) {
      shell.submitCrash(e.detail.crashID);
    }
  });
});

window.addEventListener('ContentStart', function update_onContentStart() {
  let updatePrompt = Cc["@mozilla.org/updates/update-prompt;1"]
                       .createInstance(Ci.nsIUpdatePrompt);

  let content = shell.contentBrowser.contentWindow;
  content.addEventListener("mozContentEvent", updatePrompt.wrappedJSObject);
});

(function geolocationStatusTracker() {
  let gGeolocationActiveCount = 0;

  Services.obs.addObserver(function(aSubject, aTopic, aData) {
    let oldCount = gGeolocationActiveCount;
    if (aData == "starting") {
      gGeolocationActiveCount += 1;
    } else if (aData == "shutdown") {
      gGeolocationActiveCount -= 1;
    }

    
    if (gGeolocationActiveCount + oldCount == 1) {
      shell.sendChromeEvent({
        type: 'geolocation-status',
        active: (gGeolocationActiveCount == 1)
      });
    }
}, "geolocation-device-events", false);
})();

(function headphonesStatusTracker() {
  Services.obs.addObserver(function(aSubject, aTopic, aData) {
    shell.sendChromeEvent({
      type: 'headphones-status-changed',
      state: aData
    });
}, "headphones-status-changed", false);
})();

(function recordingStatusTracker() {
  let gRecordingActiveCount = 0;

  Services.obs.addObserver(function(aSubject, aTopic, aData) {
    let oldCount = gRecordingActiveCount;
    if (aData == "starting") {
      gRecordingActiveCount += 1;
    } else if (aData == "shutdown") {
      gRecordingActiveCount -= 1;
    }

    
    if (gRecordingActiveCount + oldCount == 1) {
      shell.sendChromeEvent({
        type: 'recording-status',
        active: (gRecordingActiveCount == 1)
      });
    }
}, "recording-device-events", false);
})();

(function volumeStateTracker() {
  Services.obs.addObserver(function(aSubject, aTopic, aData) {
    shell.sendChromeEvent({
      type: 'volume-state-changed',
      active: (aData == 'Shared')
    });
}, 'volume-state-changed', false);
})();

Services.obs.addObserver(function(aSubject, aTopic, aData) {
  let data = JSON.parse(aData);
  shell.sendChromeEvent({
    type: "activity-done",
    success: data.success,
    manifestURL: data.manifestURL,
    pageURL: data.pageURL
  });
}, "activity-done", false);
