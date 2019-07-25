







































"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Components.utils.import("resource://gre/modules/FileUtils.jsm");
Components.utils.import("resource://gre/modules/NetUtil.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/AddonManager.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

var EXPORTED_SYMBOLS = [ "AddonRepository" ];

const PREF_GETADDONS_CACHE_ENABLED       = "extensions.getAddons.cache.enabled";
const PREF_GETADDONS_CACHE_TYPES         = "extensions.getAddons.cache.types";
const PREF_GETADDONS_CACHE_ID_ENABLED    = "extensions.%ID%.getAddons.cache.enabled"
const PREF_GETADDONS_BROWSEADDONS        = "extensions.getAddons.browseAddons";
const PREF_GETADDONS_BYIDS               = "extensions.getAddons.get.url";
const PREF_GETADDONS_BYIDS_PERFORMANCE   = "extensions.getAddons.getWithPerformance.url";
const PREF_GETADDONS_BROWSERECOMMENDED   = "extensions.getAddons.recommended.browseURL";
const PREF_GETADDONS_GETRECOMMENDED      = "extensions.getAddons.recommended.url";
const PREF_GETADDONS_BROWSESEARCHRESULTS = "extensions.getAddons.search.browseURL";
const PREF_GETADDONS_GETSEARCHRESULTS    = "extensions.getAddons.search.url";

const PREF_CHECK_COMPATIBILITY_BASE = "extensions.checkCompatibility";

const BRANCH_REGEXP                   = /^([^\.]+\.[0-9]+[a-z]*).*/gi;

XPCOMUtils.defineLazyGetter(this, "PREF_CHECK_COMPATIBILITY", function () {
#ifdef MOZ_COMPATIBILITY_NIGHTLY
  return PREF_CHECK_COMPATIBILITY_BASE + ".nightly";
#else
  return PREF_CHECK_COMPATIBILITY_BASE + "." +
         Services.appinfo.version.replace(BRANCH_REGEXP, "$1");
#endif
});

const PREF_EM_STRICT_COMPATIBILITY       = "extensions.strictCompatibility";

const STRICT_COMPATIBILITY_DEFAULT       = true;

const XMLURI_PARSE_ERROR  = "http://www.mozilla.org/newlayout/xml/parsererror.xml";

const API_VERSION = "1.5";
const DEFAULT_CACHE_TYPES = "extension,theme,locale,dictionary";

const KEY_PROFILEDIR = "ProfD";
const FILE_DATABASE  = "addons.sqlite";
const DB_SCHEMA      = 3;

const TOOLKIT_ID     = "toolkit@mozilla.org";

["LOG", "WARN", "ERROR"].forEach(function(aName) {
  this.__defineGetter__(aName, function() {
    Components.utils.import("resource://gre/modules/AddonLogging.jsm");

    LogManager.getLogger("addons.repository", this);
    return this[aName];
  });
}, this);





const PROP_SINGLE = ["id", "type", "name", "version", "creator", "description",
                     "fullDescription", "developerComments", "eula", "iconURL",
                     "homepageURL", "supportURL", "contributionURL",
                     "contributionAmount", "averageRating", "reviewCount",
                     "reviewURL", "totalDownloads", "weeklyDownloads",
                     "dailyUsers", "sourceURI", "repositoryStatus", "size",
                     "updateDate"];
const PROP_MULTI = ["developers", "screenshots"]



const STRING_KEY_MAP = {
  name:               "name",
  version:            "version",
  icon:               "iconURL",
  homepage:           "homepageURL",
  support:            "supportURL"
};



const HTML_KEY_MAP = {
  summary:            "description",
  description:        "fullDescription",
  developer_comments: "developerComments",
  eula:               "eula"
};



const INTEGER_KEY_MAP = {
  total_downloads:  "totalDownloads",
  weekly_downloads: "weeklyDownloads",
  daily_users:      "dailyUsers"
};


function convertHTMLToPlainText(html) {
  if (!html)
    return html;
  var converter = Cc["@mozilla.org/widget/htmlformatconverter;1"].
                  createInstance(Ci.nsIFormatConverter);

  var input = Cc["@mozilla.org/supports-string;1"].
              createInstance(Ci.nsISupportsString);
  input.data = html.replace(/\n/g, "<br>");

  var output = {};
  converter.convert("text/html", input, input.data.length, "text/unicode",
                    output, {});

  if (output.value instanceof Ci.nsISupportsString)
    return output.value.data.replace(/\r\n/g, "\n");
  return html;
}

function getAddonsToCache(aIds, aCallback) {
  try {
    var types = Services.prefs.getCharPref(PREF_GETADDONS_CACHE_TYPES);
  }
  catch (e) { }
  if (!types)
    types = DEFAULT_CACHE_TYPES;

  types = types.split(",");

  AddonManager.getAddonsByIDs(aIds, function(aAddons) {
    let enabledIds = [];
    for (var i = 0; i < aIds.length; i++) {
      var preference = PREF_GETADDONS_CACHE_ID_ENABLED.replace("%ID%", aIds[i]);
      try {
        if (!Services.prefs.getBoolPref(preference))
          continue;
      } catch(e) {
        
      }

      
      
      if (aAddons[i] && (types.indexOf(aAddons[i].type) == -1))
        continue;

      enabledIds.push(aIds[i]);
    }

    aCallback(enabledIds);
  });
}

function AddonSearchResult(aId) {
  this.id = aId;
}

AddonSearchResult.prototype = {
  


  id: null,

  


  type: null,

  


  name: null,

  


  version: null,

  


  creator: null,

  


  developers: null,

  


  description: null,

  


  fullDescription: null,

  




  developerComments: null,

  


  eula: null,

  


  iconURL: null,

  


  screenshots: null,

  


  homepageURL: null,

  


  supportURL: null,

  


  contributionURL: null,

  


  contributionAmount: null,

  


  purchaseURL: null,

  



  purchaseAmount: null,

  


  purchaseDisplayAmount: null,

  


  averageRating: null,

  


  reviewCount: null,

  


  reviewURL: null,

  


  totalDownloads: null,

  


  weeklyDownloads: null,

  


  dailyUsers: null,

  


  install: null,

  


  sourceURI: null,

  


  repositoryStatus: null,

  



  size: null,

  


  updateDate: null,

  



  isCompatible: true,

  



  isPlatformCompatible: true,

  



  compatibilityOverrides: null,

  


  providesUpdatesSecurely: true,

  


  blocklistState: Ci.nsIBlocklistService.STATE_NOT_BLOCKED,

  



  appDisabled: false,

  


  userDisabled: false,

  



  scope: AddonManager.SCOPE_PROFILE,

  


  isActive: true,

  



  pendingOperations: AddonManager.PENDING_NONE,

  



  permissions: 0,

  









  isCompatibleWith: function(aAppVerison, aPlatformVersion) {
    return true;
  },

  












  findUpdates: function(aListener, aReason, aAppVersion, aPlatformVersion) {
    if ("onNoCompatibilityUpdateAvailable" in aListener)
      aListener.onNoCompatibilityUpdateAvailable(this);
    if ("onNoUpdateAvailable" in aListener)
      aListener.onNoUpdateAvailable(this);
    if ("onUpdateFinished" in aListener)
      aListener.onUpdateFinished(this);
  }
}












var AddonRepository = {
  


  get cacheEnabled() {
    
    
    if (!AddonDatabase.databaseOk)
      return false;

    let preference = PREF_GETADDONS_CACHE_ENABLED;
    let enabled = false;
    try {
      enabled = Services.prefs.getBoolPref(preference);
    } catch(e) {
      WARN("cacheEnabled: Couldn't get pref: " + preference);
    }

    return enabled;
  },

  
  _addons: null,

  
  _pendingCallbacks: null,

  
  _searching: false,

  
  _request: null,

  
















  _callback: null,

  
  _maxResults: null,
  
  


  initialize: function() {
    Services.obs.addObserver(this, "xpcom-shutdown", false);
  },

  


  observe: function (aSubject, aTopic, aData) {
    if (aTopic == "xpcom-shutdown") {
      Services.obs.removeObserver(this, "xpcom-shutdown");
      this.shutdown();
    }
  },

  


  shutdown: function() {
    this.cancelSearch();

    this._addons = null;
    this._pendingCallbacks = null;
    AddonDatabase.shutdown(function() {
      Services.obs.notifyObservers(null, "addon-repository-shutdown", null);
    });
  },

  









  getCachedAddonByID: function(aId, aCallback) {
    if (!aId || !this.cacheEnabled) {
      aCallback(null);
      return;
    }

    let self = this;
    function getAddon(aAddons) {
      aCallback((aId in aAddons) ? aAddons[aId] : null);
    }

    if (this._addons == null) {
      if (this._pendingCallbacks == null) {
        
        this._pendingCallbacks = [];
        this._pendingCallbacks.push(getAddon);
        AddonDatabase.retrieveStoredData(function(aAddons) {
          let pendingCallbacks = self._pendingCallbacks;

          
          if (pendingCallbacks == null)
            return;

          
          
          self._pendingCallbacks = null;
          self._addons = aAddons;

          pendingCallbacks.forEach(function(aCallback) aCallback(aAddons));
        });

        return;
      }

      
      this._pendingCallbacks.push(getAddon);
      return;
    }

    
    getAddon(this._addons);
  },

  









  repopulateCache: function(aIds, aCallback) {
    this._repopulateCache(aIds, aCallback, false);
  },

  _repopulateCache: function(aIds, aCallback, aSendPerformance) {
    
    if (!this.cacheEnabled) {
      this._addons = null;
      this._pendingCallbacks = null;
      AddonDatabase.delete(aCallback);
      return;
    }

    let self = this;
    getAddonsToCache(aIds, function(aAddons) {
      
      if (aAddons.length == 0) {
        self._addons = null;
        self._pendingCallbacks = null;
        AddonDatabase.delete(aCallback);
        return;
      }

      self._beginGetAddons(aAddons, {
        searchSucceeded: function(aAddons) {
          self._addons = {};
          aAddons.forEach(function(aAddon) { self._addons[aAddon.id] = aAddon; });
          AddonDatabase.repopulate(aAddons, aCallback);
        },
        searchFailed: function() {
          WARN("Search failed when repopulating cache");
          if (aCallback)
            aCallback();
        }
      }, aSendPerformance);
    });
  },

  









  cacheAddons: function(aIds, aCallback) {
    if (!this.cacheEnabled) {
      if (aCallback)
        aCallback();
      return;
    }

    let self = this;
    getAddonsToCache(aIds, function(aAddons) {
      
      if (aAddons.length == 0) {
        if (aCallback)
          aCallback();
        return;
      }

      self.getAddonsByIDs(aAddons, {
        searchSucceeded: function(aAddons) {
          aAddons.forEach(function(aAddon) { self._addons[aAddon.id] = aAddon; });
          AddonDatabase.insertAddons(aAddons, aCallback);
        },
        searchFailed: function() {
          WARN("Search failed when adding add-ons to cache");
          if (aCallback)
            aCallback();
        }
      });
    });
  },

  



  get homepageURL() {
    let url = this._formatURLPref(PREF_GETADDONS_BROWSEADDONS, {});
    return (url != null) ? url : "about:blank";
  },

  



  get isSearching() {
    return this._searching;
  },

  



  getRecommendedURL: function() {
    let url = this._formatURLPref(PREF_GETADDONS_BROWSERECOMMENDED, {});
    return (url != null) ? url : "about:blank";
  },

  







  getSearchURL: function(aSearchTerms) {
    let url = this._formatURLPref(PREF_GETADDONS_BROWSESEARCHRESULTS, {
      TERMS : encodeURIComponent(aSearchTerms)
    });
    return (url != null) ? url : "about:blank";
  },

  



  cancelSearch: function() {
    this._searching = false;
    if (this._request) {
      this._request.abort();
      this._request = null;
    }
    this._callback = null;
  },

  








  getAddonsByIDs: function(aIDs, aCallback) {
    return this._beginGetAddons(aIDs, aCallback, false);
  },

  










  _beginGetAddons: function(aIDs, aCallback, aSendPerformance) {
    let ids = aIDs.slice(0);

    let params = {
      API_VERSION : API_VERSION,
      IDS : ids.map(encodeURIComponent).join(',')
    };

    let pref = PREF_GETADDONS_BYIDS;

    if (aSendPerformance) {
      let type = Services.prefs.getPrefType(PREF_GETADDONS_BYIDS_PERFORMANCE);
      if (type == Services.prefs.PREF_STRING) {
        pref = PREF_GETADDONS_BYIDS_PERFORMANCE;

        let startupInfo = Cc["@mozilla.org/toolkit/app-startup;1"].
                          getService(Ci.nsIAppStartup).
                          getStartupInfo();

        if (startupInfo.process) {
          if (startupInfo.main) {
            params.TIME_MAIN = startupInfo.main - startupInfo.process;
          }
          if (startupInfo.firstPaint) {
            params.TIME_FIRST_PAINT = startupInfo.firstPaint -
                                      startupInfo.process;
          }
          if (startupInfo.sessionRestored) {
            params.TIME_SESSION_RESTORED = startupInfo.sessionRestored -
                                           startupInfo.process;
          }
        }
      }
    }

    let url = this._formatURLPref(pref, params);

    let self = this;
    function handleResults(aElements, aTotalResults, aCompatData) {
      
      
      let results = [];
      for (let i = 0; i < aElements.length && results.length < self._maxResults; i++) {
        let result = self._parseAddon(aElements[i], null, aCompatData);
        if (result == null)
          continue;

        
        let idIndex = ids.indexOf(result.addon.id);
        if (idIndex == -1)
          continue;

        results.push(result);
        
        ids.splice(idIndex, 1);
      }

      
      
      for each (let addonCompat in aCompatData) {
        if (addonCompat.hosted)
          continue;

        let addon = new AddonSearchResult(addonCompat.id);
        
        addon.type = "extension";
        addon.compatibilityOverrides = addonCompat.compatRanges;
        let result = {
          addon: addon,
          xpiURL: null,
          xpiHash: null
        };
        results.push(result);
      }

      
      self._reportSuccess(results, -1);
    }

    this._beginSearch(url, ids.length, aCallback, handleResults);
  },

  












  backgroundUpdateCheck: function(aIDs, aCallback) {
    this._repopulateCache(aIDs, aCallback, true);
  },

  








  retrieveRecommendedAddons: function(aMaxResults, aCallback) {
    let url = this._formatURLPref(PREF_GETADDONS_GETRECOMMENDED, {
      API_VERSION : API_VERSION,

      
      MAX_RESULTS : 2 * aMaxResults
    });

    let self = this;
    function handleResults(aElements, aTotalResults) {
      self._getLocalAddonIds(function(aLocalAddonIds) {
        
        self._parseAddons(aElements, -1, aLocalAddonIds);
      });
    }

    this._beginSearch(url, aMaxResults, aCallback, handleResults);
  },

  










  searchAddons: function(aSearchTerms, aMaxResults, aCallback) {
    let checkCompatibility = true;
    try {
      checkCompatibility = Services.prefs.getBoolPref(PREF_CHECK_COMPATIBILITY);
    } catch(e) { }
    let strictCompatibility = STRICT_COMPATIBILITY_DEFAULT;
    try {
      strictCompatibility = Services.prefs.getBoolPref(PREF_EM_STRICT_COMPATIBILITY);
    } catch(e) { }

    let compatMode = "normal";
    if (!checkCompatibility)
      compatMode = "ignore";
    else if (strictCompatibility)
      compatMode = "strict";

    let substitutions = {
      API_VERSION : API_VERSION,
      TERMS : encodeURIComponent(aSearchTerms),
      
      MAX_RESULTS : 2 * aMaxResults,
      COMPATIBILITY_MODE : compatMode,
    };

    let url = this._formatURLPref(PREF_GETADDONS_GETSEARCHRESULTS, substitutions);

    let self = this;
    function handleResults(aElements, aTotalResults) {
      self._getLocalAddonIds(function(aLocalAddonIds) {
        self._parseAddons(aElements, aTotalResults, aLocalAddonIds);
      });
    }

    this._beginSearch(url, aMaxResults, aCallback, handleResults);
  },

  
  _reportSuccess: function(aResults, aTotalResults) {
    this._searching = false;
    this._request = null;
    
    let addons = [result.addon for each(result in aResults)];
    let callback = this._callback;
    this._callback = null;
    callback.searchSucceeded(addons, addons.length, aTotalResults);
  },

  
  _reportFailure: function() {
    this._searching = false;
    this._request = null;
    
    let callback = this._callback;
    this._callback = null;
    callback.searchFailed();
  },

  
  _getUniqueDescendant: function(aElement, aTagName) {
    let elementsList = aElement.getElementsByTagName(aTagName);
    return (elementsList.length == 1) ? elementsList[0] : null;
  },

  
  
  _getUniqueDirectDescendant: function(aElement, aTagName) {
    let elementsList = Array.filter(aElement.children,
                                    function(aChild) aChild.tagName == aTagName);
    return (elementsList.length == 1) ? elementsList[0] : null;
  },

  
  _getTextContent: function(aElement) {
    let textContent = aElement.textContent.trim();
    return (textContent.length > 0) ? textContent : null;
  },

  
  
  _getDescendantTextContent: function(aElement, aTagName) {
    let descendant = this._getUniqueDescendant(aElement, aTagName);
    return (descendant != null) ? this._getTextContent(descendant) : null;
  },

  
  
  
  _getDirectDescendantTextContent: function(aElement, aTagName) {
    let descendant = this._getUniqueDirectDescendant(aElement, aTagName);
    return (descendant != null) ? this._getTextContent(descendant) : null;
  },

  












  _parseAddon: function(aElement, aSkip, aCompatData) {
    let skipIDs = (aSkip && aSkip.ids) ? aSkip.ids : [];
    let skipSourceURIs = (aSkip && aSkip.sourceURIs) ? aSkip.sourceURIs : [];

    let guid = this._getDescendantTextContent(aElement, "guid");
    if (guid == null || skipIDs.indexOf(guid) != -1)
      return null;

    let addon = new AddonSearchResult(guid);
    let result = {
      addon: addon,
      xpiURL: null,
      xpiHash: null
    };

    if (aCompatData && guid in aCompatData)
      addon.compatibilityOverrides = aCompatData[guid].compatRanges;

    let self = this;
    for (let node = aElement.firstChild; node; node = node.nextSibling) {
      if (!(node instanceof Ci.nsIDOMElement))
        continue;

      let localName = node.localName;

      
      
      if (localName in STRING_KEY_MAP) {
        addon[STRING_KEY_MAP[localName]] = this._getTextContent(node) || addon[STRING_KEY_MAP[localName]];
        continue;
      }

      
      if (localName in HTML_KEY_MAP) {
        addon[HTML_KEY_MAP[localName]] = convertHTMLToPlainText(this._getTextContent(node));
        continue;
      }

      
      if (localName in INTEGER_KEY_MAP) {
        let value = parseInt(this._getTextContent(node));
        if (value >= 0)
          addon[INTEGER_KEY_MAP[localName]] = value;
        continue;
      }

      
      switch (localName) {
        case "type":
          
          let id = parseInt(node.getAttribute("id"));
          switch (id) {
            case 1:
              addon.type = "extension";
              break;
            case 2:
              addon.type = "theme";
              break;
            case 3:
              addon.type = "dictionary";
              break;
            default:
              WARN("Unknown type id when parsing addon: " + id);
          }
          break;
        case "authors":
          let authorNodes = node.getElementsByTagName("author");
          Array.forEach(authorNodes, function(aAuthorNode) {
            let name = self._getDescendantTextContent(aAuthorNode, "name");
            let link = self._getDescendantTextContent(aAuthorNode, "link");
            if (name == null || link == null)
              return;

            let author = new AddonManagerPrivate.AddonAuthor(name, link);
            if (addon.creator == null)
              addon.creator = author;
            else {
              if (addon.developers == null)
                addon.developers = [];

              addon.developers.push(author);
            }
          });
          break;
        case "previews":
          let previewNodes = node.getElementsByTagName("preview");
          Array.forEach(previewNodes, function(aPreviewNode) {
            let full = self._getUniqueDescendant(aPreviewNode, "full");
            if (full == null)
              return;

            let fullURL = self._getTextContent(full);
            let fullWidth = full.getAttribute("width");
            let fullHeight = full.getAttribute("height");

            let thumbnailURL, thumbnailWidth, thumbnailHeight;
            let thumbnail = self._getUniqueDescendant(aPreviewNode, "thumbnail");
            if (thumbnail) {
              thumbnailURL = self._getTextContent(thumbnail);
              thumbnailWidth = thumbnail.getAttribute("width");
              thumbnailHeight = thumbnail.getAttribute("height");
            }
            let caption = self._getDescendantTextContent(aPreviewNode, "caption");
            let screenshot = new AddonManagerPrivate.AddonScreenshot(fullURL, fullWidth, fullHeight,
                                                                     thumbnailURL, thumbnailWidth,
                                                                     thumbnailHeight, caption);

            if (addon.screenshots == null)
              addon.screenshots = [];

            if (aPreviewNode.getAttribute("primary") == 1)
              addon.screenshots.unshift(screenshot);
            else
              addon.screenshots.push(screenshot);
          });
          break;
        case "learnmore":
          addon.homepageURL = addon.homepageURL || this._getTextContent(node);
          break;
        case "contribution_data":
          let meetDevelopers = this._getDescendantTextContent(node, "meet_developers");
          let suggestedAmount = this._getDescendantTextContent(node, "suggested_amount");
          if (meetDevelopers != null) {
            addon.contributionURL = meetDevelopers;
            addon.contributionAmount = suggestedAmount;
          }
          break
        case "payment_data":
          let link = this._getDescendantTextContent(node, "link");
          let amountTag = this._getUniqueDescendant(node, "amount");
          let amount = parseFloat(amountTag.getAttribute("amount"));
          let displayAmount = this._getTextContent(amountTag);
          if (link != null && amount != null && displayAmount != null) {
            addon.purchaseURL = link;
            addon.purchaseAmount = amount;
            addon.purchaseDisplayAmount = displayAmount;
          }
          break
        case "rating":
          let averageRating = parseInt(this._getTextContent(node));
          if (averageRating >= 0)
            addon.averageRating = Math.min(5, averageRating);
          break;
        case "reviews":
          let url = this._getTextContent(node);
          let num = parseInt(node.getAttribute("num"));
          if (url != null && num >= 0) {
            addon.reviewURL = url;
            addon.reviewCount = num;
          }
          break;
        case "status":
          let repositoryStatus = parseInt(node.getAttribute("id"));
          if (!isNaN(repositoryStatus))
            addon.repositoryStatus = repositoryStatus;
          break;
        case "all_compatible_os":
          let nodes = node.getElementsByTagName("os");
          addon.isPlatformCompatible = Array.some(nodes, function(aNode) {
            let text = aNode.textContent.toLowerCase().trim();
            return text == "all" || text == Services.appinfo.OS.toLowerCase();
          });
          break;
        case "install":
          
          if (node.hasAttribute("os")) {
            let os = node.getAttribute("os").trim().toLowerCase();
            
            if (os != "all" && os != Services.appinfo.OS.toLowerCase())
              break;
          }

          let xpiURL = this._getTextContent(node);
          if (xpiURL == null)
            break;

          if (skipSourceURIs.indexOf(xpiURL) != -1)
            return null;

          result.xpiURL = xpiURL;
          addon.sourceURI = NetUtil.newURI(xpiURL);

          let size = parseInt(node.getAttribute("size"));
          addon.size = (size >= 0) ? size : null;

          let xpiHash = node.getAttribute("hash");
          if (xpiHash != null)
            xpiHash = xpiHash.trim();
          result.xpiHash = xpiHash ? xpiHash : null;
          break;
        case "last_updated":
          let epoch = parseInt(node.getAttribute("epoch"));
          if (!isNaN(epoch))
            addon.updateDate = new Date(1000 * epoch);
          break;
      }
    }

    return result;
  },

  _parseAddons: function(aElements, aTotalResults, aSkip) {
    let self = this;
    let results = [];

    let checkCompatibility = true;
    try {
      checkCompatibility = Services.prefs.getBoolPref(PREF_CHECK_COMPATIBILITY);
    } catch(e) { }

    let strictCompatibility = STRICT_COMPATIBILITY_DEFAULT;
    try {
      strictCompatibility = Services.prefs.getBoolPref(PREF_EM_STRICT_COMPATIBILITY);
    } catch (e) {}

    function isSameApplication(aAppNode) {
      return self._getTextContent(aAppNode) == Services.appinfo.ID;
    }

    for (let i = 0; i < aElements.length && results.length < this._maxResults; i++) {
      let element = aElements[i];

      let tags = this._getUniqueDescendant(element, "compatible_applications");
      if (tags == null)
        continue;

      let applications = tags.getElementsByTagName("appID");
      let compatible = Array.some(applications, function(aAppNode) {
        if (!isSameApplication(aAppNode))
          return false;

        let parent = aAppNode.parentNode;
        let minVersion = self._getDescendantTextContent(parent, "min_version");
        let maxVersion = self._getDescendantTextContent(parent, "max_version");
        if (minVersion == null || maxVersion == null)
          return false;

        let currentVersion = Services.appinfo.version;
        return (Services.vc.compare(minVersion, currentVersion) <= 0 &&
                ((!strictCompatibility) ||
                 Services.vc.compare(currentVersion, maxVersion) <= 0));
      });

      
      if (!compatible) {
        if (checkCompatibility)
          continue;

        if (!Array.some(applications, isSameApplication))
          continue;
      }

      
      
      
      let result = this._parseAddon(element, aSkip);
      if (result == null)
        continue;

      
      let requiredAttributes = ["id", "name", "version", "type", "creator"];
      if (requiredAttributes.some(function(aAttribute) !result.addon[aAttribute]))
        continue;

      
      if (!result.addon.isPlatformCompatible)
        continue;

      
      
      if (!result.xpiURL && !result.addon.purchaseURL)
        continue;

      result.addon.isCompatible = compatible;

      results.push(result);
      
      aSkip.ids.push(result.addon.id);
    }

    
    let pendingResults = results.length;
    if (pendingResults == 0) {
      this._reportSuccess(results, aTotalResults);
      return;
    }

    
    let self = this;
    results.forEach(function(aResult) {
      let addon = aResult.addon;
      let callback = function(aInstall) {
        addon.install = aInstall;
        pendingResults--;
        if (pendingResults == 0)
          self._reportSuccess(results, aTotalResults);
      }

      if (aResult.xpiURL) {
        AddonManager.getInstallForURL(aResult.xpiURL, callback,
                                      "application/x-xpinstall", aResult.xpiHash,
                                      addon.name, addon.iconURL, addon.version);
      }
      else {
        callback(null);
      }
    });
  },

  
  _parseAddonCompatElement: function(aResultObj, aElement) {
    let guid = this._getDescendantTextContent(aElement, "guid");
    if (!guid) {
        LOG("Compatibility override is missing guid.");
      return;
    }

    let compat = {id: guid};
    compat.hosted = aElement.getAttribute("hosted") != "false";

    function findMatchingAppRange(aNodes) {
      let toolkitAppRange = null;
      for (let i = 0; i < aNodes.length; i++) {
        let node = aNodes[i];
        let appID = this._getDescendantTextContent(node, "appID");
        if (appID != Services.appinfo.ID && appID != TOOLKIT_ID)
          continue;

        let minVersion = this._getDescendantTextContent(node, "min_version");
        let maxVersion = this._getDescendantTextContent(node, "max_version");
        if (minVersion == null || maxVersion == null)
          continue;

        let appRange = { appID: appID,
                         appMinVersion: minVersion,
                         appMaxVersion: maxVersion };

        
        if (appID == TOOLKIT_ID)
          toolkitAppRange = appRange;
        else
          return appRange;
      }
      return toolkitAppRange;
    }

    function parseRangeNode(aNode) {
      let type = aNode.getAttribute("type");
      
      if (type != "incompatible") {
        LOG("Compatibility override of unsupported type found.");
        return null;
      }

      let override = new AddonManagerPrivate.AddonCompatibilityOverride(type);

      override.minVersion = this._getDirectDescendantTextContent(aNode, "min_version");
      override.maxVersion = this._getDirectDescendantTextContent(aNode, "max_version");

      if (!override.minVersion) {
        LOG("Compatibility override is missing min_version.");
        return null;
      }
      if (!override.maxVersion) {
        LOG("Compatibility override is missing max_version.");
        return null;
      }

      let appRanges = aNode.querySelectorAll("compatible_applications > application");
      let appRange = findMatchingAppRange.bind(this)(appRanges);
      if (!appRange) {
        LOG("Compatibility override is missing a valid application range.");
        return null;
      }

      override.appID = appRange.appID;
      override.appMinVersion = appRange.appMinVersion;
      override.appMaxVersion = appRange.appMaxVersion;

      return override;
    }

    let rangeNodes = aElement.querySelectorAll("version_ranges > version_range");
    compat.compatRanges = Array.map(rangeNodes, parseRangeNode.bind(this))
                               .filter(function(aItem) !!aItem);
    if (compat.compatRanges.length == 0)
      return;

    aResultObj[compat.id] = compat;
  },

  
  _parseAddonCompatData: function(aElements) {
    let compatData = {};
    Array.forEach(aElements, this._parseAddonCompatElement.bind(this, compatData));
    return compatData;
  },

  
  _beginSearch: function(aURI, aMaxResults, aCallback, aHandleResults) {
    if (this._searching || aURI == null || aMaxResults <= 0) {
      aCallback.searchFailed();
      return;
    }

    this._searching = true;
    this._callback = aCallback;
    this._maxResults = aMaxResults;

    LOG("Requesting " + aURI);

    this._request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                    createInstance(Ci.nsIXMLHttpRequest);
    this._request.mozBackgroundRequest = true;
    this._request.open("GET", aURI, true);
    this._request.overrideMimeType("text/xml");

    let self = this;
    this._request.addEventListener("error", function(aEvent) {
      self._reportFailure();
    }, false);
    this._request.addEventListener("load", function(aEvent) {
      let request = aEvent.target;
      let responseXML = request.responseXML;

      if (!responseXML || responseXML.documentElement.namespaceURI == XMLURI_PARSE_ERROR ||
          (request.status != 200 && request.status != 0)) {
        self._reportFailure();
        return;
      }

      let documentElement = responseXML.documentElement;
      let elements = documentElement.getElementsByTagName("addon");
      let totalResults = elements.length;
      let parsedTotalResults = parseInt(documentElement.getAttribute("total_results"));
      
      if (parsedTotalResults >= totalResults)
        totalResults = parsedTotalResults;

      let compatElements = documentElement.getElementsByTagName("addon_compatibility");
      let compatData = self._parseAddonCompatData(compatElements);

      aHandleResults(elements, totalResults, compatData);
    }, false);
    this._request.send(null);
  },

  
  
  _getLocalAddonIds: function(aCallback) {
    let self = this;
    let localAddonIds = {ids: null, sourceURIs: null};

    AddonManager.getAllAddons(function(aAddons) {
      localAddonIds.ids = [a.id for each (a in aAddons)];
      if (localAddonIds.sourceURIs)
        aCallback(localAddonIds);
    });

    AddonManager.getAllInstalls(function(aInstalls) {
      localAddonIds.sourceURIs = [];
      aInstalls.forEach(function(aInstall) {
        if (aInstall.state != AddonManager.STATE_AVAILABLE)
          localAddonIds.sourceURIs.push(aInstall.sourceURI.spec);
      });

      if (localAddonIds.ids)
        aCallback(localAddonIds);
    });
  },

  
  _formatURLPref: function(aPreference, aSubstitutions) {
    let url = null;
    try {
      url = Services.prefs.getCharPref(aPreference);
    } catch(e) {
      WARN("_formatURLPref: Couldn't get pref: " + aPreference);
      return null;
    }

    url = url.replace(/%([A-Z_]+)%/g, function(aMatch, aKey) {
      return (aKey in aSubstitutions) ? aSubstitutions[aKey] : aMatch;
    });

    return Services.urlFormatter.formatURL(url);
  },

  
  
  findMatchingCompatOverride: function AR_findMatchingCompatOverride(aAddonVersion,
                                                                     aCompatOverrides,
                                                                     aAppVersion,
                                                                     aPlatformVersion) {
    for (let i = 0; i < aCompatOverrides.length; i++) {
      let override = aCompatOverrides[i];

      let appVersion = null;
      if (override.appID == TOOLKIT_ID)
        appVersion = aPlatformVersion || Services.appinfo.platformVersion;
      else
        appVersion = aAppVersion || Services.appinfo.version;

      if (Services.vc.compare(override.minVersion, aAddonVersion) <= 0 &&
          Services.vc.compare(aAddonVersion, override.maxVersion) <= 0 &&
          Services.vc.compare(override.appMinVersion, appVersion) <= 0 &&
          Services.vc.compare(appVersion, override.appMaxVersion) <= 0) {
        return override;
      }
    }
    return null;
  }

};
AddonRepository.initialize();

var AddonDatabase = {
  
  initialized: false,
  
  databaseOk: true,
  
  asyncStatementsCache: {},

  
  queries: {
    getAllAddons: "SELECT internal_id, id, type, name, version, " +
                  "creator, creatorURL, description, fullDescription, " +
                  "developerComments, eula, iconURL, homepageURL, supportURL, " +
                  "contributionURL, contributionAmount, averageRating, " +
                  "reviewCount, reviewURL, totalDownloads, weeklyDownloads, " +
                  "dailyUsers, sourceURI, repositoryStatus, size, updateDate " +
                  "FROM addon",

    getAllDevelopers: "SELECT addon_internal_id, name, url FROM developer " +
                      "ORDER BY addon_internal_id, num",

    getAllScreenshots: "SELECT addon_internal_id, url, width, height, " +
                       "thumbnailURL, thumbnailWidth, thumbnailHeight, caption " +
                       "FROM screenshot ORDER BY addon_internal_id, num",

    getAllCompatOverrides: "SELECT addon_internal_id, type, minVersion, " +
                           "maxVersion, appID, appMinVersion, appMaxVersion " +
                           "FROM compatibility_override " +
                           "ORDER BY addon_internal_id, num",

    insertAddon: "INSERT INTO addon VALUES (NULL, :id, :type, :name, :version, " +
                 ":creator, :creatorURL, :description, :fullDescription, " +
                 ":developerComments, :eula, :iconURL, :homepageURL, :supportURL, " +
                 ":contributionURL, :contributionAmount, :averageRating, " +
                 ":reviewCount, :reviewURL, :totalDownloads, :weeklyDownloads, " +
                 ":dailyUsers, :sourceURI, :repositoryStatus, :size, :updateDate)",

    insertDeveloper:  "INSERT INTO developer VALUES (:addon_internal_id, " +
                      ":num, :name, :url)",

    
    
    insertScreenshot: "INSERT INTO screenshot (addon_internal_id, " +
                      "num, url, width, height, thumbnailURL, " +
                      "thumbnailWidth, thumbnailHeight, caption) " +
                      "VALUES (:addon_internal_id, " +
                      ":num, :url, :width, :height, :thumbnailURL, " +
                      ":thumbnailWidth, :thumbnailHeight, :caption)",

    insertCompatibilityOverride: "INSERT INTO compatibility_override VALUES " +
                                 "(:addon_internal_id, :num, :type, " +
                                 ":minVersion, :maxVersion, :appID, " +
                                 ":appMinVersion, :appMaxVersion)",

    emptyAddon:       "DELETE FROM addon"
  },

  







  logSQLError: function AD_logSQLError(aError, aErrorString) {
    ERROR("SQL error " + aError + ": " + aErrorString);
  },

  





  asyncErrorLogger: function AD_asyncErrorLogger(aError) {
    ERROR("Async SQL error " + aError.result + ": " + aError.message);
  },

  






  openConnection: function AD_openConnection(aSecondAttempt) {
    this.initialized = true;
    delete this.connection;

    let dbfile = FileUtils.getFile(KEY_PROFILEDIR, [FILE_DATABASE], true);
    let dbMissing = !dbfile.exists();

    var tryAgain = (function() {
      LOG("Deleting database, and attempting openConnection again");
      this.initialized = false;
      if (this.connection.connectionReady)
        this.connection.close();
      if (dbfile.exists())
        dbfile.remove(false);
      return this.openConnection(true);
    }).bind(this);

    try {
      this.connection = Services.storage.openUnsharedDatabase(dbfile);
    } catch (e) {
      this.initialized = false;
      ERROR("Failed to open database", e);
      if (aSecondAttempt || dbMissing) {
        this.databaseOk = false;
        throw e;
      }
      return tryAgain();
    }

    this.connection.executeSimpleSQL("PRAGMA locking_mode = EXCLUSIVE");

    
    try {
      this.connection.beginTransaction();
      switch (this.connection.schemaVersion) {
        case 0:
          LOG("Recreating database schema");
          this._createSchema();
          break;
        case 1:
          LOG("Upgrading database schema");
          this.connection.executeSimpleSQL("ALTER TABLE screenshot ADD COLUMN width INTEGER");
          this.connection.executeSimpleSQL("ALTER TABLE screenshot ADD COLUMN height INTEGER");
          this.connection.executeSimpleSQL("ALTER TABLE screenshot ADD COLUMN thumbnailWidth INTEGER");
          this.connection.executeSimpleSQL("ALTER TABLE screenshot ADD COLUMN thumbnailHeight INTEGER");
        case 2:
          this.connection.createTable("compatibility_override",
                                      "addon_internal_id INTEGER, " +
                                      "num INTEGER, " +
                                      "type TEXT, " +
                                      "minVersion TEXT, " +
                                      "maxVersion TEXT, " +
                                      "appID TEXT, " +
                                      "appMinVersion TEXT, " +
                                      "appMaxVersion TEXT, " +
                                      "PRIMARY KEY (addon_internal_id, num)");
          this._createIndices();
          this._createTriggers();
          this.connection.schemaVersion = DB_SCHEMA;
        case 3:
          break;
        default:
          return tryAgain();
      }
      this.connection.commitTransaction();
    } catch (e) {
      ERROR("Failed to create database schema", e);
      this.logSQLError(this.connection.lastError, this.connection.lastErrorString);
      this.connection.rollbackTransaction();
      return tryAgain();
    }

    return this.connection;
  },

  


  get connection() {
    return this.openConnection();
  },

  






  shutdown: function AD_shutdown(aCallback) {
    this.databaseOk = true;
    if (!this.initialized) {
      if (aCallback)
        aCallback();
      return;
    }

    this.initialized = false;

    for each (let stmt in this.asyncStatementsCache)
      stmt.finalize();
    this.asyncStatementsCache = {};

    if (this.connection.transactionInProgress) {
      ERROR("Outstanding transaction, rolling back.");
      this.connection.rollbackTransaction();
    }

    let connection = this.connection;
    delete this.connection;

    
    
    this.__defineGetter__("connection", function() {
      return this.openConnection();
    });

    connection.asyncClose(aCallback);
  },

  






  delete: function AD_delete(aCallback) {
    this.shutdown(function() {
      let dbfile = FileUtils.getFile(KEY_PROFILEDIR, [FILE_DATABASE], true);
      if (dbfile.exists())
        dbfile.remove(false);

      if (aCallback)
        aCallback();
    });
  },

  








  getAsyncStatement: function AD_getAsyncStatement(aKey) {
    if (aKey in this.asyncStatementsCache)
      return this.asyncStatementsCache[aKey];

    let sql = this.queries[aKey];
    try {
      return this.asyncStatementsCache[aKey] = this.connection.createAsyncStatement(sql);
    } catch (e) {
      ERROR("Error creating statement " + aKey + " (" + sql + ")");
      throw e;
    }
  },

  






  retrieveStoredData: function AD_retrieveStoredData(aCallback) {
    let self = this;
    let addons = {};

    
    function getAllAddons() {
      self.getAsyncStatement("getAllAddons").executeAsync({
        handleResult: function(aResults) {
          let row = null;
          while (row = aResults.getNextRow()) {
            let internal_id = row.getResultByName("internal_id");
            addons[internal_id] = self._makeAddonFromAsyncRow(row);
          }
        },

        handleError: self.asyncErrorLogger,

        handleCompletion: function(aReason) {
          if (aReason != Ci.mozIStorageStatementCallback.REASON_FINISHED) {
            ERROR("Error retrieving add-ons from database. Returning empty results");
            aCallback({});
            return;
          }

          getAllDevelopers();
        }
      });
    }

    
    function getAllDevelopers() {
      self.getAsyncStatement("getAllDevelopers").executeAsync({
        handleResult: function(aResults) {
          let row = null;
          while (row = aResults.getNextRow()) {
            let addon_internal_id = row.getResultByName("addon_internal_id");
            if (!(addon_internal_id in addons)) {
              WARN("Found a developer not linked to an add-on in database");
              continue;
            }

            let addon = addons[addon_internal_id];
            if (!addon.developers)
              addon.developers = [];

            addon.developers.push(self._makeDeveloperFromAsyncRow(row));
          }
        },

        handleError: self.asyncErrorLogger,

        handleCompletion: function(aReason) {
          if (aReason != Ci.mozIStorageStatementCallback.REASON_FINISHED) {
            ERROR("Error retrieving developers from database. Returning empty results");
            aCallback({});
            return;
          }

          getAllScreenshots();
        }
      });
    }

    
    function getAllScreenshots() {
      self.getAsyncStatement("getAllScreenshots").executeAsync({
        handleResult: function(aResults) {
          let row = null;
          while (row = aResults.getNextRow()) {
            let addon_internal_id = row.getResultByName("addon_internal_id");
            if (!(addon_internal_id in addons)) {
              WARN("Found a screenshot not linked to an add-on in database");
              continue;
            }

            let addon = addons[addon_internal_id];
            if (!addon.screenshots)
              addon.screenshots = [];
            addon.screenshots.push(self._makeScreenshotFromAsyncRow(row));
          }
        },

        handleError: self.asyncErrorLogger,

        handleCompletion: function(aReason) {
          if (aReason != Ci.mozIStorageStatementCallback.REASON_FINISHED) {
            ERROR("Error retrieving screenshots from database. Returning empty results");
            aCallback({});
            return;
          }

          getAllCompatOverrides();
        }
      });
    }

    function getAllCompatOverrides() {
      self.getAsyncStatement("getAllCompatOverrides").executeAsync({
        handleResult: function(aResults) {
          let row = null;
          while (row = aResults.getNextRow()) {
            let addon_internal_id = row.getResultByName("addon_internal_id");
            if (!(addon_internal_id in addons)) {
              WARN("Found a compatibility override not linked to an add-on in database");
              continue;
            }

            let addon = addons[addon_internal_id];
            if (!addon.compatibilityOverrides)
              addon.compatibilityOverrides = [];
            addon.compatibilityOverrides.push(self._makeCompatOverrideFromAsyncRow(row));
          }
        },

        handleError: self.asyncErrorLogger,

        handleCompletion: function(aReason) {
          if (aReason != Ci.mozIStorageStatementCallback.REASON_FINISHED) {
            ERROR("Error retrieving compatibility overrides from database. Returning empty results");
            aCallback({});
            return;
          }

          let returnedAddons = {};
          for each (let addon in addons)
            returnedAddons[addon.id] = addon;
          aCallback(returnedAddons);
        }
      });
    }

    
    getAllAddons();
  },

  








  repopulate: function AD_repopulate(aAddons, aCallback) {
    let self = this;

    
    let stmts = [this.getAsyncStatement("emptyAddon")];

    this.connection.executeAsync(stmts, stmts.length, {
      handleResult: function() {},
      handleError: self.asyncErrorLogger,

      handleCompletion: function(aReason) {
        if (aReason != Ci.mozIStorageStatementCallback.REASON_FINISHED)
          ERROR("Error emptying database. Attempting to continue repopulating database");

        
        self.insertAddons(aAddons, aCallback);
      }
    });
  },

  







  insertAddons: function AD_insertAddons(aAddons, aCallback) {
    let self = this;
    let currentAddon = -1;

    
    function insertNextAddon() {
      if (++currentAddon == aAddons.length) {
        if (aCallback)
          aCallback();
        return;
      }

      self._insertAddon(aAddons[currentAddon], insertNextAddon);
    }

    insertNextAddon();
  },

  









  _insertAddon: function AD__insertAddon(aAddon, aCallback) {
    let self = this;
    let internal_id = null;
    this.connection.beginTransaction();

    
    
    function insertAdditionalData() {
      let stmts = [];

      
      function initializeArrayInsert(aStatementKey, aArray, aAddParams) {
        if (!aArray || aArray.length == 0)
          return;

        let stmt = self.getAsyncStatement(aStatementKey);
        let params = stmt.newBindingParamsArray();
        aArray.forEach(function(aElement, aIndex) {
          aAddParams(params, internal_id, aElement, aIndex);
        });

        stmt.bindParameters(params);
        stmts.push(stmt);
      }

      
      
      initializeArrayInsert("insertDeveloper", aAddon.developers,
                            self._addDeveloperParams);
      initializeArrayInsert("insertScreenshot", aAddon.screenshots,
                            self._addScreenshotParams);
      initializeArrayInsert("insertCompatibilityOverride",
                            aAddon.compatibilityOverrides,
                            self._addCompatOverrideParams);

      
      if (stmts.length == 0) {
        self.connection.commitTransaction();
        aCallback();
        return;
      }

      self.connection.executeAsync(stmts, stmts.length, {
        handleResult: function() {},
        handleError: self.asyncErrorLogger,
        handleCompletion: function(aReason) {
          if (aReason != Ci.mozIStorageStatementCallback.REASON_FINISHED) {
            ERROR("Error inserting additional addon metadata into database. Attempting to continue");
            self.connection.rollbackTransaction();
          }
          else {
            self.connection.commitTransaction();
          }

          aCallback();
        }
      });
    }

    
    this._makeAddonStatement(aAddon).executeAsync({
      handleResult: function() {},
      handleError: self.asyncErrorLogger,

      handleCompletion: function(aReason) {
        if (aReason != Ci.mozIStorageStatementCallback.REASON_FINISHED) {
          ERROR("Error inserting add-ons into database. Attempting to continue.");
          self.connection.rollbackTransaction();
          aCallback();
          return;
        }

        internal_id = self.connection.lastInsertRowID;
        insertAdditionalData();
      }
    });
  },

  






  _makeAddonStatement: function AD__makeAddonStatement(aAddon) {
    let stmt = this.getAsyncStatement("insertAddon");
    let params = stmt.params;

    PROP_SINGLE.forEach(function(aProperty) {
      switch (aProperty) {
        case "sourceURI":
          params.sourceURI = aAddon.sourceURI ? aAddon.sourceURI.spec : null;
          break;
        case "creator":
          params.creator =  aAddon.creator ? aAddon.creator.name : null;
          params.creatorURL =  aAddon.creator ? aAddon.creator.url : null;
          break;
        case "updateDate":
          params.updateDate = aAddon.updateDate ? aAddon.updateDate.getTime() : null;
          break;
        default:
          params[aProperty] = aAddon[aProperty];
      }
    });

    return stmt;
  },

  












  _addDeveloperParams: function AD__addDeveloperParams(aParams, aInternalID,
                                                       aDeveloper, aIndex) {
    let bp = aParams.newBindingParams();
    bp.bindByName("addon_internal_id", aInternalID);
    bp.bindByName("num", aIndex);
    bp.bindByName("name", aDeveloper.name);
    bp.bindByName("url", aDeveloper.url);
    aParams.addParams(bp);
  },

  











  _addScreenshotParams: function AD__addScreenshotParams(aParams, aInternalID,
                                                         aScreenshot, aIndex) {
    let bp = aParams.newBindingParams();
    bp.bindByName("addon_internal_id", aInternalID);
    bp.bindByName("num", aIndex);
    bp.bindByName("url", aScreenshot.url);
    bp.bindByName("width", aScreenshot.width);
    bp.bindByName("height", aScreenshot.height);
    bp.bindByName("thumbnailURL", aScreenshot.thumbnailURL);
    bp.bindByName("thumbnailWidth", aScreenshot.thumbnailWidth);
    bp.bindByName("thumbnailHeight", aScreenshot.thumbnailHeight);
    bp.bindByName("caption", aScreenshot.caption);
    aParams.addParams(bp);
  },

  












  _addCompatOverrideParams: function AD_addCompatOverrideParams(aParams,
                                                                aInternalID,
                                                                aOverride,
                                                                aIndex) {
    let bp = aParams.newBindingParams();
    bp.bindByName("addon_internal_id", aInternalID);
    bp.bindByName("num", aIndex);
    bp.bindByName("type", aOverride.type);
    bp.bindByName("minVersion", aOverride.minVersion);
    bp.bindByName("maxVersion", aOverride.maxVersion);
    bp.bindByName("appID", aOverride.appID);
    bp.bindByName("appMinVersion", aOverride.appMinVersion);
    bp.bindByName("appMaxVersion", aOverride.appMaxVersion);
    aParams.addParams(bp);
  },

  







  _makeAddonFromAsyncRow: function AD__makeAddonFromAsyncRow(aRow) {
    let addon = {};

    PROP_SINGLE.forEach(function(aProperty) {
      let value = aRow.getResultByName(aProperty);

      switch (aProperty) {
        case "sourceURI":
          addon.sourceURI = value ? NetUtil.newURI(value) : null;
          break;
        case "creator":
          let creatorURL = aRow.getResultByName("creatorURL");
          if (value || creatorURL)
            addon.creator = new AddonManagerPrivate.AddonAuthor(value, creatorURL);
          else
            addon.creator = null;
          break;
        case "updateDate":
          addon.updateDate = value ? new Date(value) : null;
          break;
        default:
          addon[aProperty] = value;
      }
    });

    return addon;
  },

  






  _makeDeveloperFromAsyncRow: function AD__makeDeveloperFromAsyncRow(aRow) {
    let name = aRow.getResultByName("name");
    let url = aRow.getResultByName("url")
    return new AddonManagerPrivate.AddonAuthor(name, url);
  },

  






  _makeScreenshotFromAsyncRow: function AD__makeScreenshotFromAsyncRow(aRow) {
    let url = aRow.getResultByName("url");
    let width = aRow.getResultByName("width");
    let height = aRow.getResultByName("height");
    let thumbnailURL = aRow.getResultByName("thumbnailURL");
    let thumbnailWidth = aRow.getResultByName("thumbnailWidth");
    let thumbnailHeight = aRow.getResultByName("thumbnailHeight");
    let caption = aRow.getResultByName("caption");
    return new AddonManagerPrivate.AddonScreenshot(url, width, height, thumbnailURL,
                                                   thumbnailWidth, thumbnailHeight, caption);
  },

  






  _makeCompatOverrideFromAsyncRow: function AD_makeCompatOverrideFromAsyncRow(aRow) {
    let type = aRow.getResultByName("type");
    let minVersion = aRow.getResultByName("minVersion");
    let maxVersion = aRow.getResultByName("maxVersion");
    let appID = aRow.getResultByName("appID");
    let appMinVersion = aRow.getResultByName("appMinVersion");
    let appMaxVersion = aRow.getResultByName("appMaxVersion");
    return new AddonManagerPrivate.AddonCompatibilityOverride(type,
                                                              minVersion,
                                                              maxVersion,
                                                              appID,
                                                              appMinVersion,
                                                              appMaxVersion);
  },

  


  _createSchema: function AD__createSchema() {
    LOG("Creating database schema");

      this.connection.createTable("addon",
                                  "internal_id INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                  "id TEXT UNIQUE, " +
                                  "type TEXT, " +
                                  "name TEXT, " +
                                  "version TEXT, " +
                                  "creator TEXT, " +
                                  "creatorURL TEXT, " +
                                  "description TEXT, " +
                                  "fullDescription TEXT, " +
                                  "developerComments TEXT, " +
                                  "eula TEXT, " +
                                  "iconURL TEXT, " +
                                  "homepageURL TEXT, " +
                                  "supportURL TEXT, " +
                                  "contributionURL TEXT, " +
                                  "contributionAmount TEXT, " +
                                  "averageRating INTEGER, " +
                                  "reviewCount INTEGER, " +
                                  "reviewURL TEXT, " +
                                  "totalDownloads INTEGER, " +
                                  "weeklyDownloads INTEGER, " +
                                  "dailyUsers INTEGER, " +
                                  "sourceURI TEXT, " +
                                  "repositoryStatus INTEGER, " +
                                  "size INTEGER, " +
                                  "updateDate INTEGER");

      this.connection.createTable("developer",
                                  "addon_internal_id INTEGER, " +
                                  "num INTEGER, " +
                                  "name TEXT, " +
                                  "url TEXT, " +
                                  "PRIMARY KEY (addon_internal_id, num)");

      this.connection.createTable("screenshot",
                                  "addon_internal_id INTEGER, " +
                                  "num INTEGER, " +
                                  "url TEXT, " +
                                  "width INTEGER, " +
                                  "height INTEGER, " +
                                  "thumbnailURL TEXT, " +
                                  "thumbnailWidth INTEGER, " +
                                  "thumbnailHeight INTEGER, " +
                                  "caption TEXT, " +
                                  "PRIMARY KEY (addon_internal_id, num)");

      this.connection.createTable("compatibility_override",
                                  "addon_internal_id INTEGER, " +
                                  "num INTEGER, " +
                                  "type TEXT, " +
                                  "minVersion TEXT, " +
                                  "maxVersion TEXT, " +
                                  "appID TEXT, " +
                                  "appMinVersion TEXT, " +
                                  "appMaxVersion TEXT, " +
                                  "PRIMARY KEY (addon_internal_id, num)");

      this._createIndices();
      this._createTriggers();

      this.connection.schemaVersion = DB_SCHEMA;
  },

  


  _createTriggers: function AD__createTriggers() {
    this.connection.executeSimpleSQL("DROP TRIGGER IF EXISTS delete_addon");
    this.connection.executeSimpleSQL("CREATE TRIGGER delete_addon AFTER DELETE " +
      "ON addon BEGIN " +
      "DELETE FROM developer WHERE addon_internal_id=old.internal_id; " +
      "DELETE FROM screenshot WHERE addon_internal_id=old.internal_id; " +
      "DELETE FROM compatibility_override WHERE addon_internal_id=old.internal_id; " +
      "END");
  },

  


  _createIndices: function AD__createIndices() {
      this.connection.executeSimpleSQL("CREATE INDEX IF NOT EXISTS developer_idx " +
                                       "ON developer (addon_internal_id)");
      this.connection.executeSimpleSQL("CREATE INDEX IF NOT EXISTS screenshot_idx " +
                                       "ON screenshot (addon_internal_id)");
      this.connection.executeSimpleSQL("CREATE INDEX IF NOT EXISTS compatibility_override_idx " +
                                       "ON compatibility_override (addon_internal_id)");
  }
};
