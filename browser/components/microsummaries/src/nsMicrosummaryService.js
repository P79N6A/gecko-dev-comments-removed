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
# The Original Code is Microsummarizer.
#
# The Initial Developer of the Original Code is Mozilla.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#  Myk Melez <myk@mozilla.org> (Original Author)
#  Simon BÃ¼nzli <zeniko@gmail.com>
#  Asaf Romano <mano@mozilla.com>
#  Dan Mills <thunder@mozilla.com>
#  Ryan Flint <rflint@dslr.net>
#  Dietrich Ayala <dietrich@mozilla.com>
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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const PERMS_FILE    = 0644;
const MODE_WRONLY   = 0x02;
const MODE_CREATE   = 0x08;
const MODE_TRUNCATE = 0x20;

const NS_ERROR_MODULE_DOM = 2152923136;
const NS_ERROR_DOM_BAD_URI = NS_ERROR_MODULE_DOM + 1012;


const CHECK_INTERVAL = 15 * 1000; 

const GENERATOR_INTERVAL = 7 * 86400; 

const MICSUM_NS = "http://www.mozilla.org/microsummaries/0.1";
const XSLT_NS = "http://www.w3.org/1999/XSL/Transform";

const ANNO_MICSUM_GEN_URI    = "microsummary/generatorURI";
const ANNO_MICSUM_EXPIRATION = "microsummary/expiration";
const ANNO_STATIC_TITLE      = "bookmarks/staticTitle";
const ANNO_CONTENT_TYPE      = "bookmarks/contentType";

const MAX_SUMMARY_LENGTH = 4096;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

__defineGetter__("NetUtil", function() {
  delete this.NetUtil;
  Cu.import("resource://gre/modules/NetUtil.jsm");
  return NetUtil;
});

XPCOMUtils.defineLazyServiceGetter(this, "gObsSvc",
                                   "@mozilla.org/observer-service;1",
                                   "nsIObserverService");

function MicrosummaryService() {
  gObsSvc.addObserver(this, "xpcom-shutdown", true);
  this._ans.addObserver(this, false);

  Cc["@mozilla.org/preferences-service;1"].
    getService(Ci.nsIPrefService).
    getBranch("browser.microsummary.").
    QueryInterface(Ci.nsIPrefBranch2).
    addObserver("", this, true);

  this._initTimers();
  this._cacheLocalGenerators();
}

MicrosummaryService.prototype = {
  
  get _bms() {
    var svc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
    this.__defineGetter__("_bms", function() svc);
    return this._bms;
  },

  
  get _ans() {
    var svc = Cc["@mozilla.org/browser/annotation-service;1"].
              getService(Ci.nsIAnnotationService);
    this.__defineGetter__("_ans", function() svc);
    return this._ans;
  },

  
  __dirs: null,
  get _dirs() {
    if (!this.__dirs)
      this.__dirs = Cc["@mozilla.org/file/directory_service;1"].
                    getService(Ci.nsIProperties);
    return this.__dirs;
  },

  
  get _updateInterval() {
    var updateInterval =
      getPref("browser.microsummary.updateInterval", 30);
    
    return Math.max(updateInterval, 1) * 60 * 1000;
  },

  
  
  _localGenerators: {},

  
  _timer: null,

  
  classDescription: "Microsummary Service",
  contractID: "@mozilla.org/microsummary/service;1",
  classID: Components.ID("{460a9792-b154-4f26-a922-0f653e2c8f91}"),
  _xpcom_categories: [{ category: "update-timer",
                        value: "@mozilla.org/microsummary/service;1," +
                               "getService,microsummary-generator-update-timer," +
                               "browser.microsummary.generatorUpdateInterval," +
                               GENERATOR_INTERVAL }],
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMicrosummaryService, 
                                         Ci.nsITimerCallback,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsIAnnotationObserver,
                                         Ci.nsIObserver]),

  
  observe: function MSS_observe(subject, topic, data) {
    switch (topic) {
      case "xpcom-shutdown":
        this._destroy();
        break;
      case "nsPref:changed":
        if (data == "enabled")
          this._initTimers();
        break;
    }
  },

  
  notify: function MSS_notify(timer) {
    this._updateGenerators();
  },

  _initTimers: function MSS__initTimers() {
    if (this._timer)
      this._timer.cancel();

    if (!getPref("browser.microsummary.enabled", true))
      return;

    
    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    var callback = {
      _svc: this,
      notify: function(timer) { this._svc._updateMicrosummaries() }
    };
    this._timer.initWithCallback(callback,
                                 CHECK_INTERVAL,
                                 this._timer.TYPE_REPEATING_SLACK);
  },
  
  _destroy: function MSS__destroy() {
    gObsSvc.removeObserver(this, "xpcom-shutdown", true);
    this._ans.removeObserver(this);
    this._timer.cancel();
    this._timer = null;
  },

  _updateMicrosummaries: function MSS__updateMicrosummaries() {
    var bookmarks = this._bookmarks;

    var now = Date.now();
    var updateInterval = this._updateInterval;
    for ( var i = 0; i < bookmarks.length; i++ ) {
      var bookmarkID = bookmarks[i];

      
      if (this._ans.itemHasAnnotation(bookmarkID, ANNO_MICSUM_EXPIRATION) &&
          this._ans.getItemAnnotation(bookmarkID, ANNO_MICSUM_EXPIRATION) > now)
        continue;

      
      
      this._setAnnotation(bookmarkID, ANNO_MICSUM_EXPIRATION, now + updateInterval);

      
      
      try {
        this.refreshMicrosummary(bookmarkID);
      }
      catch(ex) {
        Cu.reportError(ex);
      }
    }
  },

  _updateGenerators: function MSS__updateGenerators() {
    var generators = this._localGenerators;
    var update = getPref("browser.microsummary.updateGenerators", true);
    if (!generators || !update)
      return;

    for (let uri in generators)
      generators[uri].update();
  },

  _updateMicrosummary: function MSS__updateMicrosummary(bookmarkID, microsummary) {
    var title = this._bms.getItemTitle(bookmarkID);

    
    if (!this._ans.itemHasAnnotation(bookmarkID, ANNO_STATIC_TITLE))
      this._setAnnotation(bookmarkID, ANNO_STATIC_TITLE, title);

    
    var bookmarkIdentity = bookmarkID;

    
    if (!title || title != microsummary.content) {
      this._bms.setItemTitle(bookmarkID, microsummary.content);
      var subject = new LiveTitleNotificationSubject(bookmarkID, microsummary);
      LOG("updated live title for " + bookmarkIdentity +
          " from '" + (title == null ? "<no live title>" : title) +
          "' to '" + microsummary.content + "'");
      gObsSvc.notifyObservers(subject, "microsummary-livetitle-updated", title);
    }
    else {
      LOG("didn't update live title for " + bookmarkIdentity + "; it hasn't changed");
    }

    
    
    
    
    this._setAnnotation(bookmarkID, ANNO_MICSUM_EXPIRATION,
                  Date.now() + (microsummary.updateInterval || this._updateInterval));
  },

  



  _cacheLocalGenerators: function MSS__cacheLocalGenerators() {
    
    var appDir = this._dirs.get("MicsumGens", Ci.nsIFile);
    if (appDir.exists())
      this._cacheLocalGeneratorDir(appDir);

    
    var profileDir = this._dirs.get("UsrMicsumGens", Ci.nsIFile);
    if (profileDir.exists())
      this._cacheLocalGeneratorDir(profileDir);
  },

  






  _cacheLocalGeneratorDir: function MSS__cacheLocalGeneratorDir(dir) {
    var files = dir.directoryEntries.QueryInterface(Ci.nsIDirectoryEnumerator);
    var file = files.nextFile;

    while (file) {
      
      
      if (file.isDirectory())
        this._cacheLocalGeneratorDir(file);
      else
        this._cacheLocalGeneratorFile(file);

      file = files.nextFile;
    }
    files.close();
  },

  






  _cacheLocalGeneratorFile: function MSS__cacheLocalGeneratorFile(file) {
    var uri = NetUtil.ioService.newFileURI(file);

    var t = this;
    var callback =
      function MSS_cacheLocalGeneratorCallback(resource) {
        try     { t._handleLocalGenerator(resource) }
        finally { resource.destroy() }
      };

    var resource = new MicrosummaryResource(uri);
    resource.load(callback);
  },

  _handleLocalGenerator: function MSS__handleLocalGenerator(resource) {
    if (!resource.isXML)
      throw(resource.uri.spec + " microsummary generator loaded, but not XML");

    var generator = new MicrosummaryGenerator(null, resource.uri);
    generator.initFromXML(resource.content);

    
    
    
    
    this._localGenerators[generator.uri.spec.split().join()] = generator;

    LOG("loaded local microsummary generator\n" +
        "  file: " + generator.localURI.spec + "\n" +
        "    ID: " + generator.uri.spec);
  },

  

  





  getGenerator: function MSS_getGenerator(generatorURI) {
    return this._localGenerators[generatorURI.spec] ||
      new MicrosummaryGenerator(generatorURI);
  },

  







  addGenerator: function MSS_addGenerator(generatorURI) {
    var t = this;
    var callback =
      function MSS_addGeneratorCallback(resource) {
        try     { t._handleNewGenerator(resource) }
        finally { resource.destroy() }
      };

    var resource = new MicrosummaryResource(generatorURI);
    resource.load(callback);
  },

  _handleNewGenerator: function MSS__handleNewGenerator(resource) {
    if (!resource.isXML)
      throw(resource.uri.spec + " microsummary generator loaded, but not XML");

    

    var rootNode = resource.content.documentElement;

    
    
    
    rootNode.setAttribute("uri", "urn:source:" + resource.uri.spec);

    this.installGenerator(resource.content);
  },
 
  








  installGenerator: function MSS_installGenerator(xmlDefinition) {
    var rootNode = xmlDefinition.getElementsByTagNameNS(MICSUM_NS, "generator")[0];
 
    var generatorID = rootNode.getAttribute("uri");
 
    
    var generator = this._localGenerators[generatorID];

    var topic;
    if (generator)
      topic = "microsummary-generator-updated";
    else {
      
      topic = "microsummary-generator-installed";
      var generatorName = rootNode.getAttribute("name");
      var fileName = sanitizeName(generatorName) + ".xml";
      var file = this._dirs.get("UsrMicsumGens", Ci.nsIFile);
      file.append(fileName);
      file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, PERMS_FILE);
      generator = new MicrosummaryGenerator(null,
                                            NetUtil.ioService.newFileURI(file));
      this._localGenerators[generatorID] = generator;
    }
 
    
    
    generator.initFromXML(xmlDefinition);
    generator.saveXMLToFile(xmlDefinition);

    LOG("installed generator " + generatorID);

    gObsSvc.notifyObservers(generator, topic, null);

    return generator;
  },

  





















  getMicrosummaries: function MSS_getMicrosummaries(pageURI, bookmarkID) {
    var microsummaries = new MicrosummarySet();

    if (!getPref("browser.microsummary.enabled", true))
      return microsummaries;

    
    for (var genURISpec in this._localGenerators) {
      var generator = this._localGenerators[genURISpec];

      if (generator.appliesToURI(pageURI)) {
        var microsummary = new Microsummary(pageURI, generator);

        
        
        if (bookmarkID != -1 && this.isMicrosummary(bookmarkID, microsummary))
          microsummary._content = this._bms.getItemTitle(bookmarkID);

        microsummaries.AppendElement(microsummary);
      }
    }

    
    
    if (bookmarkID != -1 && this.hasMicrosummary(bookmarkID)) {
      var currentMicrosummary = this.getMicrosummary(bookmarkID);
      if (!microsummaries.hasItemForMicrosummary(currentMicrosummary))
        microsummaries.AppendElement(currentMicrosummary);
    }

    
    
    var resource = getLoadedMicrosummaryResource(pageURI);
    if (resource) {
      try     { microsummaries.extractFromPage(resource) }
      finally { resource.destroy() }
    }
    else {
      
      
      var callback = function MSS_extractFromPageCallback(resource) {
        try     { microsummaries.extractFromPage(resource) }
        finally { resource.destroy() }
      };

      try {
        resource = new MicrosummaryResource(pageURI);
        resource.load(callback);
      }
      catch(e) {
        
        
        
        if (resource)
          resource.destroy();
        LOG("error downloading page to extract its microsummaries: " + e);
      }
    }

    return microsummaries;
  },

  










  _changeField: function MSS__changeField(fieldName, oldValue, newValue) {
    var bookmarks = this._bookmarks;

    for ( var i = 0; i < bookmarks.length; i++ ) {
      var bookmarkID = bookmarks[i];

      if (this._ans.itemHasAnnotation(bookmarkID, fieldName) &&
          this._ans.getItemAnnotation(bookmarkID, fieldName) == oldValue)
        this._setAnnotation(bookmarkID, fieldName, newValue);
    }
  },

  









  get _bookmarks() {
    var bookmarks = this._ans.getItemsWithAnnotation(ANNO_MICSUM_GEN_URI);
    this.__defineGetter__("_bookmarks", function() bookmarks);
    return this._bookmarks;
  },

  _setAnnotation: function MSS__setAnnotation(aBookmarkId, aFieldName, aFieldValue) {
    this._ans.setItemAnnotation(aBookmarkId,
                                aFieldName,
                                aFieldValue,
                                0,
                                this._ans.EXPIRE_NEVER);
  },

  









  getBookmarks: function MSS_getBookmarks() {
    return new ArrayEnumerator(this._bookmarks);
  },

  









  getMicrosummary: function MSS_getMicrosummary(bookmarkID) {
    if (!this.hasMicrosummary(bookmarkID))
      return null;

    var pageURI = this._bms.getBookmarkURI(bookmarkID);
    var generatorURI = NetUtil.newURI(this._ans.getItemAnnotation(bookmarkID,
                                                                  ANNO_MICSUM_GEN_URI));
    var generator = this.getGenerator(generatorURI);

    return new Microsummary(pageURI, generator);
  },

  











  createMicrosummary: function MSS_createMicrosummary(pageURI, generatorURI) {
    var generator = this.getGenerator(generatorURI);
    return new Microsummary(pageURI, generator);
  },

  









  setMicrosummary: function MSS_setMicrosummary(bookmarkID, microsummary) {
    this._setAnnotation(bookmarkID, ANNO_MICSUM_GEN_URI, microsummary.generator.uri.spec);

    if (microsummary.content)
      this._updateMicrosummary(bookmarkID, microsummary);
    else
      this.refreshMicrosummary(bookmarkID);
  },

  






  removeMicrosummary: function MSS_removeMicrosummary(bookmarkID) {
    
    if (this._ans.itemHasAnnotation(bookmarkID, ANNO_STATIC_TITLE))
      this._bms.setItemTitle(bookmarkID, this._ans.getItemAnnotation(bookmarkID, ANNO_STATIC_TITLE));

    var fields = [ANNO_MICSUM_GEN_URI,
                  ANNO_MICSUM_EXPIRATION,
                  ANNO_STATIC_TITLE,
                  ANNO_CONTENT_TYPE];

    for (let i = 0; i < fields.length; i++) {
      var field = fields[i];
      if (this._ans.itemHasAnnotation(bookmarkID, field))
        this._ans.removeItemAnnotation(bookmarkID, field);
    }
  },

  









  hasMicrosummary: function MSS_hasMicrosummary(aBookmarkId) {
    return (this._bookmarks.indexOf(aBookmarkId) != -1);
  },

  













  isMicrosummary: function MSS_isMicrosummary(aBookmarkID, aMicrosummary) {
    if (!aMicrosummary || !aBookmarkID)
      throw Cr.NS_ERROR_INVALID_ARG;

    if (this.hasMicrosummary(aBookmarkID)) {
      var currentMicrosummarry = this.getMicrosummary(aBookmarkID);
      if (aMicrosummary.equals(currentMicrosummarry))
        return true;
    }
    return false
  },

  

















  refreshMicrosummary: function MSS_refreshMicrosummary(bookmarkID) {
    if (!this.hasMicrosummary(bookmarkID))
      throw "bookmark " + bookmarkID + " does not have a microsummary";

    var pageURI = this._bms.getBookmarkURI(bookmarkID);
    if (!pageURI)
      throw("can't get URL for bookmark with ID " + bookmarkID);
    var generatorURI = NetUtil.newURI(this._ans.getItemAnnotation(bookmarkID,
                                                                  ANNO_MICSUM_GEN_URI));

    var generator = this._localGenerators[generatorURI.spec] ||
                    new MicrosummaryGenerator(generatorURI);

    var microsummary = new Microsummary(pageURI, generator);

    
    
    var observer = {
      _svc: this,
      _bookmarkID: bookmarkID,
      onContentLoaded: function MSS_observer_onContentLoaded(microsummary) {
        try {
          this._svc._updateMicrosummary(this._bookmarkID, microsummary);
        }
        catch (ex) {
          Cu.reportError("refreshMicrosummary() observer: " + ex);
        }
        finally {
          this._svc = null;
          this._bookmarkID = null;
          microsummary.removeObserver(this);
        }
      },

      onError: function MSS_observer_onError(microsummary) {
        if (microsummary.needsRemoval)
          this._svc.removeMicrosummary(this._bookmarkID);
      }
    };

    
    
    microsummary.addObserver(observer);
    microsummary.update();
    
    return microsummary;
  },

  
  onItemAnnotationSet: function(aItemId, aAnnotationName) {
    if (aAnnotationName == ANNO_MICSUM_GEN_URI &&
        this._bookmarks.indexOf(aItemId) == -1)
      this._bookmarks.push(aItemId);
  },
  onItemAnnotationRemoved: function(aItemId, aAnnotationName) {
    var index = this._bookmarks.indexOf(aItemId);
    var isMicsumAnno = aAnnotationName == ANNO_MICSUM_GEN_URI ||
                       !aAnnotationName.length; 
    if (index > -1 && isMicsumAnno)
      this._bookmarks.splice(index, 1);
  },
  onPageAnnotationSet: function(aUri, aAnnotationName) {},
  onPageAnnotationRemoved: function(aUri, aAnnotationName) {},
};





function LiveTitleNotificationSubject(bookmarkID, microsummary) {
  this.bookmarkID = bookmarkID;
  this.microsummary = microsummary;
}

LiveTitleNotificationSubject.prototype = {
  bookmarkID: null,
  microsummary: null,

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsILiveTitleNotificationSubject]),
};





function Microsummary(aPageURI, aGenerator) {
  this._observers = [];
  this._pageURI = aPageURI || null;
  this._generator = aGenerator || null;
  this._content = null;
  this._pageContent = null;
  this._updateInterval = null;
  this._needsRemoval = false;
}

Microsummary.prototype = {
  
  __mss: null,
  get _mss() {
    if (!this.__mss)
      this.__mss = Cc["@mozilla.org/microsummary/service;1"].
                   getService(Ci.nsIMicrosummaryService);
    return this.__mss;
  },

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMicrosummary]),

  
  get content() {
    
    if (!this._content &&
        this.generator.loaded &&
        (this.pageContent || !this.generator.needsPageContent)) {
      this._content = this.generator.generateMicrosummary(this.pageContent);
      this._updateInterval = this.generator.calculateUpdateInterval(this.pageContent);
    }

    
    
    
    
    
    return this._content;
  },

  get generator()            { return this._generator },
  set generator(newValue)    { return this._generator = newValue },

  get pageURI() { return this._pageURI },

  equals: function(aOther) {
    if (this._generator &&
        this._pageURI.equals(aOther.pageURI) &&
        this._generator.equals(aOther.generator))
      return true;

    return false;
  },

  get pageContent() {
    if (!this._pageContent) {
      
      var resource = getLoadedMicrosummaryResource(this.pageURI);
      if (resource) {
        this._pageContent = resource.content;
        resource.destroy();
      }
    }

    return this._pageContent;
  },
  set pageContent(newValue) { return this._pageContent = newValue },

  get updateInterval()         { return this._updateInterval; },
  set updateInterval(newValue) { return this._updateInterval = newValue; },

  get needsRemoval() { return this._needsRemoval; },

  

  addObserver: function MS_addObserver(observer) {
    
    
    if (this._observers.indexOf(observer) == -1)
      this._observers.push(observer);
  },
  
  removeObserver: function MS_removeObserver(observer) {
    
    
  
    
    
    if (this._observers.indexOf(observer) != -1)
      this._observers.splice(this._observers.indexOf(observer), 1);
  },

  




  update: function MS_update() {
    LOG("microsummary.update called for page:\n  " + this.pageURI.spec +
        "\nwith generator:\n  " + this.generator.uri.spec);

    var t = this;

    
    
    var errorCallback = function MS_errorCallback(resource) {
      if (resource.status == 410) {
        t._needsRemoval = true;
        LOG("server indicated " + resource.uri.spec + " is gone. flagging for removal");
      }

      resource.destroy();

      for (let i = 0; i < t._observers.length; i++)
        t._observers[i].onError(t);
    };

    
    
    if (!this.generator.loaded) {
      
      
      if (this.generator.uri.scheme == "urn") {
        
        
        
        if (/^source:/.test(this.generator.uri.path)) {
          this._reinstallMissingGenerator();
          return;
        }
        else
          throw "missing local generator: " + this.generator.uri.spec;
      }

      LOG("generator not yet loaded; downloading it");
      var generatorCallback =
        function MS_generatorCallback(resource) {
          try     { t._handleGeneratorLoad(resource) }
          finally { resource.destroy() }
        };
      var resource = new MicrosummaryResource(this.generator.uri);
      resource.load(generatorCallback, errorCallback);
      return;
    }

    
    
    if (this.generator.needsPageContent && !this.pageContent) {
      LOG("page content not yet loaded; downloading it");
      var pageCallback =
        function MS_pageCallback(resource) {
          try     { t._handlePageLoad(resource) }
          finally { resource.destroy() }
        };
      var resource = new MicrosummaryResource(this.pageURI);
      resource.load(pageCallback, errorCallback);
      return;
    }

    LOG("generator (and page, if needed) both loaded; generating microsummary");

    
    
    this._content = this.generator.generateMicrosummary(this.pageContent);
    this._updateInterval = this.generator.calculateUpdateInterval(this.pageContent);
    this.pageContent = null;
    for ( var i = 0; i < this._observers.length; i++ )
      this._observers[i].onContentLoaded(this);

    LOG("generated microsummary: " + this.content);
  },

  _handleGeneratorLoad: function MS__handleGeneratorLoad(resource) {
    LOG(this.generator.uri.spec + " microsummary generator downloaded");
    if (resource.isXML)
      this.generator.initFromXML(resource.content);
    else if (resource.contentType == "text/plain")
      this.generator.initFromText(resource.content);
    else if (resource.contentType == "text/html")
      this.generator.initFromText(resource.content.body.textContent);
    else
      throw("generator is neither XML nor plain text");

    
    if (this.generator.loaded)
      this.update();
  },

  _handlePageLoad: function MS__handlePageLoad(resource) {
    if (!resource.isXML && resource.contentType != "text/html")
      throw("page is neither HTML nor XML");

    this.pageContent = resource.content;
    this.update();
  },

  




  _reinstallMissingGenerator: function MS__reinstallMissingGenerator() {
    LOG("attempting to reinstall missing generator " + this.generator.uri.spec);

    var t = this;

    var loadCallback =
      function MS_missingGeneratorLoadCallback(resource) {
        try     { t._handleMissingGeneratorLoad(resource) }
        finally { resource.destroy() }
      };

    var errorCallback =
      function MS_missingGeneratorErrorCallback(resource) {
        try     { t._handleMissingGeneratorError(resource) }
        finally { resource.destroy() }
      };

    try {
      
      var sourceURL = this.generator.uri.path.replace(/^source:/, "");
      var sourceURI = NetUtil.newURI(sourceURL);

      var resource = new MicrosummaryResource(sourceURI);
      resource.load(loadCallback, errorCallback);
    }
    catch(ex) {
      Cu.reportError(ex);
      this._handleMissingGeneratorError();
    }
  },

  










  _handleMissingGeneratorLoad: function MS__handleMissingGeneratorLoad(resource) {
    try {
      
      if (!resource.isXML)
        throw("downloaded, but not XML " + this.generator.uri.spec);

      
      var generatorID = this.generator.uri.spec;
      resource.content.documentElement.setAttribute("uri", generatorID);

      
      
      this.generator = this._mss.installGenerator(resource.content);

      
      
      
      if (!this.generator.loaded)
        throw("supposedly installed, but not in cache " + this.generator.uri.spec);
    }
    catch(ex) {
      Cu.reportError(ex);
      this._handleMissingGeneratorError(resource);
      return;
    }
  
    LOG("reinstall succeeded; resuming update " + this.generator.uri.spec);
    this.update();
  },

  









  _handleMissingGeneratorError: function MS__handleMissingGeneratorError(resource) {
    LOG("reinstall failed; removing microsummaries " + this.generator.uri.spec);
    var bookmarks = this._mss.getBookmarks();
    while (bookmarks.hasMoreElements()) {
      var bookmarkID = bookmarks.getNext();
      var microsummary = this._mss.getMicrosummary(bookmarkID);
      if (microsummary.generator.uri.equals(this.generator.uri)) {
        LOG("removing microsummary for " + microsummary.pageURI.spec);
        this._mss.removeMicrosummary(bookmarkID);
      }
    }
  }

};





function MicrosummaryGenerator(aURI, aLocalURI, aName) {
  this._uri = aURI || null;
  this._localURI = aLocalURI || null;
  this._name = aName || null;
  this._loaded = false;
  this._rules = [];
  this._template = null;
  this._content = null;
}

MicrosummaryGenerator.prototype = {
  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMicrosummaryGenerator]),

  

  
  
  
  
  get uri() { return this._uri || this.localURI },

  
  
  get localURI() { return this._localURI },
  get name() { return this._name },
  get loaded() { return this._loaded },

  equals: function(aOther) {
    
    return aOther.uri.equals(this.uri);
  },

  












  appliesToURI: function(uri) {
    var applies = false;

    for ( var i = 0 ; i < this._rules.length ; i++ ) {
      var rule = this._rules[i];

      switch (rule.type) {
      case "include":
        if (rule.regexp.test(uri.spec))
          applies = true;
        break;
      case "exclude":
        if (rule.regexp.test(uri.spec))
          return false;
        break;
      }
    }

    return applies;
  },

  get needsPageContent() {
    if (this._template)
      return true;
    if (this._content)
      return false;

    throw("needsPageContent called on uninitialized microsummary generator");
  },

  







  initFromText: function(text) {
    this._content = text;
    this._loaded = true;
  },

  






  initFromXML: function(xmlDocument) {
    

    
    
    

    
    
    
    var generatorNode = xmlDocument.getElementsByTagNameNS(MICSUM_NS, "generator")[0];
    if (!generatorNode)
      throw Cr.NS_ERROR_FAILURE;

    this._name = generatorNode.getAttribute("name");

    
    
    if (this.localURI && generatorNode.hasAttribute("uri"))
      this._uri = NetUtil.newURI(generatorNode.getAttribute("uri"), null, null);

    function getFirstChildByTagName(tagName, parentNode, namespace) {
      var nodeList = parentNode.getElementsByTagNameNS(namespace, tagName);
      for (var i = 0; i < nodeList.length; i++) {
        
        if (nodeList[i].parentNode == parentNode)
          return nodeList[i];
      }
      return null;
    }

    
    
    
    this._rules.splice(0);
    var pages = getFirstChildByTagName("pages", generatorNode, MICSUM_NS);
    if (pages) {
      
      for ( var i = 0; i < pages.childNodes.length ; i++ ) {
        var node = pages.childNodes[i];
        if (node.nodeType != node.ELEMENT_NODE ||
            node.namespaceURI != MICSUM_NS ||
            (node.nodeName != "include" && node.nodeName != "exclude"))
          continue;
        var urlRegexp = node.textContent.replace(/^\s+|\s+$/g, "");
        this._rules.push({ type: node.nodeName, regexp: new RegExp(urlRegexp) });
      }
    }

    
    
    var update = getFirstChildByTagName("update", generatorNode, MICSUM_NS);
    if (update) {
      function _parseInterval(string) {
        
        
        return Math.round(Math.max(parseFloat(string) || 0, 1) * 60 * 1000);
      }

      this._unconditionalUpdateInterval =
        update.hasAttribute("interval") ?
        _parseInterval(update.getAttribute("interval")) : null;

      
      this._updateIntervals = new Array();
      for (i = 0; i < update.childNodes.length; i++) {
        node = update.childNodes[i];
        if (node.nodeType != node.ELEMENT_NODE || node.namespaceURI != MICSUM_NS ||
            node.nodeName != "condition")
          continue;
        if (!node.getAttribute("expression") || !node.getAttribute("interval")) {
          LOG("ignoring incomplete conditional update interval for " + this.uri.spec);
          continue;
        }
        this._updateIntervals.push({
          expression: node.getAttribute("expression"),
          interval: _parseInterval(node.getAttribute("interval"))
        });
      }
    }

    var templateNode = getFirstChildByTagName("template", generatorNode, MICSUM_NS);
    if (templateNode) {
      this._template = getFirstChildByTagName("transform", templateNode, XSLT_NS) ||
                       getFirstChildByTagName("stylesheet", templateNode, XSLT_NS);
    }
    

    this._loaded = true;
  },

  generateMicrosummary: function MSD_generateMicrosummary(pageContent) {

    var content;

    if (this._content)
      content = this._content;
    else if (this._template)
      content = this._processTemplate(pageContent);
    else
      throw("generateMicrosummary called on uninitialized microsummary generator");

    
    content = content.replace(/^\s+|\s+$/g, "");
    if (content.length > MAX_SUMMARY_LENGTH) 
      content = content.substring(0, MAX_SUMMARY_LENGTH);

    return content;
  },

  calculateUpdateInterval: function MSD_calculateUpdateInterval(doc) {
    if (this._content || !this._updateIntervals || !doc)
      return null;

    for (var i = 0; i < this._updateIntervals.length; i++) {
      try {
        if (doc.evaluate(this._updateIntervals[i].expression, doc, null,
                         Ci.nsIDOMXPathResult.BOOLEAN_TYPE, null).booleanValue)
          return this._updateIntervals[i].interval;
      }
      catch (ex) {
        Cu.reportError(ex);
        
        this._updateIntervals.splice(i--, 1);
      }
    }

    return this._unconditionalUpdateInterval;
  },

  _processTemplate: function MSD__processTemplate(doc) {
    LOG("processing template " + this._template + " against document " + doc);

    
    var processor = Cc["@mozilla.org/document-transformer;1?type=xslt"].
                    createInstance(Ci.nsIXSLTProcessor);

    
    
    processor.flags |= Ci.nsIXSLTProcessorPrivate.DISABLE_ALL_LOADS;

    processor.importStylesheet(this._template);
    var fragment = processor.transformToFragment(doc, doc);

    LOG("template processing result: " + fragment.textContent);

    
    
    return fragment.textContent;
  },

  saveXMLToFile: function MSD_saveXMLToFile(xmlDefinition) {
    var file = this.localURI.QueryInterface(Ci.nsIFileURL).file.clone();

    LOG("saving definition to " + file.path);

    
    var outputStream = Cc["@mozilla.org/network/safe-file-output-stream;1"].
                       createInstance(Ci.nsIFileOutputStream);
    var localFile = file.QueryInterface(Ci.nsILocalFile);
    outputStream.init(localFile, (MODE_WRONLY | MODE_TRUNCATE | MODE_CREATE),
                      PERMS_FILE, 0);
    var serializer = Cc["@mozilla.org/xmlextras/xmlserializer;1"].
                     createInstance(Ci.nsIDOMSerializer);
    serializer.serializeToStream(xmlDefinition, outputStream, null);
    if (outputStream instanceof Ci.nsISafeOutputStream) {
      try       { outputStream.finish() }
      catch (e) { outputStream.close()  }
    }
    else
      outputStream.close();
  },

  update: function MSD_update() {
    
    
    var genURI = this.uri;
    if (genURI && /^urn:source:/i.test(genURI.spec)) {
      let genURL = genURI.spec.replace(/^urn:source:/, "");
      genURI = NetUtil.newURI(genURL, null, null);
    }

    
    if (!genURI || !/^https?/.test(genURI.scheme)) {
      LOG("generator did not have valid URI; skipping update: " + genURI.spec);
      return;
    }

    
    
    
    var t = this;
    var loadCallback = function(resource) {
      if (resource.status != 304)
        t._performUpdate(genURI);
      else
        LOG("generator is already up to date: " + genURI.spec);
      resource.destroy();
    };
    var errorCallback = function(resource) {
      resource.destroy();
    };

    var file = this.localURI.QueryInterface(Ci.nsIFileURL).file.clone();
    var lastmod = new Date(file.lastModifiedTime);
    LOG("updating generator: " + genURI.spec);
    var resource = new MicrosummaryResource(genURI);
    resource.lastMod = lastmod.toUTCString();
    resource.method = "HEAD";
    resource.load(loadCallback, errorCallback);
  },

  _performUpdate: function MSD__performUpdate(uri) {
    var t = this;
    var loadCallback = function(resource) {
      try     { t._handleUpdateLoad(resource) }
      finally { resource.destroy() }
    };
    var errorCallback = function(resource) {
      resource.destroy();
    };

    var resource = new MicrosummaryResource(uri);
    resource.load(loadCallback, errorCallback);
  },

  _handleUpdateLoad: function MSD__handleUpdateLoad(resource) {
    if (!resource.isXML)
      throw("update failed, downloaded resource is not XML: " + this.uri.spec);

    
    
    var generatorID = this.uri.spec;
    resource.content.documentElement.setAttribute("uri", generatorID);

    
    this.initFromXML(resource.content);
    this.saveXMLToFile(resource.content);

    
    gObsSvc.notifyObservers(this, "microsummary-generator-updated", null);
  }
};










function MicrosummarySet() {
  this._observers = [];
  this._elements = [];
}

MicrosummarySet.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMicrosummarySet,
                                         Ci.nsIMicrosummaryObserver]),

  

  onContentLoaded: function MSSet_onContentLoaded(microsummary) {
    for ( var i = 0; i < this._observers.length; i++ )
      this._observers[i].onContentLoaded(microsummary);
  },

  onError: function MSSet_onError(microsummary) {
    for ( var i = 0; i < this._observers.length; i++ )
      this._observers[i].onError(microsummary);
  },

  

  addObserver: function MSSet_addObserver(observer) {
    if (this._observers.length == 0) {
      for ( var i = 0 ; i < this._elements.length ; i++ )
        this._elements[i].addObserver(this);
    }

    
    
    if (this._observers.indexOf(observer) == -1)
      this._observers.push(observer);
  },
  
  removeObserver: function MSSet_removeObserver(observer) {
    
    
  
    
    
    if (this._observers.indexOf(observer) != -1)
      this._observers.splice(this._observers.indexOf(observer), 1);
    
    if (this._observers.length == 0) {
      for ( var i = 0 ; i < this._elements.length ; i++ )
        this._elements[i].removeObserver(this);
    }
  },

  extractFromPage: function MSSet_extractFromPage(resource) {
    if (!resource.isXML && resource.contentType != "text/html")
      throw("page is neither HTML nor XML");

    
    

    var links = resource.content.getElementsByTagName("link");
    for ( var i = 0; i < links.length; i++ ) {
      var link = links[i];

      if(!link.hasAttribute("rel"))
        continue;

      var relAttr = link.getAttribute("rel");

      
      
      var linkTypes = relAttr.split(/\s+/);
      if (!linkTypes.some( function(v) { return v.toLowerCase() == "microsummary"; }))
        continue;


      
      var linkTitle = link.getAttribute("title");


      
      
      var generatorURI = NetUtil.newURI(link.href, resource.content.characterSet,
                                        null);

      if (!/^https?$/i.test(generatorURI.scheme)) {
        LOG("can't load generator " + generatorURI.spec + " from page " +
            resource.uri.spec);
        continue;
      }

      var generator = new MicrosummaryGenerator(generatorURI, null, linkTitle);
      var microsummary = new Microsummary(resource.uri, generator);
      if (!this.hasItemForMicrosummary(microsummary))
        this.AppendElement(microsummary);
    }
  },

  



  hasItemForMicrosummary: function MSSet_hasItemForMicrosummary(aMicrosummary) {
    for (var i = 0; i < this._elements.length; i++) {
      if (this._elements[i].equals(aMicrosummary))
        return true;
    }
    return false;
  },

  
  AppendElement: function MSSet_AppendElement(element) {
    
    
    element = element.QueryInterface(Ci.nsIMicrosummary);

    if (this._elements.indexOf(element) == -1) {
      this._elements.push(element);
      element.addObserver(this);
    }

    
    for ( var i = 0; i < this._observers.length; i++ )
      this._observers[i].onElementAppended(element);
  },

  Enumerate: function MSSet_Enumerate() {
    return new ArrayEnumerator(this._elements);
  }
};









function ArrayEnumerator(aItems) {
  if (aItems) {
    for (var i = 0; i < aItems.length; ++i) {
      if (!aItems[i])
        aItems.splice(i--, 1);
    }
    this._contents = aItems;
  } else {
    this._contents = [];
  }
}

ArrayEnumerator.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISimpleEnumerator]),

  _index: 0,

  hasMoreElements: function() {
    return this._index < this._contents.length;
  },

  getNext: function() {
    return this._contents[this._index++];      
  }
};












function LOG(aText) {
  var f = arguments.callee;
  if (!("_enabled" in f))
    f._enabled = getPref("browser.microsummary.log", false);
  if (f._enabled) {
    dump("*** Microsummaries: " +  aText + "\n");
    var consoleService = Cc["@mozilla.org/consoleservice;1"].
                         getService(Ci.nsIConsoleService);
    consoleService.logStringMessage(aText);
  }
}
















function MicrosummaryResource(uri) {
  
  
  
  if (!(uri.schemeIs("http") || uri.schemeIs("https") || uri.schemeIs("file")))
    throw NS_ERROR_DOM_BAD_URI;

  this._uri = uri;
  this._content = null;
  this._contentType = null;
  this._isXML = false;
  this.__authFailed = false;
  this._status = null;
  this._method = "GET";
  this._lastMod = null;

  
  this._loadCallback = null;
  
  this._errorCallback = null;
  
  this._iframe = null;
}

MicrosummaryResource.prototype = {
  get uri() {
    return this._uri;
  },

  get content() {
    return this._content;
  },

  get contentType() {
    return this._contentType;
  },

  get isXML() {
    return this._isXML;
  },

  get status()        { return this._status },
  set status(aStatus) { this._status = aStatus },

  get method()        { return this._method },
  set method(aMethod) { this._method = aMethod },

  get lastMod()     { return this._lastMod },
  set lastMod(aMod) { this._lastMod = aMod },

  
  
  
  
  interfaces: [Ci.nsIAuthPromptProvider,
               Ci.nsIAuthPrompt,
               Ci.nsIBadCertListener2,
               Ci.nsISSLErrorListener,
               Ci.nsIPrompt,
               Ci.nsIProgressEventSink,
               Ci.nsIInterfaceRequestor,
               Ci.nsISupports],

  

  QueryInterface: function MSR_QueryInterface(iid) {
    if (!this.interfaces.some( function(v) { return iid.equals(v) } ))
      throw Cr.NS_ERROR_NO_INTERFACE;

    
    
    
    switch(iid) {
    case Ci.nsIAuthPrompt:
      return this.authPrompt;
    case Ci.nsIPrompt:
      return this.prompt;
    default:
      return this;
    }
  },

  
  
  getInterface: function MSR_getInterface(iid) {
    return this.QueryInterface(iid);
  },

  
  
  notifyCertProblem: function MSR_certProblem(socketInfo, status, targetSite) {
    return true;
  },

  
  
  notifySSLError: function MSR_SSLError(socketInfo, error, targetSite) {
    return true;
  },

  
  

  
  
  
  get _authFailed()         { return this.__authFailed; },
  set _authFailed(newValue) { return this.__authFailed = newValue },

  
  
  getAuthPrompt: function(aPromptReason, aIID) {
    this._authFailed = true;
    throw Cr.NS_ERROR_NOT_AVAILABLE;
  },

  
  
  

  

  get authPrompt() {
    var resource = this;
    return {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrompt]),
      prompt: function(dialogTitle, text, passwordRealm, savePassword, defaultText, result) {
        resource._authFailed = true;
        return false;
      },
      promptUsernameAndPassword: function(dialogTitle, text, passwordRealm, savePassword, user, pwd) {
        resource._authFailed = true;
        return false;
      },
      promptPassword: function(dialogTitle, text, passwordRealm, savePassword, pwd) {
        resource._authFailed = true;
        return false;
      }
    };
  },

  

  get prompt() {
    var resource = this;
    return {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrompt]),
      alert: function(dialogTitle, text) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      alertCheck: function(dialogTitle, text, checkMessage, checkValue) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      confirm: function(dialogTitle, text) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      confirmCheck: function(dialogTitle, text, checkMessage, checkValue) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      confirmEx: function(dialogTitle, text, buttonFlags, button0Title, button1Title, button2Title, checkMsg, checkValue) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      prompt: function(dialogTitle, text, value, checkMsg, checkValue) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      promptPassword: function(dialogTitle, text, password, checkMsg, checkValue) {
        resource._authFailed = true;
        return false;
      },
      promptUsernameAndPassword: function(dialogTitle, text, username, password, checkMsg, checkValue) {
        resource._authFailed = true;
        return false;
      },
      select: function(dialogTitle, text, count, selectList, outSelection) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      }
    };
  },

  
  
  
  
  

  

  onProgress: function(aRequest, aContext, aProgress, aProgressMax) {},
  onStatus: function(aRequest, aContext, aStatus, aStatusArg) {},

  






  initFromDocument: function MSR_initFromDocument(document) {
    this._content = document;
    this._contentType = document.contentType;

    
    
    
    
    this._isXML = (this.contentType == "text/xml" ||
                   this.contentType == "application/xml" ||
                   /^.+\/.+\+xml$/.test(this.contentType));
  },

  




  destroy: function MSR_destroy() {
    this._uri = null;
    this._content = null;
    this._loadCallback = null;
    this._errorCallback = null;
    this._loadTimer = null;
    this._authFailed = false;
    if (this._iframe) {
      if (this._iframe && this._iframe.parentNode)
        this._iframe.parentNode.removeChild(this._iframe);
      this._iframe = null;
    }
  },

  








  load: function MSR_load(loadCallback, errorCallback) {
    LOG(this.uri.spec + " loading");
  
    this._loadCallback = loadCallback;
    this._errorCallback = errorCallback;

    var request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance();
  
    var loadHandler = {
      _self: this,
      handleEvent: function MSR_loadHandler_handleEvent(event) {
        if (this._self._loadTimer)
          this._self._loadTimer.cancel();

        this._self.status = event.target.status;

        if (this._self._authFailed || this._self.status >= 400) {
          
          

          
          
          
          LOG(this._self.uri.spec + " load failed; HTTP status: " + this._self.status);
          try     { this._self._handleError(event) }
          finally { this._self = null }
        }
        else if (event.target.channel.contentType == "multipart/x-mixed-replace") {
          
          
          LOG(this._self.uri.spec + " load failed; contains multipart content");
          try     { this._self._handleError(event) }
          finally { this._self = null }
        }
        else {
          LOG(this._self.uri.spec + " load succeeded; invoking callback");
          try     { this._self._handleLoad(event) }
          finally { this._self = null }
        }
      }
    };

    var errorHandler = {
      _self: this,
      handleEvent: function MSR_errorHandler_handleEvent(event) {
        if (this._self._loadTimer)
          this._self._loadTimer.cancel();

        LOG(this._self.uri.spec + " load failed");
        try     { this._self._handleError(event) }
        finally { this._self = null }
      }
    };

    
    
    
    var timeout = getPref("browser.microsummary.requestTimeout", 300) * 1000;
    var timerObserver = {
      _self: this,
      observe: function MSR_timerObserver_observe() {
        LOG("timeout loading microsummary resource " + this._self.uri.spec + ", aborting request");
        request.abort();
        try     { this._self.destroy() }
        finally { this._self = null }
      }
    };
    this._loadTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._loadTimer.init(timerObserver, timeout, Ci.nsITimer.TYPE_ONE_SHOT);

    request = request.QueryInterface(Ci.nsIDOMEventTarget);
    request.addEventListener("load", loadHandler, false);
    request.addEventListener("error", errorHandler, false);
    
    request = request.QueryInterface(Ci.nsIXMLHttpRequest);
    request.open(this.method, this.uri.spec, true);
    request.setRequestHeader("X-Moz", "microsummary");
    if (this.lastMod)
      request.setRequestHeader("If-Modified-Since", this.lastMod);

    
    
    
    request.channel.notificationCallbacks = this;

    
    
    
    
    try {
      var resolver = Cc["@mozilla.org/embeddor.implemented/bookmark-charset-resolver;1"].
                     getService(Ci.nsICharsetResolver);
      if (resolver) {
        var charset = resolver.requestCharset(null, request.channel, {}, {});
        if (charset != "")
          request.channel.contentCharset = charset;
      }
    }
    catch(ex) {}

    request.send(null);
  },

  _handleLoad: function MSR__handleLoad(event) {
    var request = event.target;

    if (request.responseXML) {
      this._isXML = true;
      
      if (request.responseXML.documentElement.nodeName == "parsererror") {
        this._handleError(event);
        return;
      }
      this._content = request.responseXML;
      this._contentType = request.channel.contentType;
      this._loadCallback(this);
    }

    else if (request.channel.contentType == "text/html") {
      this._parse(request.responseText);
    }

    else {
      
      
      this._content = request.responseText;
      this._contentType = request.channel.contentType;
      this._loadCallback(this);
    }
  },

  _handleError: function MSR__handleError(event) {
    
    try     { if (this._errorCallback) this._errorCallback(this) } 
    finally { this.destroy() }
  },

  








  _parse: function MSR__parse(htmlText) {
    
    var windowMediator = Cc['@mozilla.org/appshell/window-mediator;1'].
                         getService(Ci.nsIWindowMediator);
    var window = windowMediator.getMostRecentWindow("navigator:browser");
    
    
    
    
    
    if (!window) {
      this._handleError(event);
      return;
    }
    var document = window.document;
    var rootElement = document.documentElement;
  
    
    this._iframe = document.createElement('iframe');
    this._iframe.setAttribute("collapsed", true);
    this._iframe.setAttribute("type", "content");
  
    
    rootElement.appendChild(this._iframe);

    
    
    
    var webNav = this._iframe.docShell.QueryInterface(Ci.nsIWebNavigation);
    webNav.stop(Ci.nsIWebNavigation.STOP_NETWORK);

    
    
    
    this._iframe.docShell.allowJavascript = false;
    this._iframe.docShell.allowAuth = false;
    this._iframe.docShell.allowPlugins = false;
    this._iframe.docShell.allowMetaRedirects = false;
    this._iframe.docShell.allowSubframes = false;
    this._iframe.docShell.allowImages = false;
    this._iframe.docShell.allowDNSPrefetch = false;
  
    var parseHandler = {
      _self: this,
      handleEvent: function MSR_parseHandler_handleEvent(event) {
        event.target.removeEventListener("DOMContentLoaded", this, false);
        try     { this._self._handleParse(event) }
        finally { this._self = null }
      }
    };
 
    
    var converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                    createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    var stream = converter.convertToInputStream(htmlText);

    
    var channel = Cc["@mozilla.org/network/input-stream-channel;1"].
                  createInstance(Ci.nsIInputStreamChannel);
    channel.setURI(this._uri);
    channel.contentStream = stream;

    
    var request = channel.QueryInterface(Ci.nsIRequest);
    request.loadFlags |= Ci.nsIRequest.LOAD_BACKGROUND;

    
    
    
    var baseChannel = channel.QueryInterface(Ci.nsIChannel);
    baseChannel.contentType = "text/html";

    
    
    
    baseChannel.contentCharset = "UTF-8";

    
    
    
    this._iframe.addEventListener("DOMContentLoaded", parseHandler, true);
    var uriLoader = Cc["@mozilla.org/uriloader;1"].getService(Ci.nsIURILoader);
    uriLoader.openURI(channel, true, this._iframe.docShell);
  },

  






  _handleParse: function MSR__handleParse(event) {
    

    this._content = this._iframe.contentDocument;
    this._contentType = this._iframe.contentDocument.contentType;
    this._loadCallback(this);
  }

};












function getLoadedMicrosummaryResource(uri) {
  var mediator = Cc["@mozilla.org/appshell/window-mediator;1"].
                 getService(Ci.nsIWindowMediator);

  
  
  var windows = mediator.getEnumerator("navigator:browser");

  while (windows.hasMoreElements()) {
    var win = windows.getNext();
    var tabBrowser = win.document.getElementById("content");
    for ( var i = 0; i < tabBrowser.browsers.length; i++ ) {
      var browser = tabBrowser.browsers[i];
      if (uri.equals(browser.currentURI)) {
        var resource = new MicrosummaryResource(uri);
        resource.initFromDocument(browser.contentDocument);
        return resource;
      }
    }
  }

  return null;
}








function getPref(prefName, defaultValue) {
  try {
    var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);
    var type = prefBranch.getPrefType(prefName);
    switch (type) {
      case prefBranch.PREF_BOOL:
        return prefBranch.getBoolPref(prefName);
      case prefBranch.PREF_INT:
        return prefBranch.getIntPref(prefName);
    }
  }
  catch (ex) {  }
  
  return defaultValue;
}











function sanitizeName(aName) {
  const chars = "-abcdefghijklmnopqrstuvwxyz0123456789";
  const maxLength = 60;

  var name = aName.toLowerCase();
  name = name.replace(/ /g, "-");
  
  
  
  var filteredName = "";
  for ( var i = 0 ; i < name.length ; i++ )
    if (chars.indexOf(name[i]) != -1)
      filteredName += name[i];
  name = filteredName;

  if (!name) {
    
    for (var i = 0; i < 8; ++i)
      name += chars.charAt(Math.round(Math.random() * (chars.length - 1)));
  }

  if (name.length > maxLength)
    name = name.substring(0, maxLength);

  return name;
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([MicrosummaryService]);
}
