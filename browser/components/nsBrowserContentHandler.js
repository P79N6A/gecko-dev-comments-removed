# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the Mozilla Firefox browser.
#
# The Initial Developer of the Original Code is
# Benjamin Smedberg <benjamin@smedbergs.us>
#
# Portions created by the Initial Developer are Copyright (C) 2004
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Robert Strong <robert.bugzilla@gmail.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

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
const nsIHttpProtocolHandler = Components.interfaces.nsIHttpProtocolHandler;
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
const nsIXULAppInfo          = Components.interfaces.nsIXULAppInfo;

const NS_BINDING_ABORTED = Components.results.NS_BINDING_ABORTED;
const NS_ERROR_WONT_HANDLE_CONTENT = 0x805d0001;
const NS_ERROR_ABORT = Components.results.NS_ERROR_ABORT;

const URI_INHERITS_SECURITY_CONTEXT = nsIHttpProtocolHandler
                                        .URI_INHERITS_SECURITY_CONTEXT;

function shouldLoadURI(aURI) {
  if (aURI && !aURI.schemeIs("chrome"))
    return true;

  dump("*** Preventing external load of chrome: URI into browser window\n");
  dump("    Use -chrome <uri> instead\n");
  return false;
}

function resolveURIInternal(aCmdLine, aArgument) {
  var uri = aCmdLine.resolveURI(aArgument);

  if (!(uri instanceof nsIFileURL)) {
    return uri;
  }

  try {
    if (uri.file.exists())
      return uri;
  }
  catch (e) {
    Components.utils.reportError(e);
  }

  
  
 
  try {
    var urifixup = Components.classes["@mozilla.org/docshell/urifixup;1"]
                             .getService(nsIURIFixup);

    uri = urifixup.createFixupURI(aArgument, 0);
  }
  catch (e) {
    Components.utils.reportError(e);
  }

  return uri;
}

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

  var mstone = Components.classes["@mozilla.org/network/protocol;1?name=http"]
                         .getService(nsIHttpProtocolHandler).misc;

  var savedBuildID = null;
  try {
    savedBuildID = prefb.getCharPref("browser.startup.homepage_override.buildID");
  } catch (e) {}

  var buildID =  Components.classes["@mozilla.org/xre/app-info;1"]
                           .getService(nsIXULAppInfo).platformBuildID;

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



function copyPrefOverride() {
  try {
    var fileLocator = Components.classes["@mozilla.org/file/directory_service;1"]
                                .getService(Components.interfaces.nsIProperties);
    const NS_APP_EXISTING_PREF_OVERRIDE = "ExistingPrefOverride";
    var prefOverride = fileLocator.get(NS_APP_EXISTING_PREF_OVERRIDE,
                                       Components.interfaces.nsIFile);
    if (!prefOverride.exists())
      return; 

    const NS_APP_PREFS_OVERRIDE_DIR     = "PrefDOverride";
    var prefOverridesDir = fileLocator.get(NS_APP_PREFS_OVERRIDE_DIR,
                                           Components.interfaces.nsIFile);

    
    var existingPrefOverridesFile = prefOverridesDir.clone();
    existingPrefOverridesFile.append(prefOverride.leafName);
    if (existingPrefOverridesFile.exists())
      existingPrefOverridesFile.remove(false);

    prefOverride.copyTo(prefOverridesDir, null);

    
    
    var prefSvcObs = Components.classes["@mozilla.org/preferences-service;1"]
                               .getService(Components.interfaces.nsIObserver);
    prefSvcObs.observe(null, "reload-default-prefs", null);
  } catch (ex) {
    Components.utils.reportError(ex);
  }
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
  var features = "chrome,titlebar,toolbar,centerscreen,dialog=no";
  var url = "chrome://browser/content/preferences/preferences.xul";

  var win = getMostRecentWindow("Browser:Preferences");
  if (win) {
    win.focus();
  } else {
    openWindow(null, url, "_blank", features);
  }
}

function getMostRecentWindow(aType) {
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(nsIWindowMediator);
  return wm.getMostRecentWindow(aType);
}


function getMostRecentBrowserWindow() {
  var browserGlue = Components.classes["@mozilla.org/browser/browserglue;1"]
                              .getService(Components.interfaces.nsIBrowserGlue);
  return browserGlue.getMostRecentBrowserWindow();
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

    try {
      var remoteCommand = cmdLine.handleFlagWithParam("remote", true);
    }
    catch (e) {
      throw NS_ERROR_ABORT;
    }

    if (remoteCommand != null) {
      try {
        var a = /^\s*(\w+)\(([^\)]*)\)\s*$/.exec(remoteCommand);
        var remoteVerb;
        if (a) {
          remoteVerb = a[1].toLowerCase();
          var remoteParams = [];
          var sepIndex = a[2].lastIndexOf(",");
          if (sepIndex == -1)
            remoteParams[0] = a[2];
          else {
            remoteParams[0] = a[2].substring(0, sepIndex);
            remoteParams[1] = a[2].substring(sepIndex + 1);
          }
        }

        switch (remoteVerb) {
        case "openurl":
        case "openfile":
          
          
          

          
          
          var url = remoteParams[0];
          var target = nsIBrowserDOMWindow.OPEN_DEFAULTWINDOW;
          if (remoteParams[1]) {
            var targetParam = remoteParams[1].toLowerCase()
                                             .replace(/^\s*|\s*$/g, "");
            if (targetParam == "new-tab")
              target = nsIBrowserDOMWindow.OPEN_NEWTAB;
            else if (targetParam == "new-window")
              target = nsIBrowserDOMWindow.OPEN_NEWWINDOW;
            else {
              
              
              url += "," + remoteParams[1];
            }
          }

          var uri = resolveURIInternal(cmdLine, url);
          handURIToExistingBrowser(uri, target, cmdLine);
          break;

        case "xfedocommand":
          
          if (remoteParams[0].toLowerCase() != "openbrowser")
            throw NS_ERROR_ABORT;

          
          openWindow(null, this.chromeURL, "_blank",
                     "chrome,dialog=no,all" + this.getFeatures(cmdLine),
                     this.defaultArgs, NO_EXTERNAL_URIS);
          break;

        default:
          
          
          throw "Unknown remote command.";
        }

        cmdLine.preventDefault = true;
      }
      catch (e) {
        Components.utils.reportError(e);
        
        
        
        throw NS_ERROR_ABORT;
      }
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

      
      if (chromeParam == "chrome://browser/content/pref/pref.xul") {
        openPreferences();
        cmdLine.preventDefault = true;
      } else try {
        
        var features = "chrome,dialog=no,all" + this.getFeatures(cmdLine);
        var uri = resolveURIInternal(cmdLine, chromeParam);
        var netutil = Components.classes["@mozilla.org/network/util;1"]
                                .getService(nsINetUtil);
        if (!netutil.URIChainHasFlags(uri, URI_INHERITS_SECURITY_CONTEXT)) {
          openWindow(null, uri.spec, "_blank", features);
          cmdLine.preventDefault = true;
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
    if (cmdLine.findFlag("private-toggle", false) >= 0)
      cmdLine.preventDefault = true;

    var searchParam = cmdLine.handleFlagWithParam("search", false);
    if (searchParam) {
      doSearch(searchParam, cmdLine);
      cmdLine.preventDefault = true;
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

#ifdef XP_WIN
    
    for (var i = cmdLine.length - 1; i >= 0; --i) {
      var param = cmdLine.getArgument(i);
      if (param.match(/^\? /)) {
        cmdLine.removeArguments(i, i);
        cmdLine.preventDefault = true;

        searchParam = param.substr(2);
        doSearch(searchParam, cmdLine);
      }
    }
#endif
  },

  helpInfo : "  -browser           Open a browser window.\n" +
             "  -new-window  <url> Open <url> in a new window.\n" +
             "  -new-tab     <url> Open <url> in a new tab.\n" +
#ifdef XP_WIN
             "  -preferences       Open Options dialog.\n" +
#else
             "  -preferences       Open Preferences dialog.\n" +
#endif
             "  -search     <term> Search <term> with your default search engine.\n",

  

  get defaultArgs() {
    var prefb = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(nsIPrefBranch);

    var overridePage = "";
    var haveUpdateSession = false;
    try {
      let override = needHomepageOverride(prefb);
      if (override != OVERRIDE_NONE) {
        
        AboutHomeUtils.loadDefaultSearchEngine();
        AboutHomeUtils.loadSnippetsURL();

        switch (override) {
          case OVERRIDE_NEW_PROFILE:
            
            overridePage = Services.urlFormatter.formatURLPref("startup.homepage_welcome_url");
            break;
          case OVERRIDE_NEW_MSTONE:
            
            copyPrefOverride();

            
            
            var ss = Components.classes["@mozilla.org/browser/sessionstartup;1"]
                               .getService(Components.interfaces.nsISessionStartup);
            haveUpdateSession = ss.doRestore();
            overridePage = Services.urlFormatter.formatURLPref("startup.homepage_override_url");
            if (prefb.prefHasUserValue("app.update.postupdate"))
              overridePage = getPostUpdateOverridePage(overridePage);
            break;
        }
      }
      else {
        
        
        if (Services.prefs.prefHasUserValue(AboutHomeUtils.SNIPPETS_URL_PREF)) {
          AboutHomeUtils.loadSnippetsURL();
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

      if (choice == 2)
        startPage = Components.classes["@mozilla.org/browser/global-history;2"]
                              .getService(nsIBrowserHistory).lastPageVisited;
    } catch (e) {
      Components.utils.reportError(e);
    }

    if (startPage == "about:blank")
      startPage = "";

    
    if (overridePage && startPage && !haveUpdateSession)
      return overridePage + "|" + startPage;

    return overridePage || startPage || "about:blank";
  },

  get startPage() {
    var prefb = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(nsIPrefBranch);

    var uri = prefb.getComplexValue("browser.startup.homepage",
                                    nsIPrefLocalizedString).data;

    if (!uri) {
      prefb.clearUserPref("browser.startup.homepage");
      uri = prefb.getComplexValue("browser.startup.homepage",
                                  nsIPrefLocalizedString).data;
    }
                                
    var count;
    try {
      count = prefb.getIntPref("browser.startup.homepage.count");
    }
    catch (e) {
      return uri;
    }

    for (var i = 1; i < count; ++i) {
      try {
        var page = prefb.getComplexValue("browser.startup.homepage." + i,
                                         nsIPrefLocalizedString).data;
        uri += "\n" + page;
      }
      catch (e) {
      }
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
      cmdLine.handleFlag("osint", false)
    }
  },
};
var gBrowserContentHandler = new nsBrowserContentHandler();

function handURIToExistingBrowser(uri, location, cmdLine)
{
  if (!shouldLoadURI(uri))
    return;

  var navWin = getMostRecentBrowserWindow();
  if (!navWin) {
    
    openWindow(null, gBrowserContentHandler.chromeURL, "_blank",
               "chrome,dialog=no,all" + gBrowserContentHandler.getFeatures(cmdLine),
               uri.spec);
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

  
  
  
  _handledURIs: [ ],
#ifdef XP_WIN
  _haveProfile: false,
#endif

  
  handle : function dch_handle(cmdLine) {
    var urilist = [];

#ifdef XP_WIN
    
    
    
    
    
    
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
#endif

    try {
      var ar;
      while ((ar = cmdLine.handleFlagWithParam("url", false))) {
        var found = false;
        var uri = resolveURIInternal(cmdLine, ar);
        
        var count = this._handledURIs.length;
        for (var i = 0; i < count; ++i) {
          if (this._handledURIs[i].spec == uri.spec) {
            this._handledURIs.splice(i, 1);
            found = true;
            cmdLine.preventDefault = true;
            break;
          }
        }
        if (!found) {
          urilist.push(uri);
          
          if (cmdLine.handleFlag("requestpending", false) &&
              cmdLine.state == nsICommandLine.STATE_INITIAL_LAUNCH)
            this._handledURIs.push(uri)
        }
      }
    }
    catch (e) {
      Components.utils.reportError(e);
    }

    count = cmdLine.length;

    for (i = 0; i < count; ++i) {
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
      
      openWindow(null, gBrowserContentHandler.chromeURL, "_blank",
                 "chrome,dialog=no,all" + gBrowserContentHandler.getFeatures(cmdLine),
                 gBrowserContentHandler.defaultArgs, NO_EXTERNAL_URIS);
    }
  },

  helpInfo : "",
};

let AboutHomeUtils = {
  SNIPPETS_URL_PREF: "browser.aboutHomeSnippets.updateUrl",
  get _storage() {
    let aboutHomeURI = Services.io.newURI("moz-safe-about:home", null, null);
    let principal = Components.classes["@mozilla.org/scriptsecuritymanager;1"].
                    getService(Components.interfaces.nsIScriptSecurityManager).
                    getCodebasePrincipal(aboutHomeURI);
    let dsm = Components.classes["@mozilla.org/dom/storagemanager;1"].
              getService(Components.interfaces.nsIDOMStorageManager);
    return dsm.getLocalStorageForPrincipal(principal, "");
  },

  loadDefaultSearchEngine: function AHU_loadDefaultSearchEngine()
  {
    let defaultEngine = Services.search.originalDefaultEngine;
    let submission = defaultEngine.getSubmission("_searchTerms_");
    if (submission.postData)
      throw new Error("Home page does not support POST search engines.");
    let engine = {
      name: defaultEngine.name
    , searchUrl: submission.uri.spec
    }
    this._storage.setItem("search-engine", JSON.stringify(engine));
  },

  loadSnippetsURL: function AHU_loadSnippetsURL()
  {
    const STARTPAGE_VERSION = 1;
    let updateURL = Services.prefs
                            .getCharPref(this.SNIPPETS_URL_PREF)
                            .replace("%STARTPAGE_VERSION%", STARTPAGE_VERSION);
    updateURL = Services.urlFormatter.formatURL(updateURL);
    this._storage.setItem("snippets-update-url", updateURL);
  },
};

var components = [nsBrowserContentHandler, nsDefaultCommandLineHandler];
var NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
