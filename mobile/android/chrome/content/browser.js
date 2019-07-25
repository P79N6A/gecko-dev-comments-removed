




































"use strict";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;
let Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm")
Cu.import("resource://gre/modules/AddonManager.jsm");

XPCOMUtils.defineLazyGetter(this, "PluralForm", function() {
  Cu.import("resource://gre/modules/PluralForm.jsm");
  return PluralForm;
});


[
  ["SelectHelper", "chrome://browser/content/SelectHelper.js"],
].forEach(function (aScript) {
  let [name, script] = aScript;
  XPCOMUtils.defineLazyGetter(window, name, function() {
    let sandbox = {};
    Services.scriptloader.loadSubScript(script, sandbox);
    return sandbox[name];
  });
});

XPCOMUtils.defineLazyServiceGetter(this, "Haptic",
  "@mozilla.org/widget/hapticfeedback;1", "nsIHapticFeedback");

XPCOMUtils.defineLazyServiceGetter(this, "DOMUtils",
  "@mozilla.org/inspector/dom-utils;1", "inIDOMUtils");

const kStateActive = 0x00000001; 

const kXLinkNamespace = "http://www.w3.org/1999/xlink";




const kPanDeceleration = 0.999;


const kSwipeLength = 500;



const kDragThreshold = 10;


const kLockBreakThreshold = 100;



const kMinKineticSpeed = 0.015;



const kMaxKineticSpeed = 9;



const kAxisLockRatio = 5;



const kElementsReceivingInput = {
    applet: true,
    audio: true,
    button: true,
    embed: true,
    input: true,
    map: true,
    select: true,
    textarea: true,
    video: true
};

function dump(a) {
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).logStringMessage(a);
}

function getBridge() {
  return Cc["@mozilla.org/android/bridge;1"].getService(Ci.nsIAndroidBridge);
}

function sendMessageToJava(aMessage) {
  return getBridge().handleGeckoMessage(JSON.stringify(aMessage));
}

#ifdef MOZ_CRASHREPORTER
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "CrashReporter",
  "@mozilla.org/xre/app-info;1", "nsICrashReporter");
#endif

XPCOMUtils.defineLazyGetter(this, "ContentAreaUtils", function() {
  let ContentAreaUtils = {};
  Services.scriptloader.loadSubScript("chrome://global/content/contentAreaUtils.js", ContentAreaUtils);
  return ContentAreaUtils;
});

XPCOMUtils.defineLazyGetter(this, "Rect", function() {
  Cu.import("resource://gre/modules/Geometry.jsm");
  return Rect;
});

function resolveGeckoURI(aURI) {
  if (aURI.indexOf("chrome://") == 0) {
    let registry = Cc['@mozilla.org/chrome/chrome-registry;1'].getService(Ci["nsIChromeRegistry"]);
    return registry.convertChromeURL(Services.io.newURI(aURI, null, null)).spec;
  } else if (aURI.indexOf("resource://") == 0) {
    let handler = Services.io.getProtocolHandler("resource").QueryInterface(Ci.nsIResProtocolHandler);
    return handler.resolveURI(Services.io.newURI(aURI, null, null));
  }
  return aURI;
}




var Strings = {};
[
  ["brand",      "chrome://branding/locale/brand.properties"],
  ["browser",    "chrome://browser/locale/browser.properties"],
  ["charset",    "chrome://global/locale/charsetTitles.properties"]
].forEach(function (aStringBundle) {
  let [name, bundle] = aStringBundle;
  XPCOMUtils.defineLazyGetter(Strings, name, function() {
    return Services.strings.createBundle(bundle);
  });
});

var MetadataProvider = {
  getDrawMetadata: function getDrawMetadata() {
    return BrowserApp.getDrawMetadata();
  },

  paintingSuppressed: function paintingSuppressed() {
    
    let tab = BrowserApp.selectedTab;
    if (!tab)
      return false;

    
    
    
    
    
    
    
    
    
    

    let viewportDocumentId = tab.documentIdForCurrentViewport;
    let contentDocumentId = ViewportHandler.getIdForDocument(tab.browser.contentDocument);
    if (viewportDocumentId != null && viewportDocumentId != contentDocumentId)
      return true;

    
    let cwu = tab.browser.contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                       .getInterface(Ci.nsIDOMWindowUtils);
    return cwu.paintingSuppressed;
  }
};

var BrowserApp = {
  _tabs: [],
  _selectedTab: null,

  deck: null,

  startup: function startup() {
    window.QueryInterface(Ci.nsIDOMChromeWindow).browserDOMWindow = new nsBrowserAccess();
    dump("zerdatime " + Date.now() + " - browser chrome startup finished.");

    this.deck = document.getElementById("browsers");
    BrowserEventHandler.init();
    ViewportHandler.init();

    getBridge().setDrawMetadataProvider(MetadataProvider);

    getBridge().browserApp = this;

    Services.obs.addObserver(this, "Tab:Add", false);
    Services.obs.addObserver(this, "Tab:Load", false);
    Services.obs.addObserver(this, "Tab:Selected", false);
    Services.obs.addObserver(this, "Tab:Closed", false);
    Services.obs.addObserver(this, "Session:Back", false);
    Services.obs.addObserver(this, "Session:Forward", false);
    Services.obs.addObserver(this, "Session:Reload", false);
    Services.obs.addObserver(this, "Session:Stop", false);
    Services.obs.addObserver(this, "SaveAs:PDF", false);
    Services.obs.addObserver(this, "Browser:Quit", false);
    Services.obs.addObserver(this, "Preferences:Get", false);
    Services.obs.addObserver(this, "Preferences:Set", false);
    Services.obs.addObserver(this, "ScrollTo:FocusedInput", false);
    Services.obs.addObserver(this, "Sanitize:ClearAll", false);
    Services.obs.addObserver(this, "PanZoom:PanZoom", false);
    Services.obs.addObserver(this, "FullScreen:Exit", false);
    Services.obs.addObserver(this, "Viewport:Change", false);
    Services.obs.addObserver(this, "SearchEngines:Get", false);
    Services.obs.addObserver(this, "Passwords:Init", false);
    Services.obs.addObserver(this, "FormHistory:Init", false);

    Services.obs.addObserver(this, "sessionstore-state-purge-complete", false);

    function showFullScreenWarning() {
      NativeWindow.toast.show(Strings.browser.GetStringFromName("alertFullScreenToast"), "short");
    }

    window.addEventListener("fullscreen", function() {
      sendMessageToJava({
        gecko: {
          type: window.fullScreen ? "ToggleChrome:Show" : "ToggleChrome:Hide"
        }
      });
    }, false);

    window.addEventListener("mozfullscreenchange", function() {
      sendMessageToJava({
        gecko: {
          type: document.mozFullScreen ? "DOMFullScreen:Start" : "DOMFullScreen:Stop"
        }
      });

      if (document.mozFullScreen)
        showFullScreenWarning();
    }, false);

    
    
    window.addEventListener("MozShowFullScreenWarning", showFullScreenWarning, true);

    NativeWindow.init();
    Downloads.init();
    FormAssistant.init();
    OfflineApps.init();
    IndexedDB.init();
    XPInstallObserver.init();
    ConsoleAPI.init();
    ClipboardHelper.init();
    PermissionsHelper.init();
    CharacterEncoding.init();

    
    Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);
    
    Cc["@mozilla.org/satchel/form-history;1"].getService(Ci.nsIFormHistory2);

    let url = "about:home";
    let forceRestore = false;
    if ("arguments" in window) {
      if (window.arguments[0])
        url = window.arguments[0];
      if (window.arguments[1])
        forceRestore = window.arguments[1];
      if (window.arguments[2])
        gScreenWidth = window.arguments[2];
      if (window.arguments[3])
        gScreenHeight = window.arguments[3];
    }

    
    Services.io.offline = false;

    
    let event = document.createEvent("Events");
    event.initEvent("UIReady", true, false);
    window.dispatchEvent(event);

    
    let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
    if (forceRestore || ss.shouldRestore()) {
      
      let restoreToFront = false;

      sendMessageToJava({
        gecko: {
          type: "Session:RestoreBegin"
        }
      });

      
      if (url && url != "about:home") {
        this.addTab(url);
      } else {
        
        restoreToFront = true;
      }

      
      let restoreCleanup = {
        observe: function(aSubject, aTopic, aData) {
          Services.obs.removeObserver(restoreCleanup, "sessionstore-windows-restored");
          if (aData == "fail") {
            BrowserApp.addTab("about:home", {
              showProgress: false,
              selected: restoreToFront
            });
          }

          sendMessageToJava({
            gecko: {
              type: "Session:RestoreEnd"
            }
          });
        }
      };
      Services.obs.addObserver(restoreCleanup, "sessionstore-windows-restored", false);

      
      ss.restoreLastSession(restoreToFront, forceRestore);
    } else {
      this.addTab(url, { showProgress: url != "about:home" });

      
      this._showTelemetryPrompt();
    }

    
    sendMessageToJava({
      gecko: {
        type: "Gecko:Ready"
      }
    });

    
    sendMessageToJava({
      gecko: {
        "type": "Checkerboard:Toggle",
        "value": Services.prefs.getBoolPref("gfx.show_checkerboard_pattern")
      }
    });
  },

  _showTelemetryPrompt: function _showTelemetryPrompt() {
    const PREF_TELEMETRY_PROMPTED = "toolkit.telemetry.prompted";
    const PREF_TELEMETRY_ENABLED = "toolkit.telemetry.enabled";
    const PREF_TELEMETRY_REJECTED = "toolkit.telemetry.rejected";

    
    const TELEMETRY_PROMPT_REV = 2;

    let telemetryPrompted = null;
    try {
      telemetryPrompted = Services.prefs.getIntPref(PREF_TELEMETRY_PROMPTED);
    } catch (e) {  }

    
    
    if (telemetryPrompted === TELEMETRY_PROMPT_REV)
      return;

    Services.prefs.clearUserPref(PREF_TELEMETRY_PROMPTED);
    Services.prefs.clearUserPref(PREF_TELEMETRY_ENABLED);
  
    let buttons = [
      {
        label: Strings.browser.GetStringFromName("telemetry.optin.yes"),
        callback: function () {
          Services.prefs.setIntPref(PREF_TELEMETRY_PROMPTED, TELEMETRY_PROMPT_REV);
          Services.prefs.setBoolPref(PREF_TELEMETRY_ENABLED, true);
        }
      },
      {
        label: Strings.browser.GetStringFromName("telemetry.optin.no"),
        callback: function () {
          Services.prefs.setIntPref(PREF_TELEMETRY_PROMPTED, TELEMETRY_PROMPT_REV);
          Services.prefs.setBoolPref(PREF_TELEMETRY_REJECTED, true);
        }
      }
    ];

    let brandShortName = Strings.brand.GetStringFromName("brandShortName");
    let message = Strings.browser.formatStringFromName("telemetry.optin.message", [brandShortName], 1);
    NativeWindow.doorhanger.show(message, "telemetry-optin", buttons);
  },

  shutdown: function shutdown() {
    NativeWindow.uninit();
    FormAssistant.uninit();
    OfflineApps.uninit();
    IndexedDB.uninit();
    ViewportHandler.uninit();
    XPInstallObserver.uninit();
    ConsoleAPI.uninit();
    CharacterEncoding.uninit();
  },

  get tabs() {
    return this._tabs;
  },

  get selectedTab() {
    return this._selectedTab;
  },

  set selectedTab(aTab) {
    if (this._selectedTab)
      this._selectedTab.setActive(false);

    this._selectedTab = aTab;
    if (!aTab)
      return;

    aTab.setActive(true);
    aTab.updateViewport(false);
    this.deck.selectedPanel = aTab.vbox;
  },

  get selectedBrowser() {
    if (this._selectedTab)
      return this._selectedTab.browser;
    return null;
  },

  getTabForId: function getTabForId(aId) {
    let tabs = this._tabs;
    for (let i=0; i < tabs.length; i++) {
       if (tabs[i].id == aId)
         return tabs[i];
    }
    return null;
  },

  getTabForBrowser: function getTabForBrowser(aBrowser) {
    let tabs = this._tabs;
    for (let i = 0; i < tabs.length; i++) {
      if (tabs[i].browser == aBrowser)
        return tabs[i];
    }
    return null;
  },

  getBrowserForWindow: function getBrowserForWindow(aWindow) {
    let tabs = this._tabs;
    for (let i = 0; i < tabs.length; i++) {
      if (tabs[i].browser.contentWindow == aWindow)
        return tabs[i].browser;
    }
    return null;
  },

  getBrowserForDocument: function getBrowserForDocument(aDocument) {
    let tabs = this._tabs;
    for (let i = 0; i < tabs.length; i++) {
      if (tabs[i].browser.contentDocument == aDocument)
        return tabs[i].browser;
    }
    return null;
  },

  loadURI: function loadURI(aURI, aBrowser, aParams) {
    aBrowser = aBrowser || this.selectedBrowser;
    if (!aBrowser)
      return;

    aParams = aParams || {};

    let flags = "flags" in aParams ? aParams.flags : Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
    let postData = ("postData" in aParams && aParams.postData) ? aParams.postData.value : null;
    let referrerURI = "referrerURI" in aParams ? aParams.referrerURI : null;
    let charset = "charset" in aParams ? aParams.charset : null;

    if ("showProgress" in aParams) {
      let tab = this.getTabForBrowser(aBrowser);
      if (tab)
        tab.showProgress = aParams.showProgress;
    }

    try {
      aBrowser.loadURIWithFlags(aURI, flags, referrerURI, charset, postData);
    } catch(e) {
      let tab = this.getTabForBrowser(aBrowser);
      if (tab) {
        let message = {
          gecko: {
            type: "Content:LoadError",
            tabID: tab.id,
            uri: aBrowser.currentURI.spec,
            title: aBrowser.contentTitle
          }
        };
        sendMessageToJava(message);
        dump("Handled load error: " + e)
      }
    }
  },

  addTab: function addTab(aURI, aParams) {
    aParams = aParams || {};

    let newTab = new Tab(aURI, aParams);
    this._tabs.push(newTab);

    let selected = "selected" in aParams ? aParams.selected : true;
    if (selected)
      this.selectedTab = newTab;

    let evt = document.createEvent("UIEvents");
    evt.initUIEvent("TabOpen", true, false, window, null);
    newTab.browser.dispatchEvent(evt);

    return newTab;
  },

  
  
  
  closeTab: function closeTab(aTab) {
    if (!aTab) {
      Cu.reportError("Error trying to close tab (tab doesn't exist)");
      return;
    }

    let message = {
      gecko: {
        type: "Tab:Close",
        tabID: aTab.id
      }
    };
    sendMessageToJava(message);
  },

  
  
  _handleTabClosed: function _handleTabClosed(aTab) {
    if (aTab == this.selectedTab)
      this.selectedTab = null;

    let evt = document.createEvent("UIEvents");
    evt.initUIEvent("TabClose", true, false, window, null);
    aTab.browser.dispatchEvent(evt);

    aTab.destroy();
    this._tabs.splice(this._tabs.indexOf(aTab), 1);
  },

  
  
  
  selectTab: function selectTab(aTab) {
    if (!aTab) {
      Cu.reportError("Error trying to select tab (tab doesn't exist)");
      return;
    }

    let message = {
      gecko: {
        type: "Tab:Select",
        tabID: aTab.id
      }
    };
    sendMessageToJava(message);
  },

  
  
  _handleTabSelected: function _handleTabSelected(aTab) {
    this.selectedTab = aTab;

    let evt = document.createEvent("UIEvents");
    evt.initUIEvent("TabSelect", true, false, window, null);
    aTab.browser.dispatchEvent(evt);

    let message = {
      gecko: {
        type: "Tab:Selected:Done",
        tabID: aTab.id
      }
    };
    sendMessageToJava(message);
  },

  quit: function quit() {
    
    let lastBrowser = true;
    let e = Services.wm.getEnumerator("navigator:browser");
    while (e.hasMoreElements() && lastBrowser) {
      let win = e.getNext();
      if (win != window)
        lastBrowser = false;
    }

    if (lastBrowser) {
      
      let closingCanceled = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
      Services.obs.notifyObservers(closingCanceled, "browser-lastwindow-close-requested", null);
      if (closingCanceled.data)
        return;

      Services.obs.notifyObservers(null, "browser-lastwindow-close-granted", null);
    }

    window.QueryInterface(Ci.nsIDOMChromeWindow).minimize();
    window.close();
  },

  saveAsPDF: function saveAsPDF(aBrowser) {
    
    let fileName = ContentAreaUtils.getDefaultFileName(aBrowser.contentTitle, aBrowser.currentURI, null, null);
    fileName = fileName.trim() + ".pdf";

    let dm = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
    let downloadsDir = dm.defaultDownloadsDirectory;

    let file = downloadsDir.clone();
    file.append(fileName);
    file.createUnique(file.NORMAL_FILE_TYPE, parseInt("666", 8));

    let printSettings = Cc["@mozilla.org/gfx/printsettings-service;1"].getService(Ci.nsIPrintSettingsService).newPrintSettings;
    printSettings.printSilent = true;
    printSettings.showPrintProgress = false;
    printSettings.printBGImages = true;
    printSettings.printBGColors = true;
    printSettings.printToFile = true;
    printSettings.toFileName = file.path;
    printSettings.printFrameType = Ci.nsIPrintSettings.kFramesAsIs;
    printSettings.outputFormat = Ci.nsIPrintSettings.kOutputFormatPDF;

    
    printSettings.footerStrCenter = "";
    printSettings.footerStrLeft   = "";
    printSettings.footerStrRight  = "";
    printSettings.headerStrCenter = "";
    printSettings.headerStrLeft   = "";
    printSettings.headerStrRight  = "";

    
    let ms = Cc["@mozilla.org/mime;1"].getService(Ci.nsIMIMEService);
    let mimeInfo = ms.getFromTypeAndExtension("application/pdf", "pdf");

    let webBrowserPrint = aBrowser.contentWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                                .getInterface(Ci.nsIWebBrowserPrint);

    let cancelable = {
      cancel: function (aReason) {
        webBrowserPrint.cancel();
      }
    }
    let download = dm.addDownload(Ci.nsIDownloadManager.DOWNLOAD_TYPE_DOWNLOAD,
                                  aBrowser.currentURI,
                                  Services.io.newFileURI(file), "", mimeInfo,
                                  Date.now() * 1000, null, cancelable);

    webBrowserPrint.print(printSettings, download);
  },

  getPreferences: function getPreferences(aPrefNames) {
    try {
      let json = JSON.parse(aPrefNames);
      let prefs = [];

      for each (let prefName in json) {
        let pref = {
          name: prefName
        };

        
        
        if (prefName == "plugin.enable") {
          
          pref.type = "string";
          pref.value = PluginHelper.getPluginPreference();
          prefs.push(pref);
          continue;
        } else if (prefName == MasterPassword.pref) {
          
          pref.type = "bool";
          pref.value = MasterPassword.enabled;
          prefs.push(pref);
          continue;
        }

        try {
          switch (Services.prefs.getPrefType(prefName)) {
            case Ci.nsIPrefBranch.PREF_BOOL:
              pref.type = "bool";
              pref.value = Services.prefs.getBoolPref(prefName);
              break;
            case Ci.nsIPrefBranch.PREF_INT:
              pref.type = "int";
              pref.value = Services.prefs.getIntPref(prefName);
              break;
            case Ci.nsIPrefBranch.PREF_STRING:
            default:
              pref.type = "string";
              pref.value = Services.prefs.getComplexValue(prefName, Ci.nsIPrefLocalizedString).data;
              break;
          }
        } catch (e) {
            
            continue;
        }

        
        
        
        
        switch (prefName) {
          case "network.cookie.cookieBehavior":
            pref.type = "bool";
            pref.value = pref.value == 0;
            break;
          case "font.size.inflation.minTwips":
            pref.type = "string";
            pref.value = pref.value.toString();
            break;
        }

        prefs.push(pref);
      }

      sendMessageToJava({
        gecko: {
          type: "Preferences:Data",
          preferences: prefs
        }
      });
    } catch (e) {}
  },

  setPreferences: function setPreferences(aPref) {
    let json = JSON.parse(aPref);

    
    
    if (json.name == "plugin.enable") {
      PluginHelper.setPluginPreference(json.value);
      return;
    } else if(json.name == MasterPassword.pref) {
      if (MasterPassword.enabled)
        MasterPassword.removePassword(json.value);
      else
        MasterPassword.setPassword(json.value);
    }

    
    
    
    switch (json.name) {
      case "network.cookie.cookieBehavior":
        json.type = "int";
        json.value = (json.value ? 0 : 2);
        break;
      case "font.size.inflation.minTwips":
        json.type = "int";
        json.value = parseInt(json.value);
        break;
    }

    if (json.type == "bool")
      Services.prefs.setBoolPref(json.name, json.value);
    else if (json.type == "int")
      Services.prefs.setIntPref(json.name, json.value);
    else {
      let pref = Cc["@mozilla.org/pref-localizedstring;1"].createInstance(Ci.nsIPrefLocalizedString);
      pref.data = json.value;
      Services.prefs.setComplexValue(json.name, Ci.nsISupportsString, pref);
    }
  },

  getSearchEngines: function() {
    let engineData = Services.search.getVisibleEngines({});
    let searchEngines = engineData.map(function (engine) {
      return {
        name: engine.name,
        iconURI: engine.iconURI.spec
      };
    });

    sendMessageToJava({
      gecko: {
        type: "SearchEngines:Data",
        searchEngines: searchEngines
      }
    });
  },

  getSearchOrURI: function getSearchOrURI(aParams) {
    let uri;
    if (aParams.engine) {
      let engine = Services.search.getEngineByName(aParams.engine);
      if (engine)
        uri = engine.getSubmission(aParams.url).uri;
    }
    return uri ? uri.spec : aParams.url;
  },

  scrollToFocusedInput: function(aBrowser) {
    let doc = aBrowser.contentDocument;
    if (!doc)
      return;
    let focused = doc.activeElement;
    if ((focused instanceof HTMLInputElement && focused.mozIsTextField(false)) || (focused instanceof HTMLTextAreaElement)) {
      let tab = BrowserApp.getTabForBrowser(aBrowser);
      let win = aBrowser.contentWindow;

      
      
      focused.scrollIntoView(false);

      
      
      tab.userScrollPos.x = win.scrollX;
      tab.userScrollPos.y = win.scrollY;

      
      
      
      
      
      
      
      
      
      
      
      

      let visibleContentWidth = tab._viewport.width / tab._viewport.zoom;
      let visibleContentHeight = tab._viewport.height / tab._viewport.zoom;
      
      
      let focusedRect = focused.getBoundingClientRect();
      focusedRect = {
        left: focusedRect.left - tab.viewportExcess.x,
        right: focusedRect.right - tab.viewportExcess.x,
        top: focusedRect.top - tab.viewportExcess.y,
        bottom: focusedRect.bottom - tab.viewportExcess.y
      };
      let transformChanged = false;
      if (focusedRect.right >= visibleContentWidth && focusedRect.left > 0) {
        
        tab.viewportExcess.x += Math.min(focusedRect.left, focusedRect.right - visibleContentWidth);
        transformChanged = true;
      } else if (focusedRect.left < 0) {
        
        tab.viewportExcess.x += focusedRect.left;
        transformChanged = true;
      }
      if (focusedRect.bottom >= visibleContentHeight && focusedRect.top > 0) {
        
        tab.viewportExcess.y += Math.min(focusedRect.top, focusedRect.bottom - visibleContentHeight);
        transformChanged = true;
      } else if (focusedRect.top < 0) {
        
        tab.viewportExcess.y += focusedRect.top;
        transformChanged = true;
      }
      if (transformChanged)
        tab.updateTransform();
      
      tab.sendViewportUpdate();
    }
  },

  getDrawMetadata: function getDrawMetadata() {
    let viewport = this.selectedTab.viewport;

    
    
    try {
      let browser = this.selectedBrowser;
      if (browser) {
        let { contentDocument, contentWindow } = browser;
        let computedStyle = contentWindow.getComputedStyle(contentDocument.body);
        viewport.backgroundColor = computedStyle.backgroundColor;
      }
    } catch (e) {
      
    }

    return JSON.stringify(viewport);
  },

  observe: function(aSubject, aTopic, aData) {
    let browser = this.selectedBrowser;
    if (!browser)
      return;

    if (aTopic == "Session:Back") {
      browser.goBack();
    } else if (aTopic == "Session:Forward") {
      browser.goForward();
    } else if (aTopic == "Session:Reload") {
      browser.reload();
    } else if (aTopic == "Session:Stop") {
      browser.stop();
    } else if (aTopic == "Tab:Add" || aTopic == "Tab:Load") {
      let data = JSON.parse(aData);

      
      
      let flags = Ci.nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP;
      if (data.userEntered)
        flags |= Ci.nsIWebNavigation.LOAD_FLAGS_DISALLOW_INHERIT_OWNER;

      let params = {
        selected: true,
        parentId: ("parentId" in data) ? data.parentId : -1,
        flags: flags
      };

      let url = this.getSearchOrURI(data);

      
      if (url == "about:home")
        params.showProgress = false;

      if (aTopic == "Tab:Add")
        this.addTab(url, params);
      else
        this.loadURI(url, browser, params);
    } else if (aTopic == "Tab:Selected") {
      this._handleTabSelected(this.getTabForId(parseInt(aData)));
    } else if (aTopic == "Tab:Closed") {
      this._handleTabClosed(this.getTabForId(parseInt(aData)));
    } else if (aTopic == "Browser:Quit") {
      this.quit();
    } else if (aTopic == "SaveAs:PDF") {
      this.saveAsPDF(browser);
    } else if (aTopic == "Preferences:Get") {
      this.getPreferences(aData);
    } else if (aTopic == "Preferences:Set") {
      this.setPreferences(aData);
    } else if (aTopic == "ScrollTo:FocusedInput") {
      this.scrollToFocusedInput(browser);
    } else if (aTopic == "Sanitize:ClearAll") {
      Sanitizer.sanitize();
    } else if (aTopic == "FullScreen:Exit") {
      browser.contentDocument.mozCancelFullScreen();
    } else if (aTopic == "Viewport:Change") {
      this.selectedTab.viewport = JSON.parse(aData);
      ViewportHandler.onResize();
    } else if (aTopic == "SearchEngines:Get") {
      this.getSearchEngines();
    } else if (aTopic == "Passwords:Init") {
      var storage = Components.classes["@mozilla.org/login-manager/storage/mozStorage;1"].  
        getService(Components.interfaces.nsILoginManagerStorage);
      storage.init();

      sendMessageToJava({gecko: { type: "Passwords:Init:Return" }});
      Services.obs.removeObserver(this, "Passwords:Init", false);
    } else if (aTopic == "FormHistory:Init") {
      var fh = Components.classes["@mozilla.org/satchel/form-history;1"].  
        getService(Components.interfaces.nsIFormHistory2);
      var db = fh.DBConnection;
      sendMessageToJava({gecko: { type: "FormHistory:Init:Return" }});
      Services.obs.removeObserver(this, "FormHistory:Init", false);
    } else if (aTopic == "sessionstore-state-purge-complete") {
      sendMessageToJava({ gecko: { type: "Session:StatePurged" }});
    }
  },

  get defaultBrowserWidth() {
    delete this.defaultBrowserWidth;
    let width = Services.prefs.getIntPref("browser.viewport.desktopWidth");
    return this.defaultBrowserWidth = width;
  },

  
  getWindowForTab: function(tabId) {
      let tab = this.getTabForId(tabId);
      if (!tab.browser)
	  return null;
      return tab.browser.contentWindow;
  }

};

var NativeWindow = {
  init: function() {
    Services.obs.addObserver(this, "Menu:Clicked", false);
    Services.obs.addObserver(this, "Doorhanger:Reply", false);
    this.contextmenus.init();
  },

  uninit: function() {
    Services.obs.removeObserver(this, "Menu:Clicked");
    Services.obs.removeObserver(this, "Doorhanger:Reply");
    this.contextmenus.uninit();
  },

  toast: {
    show: function(aMessage, aDuration) {
      sendMessageToJava({
        gecko: {
          type: "Toast:Show",
          message: aMessage,
          duration: aDuration
        }
      });
    }
  },

  menu: {
    _callbacks: [],
    _menuId: 0,
    add: function(aName, aIcon, aCallback) {
      sendMessageToJava({
        gecko: {
          type: "Menu:Add",
          name: aName,
          icon: aIcon,
          id: this._menuId
        }
      });
      this._callbacks[this._menuId] = aCallback;
      this._menuId++;
      return this._menuId - 1;
    },

    remove: function(aId) {
      sendMessageToJava({ gecko: {type: "Menu:Remove", id: aId }});
    }
  },

  doorhanger: {
    _callbacks: {},
    _callbacksId: 0,
    _promptId: 0,

  









    show: function(aMessage, aValue, aButtons, aTabID, aOptions) {
      aButtons.forEach((function(aButton) {
        this._callbacks[this._callbacksId] = { cb: aButton.callback, prompt: this._promptId };
        aButton.callback = this._callbacksId;
        this._callbacksId++;
      }).bind(this));

      this._promptId++;
      let json = {
        gecko: {
          type: "Doorhanger:Add",
          message: aMessage,
          value: aValue,
          buttons: aButtons,
          
          tabID: aTabID || BrowserApp.selectedTab.id,
          options: aOptions || {}
        }
      };
      sendMessageToJava(json);
    },

    hide: function(aValue, aTabID) {
      sendMessageToJava({
        type: "Doorhanger:Remove",
        value: aValue,
        tabID: aTabID
      });
    }
  },

  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "Menu:Clicked") {
      if (this.menu._callbacks[aData])
        this.menu._callbacks[aData]();
    } else if (aTopic == "Doorhanger:Reply") {
      let reply_id = aData;
      if (this.doorhanger._callbacks[reply_id]) {
        let prompt = this.doorhanger._callbacks[reply_id].prompt;
        this.doorhanger._callbacks[reply_id].cb();
        for (let id in this.doorhanger._callbacks) {
          if (this.doorhanger._callbacks[id].prompt == prompt) {
            delete this.doorhanger._callbacks[id];
          }
        }
      }
    }
  },
  contextmenus: {
    items: {}, 
    _contextId: 0, 

    init: function() {
      this.imageContext = this.SelectorContext("img");

      Services.obs.addObserver(this, "Gesture:LongPress", false);

      
      this.add(Strings.browser.GetStringFromName("contextmenu.openInNewTab"),
               this.linkOpenableContext,
               function(aTarget) {
                 let url = NativeWindow.contextmenus._getLinkURL(aTarget);
                 BrowserApp.addTab(url, { selected: false, parentId: BrowserApp.selectedTab.id });

                 let newtabStrings = Strings.browser.GetStringFromName("newtabpopup.opened");
                 let label = PluralForm.get(1, newtabStrings).replace("#1", 1);
                 NativeWindow.toast.show(label, "short");
               });

      this.add(Strings.browser.GetStringFromName("contextmenu.shareLink"),
               this.linkShareableContext,
               function(aTarget) {
                 let url = NativeWindow.contextmenus._getLinkURL(aTarget);
                 let title = aTarget.textContent || aTarget.title;
                 let sharing = Cc["@mozilla.org/uriloader/external-sharing-app-service;1"].getService(Ci.nsIExternalSharingAppService);
                 sharing.shareWithDefault(url, "text/plain", title);
               });

      this.add(Strings.browser.GetStringFromName("contextmenu.bookmarkLink"),
               this.linkBookmarkableContext,
               function(aTarget) {
                 let url = NativeWindow.contextmenus._getLinkURL(aTarget);
                 let title = aTarget.textContent || aTarget.title || url;
                 sendMessageToJava({
                   gecko: {
                     type: "Bookmark:Insert",
                     url: url,
                     title: title
                   }
                 });
               });

      this.add(Strings.browser.GetStringFromName("contextmenu.fullScreen"),
               this.SelectorContext("video:not(:-moz-full-screen)"),
               function(aTarget) {
                 aTarget.mozRequestFullScreen();
               });

      this.add(Strings.browser.GetStringFromName("contextmenu.saveImage"),
               this.imageContext,
               function(aTarget) {
                 let imageCache = Cc["@mozilla.org/image/cache;1"].getService(Ci.imgICache);
                 let props = imageCache.findEntryProperties(aTarget.currentURI, aTarget.ownerDocument.characterSet);
                 var contentDisposition = "";
                 var type = "";
                 try {
                    String(props.get("content-disposition", Ci.nsISupportsCString));
                    String(props.get("type", Ci.nsISupportsCString));
                 } catch(ex) { }
                 var browser = BrowserApp.getBrowserForDocument(aTarget.ownerDocument);
                 ContentAreaUtils.internalSave(aTarget.currentURI.spec, null, null, contentDisposition, type, false, "SaveImageTitle", null, browser.documentURI, true, null);
               });
    },

    uninit: function() {
      Services.obs.removeObserver(this, "Gesture:LongPress");
    },

    add: function(aName, aSelector, aCallback) {
      if (!aName)
        throw "Menu items must have a name";

      let item = {
        name: aName,
        context: aSelector,
        callback: aCallback,
        matches: function(aElt) {
          return this.context.matches(aElt);
        },
        getValue: function() {
          return {
            label: this.name,
            id: this.id
          }
        }
      };
      item.id = this._contextId++;
      this.items[item.id] = item;
      return item.id;
    },

    remove: function(aId) {
      delete this.items[aId];
    },

    SelectorContext: function(aSelector) {
      return {
        matches: function(aElt) {
          if (aElt.mozMatchesSelector)
            return aElt.mozMatchesSelector(aSelector);
          return false;
        }
      }
    },

    linkOpenableContext: {
      matches: function linkOpenableContextMatches(aElement) {
        let uri = NativeWindow.contextmenus._getLink(aElement);
        if (uri) {
          let scheme = uri.scheme;
          let dontOpen = /^(mailto|javascript|news|snews)$/;
          return (scheme && !dontOpen.test(scheme));
        }
        return false;
      }
    },

    linkShareableContext: {
      matches: function linkShareableContextMatches(aElement) {
        let uri = NativeWindow.contextmenus._getLink(aElement);
        if (uri) {
          let scheme = uri.scheme;
          let dontShare = /^(chrome|about|file|javascript|resource)$/;
          return (scheme && !dontShare.test(scheme));
        }
        return false;
      }
    },

    linkBookmarkableContext: {
      matches: function linkBookmarkableContextMatches(aElement) {
        let uri = NativeWindow.contextmenus._getLink(aElement);
        if (uri) {
          let scheme = uri.scheme;
          let dontBookmark = /^(mailto)$/;
          return (scheme && !dontBookmark.test(scheme));
        }
        return false;
      }
    },

    textContext: {
      matches: function textContext(aElement) {
        return ((aElement instanceof Ci.nsIDOMHTMLInputElement && aElement.mozIsTextField(false))
                || aElement instanceof Ci.nsIDOMHTMLTextAreaElement);
      }
    },

    _sendToContent: function(aX, aY) {
      
      let rootElement = ElementTouchHelper.elementFromPoint(BrowserApp.selectedBrowser.contentWindow, aX, aY);
      if (!rootElement)
        rootElement = ElementTouchHelper.anyElementFromPoint(BrowserApp.selectedBrowser.contentWindow, aX, aY)

      this.menuitems = null;
      let element = rootElement;
      if (!element)
        return;

      while (element) {
        for each (let item in this.items) {
          
          
          if ((!this.menuitems || !this.menuitems[item.id]) && item.matches(element)) {
            if (!this.menuitems)
              this.menuitems = {};
            this.menuitems[item.id] = item;
          }
        }

        if (this.linkOpenableContext.matches(element) || this.textContext.matches(element))
          break;
        element = element.parentNode;
      }

      
      if (this.menuitems) {
        BrowserEventHandler.blockClick = true;
        let event = rootElement.ownerDocument.createEvent("MouseEvent");
        event.initMouseEvent("contextmenu", true, true, content,
                             0, aX, aY, aX, aY, false, false, false, false,
                             0, null);
        rootElement.ownerDocument.defaultView.addEventListener("contextmenu", this, false);
        rootElement.dispatchEvent(event);
      }
    },

    _show: function(aEvent) {
      if (aEvent.defaultPrevented)
        return;

      let popupNode = aEvent.originalTarget;
      let title = "";
      if ((popupNode instanceof Ci.nsIDOMHTMLAnchorElement && popupNode.href) ||
              (popupNode instanceof Ci.nsIDOMHTMLAreaElement && popupNode.href)) {
        title = this._getLinkURL(popupNode);
      } else if (popupNode instanceof Ci.nsIImageLoadingContent && popupNode.currentURI) {
        title = popupNode.currentURI.spec;
      } else if (popupNode instanceof Ci.nsIDOMHTMLMediaElement) {
        title = (popupNode.currentSrc || popupNode.src);
      }

      
      let itemArray = [];
      for each (let item in this.menuitems) {
        itemArray.push(item.getValue());
      }

      let msg = {
        gecko: {
          type: "Prompt:Show",
          title: title,
          listitems: itemArray
        }
      };
      let data = JSON.parse(sendMessageToJava(msg));
      let selectedId = itemArray[data.button].id;
      let selectedItem = this.menuitems[selectedId];

      if (selectedItem && selectedItem.callback) {
        while (popupNode) {
          if (selectedItem.matches(popupNode)) {
            selectedItem.callback.call(selectedItem, popupNode);
            break;
          }
          popupNode = popupNode.parentNode;
        }
      }
      this.menuitems = null;
    },

    handleEvent: function(aEvent) {
      aEvent.target.ownerDocument.defaultView.removeEventListener("contextmenu", this, false);
      this._show(aEvent);
    },

    observe: function(aSubject, aTopic, aData) {
      BrowserEventHandler._cancelTapHighlight();
      let data = JSON.parse(aData);
      
      this._sendToContent(data.x, data.y);
    },

    
    makeURLAbsolute: function makeURLAbsolute(base, url) {
      
      return this.makeURI(url, null, this.makeURI(base)).spec;
    },

    makeURI: function makeURI(aURL, aOriginCharset, aBaseURI) {
      return Services.io.newURI(aURL, aOriginCharset, aBaseURI);
    },

    _getLink: function(aElement) {
      if (aElement.nodeType == Ci.nsIDOMNode.ELEMENT_NODE &&
          ((aElement instanceof Ci.nsIDOMHTMLAnchorElement && aElement.href) ||
          (aElement instanceof Ci.nsIDOMHTMLAreaElement && aElement.href) ||
          aElement instanceof Ci.nsIDOMHTMLLinkElement ||
          aElement.getAttributeNS(kXLinkNamespace, "type") == "simple")) {
        try {
          let url = NativeWindow.contextmenus._getLinkURL(aElement);
          return Services.io.newURI(url, null, null);
        } catch (e) {}
      }
      return null;
    },

    _getLinkURL: function ch_getLinkURL(aLink) {
      let href = aLink.href;
      if (href)
        return href;

      href = aLink.getAttributeNS(kXLinkNamespace, "href");
      if (!href || !href.match(/\S/)) {
        
        
        throw "Empty href";
      }

      return this.makeURLAbsolute(aLink.baseURI, href);
    }
  }
};


function nsBrowserAccess() {
}

nsBrowserAccess.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIBrowserDOMWindow]),

  _getBrowser: function _getBrowser(aURI, aOpener, aWhere, aContext) {
    let isExternal = (aContext == Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL);
    if (isExternal && aURI && aURI.schemeIs("chrome"))
      return null;

    let loadflags = isExternal ?
                      Ci.nsIWebNavigation.LOAD_FLAGS_FROM_EXTERNAL :
                      Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
    if (aWhere == Ci.nsIBrowserDOMWindow.OPEN_DEFAULTWINDOW) {
      switch (aContext) {
        case Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL:
          aWhere = Services.prefs.getIntPref("browser.link.open_external");
          break;
        default: 
          aWhere = Services.prefs.getIntPref("browser.link.open_newwindow");
      }
    }

    Services.io.offline = false;

    let referrer;
    if (aOpener) {
      try {
        let location = aOpener.location;
        referrer = Services.io.newURI(location, null, null);
      } catch(e) { }
    }

    let newTab = (aWhere == Ci.nsIBrowserDOMWindow.OPEN_NEWWINDOW || aWhere == Ci.nsIBrowserDOMWindow.OPEN_NEWTAB);

    if (newTab) {
      let parentId = -1;
      if (!isExternal) {
        let parent = BrowserApp.getTabForBrowser(BrowserApp.getBrowserForWindow(aOpener.top));
        if (parent)
          parentId = parent.id;
      }

      
      let tab = BrowserApp.addTab(aURI ? aURI.spec : "about:blank", { flags: loadflags,
                                                                      referrerURI: referrer,
                                                                      external: isExternal,
                                                                      parentId: parentId,
                                                                      selected: true });
      return tab.browser;
    }

    
    let browser = BrowserApp.selectedBrowser;
    if (aURI && browser)
      browser.loadURIWithFlags(aURI.spec, loadflags, referrer, null, null);

    return browser;
  },

  openURI: function browser_openURI(aURI, aOpener, aWhere, aContext) {
    let browser = this._getBrowser(aURI, aOpener, aWhere, aContext);
    return browser ? browser.contentWindow : null;
  },

  openURIInFrame: function browser_openURIInFrame(aURI, aOpener, aWhere, aContext) {
    let browser = this._getBrowser(aURI, aOpener, aWhere, aContext);
    return browser ? browser.QueryInterface(Ci.nsIFrameLoaderOwner) : null;
  },

  isTabContentWindow: function(aWindow) {
    return BrowserApp.getBrowserForWindow(aWindow) != null;
  }
};


let gTabIDFactory = 0;



let gScreenWidth = 1;
let gScreenHeight = 1;

function Tab(aURL, aParams) {
  this.browser = null;
  this.vbox = null;
  this.id = 0;
  this.showProgress = true;
  this.create(aURL, aParams);
  this._viewport = { x: 0, y: 0, width: gScreenWidth, height: gScreenHeight, offsetX: 0, offsetY: 0,
                     pageWidth: gScreenWidth, pageHeight: gScreenHeight, zoom: 1.0 };
  this.viewportExcess = { x: 0, y: 0 };
  this.documentIdForCurrentViewport = null;
  this.userScrollPos = { x: 0, y: 0 };
  this._pluginCount = 0;
  this._pluginOverlayShowing = false;
}

Tab.prototype = {
  create: function(aURL, aParams) {
    if (this.browser)
      return;

    aParams = aParams || {};

    this.vbox = document.createElement("vbox");
    this.vbox.align = "start";
    BrowserApp.deck.appendChild(this.vbox);

    this.browser = document.createElement("browser");
    this.browser.setAttribute("type", "content-targetable");
    this.setBrowserSize(980, 480);
    this.browser.style.MozTransformOrigin = "0 0";
    this.vbox.appendChild(this.browser);

    this.browser.stop();

    
    let frameLoader = this.browser.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader;
    frameLoader.clipSubdocument = false;

    
    let uri = null;
    try {
      uri = Services.io.newURI(aURL, null, null).spec;
    } catch (e) {}

    this.id = ++gTabIDFactory;

    let message = {
      gecko: {
        type: "Tab:Added",
        tabID: this.id,
        uri: uri,
        parentId: ("parentId" in aParams) ? aParams.parentId : -1,
        external: ("external" in aParams) ? aParams.external : false,
        selected: ("selected" in aParams) ? aParams.selected : true,
        title: aParams.title || aURL,
        delayLoad: aParams.delayLoad || false
      }
    };
    sendMessageToJava(message);

    let flags = Ci.nsIWebProgress.NOTIFY_STATE_ALL |
                Ci.nsIWebProgress.NOTIFY_LOCATION |
                Ci.nsIWebProgress.NOTIFY_SECURITY;
    this.browser.addProgressListener(this, flags);
    this.browser.sessionHistory.addSHistoryListener(this);

    this.browser.addEventListener("DOMContentLoaded", this, true);
    this.browser.addEventListener("DOMLinkAdded", this, true);
    this.browser.addEventListener("DOMTitleChanged", this, true);
    this.browser.addEventListener("DOMWindowClose", this, true);
    this.browser.addEventListener("scroll", this, true);
    this.browser.addEventListener("PluginClickToPlay", this, true);
    this.browser.addEventListener("pagehide", this, true);
    this.browser.addEventListener("pageshow", this, true);

    Services.obs.addObserver(this, "document-shown", false);

    if (!aParams.delayLoad) {
      let flags = "flags" in aParams ? aParams.flags : Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
      let postData = ("postData" in aParams && aParams.postData) ? aParams.postData.value : null;
      let referrerURI = "referrerURI" in aParams ? aParams.referrerURI : null;
      let charset = "charset" in aParams ? aParams.charset : null;

      
      this.showProgress = "showProgress" in aParams ? aParams.showProgress : true;

      try {
        this.browser.loadURIWithFlags(aURL, flags, referrerURI, charset, postData);
      } catch(e) {
        let message = {
          gecko: {
            type: "Content:LoadError",
            tabID: this.id,
            uri: this.browser.currentURI.spec,
            title: this.browser.contentTitle
          }
        };
        sendMessageToJava(message);
        dump("Handled load error: " + e)
      }
    }
  },

  destroy: function() {
    if (!this.browser)
      return;

    this.browser.removeProgressListener(this);
    this.browser.removeEventListener("DOMContentLoaded", this, true);
    this.browser.removeEventListener("DOMLinkAdded", this, true);
    this.browser.removeEventListener("DOMTitleChanged", this, true);
    this.browser.removeEventListener("DOMWindowClose", this, true);
    this.browser.removeEventListener("scroll", this, true);
    this.browser.removeEventListener("PluginClickToPlay", this, true);
    this.browser.removeEventListener("pagehide", this, true);
    this.browser.removeEventListener("pageshow", this, true);

    Services.obs.removeObserver(this, "document-shown");

    
    
    let selectedPanel = BrowserApp.deck.selectedPanel;
    BrowserApp.deck.removeChild(this.vbox);
    BrowserApp.deck.selectedPanel = selectedPanel;

    this.browser = null;
    this.vbox = null;
    this.documentIdForCurrentViewport = null;
  },

  
  setActive: function setActive(aActive) {
    if (!this.browser)
      return;

    if (aActive) {
      this.browser.setAttribute("type", "content-primary");
      this.browser.focus();
      this.browser.docShellIsActive = true;
    } else {
      this.browser.setAttribute("type", "content-targetable");
      this.browser.docShellIsActive = false;
    }
  },

  set viewport(aViewport) {
    
    aViewport.x /= aViewport.zoom;
    aViewport.y /= aViewport.zoom;

    
    let win = this.browser.contentWindow;
    win.scrollTo(aViewport.x, aViewport.y);
    this.userScrollPos.x = win.scrollX;
    this.userScrollPos.y = win.scrollY;

    
    
    let excessX = aViewport.x - win.scrollX;
    let excessY = aViewport.y - win.scrollY;

    this._viewport.width = gScreenWidth = aViewport.width;
    this._viewport.height = gScreenHeight = aViewport.height;

    let transformChanged = false;

    if ((aViewport.offsetX != this._viewport.offsetX) ||
        (excessX != this.viewportExcess.x)) {
      this._viewport.offsetX = aViewport.offsetX;
      this.viewportExcess.x = excessX;
      transformChanged = true;
    }
    if ((aViewport.offsetY != this._viewport.offsetY) ||
        (excessY != this.viewportExcess.y)) {
      this._viewport.offsetY = aViewport.offsetY;
      this.viewportExcess.y = excessY;
      transformChanged = true;
    }
    if (Math.abs(aViewport.zoom - this._viewport.zoom) >= 1e-6) {
      this._viewport.zoom = aViewport.zoom;
      transformChanged = true;
    }

    if (transformChanged)
      this.updateTransform();
  },

  updateTransform: function() {
    let hasZoom = (Math.abs(this._viewport.zoom - 1.0) >= 1e-6);
    let x = this._viewport.offsetX + Math.round(-this.viewportExcess.x * this._viewport.zoom);
    let y = this._viewport.offsetY + Math.round(-this.viewportExcess.y * this._viewport.zoom);

    let transform =
      "translate(" + x + "px, " +
                     y + "px)";
    if (hasZoom)
      transform += " scale(" + this._viewport.zoom + ")";

    this.browser.style.MozTransform = transform;
  },

  get viewport() {
    
    this._viewport.x = (this.browser.contentWindow.scrollX +
                        this.viewportExcess.x) || 0;
    this._viewport.y = (this.browser.contentWindow.scrollY +
                        this.viewportExcess.y) || 0;

    
    this._viewport.x = Math.round(this._viewport.x * this._viewport.zoom);
    this._viewport.y = Math.round(this._viewport.y * this._viewport.zoom);

    let doc = this.browser.contentDocument;
    if (doc != null) {
      let pageWidth = this._viewport.width, pageHeight = this._viewport.height;
      if (doc instanceof SVGDocument) {
        let rect = doc.rootElement.getBoundingClientRect();
        
        
        
        
        pageWidth = Math.ceil(rect.left + rect.width + rect.left);
        pageHeight = Math.ceil(rect.top + rect.height + rect.top);
      } else {
        let body = doc.body || { scrollWidth: pageWidth, scrollHeight: pageHeight };
        let html = doc.documentElement || { scrollWidth: pageWidth, scrollHeight: pageHeight };
        pageWidth = Math.max(body.scrollWidth, html.scrollWidth);
        pageHeight = Math.max(body.scrollHeight, html.scrollHeight);
      }

      
      pageWidth *= this._viewport.zoom;
      pageHeight *= this._viewport.zoom;

      




      if (doc.readyState === 'complete' || (pageWidth >= gScreenWidth && pageHeight >= gScreenHeight)) {
        this._viewport.pageWidth = pageWidth;
        this._viewport.pageHeight = pageHeight;
      }
    }

    return this._viewport;
  },

  updateViewport: function(aReset, aZoomLevel) {
    if (!aZoomLevel)
      aZoomLevel = this.getDefaultZoomLevel();

    let win = this.browser.contentWindow;
    let zoom = (aReset ? aZoomLevel : this._viewport.zoom);
    let xpos = ((aReset && win) ? win.scrollX * zoom : this._viewport.x);
    let ypos = ((aReset && win) ? win.scrollY * zoom : this._viewport.y);

    this.viewportExcess = { x: 0, y: 0 };
    this.viewport = { x: xpos, y: ypos,
                      offsetX: 0, offsetY: 0,
                      width: this._viewport.width, height: this._viewport.height,
                      pageWidth: gScreenWidth, pageHeight: gScreenHeight,
                      zoom: zoom };
    this.sendViewportUpdate();
  },

  sendViewportUpdate: function() {
    if (BrowserApp.selectedTab != this)
      return;
    sendMessageToJava({
      gecko: {
        type: "Viewport:UpdateAndDraw"
      }
    });
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "DOMContentLoaded": {
        let target = aEvent.originalTarget;

        
        if (target.defaultView != this.browser.contentWindow)
          return;

        sendMessageToJava({
          gecko: {
            type: "DOMContentLoaded",
            tabID: this.id,
            windowID: 0,
            uri: this.browser.currentURI.spec,
            title: this.browser.contentTitle
          }
        });

        
        
        
        
        if (/^about:/.test(target.documentURI)) {
          this.browser.addEventListener("click", ErrorPageEventHandler, false);
          this.browser.addEventListener("pagehide", function listener() {
            this.browser.removeEventListener("click", ErrorPageEventHandler, false);
            this.browser.removeEventListener("pagehide", listener, true);
          }.bind(this), true);
        }

        
        
        
        if (this._pluginCount && !this._pluginOverlayShowing)
          PluginHelper.showDoorHanger(this);

        break;
      }

      case "DOMLinkAdded": {
        let target = aEvent.originalTarget;
        if (!target.href || target.disabled)
          return;

        
        if (target.ownerDocument.defaultView != this.browser.contentWindow)
          return;

        
        let list = [];
        if (target.rel) {
          list = target.rel.toLowerCase().split(/\s+/);
          let hash = {};
          list.forEach(function(value) { hash[value] = true; });
          list = [];
          for (let rel in hash)
            list.push("[" + rel + "]");
        }

        let json = {
          type: "DOMLinkAdded",
          tabID: this.id,
          href: resolveGeckoURI(target.href),
          charset: target.ownerDocument.characterSet,
          title: target.title,
          rel: list.join(" ")
        };

        
        if (target.hasAttribute("sizes"))
          json.sizes = target.getAttribute("sizes");

        sendMessageToJava({ gecko: json });
        break;
      }

      case "DOMTitleChanged": {
        if (!aEvent.isTrusted)
          return;

        
        if (aEvent.target.defaultView != this.browser.contentWindow)
          return;

        sendMessageToJava({
          gecko: {
            type: "DOMTitleChanged",
            tabID: this.id,
            title: aEvent.target.title.substring(0, 255)
          }
        });
        break;
      }

      case "DOMWindowClose": {
        if (!aEvent.isTrusted)
          return;

        
        if (this.browser.contentWindow == aEvent.target) {
          aEvent.preventDefault();

          sendMessageToJava({
            gecko: {
              type: "DOMWindowClose",
              tabID: this.id
            }
          });
        }
        break;
      }

      case "scroll": {
        let win = this.browser.contentWindow;
        if (this.userScrollPos.x != win.scrollX || this.userScrollPos.y != win.scrollY) {
          sendMessageToJava({
            gecko: {
              type: "Viewport:UpdateLater"
            }
          });
        }
        break;
      }

      case "PluginClickToPlay": {
        
        
        this._pluginCount++;

        let plugin = aEvent.target;
        let overlay = plugin.ownerDocument.getAnonymousElementByAttribute(plugin, "class", "mainBox");
        if (!overlay)
          return;

        
        
        if (PluginHelper.isTooSmall(plugin, overlay)) {
          overlay.style.visibility = "hidden";
          return;
        }

        
        overlay.addEventListener("click", (function(event) {
          
          PluginHelper.playAllPlugins(this, event);
        }).bind(this), true);

        this._pluginOverlayShowing = true;
        break;
      }

      case "pagehide": {
        
        if (aEvent.target.defaultView == this.browser.contentWindow) {
          
          this._pluginCount = 0;
          this._pluginOverlayShowing = false;
        }
        break;
      }
    }
  },

  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus) {
    let contentWin = aWebProgress.DOMWindow;
    if (contentWin != contentWin.top)
        return;

    if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK) {
      
      let browser = BrowserApp.getBrowserForWindow(aWebProgress.DOMWindow);
      let uri = "";
      if (browser)
        uri = browser.currentURI.spec;

      
      
      let restoring = aStateFlags & Ci.nsIWebProgressListener.STATE_RESTORING;
      let showProgress = restoring ? false : this.showProgress;

      let message = {
        gecko: {
          type: "Content:StateChange",
          tabID: this.id,
          uri: uri,
          state: aStateFlags,
          showProgress: showProgress
        }
      };
      sendMessageToJava(message);

      
      this.showProgress = true;
    }
  },

  onLocationChange: function(aWebProgress, aRequest, aLocationURI, aFlags) {
    let contentWin = aWebProgress.DOMWindow;
    if (contentWin != contentWin.top)
        return;

    let browser = BrowserApp.getBrowserForWindow(contentWin);
    let uri = browser.currentURI.spec;
    let documentURI = "";
    let contentType = "";
    if (browser.contentDocument) {
      documentURI = browser.contentDocument.documentURIObject.spec;
      contentType = browser.contentDocument.contentType;
    }

    let message = {
      gecko: {
        type: "Content:LocationChange",
        tabID: this.id,
        uri: uri,
        documentURI: documentURI,
        contentType: contentType
      }
    };

    sendMessageToJava(message);

    if ((aFlags & Ci.nsIWebProgressListener.LOCATION_CHANGE_SAME_DOCUMENT) == 0) {
      this.updateViewport(true);
    } else {
      this.sendViewportUpdate();
    }
  },

  onSecurityChange: function(aWebProgress, aRequest, aState) {
    let mode = "unknown";
    if (aState & Ci.nsIWebProgressListener.STATE_IDENTITY_EV_TOPLEVEL)
      mode = "identified";
    else if (aState & Ci.nsIWebProgressListener.STATE_SECURE_HIGH)
      mode = "verified";
    else if (aState & Ci.nsIWebProgressListener.STATE_IS_BROKEN)
      mode = "mixed";
    else
      mode = "unknown";

    let message = {
      gecko: {
        type: "Content:SecurityChange",
        tabID: this.id,
        mode: mode
      }
    };

     sendMessageToJava(message);
  },

  onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress) {
  },

  onStatusChange: function(aBrowser, aWebProgress, aRequest, aStatus, aMessage) {
  },

  _sendHistoryEvent: function(aMessage, aIndex, aUri) {
    let message = {
      gecko: {
        type: "SessionHistory:" + aMessage,
        tabID: this.id,
      }
    };
    if (aIndex != -1) {
      message.gecko.index = aIndex;
    }
    if (aUri != null) {
      message.gecko.uri = aUri;
    }
    sendMessageToJava(message);
  },

  OnHistoryNewEntry: function(aUri) {
    this._sendHistoryEvent("New", -1, aUri.spec);
  },

  OnHistoryGoBack: function(aUri) {
    this._sendHistoryEvent("Back", -1, null);
    return true;
  },

  OnHistoryGoForward: function(aUri) {
    this._sendHistoryEvent("Forward", -1, null);
    return true;
  },

  OnHistoryReload: function(aUri, aFlags) {
    
    
    return true;
  },

  OnHistoryGotoIndex: function(aIndex, aUri) {
    this._sendHistoryEvent("Goto", aIndex, null);
    return true;
  },

  OnHistoryPurge: function(aNumEntries) {
    this._sendHistoryEvent("Purge", -1, null);
    return true;
  },

  get metadata() {
    return ViewportHandler.getMetadataForDocument(this.browser.contentDocument);
  },

  
  updateViewportMetadata: function updateViewportMetadata(aMetadata) {
    if (aMetadata && aMetadata.autoScale) {
      let scaleRatio = aMetadata.scaleRatio = ViewportHandler.getScaleRatio();

      if ("defaultZoom" in aMetadata && aMetadata.defaultZoom > 0)
        aMetadata.defaultZoom *= scaleRatio;
      if ("minZoom" in aMetadata && aMetadata.minZoom > 0)
        aMetadata.minZoom *= scaleRatio;
      if ("maxZoom" in aMetadata && aMetadata.maxZoom > 0)
        aMetadata.maxZoom *= scaleRatio;
    }
    ViewportHandler.setMetadataForDocument(this.browser.contentDocument, aMetadata);
    this.updateViewportSize();
    this.updateViewport(true);
  },

  
  updateViewportSize: function updateViewportSize() {
    let browser = this.browser;
    if (!browser)
      return;

    let screenW = this._viewport.width;
    let screenH = this._viewport.height;
    let viewportW, viewportH;

    let metadata = this.metadata;
    if (metadata.autoSize) {
      if ("scaleRatio" in metadata) {
        viewportW = screenW / metadata.scaleRatio;
        viewportH = screenH / metadata.scaleRatio;
      } else {
        viewportW = screenW;
        viewportH = screenH;
      }
    } else {
      viewportW = metadata.width;
      viewportH = metadata.height;

      
      let maxInitialZoom = metadata.defaultZoom || metadata.maxZoom;
      if (maxInitialZoom && viewportW)
        viewportW = Math.max(viewportW, screenW / maxInitialZoom);

      let validW = viewportW > 0;
      let validH = viewportH > 0;

      if (!validW)
        viewportW = validH ? (viewportH * (screenW / screenH)) : BrowserApp.defaultBrowserWidth;
      if (!validH)
        viewportH = viewportW * (screenH / screenW);
    }

    
    
    let minScale = this.getPageZoomLevel(screenW);
    viewportH = Math.max(viewportH, screenH / minScale);

    let oldBrowserWidth = parseInt(this.browser.style.minWidth);
    this.setBrowserSize(viewportW, viewportH);

    
    let win = this.browser.contentWindow;
    this.userScrollPos.x = win.scrollX;
    this.userScrollPos.y = win.scrollY;

    
    

    if (viewportW == oldBrowserWidth)
      return;

    let viewport = this.viewport;
    let newZoom = oldBrowserWidth * viewport.zoom / viewportW;
    this.updateViewport(true, newZoom);
  },

  getDefaultZoomLevel: function getDefaultZoomLevel() {
    let md = this.metadata;
    if ("defaultZoom" in md && md.defaultZoom)
      return md.defaultZoom;

    let browserWidth = parseInt(this.browser.style.minWidth);
    return gScreenWidth / browserWidth;
  },

  getPageZoomLevel: function getPageZoomLevel() {
    
    
    if (!this.browser.contentDocument || !this.browser.contentDocument.body)
      return 1.0;

    return this._viewport.width / this.browser.contentDocument.body.clientWidth;
  },

  setBrowserSize: function(aWidth, aHeight) {
    
    
    this.browser.style.minWidth = aWidth + "px";
    this.browser.style.minHeight = aHeight + "px";
  },

  getRequestLoadContext: function(aRequest) {
    if (aRequest && aRequest.notificationCallbacks) {
      try {
        return aRequest.notificationCallbacks.getInterface(Ci.nsILoadContext);
      } catch (ex) { }
    }

    if (aRequest && aRequest.loadGroup && aRequest.loadGroup.notificationCallbacks) {
      try {
        return aRequest.loadGroup.notificationCallbacks.getInterface(Ci.nsILoadContext);
      } catch (ex) { }
    }

    return null;
  },

  getWindowForRequest: function(aRequest) {
    let loadContext = this.getRequestLoadContext(aRequest);
    if (loadContext)
      return loadContext.associatedWindow;
    return null;
  },

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "document-shown":
        
        let contentDocument = aSubject;
        if (contentDocument == this.browser.contentDocument) {
          ViewportHandler.updateMetadata(this);
          this.documentIdForCurrentViewport = ViewportHandler.getIdForDocument(contentDocument);
        }
        break;
    }
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIWebProgressListener,
    Ci.nsISHistoryListener,
    Ci.nsIObserver,
    Ci.nsISupportsWeakReference
  ])
};

var BrowserEventHandler = {
  init: function init() {
    Services.obs.addObserver(this, "Gesture:SingleTap", false);
    Services.obs.addObserver(this, "Gesture:ShowPress", false);
    Services.obs.addObserver(this, "Gesture:CancelTouch", false);
    Services.obs.addObserver(this, "Gesture:DoubleTap", false);
    Services.obs.addObserver(this, "Gesture:Scroll", false);
    Services.obs.addObserver(this, "dom-touch-listener-added", false);

    BrowserApp.deck.addEventListener("DOMUpdatePageReport", PopupBlockerObserver.onUpdatePageReport, false);
  },

  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "Gesture:Scroll") {
      
      
      
      if (this._scrollableElement == null)
        return;

      
      
      
      let data = JSON.parse(aData);
      if (this._firstScrollEvent) {
        while (this._scrollableElement != null && !this._elementCanScroll(this._scrollableElement, data.x, data.y))
          this._scrollableElement = this._findScrollableElement(this._scrollableElement, false);

        let doc = BrowserApp.selectedBrowser.contentDocument;
        if (this._scrollableElement == doc.body || this._scrollableElement == doc.documentElement) {
          sendMessageToJava({ gecko: { type: "Panning:CancelOverride" } });
          return;
        }

        this._firstScrollEvent = false;
      }

      
      if (this._elementCanScroll(this._scrollableElement, data.x, data.y)) {
        this._scrollElementBy(this._scrollableElement, data.x, data.y);
        sendMessageToJava({ gecko: { type: "Gesture:ScrollAck", scrolled: true } });
      } else {
        sendMessageToJava({ gecko: { type: "Gesture:ScrollAck", scrolled: false } });
      }
    } else if (aTopic == "Gesture:CancelTouch") {
      this._cancelTapHighlight();
    } else if (aTopic == "Gesture:ShowPress") {
      let data = JSON.parse(aData);
      let closest = ElementTouchHelper.elementFromPoint(BrowserApp.selectedBrowser.contentWindow, data.x, data.y);
      if (!closest)
        closest = ElementTouchHelper.anyElementFromPoint(BrowserApp.selectedBrowser.contentWindow, data.x, data.y);
      if (closest) {
        this._doTapHighlight(closest);

        
        
        this._scrollableElement = this._findScrollableElement(closest, true);
        this._firstScrollEvent = true;

        if (this._scrollableElement != null) {
          
          let doc = BrowserApp.selectedBrowser.contentDocument;
          if (this._scrollableElement != doc.body && this._scrollableElement != doc.documentElement)
            sendMessageToJava({ gecko: { type: "Panning:Override" } });
        }
      }
    } else if (aTopic == "Gesture:SingleTap") {
      let element = this._highlightElement;
      if (element && !SelectHelper.handleClick(element)) {
        try {
          let data = JSON.parse(aData);
          [data.x, data.y] = ElementTouchHelper.toScreenCoords(element.ownerDocument.defaultView, data.x, data.y);
  
          this._sendMouseEvent("mousemove", element, data.x, data.y);
          this._sendMouseEvent("mousedown", element, data.x, data.y);
          this._sendMouseEvent("mouseup",   element, data.x, data.y);
  
          if (ElementTouchHelper.isElementClickable(element))
            Haptic.performSimpleAction(Haptic.LongPress);
        } catch(e) {
          Cu.reportError(e);
        }
      }
      this._cancelTapHighlight();
    } else if (aTopic == "Gesture:DoubleTap") {
      this._cancelTapHighlight();
      this.onDoubleTap(aData);
    } else if (aTopic == "dom-touch-listener-added") {
      let browser = BrowserApp.getBrowserForWindow(aSubject);
      if (!browser)
        return;

      let tab = BrowserApp.getTabForBrowser(browser);
      if (!tab)
        return;

      sendMessageToJava({
        gecko: {
          type: "Tab:HasTouchListener",
          tabID: tab.id
        }
      });
    }
  },
 
  _zoomOut: function() {
    sendMessageToJava({ gecko: { type: "Browser:ZoomToPageWidth"} });
  },

  onDoubleTap: function(aData) {
    let data = JSON.parse(aData);

    let win = BrowserApp.selectedBrowser.contentWindow;
    
    let zoom = BrowserApp.selectedTab._viewport.zoom;
    let element = ElementTouchHelper.anyElementFromPoint(win, data.x, data.y);
    if (!element) {
      this._zoomOut();
      return;
    }

    win = element.ownerDocument.defaultView;
    while (element && win.getComputedStyle(element,null).display == "inline")
      element = element.parentNode;

    if (!element) {
      this._zoomOut();
    } else {
      const margin = 15;
      const minDifference = -20;
      const maxDifference = 20;
      let rect = ElementTouchHelper.getBoundingContentRect(element);

      let viewport = BrowserApp.selectedTab.viewport;
      let vRect = new Rect(viewport.x, viewport.y, viewport.width, viewport.height);

      let zoom = viewport.zoom;
      let bRect = new Rect(Math.max(0,rect.x - margin),
                           rect.y,
                           rect.w + 2*margin,
                           rect.h);
      
      bRect.width = Math.min(bRect.width, viewport.pageWidth/zoom - bRect.x);
      bRect.scale(zoom, zoom);

      let overlap = vRect.intersect(bRect);
      let overlapArea = overlap.width*overlap.height;
      
      
      let availHeight = Math.min(bRect.width*vRect.height/vRect.width, bRect.height);
      let showing = overlapArea/(bRect.width*availHeight);
      let dw = (bRect.width - vRect.width)/zoom;
      let dx = (bRect.x - vRect.x)/zoom;

      if (showing > 0.9 &&
          dx > minDifference && dx < maxDifference &&
          dw > minDifference && dw < maxDifference) {
            this._zoomOut();
            return;
      }

      rect.type = "Browser:ZoomToRect";
      rect.x = bRect.x; rect.y = bRect.y;
      rect.w = bRect.width; rect.h = availHeight;
      sendMessageToJava({ gecko: rect });
    }
  },

  _firstScrollEvent: false,

  _scrollableElement: null,

  _highlightElement: null,

  _doTapHighlight: function _doTapHighlight(aElement) {
    DOMUtils.setContentState(aElement, kStateActive);
    this._highlightElement = aElement;
  },

  _cancelTapHighlight: function _cancelTapHighlight() {
    DOMUtils.setContentState(BrowserApp.selectedBrowser.contentWindow.document.documentElement, kStateActive);
    this._highlightElement = null;
  },

  _updateLastPosition: function(x, y, dx, dy) {
    this.lastX = x;
    this.lastY = y;
    this.lastTime = Date.now();

    this.motionBuffer.push({ dx: dx, dy: dy, time: this.lastTime });
  },

  _sendMouseEvent: function _sendMouseEvent(aName, aElement, aX, aY, aButton) {
    
    
    if (!(aElement instanceof HTMLHtmlElement)) {
      let isTouchClick = true;
      let rects = ElementTouchHelper.getContentClientRects(aElement);
      for (let i = 0; i < rects.length; i++) {
        let rect = rects[i];
        
        
        let inBounds =
          (aX > rect.left + 1 && aX < (rect.left + rect.width - 1)) &&
          (aY > rect.top + 1 && aY < (rect.top + rect.height - 1));
        if (inBounds) {
          isTouchClick = false;
          break;
        }
      }

      if (isTouchClick) {
        let rect = {x: rects[0].left, y: rects[0].top, w: rects[0].width, h: rects[0].height};
        if (rect.w == 0 && rect.h == 0)
          return;

        let point = { x: rect.x + rect.w/2, y: rect.y + rect.h/2 };
        aX = point.x;
        aY = point.y;
      }
    }

    let window = aElement.ownerDocument.defaultView;
    try {
      [aX, aY] = ElementTouchHelper.toBrowserCoords(window, aX, aY);
      let cwu = window.top.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
      aButton = aButton || 0;
      cwu.sendMouseEventToWindow(aName, Math.round(aX), Math.round(aY), aButton, 1, 0, true);
    } catch(e) {
      Cu.reportError(e);
    }
  },

  _hasScrollableOverflow: function(elem) {
    var win = elem.ownerDocument.defaultView;
    if (!win)
      return false;
    var computedStyle = win.getComputedStyle(elem);
    if (!computedStyle)
      return false;
    return computedStyle.overflow == 'auto' || computedStyle.overflow == 'scroll';
  },

  _findScrollableElement: function(elem, checkElem) {
    
    let scrollable = false;
    while (elem) {
      





      if (checkElem) {
        if (((elem.scrollHeight > elem.clientHeight) ||
             (elem.scrollWidth > elem.clientWidth)) &&
            (this._hasScrollableOverflow(elem) ||
             elem.mozMatchesSelector("html, body, textarea")) ||
            (elem instanceof HTMLSelectElement && (elem.size > 1 || elem.multiple))) {
          scrollable = true;
          break;
        }
      } else {
        checkElem = true;
      }

      
      if (!elem.parentNode && elem.documentElement && elem.documentElement.ownerDocument)
        elem = elem.documentElement.ownerDocument.defaultView.frameElement;
      else
        elem = elem.parentNode;
    }

    if (!scrollable)
      return null;

    return elem;
  },

  _elementReceivesInput: function(aElement) {
    return aElement instanceof Element &&
        kElementsReceivingInput.hasOwnProperty(aElement.tagName.toLowerCase()) ||
        this._isEditable(aElement);
  },

  _isEditable: function(aElement) {
    let canEdit = false;

    if (aElement.isContentEditable || aElement.designMode == "on") {
      canEdit = true;
    } else if (aElement instanceof HTMLIFrameElement && (aElement.contentDocument.body.isContentEditable || aElement.contentDocument.designMode == "on")) {
      canEdit = true;
    } else {
      canEdit = aElement.ownerDocument && aElement.ownerDocument.designMode == "on";
    }

    return canEdit;
  },

  _scrollElementBy: function(elem, x, y) {
    elem.scrollTop = elem.scrollTop + y;
    elem.scrollLeft = elem.scrollLeft + x;
  },

  _elementCanScroll: function(elem, x, y) {
    let scrollX = true;
    let scrollY = true;

    if (x < 0) {
      if (elem.scrollLeft <= 0) {
        scrollX = false;
      }
    } else if (elem.scrollLeft >= (elem.scrollWidth - elem.clientWidth)) {
      scrollX = false;
    }

    if (y < 0) {
      if (elem.scrollTop <= 0) {
        scrollY = false;
      }
    } else if (elem.scrollTop >= (elem.scrollHeight - elem.clientHeight)) {
      scrollY = false;
    }

    return scrollX || scrollY;
  }
};

const kReferenceDpi = 240; 

const ElementTouchHelper = {
  toBrowserCoords: function(aWindow, aX, aY) {
    if (!aWindow)
      throw "Must provide a window";
  
    let browser = BrowserApp.getBrowserForWindow(aWindow.top);
    if (!browser)
      throw "Unable to find a browser";

    let tab = BrowserApp.getTabForBrowser(browser);
    if (!tab)
      throw "Unable to find a tab";

    let viewport = tab.viewport;
    return [
        ((aX - tab.viewportExcess.x) * viewport.zoom + viewport.offsetX),
        ((aY - tab.viewportExcess.y) * viewport.zoom + viewport.offsetY)
    ];
  },

  toScreenCoords: function(aWindow, aX, aY) {
    if (!aWindow)
      throw "Must provide a window";
  
    let browser = BrowserApp.getBrowserForWindow(aWindow.top);
    if (!browser)
      throw "Unable to find a browser";

    let tab = BrowserApp.getTabForBrowser(browser);
    if (!tab)
      throw "Unable to find a tab";

    let viewport = tab.viewport;
    return [
        (aX - viewport.offsetX)/viewport.zoom + tab.viewportExcess.x,
        (aY - viewport.offsetY)/viewport.zoom + tab.viewportExcess.y
    ];
  },

  anyElementFromPoint: function(aWindow, aX, aY) {
    [aX, aY] = this.toScreenCoords(aWindow, aX, aY);
    let cwu = aWindow.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    let elem = cwu.elementFromPoint(aX, aY, false, true);

    while (elem && (elem instanceof HTMLIFrameElement || elem instanceof HTMLFrameElement)) {
      let rect = elem.getBoundingClientRect();
      aX -= rect.left;
      aY -= rect.top;
      cwu = elem.contentDocument.defaultView.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
      elem = cwu.elementFromPoint(aX, aY, false, true);
    }

    return elem;
  },

  elementFromPoint: function(aWindow, aX, aY) {
    [aX, aY] = this.toScreenCoords(aWindow, aX, aY);
    
    
    let cwu = aWindow.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    let elem = this.getClosest(cwu, aX, aY);

    
    while (elem && (elem instanceof HTMLIFrameElement || elem instanceof HTMLFrameElement)) {
      
      let rect = elem.getBoundingClientRect();
      aX -= rect.left;
      aY -= rect.top;
      cwu = elem.contentDocument.defaultView.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
      elem = ElementTouchHelper.getClosest(cwu, aX, aY);
    }

    return elem;
  },

  get radius() {
    let prefs = Services.prefs;
    delete this.radius;
    return this.radius = { "top": prefs.getIntPref("browser.ui.touch.top"),
                           "right": prefs.getIntPref("browser.ui.touch.right"),
                           "bottom": prefs.getIntPref("browser.ui.touch.bottom"),
                           "left": prefs.getIntPref("browser.ui.touch.left")
                         };
  },

  get weight() {
    delete this.weight;
    return this.weight = { "visited": Services.prefs.getIntPref("browser.ui.touch.weight.visited") };
  },

  
  getClosest: function getClosest(aWindowUtils, aX, aY) {
    if (!this.dpiRatio)
      this.dpiRatio = aWindowUtils.displayDPI / kReferenceDpi;

    let dpiRatio = this.dpiRatio;

    let target = aWindowUtils.elementFromPoint(aX, aY,
                                               true,   
                                               false); 

    
    if (this.isElementClickable(target))
      return target;

    let target = null;
    let nodes = aWindowUtils.nodesFromRect(aX, aY, this.radius.top * dpiRatio,
                                                   this.radius.right * dpiRatio,
                                                   this.radius.bottom * dpiRatio,
                                                   this.radius.left * dpiRatio, true, false);

    let threshold = Number.POSITIVE_INFINITY;
    for (let i = 0; i < nodes.length; i++) {
      let current = nodes[i];
      if (!current.mozMatchesSelector || !this.isElementClickable(current))
        continue;

      let rect = current.getBoundingClientRect();
      let distance = this._computeDistanceFromRect(aX, aY, rect);

      
      if (current && current.mozMatchesSelector("*:visited"))
        distance *= (this.weight.visited / 100);

      if (distance < threshold) {
        target = current;
        threshold = distance;
      }
    }

    return target;
  },

  isElementClickable: function isElementClickable(aElement) {
    const selector = "a,:link,:visited,[role=button],button,input,select,textarea,label";
    for (let elem = aElement; elem; elem = elem.parentNode) {
      if (this._hasMouseListener(elem))
        return true;
      if (elem.mozMatchesSelector && elem.mozMatchesSelector(selector))
        return true;
    }
    return false;
  },

  _computeDistanceFromRect: function _computeDistanceFromRect(aX, aY, aRect) {
    let x = 0, y = 0;
    let xmost = aRect.left + aRect.width;
    let ymost = aRect.top + aRect.height;

    
    
    if (aRect.left < aX && aX < xmost)
      x = Math.min(xmost - aX, aX - aRect.left);
    else if (aX < aRect.left)
      x = aRect.left - aX;
    else if (aX > xmost)
      x = aX - xmost;

    
    
    if (aRect.top < aY && aY < ymost)
      y = Math.min(ymost - aY, aY - aRect.top);
    else if (aY < aRect.top)
      y = aRect.top - aY;
    if (aY > ymost)
      y = aY - ymost;

    return Math.sqrt(Math.pow(x, 2) + Math.pow(y, 2));
  },

  _els: Cc["@mozilla.org/eventlistenerservice;1"].getService(Ci.nsIEventListenerService),
  _clickableEvents: ["mousedown", "mouseup", "click"],
  _hasMouseListener: function _hasMouseListener(aElement) {
    let els = this._els;
    let listeners = els.getListenerInfoFor(aElement, {});
    for (let i = 0; i < listeners.length; i++) {
      if (this._clickableEvents.indexOf(listeners[i].type) != -1)
        return true;
    }
    return false;
  },

  getContentClientRects: function(aElement) {
    let offset = { x: 0, y: 0 };

    let nativeRects = aElement.getClientRects();
    
    for (let frame = aElement.ownerDocument.defaultView; frame.frameElement; frame = frame.parent) {
      
      let rect = frame.frameElement.getBoundingClientRect();
      let left = frame.getComputedStyle(frame.frameElement, "").borderLeftWidth;
      let top = frame.getComputedStyle(frame.frameElement, "").borderTopWidth;
      offset.x += rect.left + parseInt(left);
      offset.y += rect.top + parseInt(top);
    }

    let result = [];
    for (let i = nativeRects.length - 1; i >= 0; i--) {
      let r = nativeRects[i];
      result.push({ left: r.left + offset.x,
                    top: r.top + offset.y,
                    width: r.width,
                    height: r.height
                  });
    }
    return result;
  },

  getBoundingContentRect: function(aElement) {
    if (!aElement)
      return {x: 0, y: 0, w: 0, h: 0};

    let document = aElement.ownerDocument;
    while (document.defaultView.frameElement)
      document = document.defaultView.frameElement.ownerDocument;

    let cwu = document.defaultView.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    let scrollX = {}, scrollY = {};
    cwu.getScrollXY(false, scrollX, scrollY);

    let r = aElement.getBoundingClientRect();

    
    for (let frame = aElement.ownerDocument.defaultView; frame.frameElement && frame != content; frame = frame.parent) {
      
      let rect = frame.frameElement.getBoundingClientRect();
      let left = frame.getComputedStyle(frame.frameElement, "").borderLeftWidth;
      let top = frame.getComputedStyle(frame.frameElement, "").borderTopWidth;
      scrollX.value += rect.left + parseInt(left);
      scrollY.value += rect.top + parseInt(top);
    }

    return {x: r.left + scrollX.value,
            y: r.top + scrollY.value,
            w: r.width,
            h: r.height };
  }
};

var ErrorPageEventHandler = {
  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "click": {
        
        if (!aEvent.isTrusted)
          return;

        let target = aEvent.originalTarget;
        let errorDoc = target.ownerDocument;

        
        
        if (/^about:certerror\?e=nssBadCert/.test(errorDoc.documentURI)) {
          let perm = errorDoc.getElementById("permanentExceptionButton");
          let temp = errorDoc.getElementById("temporaryExceptionButton");
          if (target == temp || target == perm) {
            
            try {
              
              let uri = Services.io.newURI(errorDoc.location.href, null, null);
              let sslExceptions = new SSLExceptions();

              if (target == perm)
                sslExceptions.addPermanentException(uri);
              else
                sslExceptions.addTemporaryException(uri);
            } catch (e) {
              dump("Failed to set cert exception: " + e + "\n");
            }
            errorDoc.location.reload();
          } else if (target == errorDoc.getElementById("getMeOutOfHereButton")) {
            errorDoc.location = "about:home";
          }
        }
        break;
      }
    }
  }
};

var FormAssistant = {
  
  
  _currentInputElement: null,

  init: function() {
    Services.obs.addObserver(this, "FormAssist:AutoComplete", false);
    Services.obs.addObserver(this, "FormAssist:Closed", false);

    BrowserApp.deck.addEventListener("input", this, false);
  },

  uninit: function() {
    Services.obs.removeObserver(this, "FormAssist:AutoComplete");
    Services.obs.removeObserver(this, "FormAssist:Closed");
  },

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "FormAssist:AutoComplete":
        if (!this._currentInputElement)
          break;

        
        this._currentInputElement.blur();
        this._currentInputElement.value = aData;
        break;

      case "FormAssist:Closed":
        this._currentInputElement = null;
        break;
    }
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "input":
        let currentElement = aEvent.target;
        if (!this._isAutocomplete(currentElement))
          break;

        
        
        this._currentInputElement = currentElement;
        let suggestions = this._getAutocompleteSuggestions(currentElement.value, currentElement);

        let rect = ElementTouchHelper.getBoundingContentRect(currentElement);
        let viewport = BrowserApp.selectedTab.viewport;

        sendMessageToJava({
          gecko: {
            type:  "FormAssist:AutoComplete",
            suggestions: suggestions,
            rect: [rect.x - (viewport.x / viewport.zoom), rect.y - (viewport.y / viewport.zoom), rect.w, rect.h],
            zoom: viewport.zoom
          }
        });
    }
  },

  _isAutocomplete: function (aElement) {
    if (!(aElement instanceof HTMLInputElement) ||
        (aElement.getAttribute("type") == "password") ||
        (aElement.hasAttribute("autocomplete") &&
         aElement.getAttribute("autocomplete").toLowerCase() == "off"))
      return false;

    return true;
  },

  
  _getAutocompleteSuggestions: function(aSearchString, aElement) {
    let results = Cc["@mozilla.org/satchel/form-autocomplete;1"].
                  getService(Ci.nsIFormAutoComplete).
                  autoCompleteSearch(aElement.name || aElement.id, aSearchString, aElement, null);

    let suggestions = [];
    if (results.matchCount > 0) {
      for (let i = 0; i < results.matchCount; i++) {
        let value = results.getValueAt(i);
        
        if (value == aSearchString)
          continue;

        suggestions.push(value);
      }
    }

    return suggestions;
  }
};

var XPInstallObserver = {
  init: function xpi_init() {
    Services.obs.addObserver(XPInstallObserver, "addon-install-blocked", false);
    Services.obs.addObserver(XPInstallObserver, "addon-install-started", false);

    AddonManager.addInstallListener(XPInstallObserver);
  },

  uninit: function xpi_uninit() {
    Services.obs.removeObserver(XPInstallObserver, "addon-install-blocked");
    Services.obs.removeObserver(XPInstallObserver, "addon-install-started");

    AddonManager.removeInstallListener(XPInstallObserver);
  },

  observe: function xpi_observer(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "addon-install-started":
        NativeWindow.toast.show(Strings.browser.GetStringFromName("alertAddonsDownloading"), "short");
        break;
      case "addon-install-blocked":
        let installInfo = aSubject.QueryInterface(Ci.amIWebInstallInfo);
        let host = installInfo.originatingURI.host;

        let brandShortName = Strings.brand.GetStringFromName("brandShortName");
        let notificationName, buttons, message;
        let strings = Strings.browser;
        let enabled = true;
        try {
          enabled = Services.prefs.getBoolPref("xpinstall.enabled");
        }
        catch (e) {}

        if (!enabled) {
          notificationName = "xpinstall-disabled";
          if (Services.prefs.prefIsLocked("xpinstall.enabled")) {
            message = strings.GetStringFromName("xpinstallDisabledMessageLocked");
            buttons = [];
          } else {
            message = strings.formatStringFromName("xpinstallDisabledMessage2", [brandShortName, host], 2);
            buttons = [{
              label: strings.GetStringFromName("xpinstallDisabledButton"),
              callback: function editPrefs() {
                Services.prefs.setBoolPref("xpinstall.enabled", true);
                return false;
              }
            }];
          }
        } else {
          notificationName = "xpinstall";
          message = strings.formatStringFromName("xpinstallPromptWarning2", [brandShortName, host], 2);

          buttons = [{
            label: strings.GetStringFromName("xpinstallPromptAllowButton"),
            callback: function() {
              
              installInfo.install();
              return false;
            }
          }];
        }
        NativeWindow.doorhanger.show(message, aTopic, buttons);
        break;
    }
  },

  onInstallEnded: function(aInstall, aAddon) {
    let needsRestart = false;
    if (aInstall.existingAddon && (aInstall.existingAddon.pendingOperations & AddonManager.PENDING_UPGRADE))
      needsRestart = true;
    else if (aAddon.pendingOperations & AddonManager.PENDING_INSTALL)
      needsRestart = true;

    if (needsRestart) {
      let buttons = [{
        label: Strings.browser.GetStringFromName("notificationRestart.button"),
        callback: function() {
          
          let cancelQuit = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
          Services.obs.notifyObservers(cancelQuit, "quit-application-requested", "restart");

          
          if (cancelQuit.data == false) {
            let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
            appStartup.quit(Ci.nsIAppStartup.eRestart | Ci.nsIAppStartup.eAttemptQuit);
          }
        }
      }];

      let message = Strings.browser.GetStringFromName("notificationRestart.normal");
      NativeWindow.doorhanger.show(message, "addon-app-restart", buttons, BrowserApp.selectedTab.id, { persistence: -1 });
    } else {
      let message = Strings.browser.GetStringFromName("alertAddonsInstalledNoRestart");
      NativeWindow.toast.show(message, "short");
    }
  },

  onInstallFailed: function(aInstall) {
    NativeWindow.toast.show(Strings.browser.GetStringFromName("alertAddonsFail"), "short");
  },

  onDownloadProgress: function xpidm_onDownloadProgress(aInstall) {},

  onDownloadFailed: function(aInstall) {
    this.onInstallFailed(aInstall);
  },

  onDownloadCancelled: function(aInstall) {
    let host = (aInstall.originatingURI instanceof Ci.nsIStandardURL) && aInstall.originatingURI.host;
    if (!host)
      host = (aInstall.sourceURI instanceof Ci.nsIStandardURL) && aInstall.sourceURI.host;

    let error = (host || aInstall.error == 0) ? "addonError" : "addonLocalError";
    if (aInstall.error != 0)
      error += aInstall.error;
    else if (aInstall.addon && aInstall.addon.blocklistState == Ci.nsIBlocklistService.STATE_BLOCKED)
      error += "Blocklisted";
    else if (aInstall.addon && (!aInstall.addon.isCompatible || !aInstall.addon.isPlatformCompatible))
      error += "Incompatible";
    else
      return; 

    let msg = Strings.browser.GetStringFromName(error);
    
    msg = msg.replace("#1", aInstall.name);
    if (host)
      msg = msg.replace("#2", host);
    msg = msg.replace("#3", Strings.brand.GetStringFromName("brandShortName"));
    msg = msg.replace("#4", Services.appinfo.version);

    NativeWindow.toast.show(msg, "short");
  }
};


const kViewportMinScale  = 0;
const kViewportMaxScale  = 10;
const kViewportMinWidth  = 200;
const kViewportMaxWidth  = 10000;
const kViewportMinHeight = 223;
const kViewportMaxHeight = 10000;

var ViewportHandler = {
  
  
  
  _metadata: new WeakMap(),

  
  
  _documentIds: new WeakMap(),
  _nextDocumentId: 0,

  init: function init() {
    addEventListener("DOMMetaAdded", this, false);
    addEventListener("resize", this, false);
  },

  uninit: function uninit() {
    removeEventListener("DOMMetaAdded", this, false);
    removeEventListener("resize", this, false);
  },

  handleEvent: function handleEvent(aEvent) {
    let target = aEvent.originalTarget;
    let document = target.ownerDocument || target;
    let browser = BrowserApp.getBrowserForDocument(document);
    let tab = BrowserApp.getTabForBrowser(browser);
    if (!tab)
      return;

    switch (aEvent.type) {
      case "DOMMetaAdded":
        if (target.name == "viewport")
          this.updateMetadata(tab);
        break;

      case "resize":
        this.onResize();
        break;
    }
  },

  resetMetadata: function resetMetadata(tab) {
    tab.updateViewportMetadata(null);
  },

  updateMetadata: function updateMetadata(tab) {
    let metadata = this.getViewportMetadata(tab.browser.contentWindow);
    tab.updateViewportMetadata(metadata);
  },

  










  getViewportMetadata: function getViewportMetadata(aWindow) {
    let doctype = aWindow.document.doctype;
    if (doctype && /(WAP|WML|Mobile)/.test(doctype.publicId))
      return { defaultZoom: 1, autoSize: true, allowZoom: true, autoScale: true };

    let windowUtils = aWindow.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    let handheldFriendly = windowUtils.getDocumentMetadata("HandheldFriendly");
    if (handheldFriendly == "true")
      return { defaultZoom: 1, autoSize: true, allowZoom: true, autoScale: true };

    if (aWindow.document instanceof XULDocument)
      return { defaultZoom: 1, autoSize: true, allowZoom: false, autoScale: false };

    
    
    

    
    
    let scale = parseFloat(windowUtils.getDocumentMetadata("viewport-initial-scale"));
    let minScale = parseFloat(windowUtils.getDocumentMetadata("viewport-minimum-scale"));
    let maxScale = parseFloat(windowUtils.getDocumentMetadata("viewport-maximum-scale"));

    let widthStr = windowUtils.getDocumentMetadata("viewport-width");
    let heightStr = windowUtils.getDocumentMetadata("viewport-height");
    let width = this.clamp(parseInt(widthStr), kViewportMinWidth, kViewportMaxWidth);
    let height = this.clamp(parseInt(heightStr), kViewportMinHeight, kViewportMaxHeight);

    let allowZoomStr = windowUtils.getDocumentMetadata("viewport-user-scalable");
    let allowZoom = !/^(0|no|false)$/.test(allowZoomStr); 

    scale = this.clamp(scale, kViewportMinScale, kViewportMaxScale);
    minScale = this.clamp(minScale, kViewportMinScale, kViewportMaxScale);
    maxScale = this.clamp(maxScale, kViewportMinScale, kViewportMaxScale);

    
    let autoSize = (widthStr == "device-width" ||
                    (!widthStr && (heightStr == "device-height" || scale == 1.0)));

    return {
      defaultZoom: scale,
      minZoom: minScale,
      maxZoom: maxScale,
      width: width,
      height: height,
      autoSize: autoSize,
      allowZoom: allowZoom,
      autoScale: true
    };
  },

  onResize: function onResize() {
    for (let i = 0; i < BrowserApp.tabs.length; i++)
      BrowserApp.tabs[i].updateViewportSize();
  },

  clamp: function(num, min, max) {
    return Math.max(min, Math.min(max, num));
  },

  
  
  getScaleRatio: function getScaleRatio() {
    let prefValue = Services.prefs.getIntPref("browser.viewport.scaleRatio");
    if (prefValue > 0)
      return prefValue / 100;

    let dpi = this.displayDPI;
    if (dpi < 200) 
      return 1;
    else if (dpi < 300) 
      return 1.5;

    
    return Math.floor(dpi / 150);
  },

  get displayDPI() {
    let utils = window.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    delete this.displayDPI;
    return this.displayDPI = utils.displayDPI;
  },

  



  getMetadataForDocument: function getMetadataForDocument(aDocument) {
    let metadata = this._metadata.get(aDocument, this.getDefaultMetadata());
    return metadata;
  },

  
  setMetadataForDocument: function setMetadataForDocument(aDocument, aMetadata) {
    if (!aMetadata)
      this._metadata.delete(aDocument);
    else
      this._metadata.set(aDocument, aMetadata);
  },

  
  getDefaultMetadata: function getDefaultMetadata() {
    return {
      autoSize: false,
      allowZoom: true,
      autoScale: true,
      scaleRatio: ViewportHandler.getScaleRatio()
    };
  },

  



  getIdForDocument: function getIdForDocument(aDocument) {
    let id = this._documentIds.get(aDocument, null);
    if (id == null) {
      id = this._nextDocumentId++;
      this._documentIds.set(aDocument, id);
    }
    return id;
  }
};




var PopupBlockerObserver = {
  onUpdatePageReport: function onUpdatePageReport(aEvent) {
    let browser = BrowserApp.selectedBrowser;
    if (aEvent.originalTarget != browser)
      return;

    if (!browser.pageReport)
      return;

    let result = Services.perms.testExactPermission(BrowserApp.selectedBrowser.currentURI, "popup");
    if (result == Ci.nsIPermissionManager.DENY_ACTION)
      return;

    
    
    
    if (!browser.pageReport.reported) {
      if (Services.prefs.getBoolPref("privacy.popups.showBrowserMessage")) {
        let brandShortName = Strings.brand.GetStringFromName("brandShortName");
        let message;
        let popupCount = browser.pageReport.length;

        let strings = Strings.browser;
        if (popupCount > 1)
          message = strings.formatStringFromName("popupWarningMultiple", [brandShortName, popupCount], 2);
        else
          message = strings.formatStringFromName("popupWarning", [brandShortName], 1);

        let buttons = [
          {
            label: strings.GetStringFromName("popupButtonAllowOnce"),
            callback: function() { PopupBlockerObserver.showPopupsForSite(); }
          },
          {
            label: strings.GetStringFromName("popupButtonAlwaysAllow2"),
            callback: function() { PopupBlockerObserver.allowPopupsForSite(true); }
          },
          {
            label: strings.GetStringFromName("popupButtonNeverWarn2"),
            callback: function() { PopupBlockerObserver.allowPopupsForSite(false); }
          }
        ];

        NativeWindow.doorhanger.show(message, "popup-blocked", buttons);
      }
      
      
      browser.pageReport.reported = true;
    }
  },

  allowPopupsForSite: function allowPopupsForSite(aAllow) {
    let currentURI = BrowserApp.selectedBrowser.currentURI;
    Services.perms.add(currentURI, "popup", aAllow
                       ?  Ci.nsIPermissionManager.ALLOW_ACTION
                       :  Ci.nsIPermissionManager.DENY_ACTION);
    dump("Allowing popups for: " + currentURI);
  },

  showPopupsForSite: function showPopupsForSite() {
    let uri = BrowserApp.selectedBrowser.currentURI;
    let pageReport = BrowserApp.selectedBrowser.pageReport;
    if (pageReport) {
      for (let i = 0; i < pageReport.length; ++i) {
        let popupURIspec = pageReport[i].popupWindowURI.spec;

        
        
        
        
        
        if (popupURIspec == "" || popupURIspec == "about:blank" || popupURIspec == uri.spec)
          continue;

        let popupFeatures = pageReport[i].popupWindowFeatures;
        let popupName = pageReport[i].popupWindowName;

        BrowserApp.addTab(popupURIspec);
      }
    }
  }
};


var OfflineApps = {
  init: function() {
    BrowserApp.deck.addEventListener("MozApplicationManifest", this, false);
  },

  uninit: function() {
    BrowserApp.deck.removeEventListener("MozApplicationManifest", this, false);
  },

  handleEvent: function(aEvent) {
    if (aEvent.type == "MozApplicationManifest")
      this.offlineAppRequested(aEvent.originalTarget.defaultView);
  },

  offlineAppRequested: function(aContentWindow) {
    if (!Services.prefs.getBoolPref("browser.offline-apps.notify"))
      return;

    let browser = BrowserApp.getBrowserForWindow(aContentWindow);
    let tab = BrowserApp.getTabForBrowser(browser);
    let currentURI = aContentWindow.document.documentURIObject;

    
    if (Services.perms.testExactPermission(currentURI, "offline-app") != Services.perms.UNKNOWN_ACTION)
      return;

    try {
      if (Services.prefs.getBoolPref("offline-apps.allow_by_default")) {
        
        return;
      }
    } catch(e) {
      
    }

    let host = currentURI.asciiHost;
    let notificationID = "offline-app-requested-" + host;

    let strings = Strings.browser;
    let buttons = [{
      label: strings.GetStringFromName("offlineApps.allow"),
      callback: function() {
        OfflineApps.allowSite(aContentWindow.document);
      }
    },
    {
      label: strings.GetStringFromName("offlineApps.never"),
      callback: function() {
        OfflineApps.disallowSite(aContentWindow.document);
      }
    },
    {
      label: strings.GetStringFromName("offlineApps.notNow"),
      callback: function() {  }
    }];

    let message = strings.formatStringFromName("offlineApps.available2", [host], 1);
    NativeWindow.doorhanger.show(message, notificationID, buttons, tab.id);
  },

  allowSite: function(aDocument) {
    Services.perms.add(aDocument.documentURIObject, "offline-app", Services.perms.ALLOW_ACTION);

    
    
    
    this._startFetching(aDocument);
  },

  disallowSite: function(aDocument) {
    Services.perms.add(aDocument.documentURIObject, "offline-app", Services.perms.DENY_ACTION);
  },

  _startFetching: function(aDocument) {
    if (!aDocument.documentElement)
      return;

    let manifest = aDocument.documentElement.getAttribute("manifest");
    if (!manifest)
      return;

    let manifestURI = Services.io.newURI(manifest, aDocument.characterSet, aDocument.documentURIObject);
    let updateService = Cc["@mozilla.org/offlinecacheupdate-service;1"].getService(Ci.nsIOfflineCacheUpdateService);
    updateService.scheduleUpdate(manifestURI, aDocument.documentURIObject, window);
  }
};

var IndexedDB = {
  _permissionsPrompt: "indexedDB-permissions-prompt",
  _permissionsResponse: "indexedDB-permissions-response",

  _quotaPrompt: "indexedDB-quota-prompt",
  _quotaResponse: "indexedDB-quota-response",
  _quotaCancel: "indexedDB-quota-cancel",

  init: function IndexedDB_init() {
    Services.obs.addObserver(this, this._permissionsPrompt, false);
    Services.obs.addObserver(this, this._quotaPrompt, false);
    Services.obs.addObserver(this, this._quotaCancel, false);
  },

  uninit: function IndexedDB_uninit() {
    Services.obs.removeObserver(this, this._permissionsPrompt, false);
    Services.obs.removeObserver(this, this._quotaPrompt, false);
    Services.obs.removeObserver(this, this._quotaCancel, false);
  },

  observe: function IndexedDB_observe(subject, topic, data) {
    if (topic != this._permissionsPrompt &&
        topic != this._quotaPrompt &&
        topic != this._quotaCancel) {
      throw new Error("Unexpected topic!");
    }

    let requestor = subject.QueryInterface(Ci.nsIInterfaceRequestor);

    let contentWindow = requestor.getInterface(Ci.nsIDOMWindow);
    let contentDocument = contentWindow.document;
    let browser = BrowserApp.getBrowserForWindow(contentWindow);
    if (!browser)
      return;

    let host = contentDocument.documentURIObject.asciiHost;

    let strings = Strings.browser;

    let message, responseTopic;
    if (topic == this._permissionsPrompt) {
      message = strings.formatStringFromName("offlineApps.available2", [host], 1);
      responseTopic = this._permissionsResponse;
    } else if (topic == this._quotaPrompt) {
      message = strings.formatStringFromName("indexedDBQuota.wantsTo", [ host, data ], 2);
      responseTopic = this._quotaResponse;
    } else if (topic == this._quotaCancel) {
      responseTopic = this._quotaResponse;
    }

    let notificationID = responseTopic + host;
    let tab = BrowserApp.getTabForBrowser(browser);
    let observer = requestor.getInterface(Ci.nsIObserver);

    if (topic == this._quotaCancel) {
      NativeWindow.doorhanger.hide(notificationID, tab.id);
      observer.observe(null, responseTopic, Ci.nsIPermissionManager.UNKNOWN_ACTION);
      return;
    }

    let buttons = [{
      label: strings.GetStringFromName("offlineApps.allow"),
      callback: function() {
        observer.observe(null, responseTopic, Ci.nsIPermissionManager.ALLOW_ACTION);
      }
    },
    {
      label: strings.GetStringFromName("offlineApps.never"),
      callback: function() {
        observer.observe(null, responseTopic, Ci.nsIPermissionManager.DENY_ACTION);
      }
    },
    {
      label: strings.GetStringFromName("offlineApps.notNow"),
      callback: function() {
        observer.observe(null, responseTopic, Ci.nsIPermissionManager.UNKNOWN_ACTION);
      }
    }];

    NativeWindow.doorhanger.show(message, notificationID, buttons, tab.id);
  }
};

var ConsoleAPI = {
  init: function init() {
    Services.obs.addObserver(this, "console-api-log-event", false);
  },

  uninit: function uninit() {
    Services.obs.removeObserver(this, "console-api-log-event", false);
  },

  observe: function observe(aMessage, aTopic, aData) {
    aMessage = aMessage.wrappedJSObject;

    let mappedArguments = Array.map(aMessage.arguments, this.formatResult, this);
    let joinedArguments = Array.join(mappedArguments, " ");

    if (aMessage.level == "error" || aMessage.level == "warn") {
      let flag = (aMessage.level == "error" ? Ci.nsIScriptError.errorFlag : Ci.nsIScriptError.warningFlag);
      let consoleMsg = Cc["@mozilla.org/scripterror;1"].createInstance(Ci.nsIScriptError);
      consoleMsg.init(joinedArguments, null, null, 0, 0, flag, "content javascript");
      Services.console.logMessage(consoleMsg);
    } else if (aMessage.level == "trace") {
      let bundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");
      let args = aMessage.arguments;
      let filename = this.abbreviateSourceURL(args[0].filename);
      let functionName = args[0].functionName || bundle.GetStringFromName("stacktrace.anonymousFunction");
      let lineNumber = args[0].lineNumber;

      let body = bundle.formatStringFromName("stacktrace.outputMessage", [filename, functionName, lineNumber], 3);
      body += "\n";
      args.forEach(function(aFrame) {
        let functionName = aFrame.functionName || bundle.GetStringFromName("stacktrace.anonymousFunction");
        body += "  " + aFrame.filename + " :: " + functionName + " :: " + aFrame.lineNumber + "\n";
      });

      Services.console.logStringMessage(body);
    } else if (aMessage.level == "time" && aMessage.arguments) {
      let bundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");
      let body = bundle.formatStringFromName("timer.start", [aMessage.arguments.name], 1);
      Services.console.logStringMessage(body);
    } else if (aMessage.level == "timeEnd" && aMessage.arguments) {
      let bundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");
      let body = bundle.formatStringFromName("timer.end", [aMessage.arguments.name, aMessage.arguments.duration], 2);
      Services.console.logStringMessage(body);
    } else if (["group", "groupCollapsed", "groupEnd"].indexOf(aMessage.level) != -1) {
      
    } else {
      Services.console.logStringMessage(joinedArguments);
    }
  },

  getResultType: function getResultType(aResult) {
    let type = aResult === null ? "null" : typeof aResult;
    if (type == "object" && aResult.constructor && aResult.constructor.name)
      type = aResult.constructor.name;
    return type.toLowerCase();
  },

  formatResult: function formatResult(aResult) {
    let output = "";
    let type = this.getResultType(aResult);
    switch (type) {
      case "string":
      case "boolean":
      case "date":
      case "error":
      case "number":
      case "regexp":
        output = aResult.toString();
        break;
      case "null":
      case "undefined":
        output = type;
        break;
      default:
        if (aResult.toSource) {
          try {
            output = aResult.toSource();
          } catch (ex) { }
        }
        if (!output || output == "({})") {
          output = aResult.toString();
        }
        break;
    }

    return output;
  },

  abbreviateSourceURL: function abbreviateSourceURL(aSourceURL) {
    
    let hookIndex = aSourceURL.indexOf("?");
    if (hookIndex > -1)
      aSourceURL = aSourceURL.substring(0, hookIndex);

    
    if (aSourceURL[aSourceURL.length - 1] == "/")
      aSourceURL = aSourceURL.substring(0, aSourceURL.length - 1);

    
    let slashIndex = aSourceURL.lastIndexOf("/");
    if (slashIndex > -1)
      aSourceURL = aSourceURL.substring(slashIndex + 1);

    return aSourceURL;
  }
};

var ClipboardHelper = {
  init: function() {
    NativeWindow.contextmenus.add(Strings.browser.GetStringFromName("contextmenu.copy"), ClipboardHelper.getCopyContext(false), ClipboardHelper.copy.bind(ClipboardHelper));
    NativeWindow.contextmenus.add(Strings.browser.GetStringFromName("contextmenu.copyAll"), ClipboardHelper.getCopyContext(true), ClipboardHelper.copy.bind(ClipboardHelper));
    NativeWindow.contextmenus.add(Strings.browser.GetStringFromName("contextmenu.selectAll"), ClipboardHelper.selectAllContext, ClipboardHelper.select.bind(ClipboardHelper));
    NativeWindow.contextmenus.add(Strings.browser.GetStringFromName("contextmenu.paste"), ClipboardHelper.pasteContext, ClipboardHelper.paste.bind(ClipboardHelper));
    NativeWindow.contextmenus.add(Strings.browser.GetStringFromName("contextmenu.changeInputMethod"), NativeWindow.contextmenus.textContext, ClipboardHelper.inputMethod.bind(ClipboardHelper));
  },

  get clipboardHelper() {
    delete this.clipboardHelper;
    return this.clipboardHelper = Cc["@mozilla.org/widget/clipboardhelper;1"].getService(Ci.nsIClipboardHelper);
  },

  get clipboard() {
    delete this.clipboard;
    return this.clipboard = Cc["@mozilla.org/widget/clipboard;1"].getService(Ci.nsIClipboard);
  },

  copy: function(aElement) {
    let selectionStart = aElement.selectionStart;
    let selectionEnd = aElement.selectionEnd;
    if (selectionStart != selectionEnd) {
      string = aElement.value.slice(selectionStart, selectionEnd);
      this.clipboardHelper.copyString(string);
    } else {
      this.clipboardHelper.copyString(aElement.value);
    }
  },

  select: function(aElement) {
    if (!aElement || !(aElement instanceof Ci.nsIDOMNSEditableElement))
      return;
    let target = aElement.QueryInterface(Ci.nsIDOMNSEditableElement);
    target.editor.selectAll();
    target.focus();
  },

  paste: function(aElement) {
    if (!aElement || !(aElement instanceof Ci.nsIDOMNSEditableElement))
      return;
    let target = aElement.QueryInterface(Ci.nsIDOMNSEditableElement);
    target.editor.paste(Ci.nsIClipboard.kGlobalClipboard);
    target.focus();  
  },

  inputMethod: function(aElement) {
    Cc["@mozilla.org/imepicker;1"].getService(Ci.nsIIMEPicker).show();
  },

  getCopyContext: function(isCopyAll) {
    return {
      matches: function(aElement) {
        if (NativeWindow.contextmenus.textContext.matches(aElement)) {
          
          
          if (aElement instanceof Ci.nsIDOMHTMLInputElement && !aElement.mozIsTextField(true))
            return false;

          let selectionStart = aElement.selectionStart;
          let selectionEnd = aElement.selectionEnd;
          if (selectionStart != selectionEnd)
            return true;

          if (isCopyAll && aElement.textLength > 0)
            return true;
        }
        return false;
      }
    }
  },

  selectAllContext: {
    matches: function selectAllContextMatches(aElement) {
      if (NativeWindow.contextmenus.textContext.matches(aElement)) {
          let selectionStart = aElement.selectionStart;
          let selectionEnd = aElement.selectionEnd;
          return (selectionStart > 0 || selectionEnd < aElement.textLength);
      }
      return false;
    }
  },

  pasteContext: {
    matches: function(aElement) {
      if (NativeWindow.contextmenus.textContext.matches(aElement)) {
        let flavors = ["text/unicode"];
        return ClipboardHelper.clipboard.hasDataMatchingFlavors(flavors, flavors.length, Ci.nsIClipboard.kGlobalClipboard);
      }
      return false;
    }
  }
};

var PluginHelper = {
  showDoorHanger: function(aTab) {
    let message = Strings.browser.GetStringFromName("clickToPlayPlugins.message");
    let buttons = [
      {
        label: Strings.browser.GetStringFromName("clickToPlayPlugins.yes"),
        callback: function() {
          PluginHelper.playAllPlugins(aTab);
        }
      },
      {
        label: Strings.browser.GetStringFromName("clickToPlayPlugins.no"),
        callback: function() {
          
        }
      }
    ]
    NativeWindow.doorhanger.show(message, "ask-to-play-plugins", buttons, aTab.id);
  },

  playAllPlugins: function(aTab, aEvent) {
    if (aEvent) {
      if (!aEvent.isTrusted)
        return;
      aEvent.preventDefault();
    }

    this._findAndPlayAllPlugins(aTab.browser.contentWindow);
  },

  
  _findAndPlayAllPlugins: function _findAndPlayAllPlugins(aWindow) {
    let embeds = aWindow.document.getElementsByTagName("embed");
    for (let i = 0; i < embeds.length; i++) {
      if (!embeds[i].hasAttribute("played"))
        this._playPlugin(embeds[i]);
    }

    let objects = aWindow.document.getElementsByTagName("object");
    for (let i = 0; i < objects.length; i++) {
      if (!objects[i].hasAttribute("played"))
        this._playPlugin(objects[i]);
    }

    for (let i = 0; i < aWindow.frames.length; i++) {
      this._findAndPlayAllPlugins(aWindow.frames[i]);
    }
  },

  _playPlugin: function _playPlugin(aPlugin) {
    let objLoadingContent = aPlugin.QueryInterface(Ci.nsIObjectLoadingContent);
    objLoadingContent.playPlugin();

    
    aPlugin.setAttribute("played", true);
  },

  getPluginPreference: function getPluginPreference() {
    let pluginDisable = Services.prefs.getBoolPref("plugin.disable");
    if (pluginDisable)
      return "0";

    let clickToPlay = Services.prefs.getBoolPref("plugins.click_to_play");
    return clickToPlay ? "2" : "1";
  },

  setPluginPreference: function setPluginPreference(aValue) {
    switch (aValue) {
      case "0": 
        Services.prefs.setBoolPref("plugin.disable", true);
        Services.prefs.clearUserPref("plugins.click_to_play");
        break;
      case "1": 
        Services.prefs.clearUserPref("plugin.disable");
        Services.prefs.setBoolPref("plugins.click_to_play", false);
        break;
      case "2": 
        Services.prefs.clearUserPref("plugin.disable");
        Services.prefs.clearUserPref("plugins.click_to_play");
        break;
    }
  },

  
  isTooSmall : function (plugin, overlay) {
    
    let pluginRect = plugin.getBoundingClientRect();
    
    
    let overflows = (overlay.scrollWidth > pluginRect.width) ||
                    (overlay.scrollHeight - 5 > pluginRect.height);

    return overflows;
  }
};

var PermissionsHelper = {

  _permissonTypes: ["password", "geolocation", "popup", "indexedDB",
                    "offline-app", "desktop-notification"],
  _permissionStrings: {
    "password": {
      label: "password.rememberPassword",
      allowed: "password.remember",
      denied: "password.never"
    },
    "geolocation": {
      label: "geolocation.shareLocation",
      allowed: "geolocation.alwaysAllow",
      denied: "geolocation.neverAllow"
    },
    "popup": {
      label: "blockPopups.label",
      allowed: "popupButtonAlwaysAllow2",
      denied: "popupButtonNeverWarn2"
    },
    "indexedDB": {
      label: "offlineApps.storeOfflineData",
      allowed: "offlineApps.allow",
      denied: "offlineApps.never"
    },
    "offline-app": {
      label: "offlineApps.storeOfflineData",
      allowed: "offlineApps.allow",
      denied: "offlineApps.never"
    },
    "desktop-notification": {
      label: "desktopNotification.useNotifications",
      allowed: "desktopNotification.allow",
      denied: "desktopNotification.dontAllow"
    }
  },

  init: function init() {
    Services.obs.addObserver(this, "Permissions:Get", false);
    Services.obs.addObserver(this, "Permissions:Clear", false);
  },

  observe: function observe(aSubject, aTopic, aData) {
    let uri = BrowserApp.selectedBrowser.currentURI;

    switch (aTopic) {
      case "Permissions:Get":
        let permissions = [];
        for (let i = 0; i < this._permissonTypes.length; i++) {
          let type = this._permissonTypes[i];
          let value = this.getPermission(uri, type);

          
          if (value == Services.perms.UNKNOWN_ACTION)
            continue;

          
          let typeStrings = this._permissionStrings[type];
          let label = Strings.browser.GetStringFromName(typeStrings["label"]);

          
          let valueKey = value == Services.perms.ALLOW_ACTION ?
                         "allowed" : "denied";
          let valueString = Strings.browser.GetStringFromName(typeStrings[valueKey]);

          
          
          let setting = Strings.browser.formatStringFromName("siteSettings.labelToValue",
                                                             [ label, valueString ], 2)
          permissions.push({
            type: type,
            setting: setting
          });
        }

        
        this._currentPermissions = permissions; 

        let host;
        try {
          host = uri.host;
        } catch(e) {
          host = uri.spec;
        }
        sendMessageToJava({
          gecko: {
            type: "Permissions:Data",
            host: host,
            permissions: permissions
          }
        });
        break;
 
      case "Permissions:Clear":
        
        let permissionsToClear = JSON.parse(aData);

        for (let i = 0; i < permissionsToClear.length; i++) {
          let indexToClear = permissionsToClear[i];
          let permissionType = this._currentPermissions[indexToClear]["type"];
          this.clearPermission(uri, permissionType);
        }
        break;
    }
  },

  








  getPermission: function getPermission(aURI, aType) {
    
    
    if (aType == "password") {
      
      
      if (!Services.logins.getLoginSavingEnabled(aURI.prePath))
        return Services.perms.DENY_ACTION;

      
      if (Services.logins.countLogins(aURI.prePath, "", ""))
        return Services.perms.ALLOW_ACTION;

      return Services.perms.UNKNOWN_ACTION;
    }

    
    if (aType == "geolocation")
      return Services.perms.testExactPermission(aURI, aType);

    return Services.perms.testPermission(aURI, aType);
  },

  






  clearPermission: function clearPermission(aURI, aType) {
    
    
    if (aType == "password") {
      
      let logins = Services.logins.findLogins({}, aURI.prePath, "", "");
      for (let i = 0; i < logins.length; i++) {
        Services.logins.removeLogin(logins[i]);
      }
      
      Services.logins.setLoginSavingEnabled(aURI.prePath, true);
    } else {
      Services.perms.remove(aURI.host, aType);
      
      Services.contentPrefs.removePref(aURI, aType + ".request.remember");
    }
  }
};

var MasterPassword = {
  pref: "privacy.masterpassword.enabled",
  _tokenName: "",

  get _secModuleDB() {
    delete this._secModuleDB;
    return this._secModuleDB = Cc["@mozilla.org/security/pkcs11moduledb;1"].getService(Ci.nsIPKCS11ModuleDB);
  },

  get _pk11DB() {
    delete this._pk11DB;
    return this._pk11DB = Cc["@mozilla.org/security/pk11tokendb;1"].getService(Ci.nsIPK11TokenDB);
  },

  get enabled() {
    let slot = this._secModuleDB.findSlotByName(this._tokenName);
    if (slot) {
      let status = slot.status;
      return status != Ci.nsIPKCS11Slot.SLOT_UNINITIALIZED && status != Ci.nsIPKCS11Slot.SLOT_READY;
    }
    return false;
  },

  setPassword: function setPassword(aPassword) {
    try {
      let status;
      let slot = this._secModuleDB.findSlotByName(this._tokenName);
      if (slot)
        status = slot.status;
      else
        return false;

      let token = this._pk11DB.findTokenByName(this._tokenName);

      if (status == Ci.nsIPKCS11Slot.SLOT_UNINITIALIZED)
        token.initPassword(aPassword);
      else if (status == Ci.nsIPKCS11Slot.SLOT_READY)
        token.changePassword("", aPassword);

      this.updatePref();
      return true;
    } catch(e) {
      dump("MasterPassword.setPassword: " + e);
    }
    return false;
  },

  removePassword: function removePassword(aOldPassword) {
    try {
      let token = this._pk11DB.getInternalKeyToken();
      if (token.checkPassword(aOldPassword)) {
        token.changePassword(aOldPassword, "");
        this.updatePref();
        return true;
      }
    } catch(e) {
      dump("MasterPassword.removePassword: " + e + "\n");
    }
    NativeWindow.toast.show(Strings.browser.GetStringFromName("masterPassword.incorrect"), "short");
    return false;
  },

  updatePref: function() {
    var prefs = [];
    let pref = {
      name: this.pref,
      type: "bool",
      value: this.enabled
    };
    prefs.push(pref);

    sendMessageToJava({
      gecko: {
        type: "Preferences:Data",
        preferences: prefs
      }
    });
  }
};

var CharacterEncoding = {
  _charsets: [],

  init: function init() {
    Services.obs.addObserver(this, "CharEncoding:Get", false);
    Services.obs.addObserver(this, "CharEncoding:Set", false);
    this.sendState();
  },

  uninit: function uninit() {
    Services.obs.removeObserver(this, "CharEncoding:Get", false);
    Services.obs.removeObserver(this, "CharEncoding:Set", false);
  },

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "CharEncoding:Get":
        this.getEncoding();
        break;
      case "CharEncoding:Set":
        this.setEncoding(aData);
        break;
    }
  },

  sendState: function sendState() {
    let showCharEncoding = "false";
    try {
      showCharEncoding = Services.prefs.getComplexValue("browser.menu.showCharacterEncoding", Ci.nsIPrefLocalizedString).data;
    } catch (e) {  }

    sendMessageToJava({
      gecko: {
        type: "CharEncoding:State",
        visible: showCharEncoding
      }
    });
  },

  getEncoding: function getEncoding() {
    function normalizeCharsetCode(charsetCode) {
      return charsetCode.trim().toLowerCase();
    }

    function getTitle(charsetCode) {
      let charsetTitle = charsetCode;
      try {
        charsetTitle = Strings.charset.GetStringFromName(charsetCode + ".title");
      } catch (e) {
        dump("error: title not found for " + charsetCode);
      }
      return charsetTitle;
    }

    if (!this._charsets.length) {
      let charsets = Services.prefs.getComplexValue("intl.charsetmenu.browser.static", Ci.nsIPrefLocalizedString).data;
      this._charsets = charsets.split(",").map(function (charset) {
        return {
          code: normalizeCharsetCode(charset),
          title: getTitle(charset)
        };
      });
    }

    
    let docCharset = normalizeCharsetCode(BrowserApp.selectedBrowser.contentDocument.characterSet);
    let selected = 0;
    let charsetCount = this._charsets.length;
    for (; selected < charsetCount && this._charsets[selected].code != docCharset; selected++);
    if (selected == charsetCount) {
      this._charsets.push({
        code: docCharset,
        title: getTitle(docCharset)
      });
    }

    sendMessageToJava({
      gecko: {
        type: "CharEncoding:Data",
        charsets: this._charsets,
        selected: selected
      }
    });
  },

  setEncoding: function setEncoding(aEncoding) {
    let browser = BrowserApp.selectedBrowser;
    let docCharset = browser.docShell.QueryInterface(Ci.nsIDocCharset);
    docCharset.charset = aEncoding;
    browser.reload(Ci.nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
  }
};

