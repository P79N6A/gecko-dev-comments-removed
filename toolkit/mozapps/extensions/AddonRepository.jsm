





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/AddonManager.jsm");

var EXPORTED_SYMBOLS = [ "AddonRepository" ];

const PREF_GETADDONS_BROWSEADDONS        = "extensions.getAddons.browseAddons";
const PREF_GETADDONS_BROWSERECOMMENDED   = "extensions.getAddons.recommended.browseURL";
const PREF_GETADDONS_GETRECOMMENDED      = "extensions.getAddons.recommended.url";
const PREF_GETADDONS_BROWSESEARCHRESULTS = "extensions.getAddons.search.browseURL";
const PREF_GETADDONS_GETSEARCHRESULTS    = "extensions.getAddons.search.url";

const XMLURI_PARSE_ERROR  = "http://www.mozilla.org/newlayout/xml/parsererror.xml";

const API_VERSION = "1.2";

function AddonSearchResult(aId) {
  this.id = aId;
  this.screenshots = [];
}

AddonSearchResult.prototype = {
  


  id: null,

  


  name: null,

  


  version: null,

  


  description: null,

  


  fullDescription: null,

  


  rating: -1,

  


  iconURL: null,

  


  screenshots: null,

  


  homepageURL: null,

  


  type: null,

  


  install: null,

  



  isCompatible: true,

  


  providesUpdatesSecurely: true,

  


  blocklistState: Ci.nsIBlocklistService.STATE_NOT_BLOCKED,

  



  appDisabled: false,

  


  userDisabled: false,

  



  size: null,

  



  scope: AddonManager.SCOPE_PROFILE,

  


  isActive: true,

  


  creator: null,

  



  pendingOperations: AddonManager.PENDING_NONE,

  



  permissions: 0
}












var AddonRepository = {
  
  _results: null,

  
  _searching: false,

  
  _recommended: false,

  
  _request: null,

  














  _callback: null,

  
  _maxResults: null,

  



  get homepageURL() {
    return Cc["@mozilla.org/toolkit/URLFormatterService;1"].
           getService(Ci.nsIURLFormatter).
           formatURLPref(PREF_GETADDONS_BROWSEADDONS);
  },

  



  get isSearching() {
    return this._searching;
  },

  


  getRecommendedURL: function() {
    var urlf = Cc["@mozilla.org/toolkit/URLFormatterService;1"].
               getService(Ci.nsIURLFormatter);

    return urlf.formatURLPref(PREF_GETADDONS_BROWSERECOMMENDED);
  },

  





  getSearchURL: function(aSearchTerms) {
    var prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefBranch);
    var urlf = Cc["@mozilla.org/toolkit/URLFormatterService;1"].
               getService(Ci.nsIURLFormatter);

    var url = prefs.getCharPref(PREF_GETADDONS_BROWSESEARCHRESULTS);
    url = url.replace(/%TERMS%/g, encodeURIComponent(aSearchTerms));
    return urlf.formatURL(url);
  },

  



  cancelSearch: function() {
    this._searching = false;
    if (this._request) {
      this._request.abort();
      this._request = null;
    }
    this._callback = null;
    this._results = null;
  },

  






  retrieveRecommendedAddons: function(aMaxResults, aCallback) {
    if (this._searching)
      return;

    this._searching = true;
    this._results = [];
    this._callback = aCallback;
    this._recommended = true;
    this._maxResults = aMaxResults;

    var prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefBranch);
    var urlf = Cc["@mozilla.org/toolkit/URLFormatterService;1"].
               getService(Ci.nsIURLFormatter);

    var uri = prefs.getCharPref(PREF_GETADDONS_GETRECOMMENDED);
    uri = uri.replace(/%API_VERSION%/g, API_VERSION);
    uri = urlf.formatURL(uri);
    this._loadList(uri);
  },

  







  searchAddons: function(aSearchTerms, aMaxResults, aCallback) {
    if (this._searching)
      return;

    this._searching = true;
    this._results = [];
    this._callback = aCallback;
    this._recommended = false;
    this._maxResults = aMaxResults;

    var prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefBranch);
    var urlf = Cc["@mozilla.org/toolkit/URLFormatterService;1"].
               getService(Ci.nsIURLFormatter);

    var uri = prefs.getCharPref(PREF_GETADDONS_GETSEARCHRESULTS);
    uri = uri.replace(/%API_VERSION%/g, API_VERSION);
    
    uri = uri.replace(/%TERMS%/g, encodeURIComponent(encodeURIComponent(aSearchTerms)));
    uri = urlf.formatURL(uri);
    this._loadList(uri);
  },

  
  _reportSuccess: function(aTotalResults) {
    this._searching = false;
    this._request = null;
    
    var addons = [result.addon for each(result in this._results)];
    var callback = this._callback;
    this._callback = null;
    this._results = null;
    callback.searchSucceeded(addons, addons.length, this._recommended ? -1 : aTotalResults);
  },

  
  _reportFailure: function() {
    this._searching = false;
    this._request = null;
    
    var callback = this._callback;
    this._callback = null;
    this._results = null;
    callback.searchFailed();
  },

  
  _parseAddon: function(aElement, aSkip) {
    var app = Cc["@mozilla.org/xre/app-info;1"].
              getService(Ci.nsIXULAppInfo).
              QueryInterface(Ci.nsIXULRuntime);

    var guidList = aElement.getElementsByTagName("guid");
    if (guidList.length != 1)
      return;

    var guid = guidList[0].textContent.trim();

    
    for (var i = 0; i < this._results.length; i++)
      if (this._results[i].addon.id == guid)
        return;

    
    if (aSkip.ids.indexOf(guid) != -1)
      return;

    
    var status = aElement.getElementsByTagName("status");
    
    if (status.length != 1 || status[0].getAttribute("id") != 4)
      return;

    
    var osList = aElement.getElementsByTagName("compatible_os");
    
    
    if (osList.length > 0) {
      var compatible = false;
      var i = 0;
      while (i < osList.length && !compatible) {
        var os = osList[i].textContent.trim();
        if (os == "ALL" || os == app.OS) {
          compatible = true;
          break;
        }
        i++;
      }
      if (!compatible)
        return;
    }

    
    compatible = false;
    var tags = aElement.getElementsByTagName("compatible_applications");
    if (tags.length != 1)
      return;
    var vc = Cc["@mozilla.org/xpcom/version-comparator;1"].
             getService(Ci.nsIVersionComparator);
    var apps = tags[0].getElementsByTagName("appID");
    var i = 0;
    while (i < apps.length) {
      if (apps[i].textContent.trim() == app.ID) {
        var parent = apps[i].parentNode;
        var minversion = parent.getElementsByTagName("min_version")[0].textContent.trim();
        var maxversion = parent.getElementsByTagName("max_version")[0].textContent.trim();
        if ((vc.compare(minversion, app.version) > 0) ||
            (vc.compare(app.version, maxversion) > 0))
          return;
        compatible = true;
        break;
      }
      i++;
    }
    if (!compatible)
      return;

    var addon = new AddonSearchResult(guid);
    var result = {
      addon: addon,
      xpiURL: null,
      xpiHash: null
    };
    var node = aElement.firstChild;
    while (node) {
      if (node instanceof Ci.nsIDOMElement) {
        switch (node.localName) {
          case "name":
          case "version":
            addon[node.localName] = node.textContent.trim();
            break;
          case "summary":
            addon.description = node.textContent.trim();
            break;
          case "description":
            addon.fullDescription = node.textContent.trim();
            break;
          case "rating":
            if (node.textContent.length > 0) {
              var rating = parseInt(node.textContent);
              if (rating >= 0)
                addon.rating = Math.min(5, rating);
            }
            break;
          case "thumbnail":
            addon.screenshots.push(node.textContent.trim());
            break;
          case "icon":
            addon.iconURL = node.textContent.trim();
            break;
          case "learnmore":
            addon.homepageURL = node.textContent.trim();
            break;
          case "type":
            
            
            addon.type = (node.getAttribute("id") == 2) ? "theme" : "extension";
            break;
          case "install":
            
            if (node.hasAttribute("os")) {
              var os = node.getAttribute("os").toLowerCase();
              
              if (os != "all" && os != app.OS.toLowerCase())
                break;
            }
            result.xpiURL = node.textContent.trim();
            if (node.hasAttribute("size"))
              addon.size = node.getAttribute("size");

            
            if (aSkip.sourceURIs.indexOf(result.xpiURL) != -1)
              return;

            result.xpiHash = node.hasAttribute("hash") ? node.getAttribute("hash") : null;
            break;
        }
      }
      node = node.nextSibling;
    }

    
    if (result.xpiURL)
      this._results.push(result);
  },

  _parseAddons: function(aElements, aTotalResults, aSkip) {
    for (var i = 0; i < aElements.length && this._results.length < this._maxResults; i++)
      this._parseAddon(aElements[i], aSkip);

    var pendingResults = this._results.length;
    if (pendingResults == 0) {
      this._reportSuccess(aTotalResults);
      return;
    }

    var self = this;
    this._results.forEach(function(aResult) {
      var addon = aResult.addon;
      var callback = function(aInstall) {
        addon.install = aInstall;
        pendingResults--;
        if (pendingResults == 0)
          self._reportSuccess(aTotalResults);
      }

      AddonManager.getInstallForURL(aResult.xpiURL, callback,
                                    "application/x-xpinstall", aResult.xpiHash,
                                    addon.name, addon.iconURL, addon.version);
    });
  },

  
  
  _listLoaded: function(aEvent) {
    var request = aEvent.target;
    var responseXML = request.responseXML;

    if (!responseXML || responseXML.documentElement.namespaceURI == XMLURI_PARSE_ERROR ||
        (request.status != 200 && request.status != 0)) {
      this._reportFailure();
      return;
    }

    var elements = responseXML.documentElement.getElementsByTagName("addon");
    if (responseXML.documentElement.hasAttribute("total_results"))
      var totalResults = responseXML.documentElement.getAttribute("total_results");
    else
      var totalResults = elements.length;

    var self = this;
    var skip = {ids: null, sourceURIs: null};

    AddonManager.getAllAddons(function(aAddons) {
      skip.ids  = [a.id for each (a in aAddons)];
      if (skip.sourceURIs)
        self._parseAddons(elements, totalResults, skip);
    });

    AddonManager.getAllInstalls(function(aInstalls) {
      skip.sourceURIs = [];
      aInstalls.forEach(function(aInstall) {
        if (aInstall.state != AddonManager.STATE_AVAILABLE)
          skip.sourceURIs.push(aInstall.sourceURI.spec);
      });

      if (skip.ids)
        self._parseAddons(elements, totalResults, skip);
    });
  },

  
  _loadList: function(aURI) {
    this._request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                    createInstance(Ci.nsIXMLHttpRequest);
    this._request.open("GET", aURI, true);
    this._request.overrideMimeType("text/xml");

    var self = this;
    this._request.onerror = function(aEvent) { self._reportFailure(); };
    this._request.onload = function(aEvent) { self._listLoaded(aEvent); };
    this._request.send(null);
  }
}

