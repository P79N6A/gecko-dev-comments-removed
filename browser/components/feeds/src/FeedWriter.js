# -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
# The Original Code is the Feed Writer.
#
# The Initial Developer of the Original Code is Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Ben Goodger <beng@google.com>
#   Jeff Walden <jwalden+code@mit.edu>
#   Asaf Romano <mano@mozilla.com>
#   Robert Sayre <sayrer@gmail.com>
#   Michael Ventnor <m.ventnor@gmail.com>
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
# ***** END LICENSE BLOCK ***** */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

function LOG(str) {
  var prefB = 
    Cc["@mozilla.org/preferences-service;1"].
    getService(Ci.nsIPrefBranch);

  var shouldLog = false;
  try {
    shouldLog = prefB.getBoolPref("feeds.log");
  } 
  catch (ex) {
  }

  if (shouldLog)
    dump("*** Feeds: " + str + "\n");
}







function makeURI(aURLSpec, aCharset) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  try {
    return ios.newURI(aURLSpec, aCharset, null);
  } catch (ex) { }

  return null;
}


const XML_NS = "http://www.w3.org/XML/1998/namespace"
const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const TYPE_MAYBE_FEED = "application/vnd.mozilla.maybe.feed";
const URI_BUNDLE = "chrome://browser/locale/feeds/subscribe.properties";

const PREF_SELECTED_APP = "browser.feeds.handlers.application";
const PREF_SELECTED_WEB = "browser.feeds.handlers.webservice";
const PREF_SELECTED_ACTION = "browser.feeds.handler";
const PREF_SELECTED_READER = "browser.feeds.handler.default";
const PREF_SHOW_FIRST_RUN_UI = "browser.feeds.showFirstRunUI";

const FW_CLASSID = Components.ID("{49bb6593-3aff-4eb3-a068-2712c28bd58e}");
const FW_CLASSNAME = "Feed Writer";
const FW_CONTRACTID = "@mozilla.org/browser/feeds/result-writer;1";

const TITLE_ID = "feedTitleText";
const SUBTITLE_ID = "feedSubtitleText";

const ICON_DATAURL_PREFIX = "data:image/x-icon;base64,";

function FeedWriter() {
}
FeedWriter.prototype = {
  _getPropertyAsBag: function FW__getPropertyAsBag(container, property) {
    return container.fields.getProperty(property).
                     QueryInterface(Ci.nsIPropertyBag2);
  },
  
  _getPropertyAsString: function FW__getPropertyAsString(container, property) {
    try {
      return container.fields.getPropertyAsAString(property);
    }
    catch (e) {
    }
    return "";
  },
  
  _setContentText: function FW__setContentText(id, text) {
    var element = this._document.getElementById(id);
    while (element.hasChildNodes())
      element.removeChild(element.firstChild);
    element.appendChild(this._document.createTextNode(text));
  },
  
  









  _safeSetURIAttribute: 
  function FW__safeSetURIAttribute(element, attribute, uri) {
    var secman = 
        Cc["@mozilla.org/scriptsecuritymanager;1"].
        getService(Ci.nsIScriptSecurityManager);    
    const flags = Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL;
    try {
      secman.checkLoadURIStr(this._window.location.href, uri, flags);
      
      
      element.setAttribute(attribute, uri);
    }
    catch (e) {
      
    }
  },

  __bundle: null,
  get _bundle() {
    if (!this.__bundle) {
      this.__bundle = Cc["@mozilla.org/intl/stringbundle;1"].
                      getService(Ci.nsIStringBundleService).
                      createBundle(URI_BUNDLE);
    }
    return this.__bundle;
  },
  
  _getFormattedString: function FW__getFormattedString(key, params) {
    return this._bundle.formatStringFromName(key, params, params.length);
  },
  
  _getString: function FW__getString(key) {
    return this._bundle.GetStringFromName(key);
  },

  
  _getSelectedItemFromMenulist: function FW__getSelectedItemFromList(aList) {
    var node = aList.firstChild.firstChild;
    while (node) {
      if (node.localName == "menuitem" && node.getAttribute("selected") == "true")
        return node;

      node = node.nextSibling;
    }

    return null;
  },

  _setCheckboxCheckedState: function FW__setCheckboxCheckedState(aCheckbox, aValue) {
    
    var change = (aValue != (aCheckbox.getAttribute('checked') == 'true'));
    if (aValue)
      aCheckbox.setAttribute('checked', 'true');
    else
      aCheckbox.removeAttribute('checked');

    if (change) {
      var event = this._document.createEvent('Events');
      event.initEvent('CheckboxStateChange', true, true);
      aCheckbox.dispatchEvent(event);
    }
  },

  
  
  _selectedApplicationItemWrapped: null,
  get selectedApplicationItemWrapped() {
    if (!this._selectedApplicationItemWrapped) {
      this._selectedApplicationItemWrapped =
        XPCNativeWrapper(this._document.getElementById("selectedAppMenuItem"));
    }

    return this._selectedApplicationItemWrapped;
  },

  _defaultSystemReaderItemWrapped: null,
  get defaultSystemReaderItemWrapped() {
    if (!this._defaultSystemReaderItemWrapped) {
      
      
      var menuItem = this._document.getElementById("defaultHandlerMenuItem");
      if (menuItem)
        this._defaultSystemReaderItemWrapped = XPCNativeWrapper(menuItem);
    }

    return this._defaultSystemReaderItemWrapped;
  },

   





  _parseDate: function FW__parseDate(dateString) {
    
    dateObj = new Date(dateString);

    
    if (!dateObj.getTime())
      return false;

    var dateService = Cc["@mozilla.org/intl/scriptabledateformat;1"].
                      getService(Ci.nsIScriptableDateFormat);
    return dateService.FormatDateTime("", dateService.dateFormatLong, dateService.timeFormatNoSeconds,
                                      dateObj.getFullYear(), dateObj.getMonth()+1, dateObj.getDate(),
                                      dateObj.getHours(), dateObj.getMinutes(), dateObj.getSeconds());
  },

  




  _setTitleText: function FW__setTitleText(container) {
    if (container.title) {
      this._setContentText(TITLE_ID, container.title.plainText());
      this._document.title = container.title.plainText();
    }
    
    var feed = container.QueryInterface(Ci.nsIFeed);
    if (feed && feed.subtitle)
      this._setContentText(SUBTITLE_ID, container.subtitle.plainText());
  },
  
  




  _setTitleImage: function FW__setTitleImage(container) {
    try {
      var parts = container.image;
      
      
      var feedTitleImage = this._document.getElementById("feedTitleImage");
      this._safeSetURIAttribute(feedTitleImage, "src", 
                                parts.getPropertyAsAString("url"));
      
      
      var feedTitleLink = this._document.getElementById("feedTitleLink");
      
      var titleText = 
        this._getFormattedString("linkTitleTextFormat", 
                                 [parts.getPropertyAsAString("title")]);
      feedTitleLink.setAttribute("title", titleText);
      this._safeSetURIAttribute(feedTitleLink, "href", 
                                parts.getPropertyAsAString("link"));

      
      
      var feedTitleText = this._document.getElementById("feedTitleText");
      var titleImageWidth = parseInt(parts.getPropertyAsAString("width")) + 15;
      feedTitleText.style.marginRight = titleImageWidth + "px";
    }
    catch (e) {
      LOG("Failed to set Title Image (this is benign): " + e);
    }
  },
  
  




  _writeFeedContent: function FW__writeFeedContent(container) {
    
    var feedContent = this._document.getElementById("feedContent");
    var feed = container.QueryInterface(Ci.nsIFeed);
    
    for (var i = 0; i < feed.items.length; ++i) {
      var entry = feed.items.queryElementAt(i, Ci.nsIFeedEntry);
      entry.QueryInterface(Ci.nsIFeedContainer);
      
      var entryContainer = this._document.createElementNS(HTML_NS, "div");
      entryContainer.className = "entry";

      
      if (entry.title) {
        var a = this._document.createElementNS(HTML_NS, "a");
        a.appendChild(this._document.createTextNode(entry.title.plainText()));
      
        
        if (entry.link)
          this._safeSetURIAttribute(a, "href", entry.link.spec);

        var title = this._document.createElementNS(HTML_NS, "h3");
        title.appendChild(a);
        entryContainer.appendChild(title);

        var lastUpdated = this._parseDate(entry.updated);
        if (lastUpdated) {
          var dateDiv = this._document.createElementNS(HTML_NS, "div");
          dateDiv.setAttribute("class", "lastUpdated");
          title.appendChild(dateDiv);
          dateDiv.textContent = lastUpdated;
        }
      }

      var body = this._document.createElementNS(HTML_NS, "div");
      var summary = entry.summary || entry.content;
      var docFragment = null;
      if (summary) {

        if (summary.base)
          body.setAttributeNS(XML_NS, "base", summary.base.spec);
        else
          LOG("no base?");
        docFragment = summary.createDocumentFragment(body);
        if (docFragment)
          body.appendChild(docFragment);

        
        
        if (!entry.title && entry.link) {
          var a = this._document.createElementNS(HTML_NS, "a");
          a.appendChild(this._document.createTextNode("#"));
          this._safeSetURIAttribute(a, "href", entry.link.spec);
          body.appendChild(this._document.createTextNode(" "));
          body.appendChild(a);
        }

      }
      body.className = "feedEntryContent";
      entryContainer.appendChild(body);
      feedContent.appendChild(entryContainer);
      var clearDiv = this._document.createElementNS(HTML_NS, "div");
      clearDiv.style.clear = "both";
      feedContent.appendChild(clearDiv);
    }
  },
  
  







  _getContainer: function FW__getContainer(result) {
    var feedService = 
        Cc["@mozilla.org/browser/feeds/result-service;1"].
        getService(Ci.nsIFeedResultService);
 
    try {
      var result = 
        feedService.getFeedResult(this._getOriginalURI(this._window));
    }
    catch (e) {
      LOG("Subscribe Preview: feed not available?!");
    }
    
    if (result.bozo) {
      LOG("Subscribe Preview: feed result is bozo?!");
    }

    try {
      var container = result.doc;
      container.title;
    }
    catch (e) {
      LOG("Subscribe Preview: An error occurred in parsing! Fortunately, you can still subscribe...");
      var feedError = this._document.getElementById("feedError");
      feedError.removeAttribute("style");
      var feedBody = this._document.getElementById("feedBody");
      feedBody.setAttribute("style", "display:none;");
      this._setContentText("errorCode", e);
      return null;
    }
    return container;
  },
  
  






  _getFileDisplayName: function FW__getFileDisplayName(file) {
#ifdef XP_WIN
    if (file instanceof Ci.nsILocalFileWin) {
      try {
        return file.getVersionInfoField("FileDescription");
      }
      catch (e) {
      }
    }
#endif
#ifdef XP_MACOSX
    var lfm = file.QueryInterface(Ci.nsILocalFileMac);
    try {
      return lfm.bundleDisplayName;
    }
    catch (e) {
      
    }
#endif
    var ios = 
        Cc["@mozilla.org/network/io-service;1"].
        getService(Ci.nsIIOService);
    var url = ios.newFileURI(file).QueryInterface(Ci.nsIURL);
    return url.fileName;
  },

  





  _getFileIconURL: function FW__getFileIconURL(file) {
    var ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Components.interfaces.nsIIOService);
    var fph = ios.getProtocolHandler("file")
                 .QueryInterface(Ci.nsIFileProtocolHandler);
    var urlSpec = fph.getURLSpecFromFile(file);
    return "moz-icon://" + urlSpec + "?size=16";
  },

  







  _initMenuItemWithFile: function(aMenuItem, aFile) {
    aMenuItem.setAttribute("label", this._getFileDisplayName(aFile));
    aMenuItem.setAttribute("image", this._getFileIconURL(aFile));
    aMenuItem.file = aFile;
  },

  



  _chooseClientApp: function FW__chooseClientApp() {
    try {
      var fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
      fp.init(this._window,
              this._getString("chooseApplicationDialogTitle"),
              Ci.nsIFilePicker.modeOpen);
      fp.appendFilters(Ci.nsIFilePicker.filterApps);

      if (fp.show() == Ci.nsIFilePicker.returnOK) {
        var selectedApp = fp.file;
        if (selectedApp) {
          
          
          
#ifdef XP_WIN
#expand           if (fp.file.leafName != "__MOZ_APP_NAME__.exe") {
#else
#ifdef XP_MACOSX
#expand           if (fp.file.leafName != "__MOZ_APP_DISPLAYNAME__.app") {
#else
#expand           if (fp.file.leafName != "__MOZ_APP_NAME__-bin") {
#endif
#endif
            var selectedAppMenuItem = this.selectedApplicationItemWrapped;
            this._initMenuItemWithFile(selectedAppMenuItem, selectedApp);

            
            selectedAppMenuItem.hidden = false;
            selectedAppMenuItem.doCommand();
            return true;
          }
        }
      }
    }
    catch(ex) { }

    return false;
  },

  _setAlwaysUseCheckedState: function FW__setAlwaysUseCheckedState() {
    var checkbox = this._document.getElementById("alwaysUse");
    if (checkbox) {
      var alwaysUse = false;
      try {
        var prefs = Cc["@mozilla.org/preferences-service;1"].
                    getService(Ci.nsIPrefBranch);
        if (prefs.getCharPref(PREF_SELECTED_ACTION) != "ask")
          alwaysUse = true;
      }
      catch(ex) { }
      this._setCheckboxCheckedState(checkbox, alwaysUse);
    }
  },

  _setAlwaysUseLabel: function FW__setAlwaysUseLabel() {
    var checkbox = this._document.getElementById("alwaysUse");
    if (checkbox) {
      var handlersMenuList = this._document.getElementById("handlersMenuList");
      if (handlersMenuList) {
        var handlerName = this._getSelectedItemFromMenulist(handlersMenuList)
                              .getAttribute("label");
        checkbox.setAttribute("label", this._getFormattedString("alwaysUse", [handlerName]));
      }
    }
  },

  


  handleEvent: function(event) {
    
    event = new XPCNativeWrapper(event);
    if (event.target.ownerDocument != this._document) {
      LOG("FeedWriter.handleEvent: Someone passed the feed writer as a listener to the events of another document!");
      return;
    }

    if (event.type == "command") {
      switch (event.target.id) {
        case "subscribeButton":
          this.subscribe();
          break;
        case "chooseApplicationMenuItem":
          







          if (this._document.getElementById("handlersMenuList")
                  .getAttribute("open") == "true") {
            if (!this._chooseClientApp()) {
              
              
              this._setSelectedHandler();
            }
          }
          break;
        default:
          this._setAlwaysUseLabel();
      }
    }
  },

  _setSelectedHandler: function FW__setSelectedHandler() {
    var prefs =   
        Cc["@mozilla.org/preferences-service;1"].
        getService(Ci.nsIPrefBranch);

    var handler = "bookmarks";
    try {
      handler = prefs.getCharPref(PREF_SELECTED_READER);
    }
    catch (ex) { }
    
    switch (handler) {
      case "web": {
        var handlersMenuList = this._document.getElementById("handlersMenuList");
        if (handlersMenuList) {
          var url = prefs.getComplexValue(PREF_SELECTED_WEB, Ci.nsISupportsString).data;
          var handlers =
            handlersMenuList.getElementsByAttribute("webhandlerurl", url);
          if (handlers.length == 0) {
            LOG("FeedWriter._setSelectedHandler: selected web handler isn't in the menulist")
            return;
          }

          handlers[0].doCommand();
        }
        break;
      }
      case "client": {
        var selectedAppMenuItem = this.selectedApplicationItemWrapped;
        if (selectedAppMenuItem) {
          try {
            var selectedApp = prefs.getComplexValue(PREF_SELECTED_APP,
                                                    Ci.nsILocalFile);
          } catch(ex) { }

          if (selectedApp) {
            this._initMenuItemWithFile(selectedAppMenuItem, selectedApp);
            selectedAppMenuItem.hidden = false;
            selectedAppMenuItem.doCommand();

            
            
            var defaultHandlerMenuItem = this.defaultSystemReaderItemWrapped;
            if (defaultHandlerMenuItem) {
              defaultHandlerMenuItem.hidden =
                defaultHandlerMenuItem.file.path == selectedApp.path;
            }
            break;
          }
        }
      }
      case "bookmarks":
      default: {
        var liveBookmarksMenuItem =
          this._document.getElementById("liveBookmarksMenuItem");
        if (liveBookmarksMenuItem)
          liveBookmarksMenuItem.doCommand();
      } 
    }
  },

  _initSubscriptionUI: function FW__initSubscriptionUI() {
    var handlersMenuPopup =
      this._document.getElementById("handlersMenuPopup");
    if (!handlersMenuPopup)
      return;

    
    var selectedApp;
    menuItem = this._document.createElementNS(XUL_NS, "menuitem");
    menuItem.id = "selectedAppMenuItem";
    menuItem.className = "menuitem-iconic";
    menuItem.setAttribute("handlerType", "client");
    handlersMenuPopup.appendChild(menuItem);

    var selectedApplicationItem = this.selectedApplicationItemWrapped;
    try {
      var prefs = Cc["@mozilla.org/preferences-service;1"].
                  getService(Ci.nsIPrefBranch);
      selectedApp = prefs.getComplexValue(PREF_SELECTED_APP,
                                          Ci.nsILocalFile);

      if (selectedApp.exists()) {
        this._initMenuItemWithFile(selectedApplicationItem, selectedApp);
      }
      else {
        
        selectedApplicationItem.hidden = true;
      }
    }
    catch(ex) {
      
      selectedApplicationItem.hidden = true;
    }

    
    var defaultReader = null;
    try {
      var defaultReader = Cc["@mozilla.org/browser/shell-service;1"].
                          getService(Ci.nsIShellService).defaultFeedReader;
      menuItem = this._document.createElementNS(XUL_NS, "menuitem");
      menuItem.id = "defaultHandlerMenuItem";
      menuItem.className = "menuitem-iconic";
      menuItem.setAttribute("handlerType", "client");
      handlersMenuPopup.appendChild(menuItem);

      var defaultSystemReaderItem = this.defaultSystemReaderItemWrapped;
      this._initMenuItemWithFile(defaultSystemReaderItem, defaultReader);

      
      
      if (selectedApp && selectedApp.path == defaultReader.path)
        defaultSystemReaderItem.hidden = true;
    }
    catch(ex) {  }

    
    menuItem = this._document.createElementNS(XUL_NS, "menuitem");
    menuItem.id = "chooseApplicationMenuItem";
    menuItem.setAttribute("label", this._getString("chooseApplicationMenuItem"));
    handlersMenuPopup.appendChild(menuItem);

    
    handlersMenuPopup.appendChild(this._document.createElementNS(XUL_NS,
                                  "menuseparator"));

    
    var wccr = 
      Cc["@mozilla.org/embeddor.implemented/web-content-handler-registrar;1"].
      getService(Ci.nsIWebContentConverterService);
    var handlers = wccr.getContentHandlers(TYPE_MAYBE_FEED, {});
    if (handlers.length != 0) {
      for (var i = 0; i < handlers.length; ++i) {
        menuItem = this._document.createElementNS(XUL_NS, "menuitem");
        menuItem.className = "menuitem-iconic";
        menuItem.setAttribute("label", handlers[i].name);
        menuItem.setAttribute("handlerType", "web");
        menuItem.setAttribute("webhandlerurl", handlers[i].uri);
        handlersMenuPopup.appendChild(menuItem);

        
        
        var uri = makeURI(handlers[i].uri);
        if (uri && /^https?/.test(uri.scheme))
          new iconDataURIGenerator(uri.prePath + "/favicon.ico", menuItem)
      }
    }

    this._setSelectedHandler();

    
    this._setAlwaysUseCheckedState();
    this._setAlwaysUseLabel();

    
    
    handlersMenuPopup.addEventListener("command", this, false);

    
    this._document
        .getElementById("subscribeButton")
        .addEventListener("command", this, false);
    
    
    var showFirstRunUI = true;
    try {
      showFirstRunUI = prefs.getBoolPref(PREF_SHOW_FIRST_RUN_UI);
    }
    catch (ex) { }
    if (showFirstRunUI) {
      var feedHeader = this._document.getElementById("feedHeader");
      if (feedHeader)
        feedHeader.setAttribute("firstrun", "true");

      prefs.setBoolPref(PREF_SHOW_FIRST_RUN_UI, false);
    }
  },
  
  





  _getOriginalURI: function FW__getOriginalURI(aWindow) {
    var chan = 
        aWindow.QueryInterface(Ci.nsIInterfaceRequestor).
        getInterface(Ci.nsIWebNavigation).
        QueryInterface(Ci.nsIDocShell).currentDocumentChannel;

    const SUBSCRIBE_PAGE_URI = "chrome://browser/content/feeds/subscribe.xhtml";
    var uri = makeURI(SUBSCRIBE_PAGE_URI);
    var resolvedURI = Cc["@mozilla.org/chrome/chrome-registry;1"].
                      getService(Ci.nsIChromeRegistry).
                      convertChromeURL(uri);

    if (resolvedURI.equals(chan.URI))
      return chan.originalURI;

    return null;
  },

  _window: null,
  _document: null,
  _feedURI: null,

  


  init: function FW_init(aWindow) {
    
    
    
    var window = new XPCNativeWrapper(aWindow);
    this._feedURI = this._getOriginalURI(window);
    if (!this._feedURI)
      return;

    this._window = window;
    this._document = window.document;

    LOG("Subscribe Preview: feed uri = " + this._window.location.href);

    
    this._initSubscriptionUI();
    var prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefBranch2);
    prefs.addObserver(PREF_SELECTED_ACTION, this, false);
    prefs.addObserver(PREF_SELECTED_READER, this, false);
    prefs.addObserver(PREF_SELECTED_WEB, this, false);
    prefs.addObserver(PREF_SELECTED_APP, this, false);
  },

  


  writeContent: function FW_writeContent() {
    if (!this._window)
      return;

    try {
      
      var container = this._getContainer();
      if (!container)
        return;
      
      this._setTitleText(container);
      this._setTitleImage(container);
      this._writeFeedContent(container);
    }
    finally {
      this._removeFeedFromCache();
    }
  },

  


  close: function FW_close() {
    this._document = null;
    this._window = null;
    var prefs =   
        Cc["@mozilla.org/preferences-service;1"].
        getService(Ci.nsIPrefBranch2);
    prefs.removeObserver(PREF_SELECTED_ACTION, this);
    prefs.removeObserver(PREF_SELECTED_READER, this);
    prefs.removeObserver(PREF_SELECTED_WEB, this);
    prefs.removeObserver(PREF_SELECTED_APP, this);
    this._removeFeedFromCache();
  },

  _removeFeedFromCache: function FW__removeFeedFromCache() {
    if (this._feedURI) {
      var feedService = 
          Cc["@mozilla.org/browser/feeds/result-service;1"].
          getService(Ci.nsIFeedResultService);
      feedService.removeFeedResult(this._feedURI);
      this._feedURI = null;
    }
  },

  subscribe: function FW_subscribe() {
    
    var prefs =   
        Cc["@mozilla.org/preferences-service;1"].
        getService(Ci.nsIPrefBranch);
    var defaultHandler = "reader";
    var useAsDefault = this._document.getElementById("alwaysUse")
                                     .getAttribute("checked");

    var handlersMenuList = this._document.getElementById("handlersMenuList");
    var selectedItem = this._getSelectedItemFromMenulist(handlersMenuList);

    
    
    if (selectedItem.id == "chooseApplicationMenuItem") {
      if (!this._chooseClientApp())
        return;
      
      selectedItem = this._getSelectedItemFromMenulist(handlersMenuList);
    }

    if (selectedItem.hasAttribute("webhandlerurl")) {
      var webURI = selectedItem.getAttribute("webhandlerurl");
      prefs.setCharPref(PREF_SELECTED_READER, "web");

      var supportsString = Cc["@mozilla.org/supports-string;1"].
                           createInstance(Ci.nsISupportsString);
      supportsString.data = webURI;
      prefs.setComplexValue(PREF_SELECTED_WEB, Ci.nsISupportsString,
                            supportsString);

      var wccr = 
        Cc["@mozilla.org/embeddor.implemented/web-content-handler-registrar;1"].
        getService(Ci.nsIWebContentConverterService);
      var handler = wccr.getWebContentHandlerByURI(TYPE_MAYBE_FEED, webURI);
      if (handler) {
        if (useAsDefault)
          wccr.setAutoHandler(TYPE_MAYBE_FEED, handler);

        this._window.location.href =
          handler.getHandlerURI(this._window.location.href);
      }
    }
    else {
      switch (selectedItem.id) {
        case "selectedAppMenuItem":
          prefs.setCharPref(PREF_SELECTED_READER, "client");
          prefs.setComplexValue(PREF_SELECTED_APP, Ci.nsILocalFile, 
                                this.selectedApplicationItemWrapped.file);
          break;
        case "defaultHandlerMenuItem":
          prefs.setCharPref(PREF_SELECTED_READER, "client");
          prefs.setComplexValue(PREF_SELECTED_APP, Ci.nsILocalFile, 
                                this.defaultSystemReaderItemWrapped.file);
          break;
        case "liveBookmarksMenuItem":
          defaultHandler = "bookmarks";
          prefs.setCharPref(PREF_SELECTED_READER, "bookmarks");
          break;
      }
      var feedService = Cc["@mozilla.org/browser/feeds/result-service;1"].
                        getService(Ci.nsIFeedResultService);

      
      var feedTitle = this._document.getElementById(TITLE_ID).textContent;
      var feedSubtitle =
        this._document.getElementById(SUBTITLE_ID).textContent;
      feedService.addToClientReader(this._window.location.href,
                                    feedTitle, feedSubtitle);
    }

    
    
    
    
    if (useAsDefault)
      prefs.setCharPref(PREF_SELECTED_ACTION, defaultHandler);
    else
      prefs.setCharPref(PREF_SELECTED_ACTION, "ask");
  },

  


  observe: function FW_observe(subject, topic, data) {
    if (!this._window) {
      
      
      return;
    }

    if (topic == "nsPref:changed") {
      switch (data) {
        case PREF_SELECTED_READER:
        case PREF_SELECTED_WEB:
        case PREF_SELECTED_APP:
          this._setSelectedHandler();
          break;
        case PREF_SELECTED_ACTION:
          this._setAlwaysUseCheckedState();
      }
    } 
  },

  


  getInterfaces: function WCCR_getInterfaces(countRef) {
    var interfaces = 
        [Ci.nsIFeedWriter, Ci.nsIClassInfo, Ci.nsISupports];
    countRef.value = interfaces.length;
    return interfaces;
  },
  getHelperForLanguage: function WCCR_getHelperForLanguage(language) {
    return null;
  },
  contractID: FW_CONTRACTID,
  classDescription: FW_CLASSNAME,
  classID: FW_CLASSID,
  implementationLanguage: Ci.nsIProgrammingLanguage.JAVASCRIPT,
  flags: Ci.nsIClassInfo.DOM_OBJECT,

  QueryInterface: function FW_QueryInterface(iid) {
    if (iid.equals(Ci.nsIFeedWriter) ||
        iid.equals(Ci.nsIClassInfo) ||
        iid.equals(Ci.nsIDOMEventListener) ||
        iid.equals(Ci.nsIObserver) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};

function iconDataURIGenerator(aURISpec, aElement) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  var chan = ios.newChannelFromURI(makeURI(aURISpec));
  chan.notificationCallbacks = this;
  chan.asyncOpen(this, null);

  this._channel = chan;
  this._bytes = [];
  this._element = aElement;
}
iconDataURIGenerator.prototype = {
  _channel: null,
  _countRead: 0,
  _stream: null,

  QueryInterface: function FW_IDUG_loadQI(aIID) {
    if (aIID.equals(Ci.nsISupports)           ||
        aIID.equals(Ci.nsIRequestObserver)    ||
        aIID.equals(Ci.nsIStreamListener)     ||
        aIID.equals(Ci.nsIChannelEventSink)   ||
        aIID.equals(Ci.nsIInterfaceRequestor) ||
        aIID.equals(Ci.nsIBadCertListener)    ||
        
        aIID.equals(Ci.nsIPrompt)             ||
        
        aIID.equals(Ci.nsIHttpEventSink)      ||
        aIID.equals(Ci.nsIProgressEventSink)  ||
        false)
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  
  onStartRequest: function FW_IDUG_loadStartR(aRequest, aContext) {
    this._stream = Cc["@mozilla.org/binaryinputstream;1"].
                   createInstance(Ci.nsIBinaryInputStream);
  },

  onStopRequest: function FW_IDUG_loadStopR(aRequest, aContext, aStatusCode) {
    var requestFailed = !Components.isSuccessCode(aStatusCode);
    if (!requestFailed && (aRequest instanceof Ci.nsIHttpChannel))
      requestFailed = !aRequest.requestSucceeded;

    if (!requestFailed && this._countRead != 0) {
      var str = String.fromCharCode.apply(null, this._bytes);
      try {
        var dataURI = ICON_DATAURL_PREFIX + btoa(str);
        this._element.setAttribute("image", dataURI);
      }
      catch(ex) {}
    }
    this._channel = null;
    this._element  = null;
  },

  
  onDataAvailable: function FW_IDUG_loadDAvailable(aRequest, aContext,
                                                   aInputStream, aOffset,
                                                   aCount) {
    this._stream.setInputStream(aInputStream);

    
    this._bytes = this._bytes.concat(this._stream.readByteArray(aCount));
    this._countRead += aCount;
  },

  
  onChannelRedirect: function FW_IDUG_loadCRedirect(aOldChannel, aNewChannel,
                                                    aFlags) {
    this._channel = aNewChannel;
  },

  
  getInterface: function FW_IDUG_load_GI(aIID) {
    return this.QueryInterface(aIID);
  },

  
  confirmUnknownIssuer: function FW_IDUG_load_CUI(aSocketInfo, aCert,
                                                  aCertAddType) {
    return false;
  },

  confirmMismatchDomain: function FW_IDUG_load_CMD(aSocketInfo, aTargetURL,
                                                   aCert) {
    return false;
  },

  confirmCertExpired: function FW_IDUG_load_CCE(aSocketInfo, aCert) {
    return false;
  },

  notifyCrlNextupdate: function FW_IDUG_load_NCN(aSocketInfo, aTargetURL, aCert) {
  },

  
  
  onRedirect: function (aChannel, aNewChannel) { },
  
  onProgress: function (aRequest, aContext, aProgress, aProgressMax) { },
  onStatus: function (aRequest, aContext, aStatus, aStatusArg) { }
};

var Module = {
  QueryInterface: function M_QueryInterface(iid) {
    if (iid.equals(Ci.nsIModule) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  getClassObject: function M_getClassObject(cm, cid, iid) {
    if (!iid.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;
    
    if (cid.equals(FW_CLASSID))
      return new GenericComponentFactory(FeedWriter);

    throw Cr.NS_ERROR_NO_INTERFACE;
  },
  
  registerSelf: function M_registerSelf(cm, file, location, type) {
    var cr = cm.QueryInterface(Ci.nsIComponentRegistrar);
    
    cr.registerFactoryLocation(FW_CLASSID, FW_CLASSNAME, FW_CONTRACTID,
                               file, location, type);
    
    var catman = 
        Cc["@mozilla.org/categorymanager;1"].
        getService(Ci.nsICategoryManager);
    catman.addCategoryEntry("JavaScript global constructor",
                            "BrowserFeedWriter", FW_CONTRACTID, true, true);
  },

  unregisterSelf: function M_unregisterSelf(cm, location, type) {
    var cr = cm.QueryInterface(Ci.nsIComponentRegistrar);
    cr.unregisterFactoryLocation(FW_CLASSID, location);
  },

  canUnload: function M_canUnload(cm) {
    return true;
  }
};

function NSGetModule(cm, file) {
  return Module;
}

#include ../../../../toolkit/content/debug.js
#include GenericFactory.js
