






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Components.utils.import("resource://gre/modules/NetUtil.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/AddonManager.jsm");

var EXPORTED_SYMBOLS = [ "AddonRepository" ];

const PREF_GETADDONS_BROWSEADDONS        = "extensions.getAddons.browseAddons";
const PREF_GETADDONS_BYIDS               = "extensions.getAddons.get.url";
const PREF_GETADDONS_BROWSERECOMMENDED   = "extensions.getAddons.recommended.browseURL";
const PREF_GETADDONS_GETRECOMMENDED      = "extensions.getAddons.recommended.url";
const PREF_GETADDONS_BROWSESEARCHRESULTS = "extensions.getAddons.search.browseURL";
const PREF_GETADDONS_GETSEARCHRESULTS    = "extensions.getAddons.search.url";

const XMLURI_PARSE_ERROR  = "http://www.mozilla.org/newlayout/xml/parsererror.xml";

const API_VERSION = "1.5";



const STRING_KEY_MAP = {
  name:               "name",
  version:            "version",
  summary:            "description",
  description:        "fullDescription",
  developer_comments: "developerComments",
  eula:               "eula",
  icon:               "iconURL",
  homepage:           "homepageURL",
  support:            "supportURL"
};



const INTEGER_KEY_MAP = {
  total_downloads:  "totalDownloads",
  weekly_downloads: "weeklyDownloads",
  daily_users:      "dailyUsers"
};


function AddonSearchResult(aId) {
  this.id = aId;
  this.screenshots = [];
  this.developers = [];
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

function AddonAuthor(aName, aURL) {
  this.name = aName;
  this.url = aURL;
}

AddonAuthor.prototype = {
  


  name: null,

  


  url: null,

  


  toString: function() {
    return this.name || "";
  }
}

function AddonScreenshot(aURL, aThumbnailURL, aCaption) {
  this.url = aURL;
  this.thumbnailURL = aThumbnailURL;
  this.caption = aCaption;
}

AddonScreenshot.prototype = {
  


  url: null,

  


  thumbnailURL: null,

  


  caption: null,

  


  toString: function() {
    return this.url || "";
  }
}












var AddonRepository = {
  
  _searching: false,

  
  _request: null,

  
















  _callback: null,

  
  _maxResults: null,

  



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
    let url = this._formatURLPref(PREF_GETADDONS_BYIDS, {
      API_VERSION : API_VERSION,
      IDS : aIDs.map(encodeURIComponent).join(',')
    });

    let self = this;
    function handleResults(aElements, aTotalResults) {
      
      
      let results = [];
      for (let i = 0; i < aElements.length && results.length < self._maxResults; i++) {
        let result = self._parseAddon(aElements[i]);
        if (result == null)
          continue;

        
        let idIndex = aIDs.indexOf(result.addon.id);
        if (idIndex == -1)
          continue;

        results.push(result);
        
        aIDs.splice(idIndex, 1);
      }

      
      self._reportSuccess(results, -1);
    }

    this._beginSearch(url, aIDs.length, aCallback, handleResults);
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
    let url = this._formatURLPref(PREF_GETADDONS_GETSEARCHRESULTS, {
      API_VERSION : API_VERSION,
      TERMS : encodeURIComponent(aSearchTerms),

      
      MAX_RESULTS : 2 * aMaxResults
    });

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

  
  _getTextContent: function(aElement) {
    let textContent = aElement.textContent.trim();
    return (textContent.length > 0) ? textContent : null;
  },

  
  
  _getDescendantTextContent: function(aElement, aTagName) {
    let descendant = this._getUniqueDescendant(aElement, aTagName);
    return (descendant != null) ? this._getTextContent(descendant) : null;
  },

  









  _parseAddon: function(aElement, aSkip) {
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

    let self = this;
    for (let node = aElement.firstChild; node; node = node.nextSibling) {
      if (!(node instanceof Ci.nsIDOMElement))
        continue;

      let localName = node.localName;

      
      if (localName in STRING_KEY_MAP) {
        addon[STRING_KEY_MAP[localName]] = this._getTextContent(node);
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
            default:
              Cu.reportError("Unknown type id when parsing addon: " + id);
          }
          break;
        case "authors":
          let authorNodes = node.getElementsByTagName("author");
          Array.forEach(authorNodes, function(aAuthorNode) {
            let name = self._getDescendantTextContent(aAuthorNode, "name");
            let link = self._getDescendantTextContent(aAuthorNode, "link");
            if (name == null || link == null)
              return;

            let author = new AddonAuthor(name, link);
            if (addon.creator == null)
              addon.creator = author;
            else
              addon.developers.push(author);
          });
          break;
        case "previews":
          let previewNodes = node.getElementsByTagName("preview");
          Array.forEach(previewNodes, function(aPreviewNode) {
            let full = self._getDescendantTextContent(aPreviewNode, "full");
            if (full == null)
              return;

            let thumbnail = self._getDescendantTextContent(aPreviewNode, "thumbnail");
            let caption = self._getDescendantTextContent(aPreviewNode, "caption");
            let screenshot = new AddonScreenshot(full, thumbnail, caption);

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
          if (meetDevelopers != null && suggestedAmount != null) {
            addon.contributionURL = meetDevelopers;
            addon.contributionAmount = suggestedAmount;
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
    for (let i = 0; i < aElements.length && results.length < this._maxResults; i++) {
      let element = aElements[i];

      
      let status = this._getUniqueDescendant(element, "status");
      
      if (status == null || status.getAttribute("id") != 4)
        continue;

      
      let tags = this._getUniqueDescendant(element, "compatible_applications");
      if (tags == null)
        continue;

      let applications = tags.getElementsByTagName("appID");
      let compatible = Array.some(applications, function(aAppNode) {
        if (self._getTextContent(aAppNode) != Services.appinfo.ID)
          return false;

        let parent = aAppNode.parentNode;
        let minVersion = self._getDescendantTextContent(parent, "min_version");
        let maxVersion = self._getDescendantTextContent(parent, "max_version");
        if (minVersion == null || maxVersion == null)
          return false;

        let currentVersion = Services.appinfo.version;
        return (Services.vc.compare(minVersion, currentVersion) <= 0 &&
                Services.vc.compare(currentVersion, maxVersion) <= 0);
      });

      if (!compatible)
        continue;

      
      let result = this._parseAddon(element, aSkip);
      if (result == null)
        continue;

      
      let requiredAttributes = ["id", "name", "version", "type", "creator"];
      if (requiredAttributes.some(function(aAttribute) !result.addon[aAttribute]))
        continue;

      
      if (!result.xpiURL)
        continue;

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

      AddonManager.getInstallForURL(aResult.xpiURL, callback,
                                    "application/x-xpinstall", aResult.xpiHash,
                                    addon.name, addon.iconURL, addon.version);
    });
  },

  
  _beginSearch: function(aURI, aMaxResults, aCallback, aHandleResults) {
    if (this._searching || aURI == null || aMaxResults <= 0) {
      aCallback.searchFailed();
      return;
    }

    this._searching = true;
    this._callback = aCallback;
    this._maxResults = aMaxResults;

    this._request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                    createInstance(Ci.nsIXMLHttpRequest);
    this._request.open("GET", aURI, true);
    this._request.overrideMimeType("text/xml");

    let self = this;
    this._request.onerror = function(aEvent) { self._reportFailure(); };
    this._request.onload = function(aEvent) {
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

      aHandleResults(elements, totalResults);
    };
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
      Cu.reportError("_formatURLPref: Couldn't get pref: " + aPreference);
      return null;
    }

    url = url.replace(/%([A-Z_]+)%/g, function(aMatch, aKey) {
      return (aKey in aSubstitutions) ? aSubstitutions[aKey] : aMatch;
    });

    return Services.urlFormatter.formatURL(url);
  }
}

