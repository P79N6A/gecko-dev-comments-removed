



Components.utils.importGlobalProperties(["URLSearchParams"]);

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/AppConstants.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
                                  "resource://gre/modules/PrivateBrowsingUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RecentWindow",
                                  "resource:///modules/RecentWindow.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "WindowsUIUtils",
                                   "@mozilla.org/windows-ui-utils;1", "nsIWindowsUIUtils");

const nsISupports            = Components.interfaces.nsISupports;

const nsIBrowserDOMWindow    = Components.interfaces.nsIBrowserDOMWindow;
const nsIBrowserHandler      = Components.interfaces.nsIBrowserHandler;
const nsIBrowserHistory      = Components.interfaces.nsIBrowserHistory;
const nsIChannel             = Components.interfaces.nsIChannel;
const nsICommandLine         = Components.interfaces.nsICommandLine;
const nsICommandLineHandler  = Components.interfaces.nsICommandLineHandler;
const nsIContentHandler      = Components.interfaces.nsIContentHandler;
const nsIDocShellTreeItem    = Components.interfaces.nsIDocShellTreeItem;
const nsIDOMChromeWindow     = Components.interfaces.nsIDOMChromeWindow;
const nsIDOMWindow           = Components.interfaces.nsIDOMWindow;
const nsIFileURL             = Components.interfaces.nsIFileURL;
const nsIInterfaceRequestor  = Components.interfaces.nsIInterfaceRequestor;
const nsINetUtil             = Components.interfaces.nsINetUtil;
const nsIPrefBranch          = Components.interfaces.nsIPrefBranch;
const nsIPrefLocalizedString = Components.interfaces.nsIPrefLocalizedString;
const nsISupportsString      = Components.interfaces.nsISupportsString;
const nsIURIFixup            = Components.interfaces.nsIURIFixup;
const nsIWebNavigation       = Components.interfaces.nsIWebNavigation;
const nsIWindowMediator      = Components.interfaces.nsIWindowMediator;
const nsIWindowWatcher       = Components.interfaces.nsIWindowWatcher;
const nsIWebNavigationInfo   = Components.interfaces.nsIWebNavigationInfo;
const nsIBrowserSearchService = Components.interfaces.nsIBrowserSearchService;
const nsICommandLineValidator = Components.interfaces.nsICommandLineValidator;

const NS_BINDING_ABORTED = Components.results.NS_BINDING_ABORTED;
const NS_ERROR_WONT_HANDLE_CONTENT = 0x805d0001;
const NS_ERROR_ABORT = Components.results.NS_ERROR_ABORT;

function shouldLoadURI(aURI) {
  if (aURI && !aURI.schemeIs("chrome"))
    return true;

  dump("*** Preventing external load of chrome: URI into browser window\n");
  dump("    Use --chrome <uri> instead\n");
  return false;
}

function resolveURIInternal(aCmdLine, aArgument) {
  var uri = aCmdLine.resolveURI(aArgument);
  var urifixup = Components.classes["@mozilla.org/docshell/urifixup;1"]
                           .getService(nsIURIFixup);

  if (!(uri instanceof nsIFileURL)) {
    return urifixup.createFixupURI(aArgument,
                                   urifixup.FIXUP_FLAG_FIX_SCHEME_TYPOS);
  }

  try {
    if (uri.file.exists())
      return uri;
  }
  catch (e) {
    Components.utils.reportError(e);
  }

  
  
 
  try {
    uri = urifixup.createFixupURI(aArgument, 0);
  }
  catch (e) {
    Components.utils.reportError(e);
  }

  return uri;
}

var gFirstWindow = false;

const OVERRIDE_NONE        = 0;
const OVERRIDE_NEW_PROFILE = 1;
const OVERRIDE_NEW_MSTONE  = 2;
const OVERRIDE_NEW_BUILD_ID = 3;










function needHomepageOverride(prefb) {
  var savedmstone = null;
  try {
    savedmstone = prefb.getCharPref("browser.startup.homepage_override.mstone");
  } catch (e) {}

  if (savedmstone == "ignore")
    return OVERRIDE_NONE;

  var mstone = Services.appinfo.platformVersion;

  var savedBuildID = null;
  try {
    savedBuildID = prefb.getCharPref("browser.startup.homepage_override.buildID");
  } catch (e) {}

  var buildID = Services.appinfo.platformBuildID;

  if (mstone != savedmstone) {
    
    
    
    
    if (savedmstone)
      prefb.setBoolPref("browser.rights.3.shown", true);
    
    prefb.setCharPref("browser.startup.homepage_override.mstone", mstone);
    prefb.setCharPref("browser.startup.homepage_override.buildID", buildID);
    return (savedmstone ? OVERRIDE_NEW_MSTONE : OVERRIDE_NEW_PROFILE);
  }

  if (buildID != savedBuildID) {
    prefb.setCharPref("browser.startup.homepage_override.buildID", buildID);
    return OVERRIDE_NEW_BUILD_ID;
  }

  return OVERRIDE_NONE;
}








function getPostUpdateOverridePage(defaultOverridePage) {
  var um = Components.classes["@mozilla.org/updates/update-manager;1"]
                     .getService(Components.interfaces.nsIUpdateManager);
  try {
    
    var update = um.getUpdateAt(0)
                   .QueryInterface(Components.interfaces.nsIPropertyBag);
  } catch (e) {
    
    Components.utils.reportError("Unable to find update: " + e);
    return defaultOverridePage;
  }

  let actions = update.getProperty("actions");
  
  
  if (!actions)
    return defaultOverridePage;

  
  
  if (actions.indexOf("silent") != -1 || actions.indexOf("showURL") == -1)
    return "";

  return update.getProperty("openURL") || defaultOverridePage;
}


const NO_EXTERNAL_URIS = 1;

function openWindow(parent, url, target, features, args, noExternalArgs) {
  var wwatch = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                         .getService(nsIWindowWatcher);

  if (noExternalArgs == NO_EXTERNAL_URIS) {
    
    var argstring;
    if (args) {
      argstring = Components.classes["@mozilla.org/supports-string;1"]
                            .createInstance(nsISupportsString);
      argstring.data = args;
    }

    return wwatch.openWindow(parent, url, target, features, argstring);
  }
  
  
  var argArray = Components.classes["@mozilla.org/supports-array;1"]
                    .createInstance(Components.interfaces.nsISupportsArray);

  
  var stringArgs = null;
  if (args instanceof Array) 
    stringArgs = args;
  else if (args) 
    stringArgs = [args];

  if (stringArgs) {
    
    var uriArray = Components.classes["@mozilla.org/supports-array;1"]
                       .createInstance(Components.interfaces.nsISupportsArray);
    stringArgs.forEach(function (uri) {
      var sstring = Components.classes["@mozilla.org/supports-string;1"]
                              .createInstance(nsISupportsString);
      sstring.data = uri;
      uriArray.AppendElement(sstring);
    });
    argArray.AppendElement(uriArray);
  } else {
    argArray.AppendElement(null);
  }

  
  
  
  argArray.AppendElement(null); 
  argArray.AppendElement(null); 
  argArray.AppendElement(null); 
  argArray.AppendElement(null); 

  return wwatch.openWindow(parent, url, target, features, argArray);
}

function openPreferences() {
  if (Services.prefs.getBoolPref("browser.preferences.inContent")) { 
    var sa = Components.classes["@mozilla.org/supports-array;1"]
                       .createInstance(Components.interfaces.nsISupportsArray);

    var wuri = Components.classes["@mozilla.org/supports-string;1"]
                         .createInstance(Components.interfaces.nsISupportsString);
    wuri.data = "about:preferences";

    sa.AppendElement(wuri);

    var wwatch = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                           .getService(nsIWindowWatcher);

    wwatch.openWindow(null, gBrowserContentHandler.chromeURL,
                      "_blank",
                      "chrome,dialog=no,all",
                      sa);
  } else {
    var features = "chrome,titlebar,toolbar,centerscreen,dialog=no";
    var url = "chrome://browser/content/preferences/preferences.xul";
    
    var win = getMostRecentWindow("Browser:Preferences");
    if (win) {
      win.focus();
    } else {
      openWindow(null, url, "_blank", features);
    }
  }
}

function getMostRecentWindow(aType) {
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(nsIWindowMediator);
  return wm.getMostRecentWindow(aType);
}

function doSearch(searchTerm, cmdLine) {
  var ss = Components.classes["@mozilla.org/browser/search-service;1"]
                     .getService(nsIBrowserSearchService);

  var submission = ss.defaultEngine.getSubmission(searchTerm);

  
  var sa = Components.classes["@mozilla.org/supports-array;1"]
                     .createInstance(Components.interfaces.nsISupportsArray);

  var wuri = Components.classes["@mozilla.org/supports-string;1"]
                       .createInstance(Components.interfaces.nsISupportsString);
  wuri.data = submission.uri.spec;

  sa.AppendElement(wuri);
  sa.AppendElement(null);
  sa.AppendElement(null);
  sa.AppendElement(submission.postData);

  
  

  var wwatch = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                         .getService(nsIWindowWatcher);

  return wwatch.openWindow(null, gBrowserContentHandler.chromeURL,
                           "_blank",
                           "chrome,dialog=no,all" +
                           gBrowserContentHandler.getFeatures(cmdLine),
                           sa);
}

function nsBrowserContentHandler() {
}
nsBrowserContentHandler.prototype = {
  classID: Components.ID("{5d0ce354-df01-421a-83fb-7ead0990c24e}"),

  _xpcom_factory: {
    createInstance: function bch_factory_ci(outer, iid) {
      if (outer)
        throw Components.results.NS_ERROR_NO_AGGREGATION;
      return gBrowserContentHandler.QueryInterface(iid);
    }
  },

  

  mChromeURL : null,

  get chromeURL() {
    if (this.mChromeURL) {
      return this.mChromeURL;
    }

    var prefb = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(nsIPrefBranch);
    this.mChromeURL = prefb.getCharPref("browser.chromeURL");

    return this.mChromeURL;
  },

  
  QueryInterface : XPCOMUtils.generateQI([nsICommandLineHandler,
                                          nsIBrowserHandler,
                                          nsIContentHandler,
                                          nsICommandLineValidator]),

  
  handle : function bch_handle(cmdLine) {
    if (cmdLine.handleFlag("browser", false)) {
      
      openWindow(null, this.chromeURL, "_blank",
                 "chrome,dialog=no,all" + this.getFeatures(cmdLine),
                 this.defaultArgs, NO_EXTERNAL_URIS);
      cmdLine.preventDefault = true;
    }

    
    
    
    
    
    
    
    if (cmdLine.handleFlag("remote", true)) {
      throw NS_ERROR_ABORT;
    }

    var uriparam;
    try {
      while ((uriparam = cmdLine.handleFlagWithParam("new-window", false))) {
        var uri = resolveURIInternal(cmdLine, uriparam);
        if (!shouldLoadURI(uri))
          continue;
        openWindow(null, this.chromeURL, "_blank",
                   "chrome,dialog=no,all" + this.getFeatures(cmdLine),
                   uri.spec);
        cmdLine.preventDefault = true;
      }
    }
    catch (e) {
      Components.utils.reportError(e);
    }

    try {
      while ((uriparam = cmdLine.handleFlagWithParam("new-tab", false))) {
        var uri = resolveURIInternal(cmdLine, uriparam);
        handURIToExistingBrowser(uri, nsIBrowserDOMWindow.OPEN_NEWTAB, cmdLine);
        cmdLine.preventDefault = true;
      }
    }
    catch (e) {
      Components.utils.reportError(e);
    }

    var chromeParam = cmdLine.handleFlagWithParam("chrome", false);
    if (chromeParam) {

      
      if (chromeParam == "chrome://browser/content/pref/pref.xul" ||
          (Services.prefs.getBoolPref("browser.preferences.inContent") &&
           chromeParam == "chrome://browser/content/preferences/preferences.xul")) {
        openPreferences();
        cmdLine.preventDefault = true;
      } else try {
        var uri = resolveURIInternal(cmdLine, chromeParam);
        let isLocal = (uri) => {
          let localSchemes = new Set(["chrome", "file", "resource"]);
          if (uri instanceof Components.interfaces.nsINestedURI) {
            uri = uri.QueryInterface(Components.interfaces.nsINestedURI).innerMostURI;
          }
          return localSchemes.has(uri.scheme);
        };
        if (isLocal(uri)) {
          
          var features = "chrome,dialog=no,all" + this.getFeatures(cmdLine);
          openWindow(null, uri.spec, "_blank", features);
          cmdLine.preventDefault = true;
        } else {
          dump("*** Preventing load of web URI as chrome\n");
          dump("    If you're trying to load a webpage, do not pass --chrome.\n");
        }
      }
      catch (e) {
        Components.utils.reportError(e);
      }
    }
    if (cmdLine.handleFlag("preferences", false)) {
      openPreferences();
      cmdLine.preventDefault = true;
    }
    if (cmdLine.handleFlag("silent", false))
      cmdLine.preventDefault = true;

    try {
      var privateWindowParam = cmdLine.handleFlagWithParam("private-window", false);
      if (privateWindowParam) {
        var uri = resolveURIInternal(cmdLine, privateWindowParam);
        handURIToExistingBrowser(uri, nsIBrowserDOMWindow.OPEN_NEWTAB, cmdLine, true);
        cmdLine.preventDefault = true;
      }
    } catch (e if e.result == Components.results.NS_ERROR_INVALID_ARG) {
      
      if (cmdLine.handleFlag("private-window", false)) {
        openWindow(null, this.chromeURL, "_blank",
          "chrome,dialog=no,private,all" + this.getFeatures(cmdLine),
          "about:privatebrowsing");
        cmdLine.preventDefault = true;
      }
    }

    var searchParam = cmdLine.handleFlagWithParam("search", false);
    if (searchParam) {
      doSearch(searchParam, cmdLine);
      cmdLine.preventDefault = true;
    }

    
    
    if (cmdLine.handleFlag("private", false)) {
      PrivateBrowsingUtils.enterTemporaryAutoStartMode();
    }

    var fileParam = cmdLine.handleFlagWithParam("file", false);
    if (fileParam) {
      var file = cmdLine.resolveFile(fileParam);
      var ios = Components.classes["@mozilla.org/network/io-service;1"]
                          .getService(Components.interfaces.nsIIOService);
      var uri = ios.newFileURI(file);
      openWindow(null, this.chromeURL, "_blank", 
                 "chrome,dialog=no,all" + this.getFeatures(cmdLine),
                 uri.spec);
      cmdLine.preventDefault = true;
    }

    if (AppConstants.platform  == "win") {
      
      for (var i = cmdLine.length - 1; i >= 0; --i) {
        var param = cmdLine.getArgument(i);
        if (param.match(/^\? /)) {
          cmdLine.removeArguments(i, i);
          cmdLine.preventDefault = true;

          searchParam = param.substr(2);
          doSearch(searchParam, cmdLine);
        }
      }
    }
  },

  get helpInfo() {
    let info =
              "  --browser          Open a browser window.\n" +
              "  --new-window <url> Open <url> in a new window.\n" +
              "  --new-tab <url>    Open <url> in a new tab.\n" +
              "  --private-window <url> Open <url> in a new private window.\n";
    if (AppConstants.platform == "win") {
      info += "  --preferences      Open Options dialog.\n";
    } else {
      info += "  --preferences      Open Preferences dialog.\n";
    }
    info += "  --search <term>    Search <term> with your default search engine.\n";
    return info;
  },

  

  get defaultArgs() {
    var prefb = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(nsIPrefBranch);

    if (!gFirstWindow) {
      gFirstWindow = true;
      if (PrivateBrowsingUtils.isInTemporaryAutoStartMode) {
        return "about:privatebrowsing";
      }
    }

    var overridePage = "";
    var willRestoreSession = false;
    try {
      
      
      
      
      
      let old_mstone = "unknown";
      try {
        old_mstone = Services.prefs.getCharPref("browser.startup.homepage_override.mstone");
      } catch (ex) {}
      let override = needHomepageOverride(prefb);
      if (override != OVERRIDE_NONE) {
        switch (override) {
          case OVERRIDE_NEW_PROFILE:
            
            overridePage = Services.urlFormatter.formatURLPref("startup.homepage_welcome_url");
            break;
          case OVERRIDE_NEW_MSTONE:
            
            
            
            
            
            var ss = Components.classes["@mozilla.org/browser/sessionstartup;1"]
                               .getService(Components.interfaces.nsISessionStartup);
            willRestoreSession = ss.isAutomaticRestoreEnabled();

            overridePage = Services.urlFormatter.formatURLPref("startup.homepage_override_url");
            if (prefb.prefHasUserValue("app.update.postupdate"))
              overridePage = getPostUpdateOverridePage(overridePage);

            overridePage = overridePage.replace("%OLD_VERSION%", old_mstone);
            break;
        }
      }
    } catch (ex) {}

    
    if (overridePage == "about:blank")
      overridePage = "";

    var startPage = "";
    try {
      var choice = prefb.getIntPref("browser.startup.page");
      if (choice == 1 || choice == 3)
        startPage = this.startPage;
    } catch (e) {
      Components.utils.reportError(e);
    }

    if (startPage == "about:blank")
      startPage = "";

    
    if (overridePage && startPage && !willRestoreSession)
      return overridePage + "|" + startPage;

    return overridePage || startPage || "about:blank";
  },

  get startPage() {
    var uri = Services.prefs.getComplexValue("browser.startup.homepage",
                                             nsIPrefLocalizedString).data;
    if (!uri) {
      Services.prefs.clearUserPref("browser.startup.homepage");
      uri = Services.prefs.getComplexValue("browser.startup.homepage",
                                           nsIPrefLocalizedString).data;
    }
    return uri;
  },

  mFeatures : null,

  getFeatures : function bch_features(cmdLine) {
    if (this.mFeatures === null) {
      this.mFeatures = "";

      try {
        var width = cmdLine.handleFlagWithParam("width", false);
        var height = cmdLine.handleFlagWithParam("height", false);

        if (width)
          this.mFeatures += ",width=" + width;
        if (height)
          this.mFeatures += ",height=" + height;
      }
      catch (e) {
      }

      
      
      if (PrivateBrowsingUtils.isInTemporaryAutoStartMode) {
        this.mFeatures = ",private";
      }
    }

    return this.mFeatures;
  },

  

  handleContent : function bch_handleContent(contentType, context, request) {
    try {
      var webNavInfo = Components.classes["@mozilla.org/webnavigation-info;1"]
                                 .getService(nsIWebNavigationInfo);
      if (!webNavInfo.isTypeSupported(contentType, null)) {
        throw NS_ERROR_WONT_HANDLE_CONTENT;
      }
    } catch (e) {
      throw NS_ERROR_WONT_HANDLE_CONTENT;
    }

    request.QueryInterface(nsIChannel);
    handURIToExistingBrowser(request.URI,
      nsIBrowserDOMWindow.OPEN_DEFAULTWINDOW, null);
    request.cancel(NS_BINDING_ABORTED);
  },

  
  validate : function bch_validate(cmdLine) {
    
    
    var osintFlagIdx = cmdLine.findFlag("osint", false);
    var urlFlagIdx = cmdLine.findFlag("url", false);
    if (urlFlagIdx > -1 && (osintFlagIdx > -1 ||
        cmdLine.state == nsICommandLine.STATE_REMOTE_EXPLICIT)) {
      var urlParam = cmdLine.getArgument(urlFlagIdx + 1);
      if (cmdLine.length != urlFlagIdx + 2 || /firefoxurl:/.test(urlParam))
        throw NS_ERROR_ABORT;
      var isDefault = false;
      try {
        var url = Services.urlFormatter.formatURLPref("app.support.baseURL") +
                  "win10-default-browser";
        if (urlParam == url) {
          var shellSvc = Components.classes["@mozilla.org/browser/shell-service;1"]
                                   .getService(Components.interfaces.nsIShellService);
          isDefault = shellSvc.isDefaultBrowser(false, false);
        }
      } catch (ex) {}
      if (isDefault) {
        
        
        throw NS_ERROR_ABORT;
      }
      cmdLine.handleFlag("osint", false)
    }
  },
};
var gBrowserContentHandler = new nsBrowserContentHandler();

function handURIToExistingBrowser(uri, location, cmdLine, forcePrivate)
{
  if (!shouldLoadURI(uri))
    return;

  
  
  var allowPrivate = forcePrivate || PrivateBrowsingUtils.permanentPrivateBrowsing;
  var navWin = RecentWindow.getMostRecentBrowserWindow({private: allowPrivate});
  if (!navWin) {
    
    var features = "chrome,dialog=no,all" + gBrowserContentHandler.getFeatures(cmdLine);
    if (forcePrivate) {
      features += ",private";
    }
    openWindow(null, gBrowserContentHandler.chromeURL, "_blank", features, uri.spec);
    return;
  }

  var navNav = navWin.QueryInterface(nsIInterfaceRequestor)
                     .getInterface(nsIWebNavigation);
  var rootItem = navNav.QueryInterface(nsIDocShellTreeItem).rootTreeItem;
  var rootWin = rootItem.QueryInterface(nsIInterfaceRequestor)
                        .getInterface(nsIDOMWindow);
  var bwin = rootWin.QueryInterface(nsIDOMChromeWindow).browserDOMWindow;
  bwin.openURI(uri, null, location,
               nsIBrowserDOMWindow.OPEN_EXTERNAL);
}

function nsDefaultCommandLineHandler() {
}

nsDefaultCommandLineHandler.prototype = {
  classID: Components.ID("{47cd0651-b1be-4a0f-b5c4-10e5a573ef71}"),

  
  QueryInterface : function dch_QI(iid) {
    if (!iid.equals(nsISupports) &&
        !iid.equals(nsICommandLineHandler))
      throw Components.results.NS_ERROR_NO_INTERFACE;

    return this;
  },

  _haveProfile: false,

  
  handle : function dch_handle(cmdLine) {
    var urilist = [];

    if (AppConstants.platform == "win") {
      
      
      
      
      
      
      if (!this._haveProfile) {
        try {
          
          var fl = Components.classes["@mozilla.org/file/directory_service;1"]
                             .getService(Components.interfaces.nsIProperties);
          var dir = fl.get("ProfD", Components.interfaces.nsILocalFile);
          this._haveProfile = true;
        }
        catch (e) {
          while ((ar = cmdLine.handleFlagWithParam("url", false))) { }
          cmdLine.preventDefault = true;
        }
      }
    }

    let redirectWinSearch = false;
    if (AppConstants.isPlatformAndVersionAtLeast("win", "10")) {
      redirectWinSearch = Services.prefs.getBoolPref("browser.search.redirectWindowsSearch");
    }

    try {
      var ar;
      while ((ar = cmdLine.handleFlagWithParam("url", false))) {
        var uri = resolveURIInternal(cmdLine, ar);

        
        
        
        if (redirectWinSearch && uri.spec.startsWith("https://www.bing.com/search")) {
          try {
            var url = uri.QueryInterface(Components.interfaces.nsIURL);
            var params = new URLSearchParams(url.query);
            
            
            
            var formParam = params.get("form");
            if (!formParam) {
              formParam = params.get("FORM");
            }
            if (formParam == "WNSGPH" || formParam == "WNSBOX") {
              var term = params.get("q");
              var ss = Components.classes["@mozilla.org/browser/search-service;1"]
                                 .getService(nsIBrowserSearchService);
              var submission = ss.defaultEngine.getSubmission(term, null, "searchbar");
              uri = submission.uri;
            }
          } catch (e) {
            Components.utils.reportError("Couldn't redirect Windows search: " + e);
          }
        }

        urilist.push(uri);
      }
    }
    catch (e) {
      Components.utils.reportError(e);
    }

    for (let i = 0; i < cmdLine.length; ++i) {
      var curarg = cmdLine.getArgument(i);
      if (curarg.match(/^-/)) {
        Components.utils.reportError("Warning: unrecognized command line flag " + curarg + "\n");
        
        
        ++i;
      } else {
        try {
          urilist.push(resolveURIInternal(cmdLine, curarg));
        }
        catch (e) {
          Components.utils.reportError("Error opening URI '" + curarg + "' from the command line: " + e + "\n");
        }
      }
    }

    if (urilist.length) {
      if (cmdLine.state != nsICommandLine.STATE_INITIAL_LAUNCH &&
          urilist.length == 1) {
        
        
        try {
          handURIToExistingBrowser(urilist[0], nsIBrowserDOMWindow.OPEN_DEFAULTWINDOW, cmdLine);
          return;
        }
        catch (e) {
        }
      }

      var URLlist = urilist.filter(shouldLoadURI).map(function (u) u.spec);
      if (URLlist.length) {
        openWindow(null, gBrowserContentHandler.chromeURL, "_blank",
                   "chrome,dialog=no,all" + gBrowserContentHandler.getFeatures(cmdLine),
                   URLlist);
      }

    }
    else if (!cmdLine.preventDefault) {
      if (AppConstants.isPlatformAndVersionAtLeast("win", "10") &&
          cmdLine.state != nsICommandLine.STATE_INITIAL_LAUNCH &&
          WindowsUIUtils.inTabletMode) {
        
        let win = RecentWindow.getMostRecentBrowserWindow();
        if (win) {
          win.focus();
          return;
        }
      }
      
      openWindow(null, gBrowserContentHandler.chromeURL, "_blank",
                 "chrome,dialog=no,all" + gBrowserContentHandler.getFeatures(cmdLine),
                 gBrowserContentHandler.defaultArgs, NO_EXTERNAL_URIS);
    }
  },

  helpInfo : "",
};

var components = [nsBrowserContentHandler, nsDefaultCommandLineHandler];
this.NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
