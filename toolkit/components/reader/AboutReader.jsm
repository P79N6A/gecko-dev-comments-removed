



"use strict";

let Ci = Components.interfaces, Cc = Components.classes, Cu = Components.utils;

this.EXPORTED_SYMBOLS = [ "AboutReader" ];

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Rect", "resource://gre/modules/Geometry.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task", "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UITelemetry", "resource://gre/modules/UITelemetry.jsm");

const READINGLIST_COMMAND_ID = "readingListSidebar";

let gStrings = Services.strings.createBundle("chrome://global/locale/aboutReader.properties");

let AboutReader = function(mm, win, articlePromise) {
  let url = this._getOriginalUrl(win);
  if (!(url.startsWith("http://") || url.startsWith("https://"))) {
    Cu.reportError("Only http:// and https:// URLs can be loaded in about:reader");
    win.location.href = "about:blank";
    return;
  }

  let doc = win.document;

  this._mm = mm;
  this._mm.addMessageListener("Reader:Added", this);
  this._mm.addMessageListener("Reader:Removed", this);
  this._mm.addMessageListener("Sidebar:VisibilityChange", this);
  this._mm.addMessageListener("ReadingList:VisibilityStatus", this);

  this._docRef = Cu.getWeakReference(doc);
  this._winRef = Cu.getWeakReference(win);

  this._article = null;

  if (articlePromise) {
    this._articlePromise = articlePromise;
  }

  this._headerElementRef = Cu.getWeakReference(doc.getElementById("reader-header"));
  this._domainElementRef = Cu.getWeakReference(doc.getElementById("reader-domain"));
  this._titleElementRef = Cu.getWeakReference(doc.getElementById("reader-title"));
  this._creditsElementRef = Cu.getWeakReference(doc.getElementById("reader-credits"));
  this._contentElementRef = Cu.getWeakReference(doc.getElementById("moz-reader-content"));
  this._toolbarElementRef = Cu.getWeakReference(doc.getElementById("reader-toolbar"));
  this._messageElementRef = Cu.getWeakReference(doc.getElementById("reader-message"));

  this._scrollOffset = win.pageYOffset;

  doc.addEventListener("click", this, false);

  win.addEventListener("unload", this, false);
  win.addEventListener("scroll", this, false);
  win.addEventListener("resize", this, false);

  doc.addEventListener("visibilitychange", this, false);

  this._setupStyleDropdown();
  this._setupButton("close-button", this._onReaderClose.bind(this), "aboutReader.toolbar.close");
  this._setupButton("share-button", this._onShare.bind(this), "aboutReader.toolbar.share");

  try {
    if (Services.prefs.getBoolPref("browser.readinglist.enabled")) {
      this._setupButton("toggle-button", this._onReaderToggle.bind(this, "button"), "aboutReader.toolbar.addToReadingList");
      this._setupButton("list-button", this._onList.bind(this), "aboutReader.toolbar.openReadingList");
      this._setupButton("remove-button", this._onReaderToggle.bind(this, "footer"),
        "aboutReader.footer.deleteThisArticle", "aboutReader.footer.deleteThisArticle");
      this._doc.getElementById("reader-footer").setAttribute('readinglist-enabled', "true");
    }
  } catch (e) {
    
  }

  let colorSchemeValues = JSON.parse(Services.prefs.getCharPref("reader.color_scheme.values"));
  let colorSchemeOptions = colorSchemeValues.map((value) => {
    return { name: gStrings.GetStringFromName("aboutReader.colorScheme." + value),
             value: value,
             itemClass: value + "-button" };
  });

  let colorScheme = Services.prefs.getCharPref("reader.color_scheme");
  this._setupSegmentedButton("color-scheme-buttons", colorSchemeOptions, colorScheme, this._setColorSchemePref.bind(this));
  this._setColorSchemePref(colorScheme);

  let fontTypeSample = gStrings.GetStringFromName("aboutReader.fontTypeSample");
  let fontTypeOptions = [
    { name: fontTypeSample,
      description: gStrings.GetStringFromName("aboutReader.fontType.sans-serif"),
      value: "sans-serif",
      itemClass: "sans-serif-button"
    },
    { name: fontTypeSample,
      description: gStrings.GetStringFromName("aboutReader.fontType.serif"),
      value: "serif",
      itemClass: "serif-button" },
  ];

  let fontType = Services.prefs.getCharPref("reader.font_type");
  this._setupSegmentedButton("font-type-buttons", fontTypeOptions, fontType, this._setFontType.bind(this));
  this._setFontType(fontType);

  this._setupFontSizeButtons();

  
  this._isReadingListItem = -1;
  this._updateToggleButton();

  
  this._updateListButton();

  this._loadArticle();
}

AboutReader.prototype = {
  _BLOCK_IMAGES_SELECTOR: ".content p > img:only-child, " +
                          ".content p > a:only-child > img:only-child, " +
                          ".content .wp-caption img, " +
                          ".content figure img",

  get _doc() {
    return this._docRef.get();
  },

  get _win() {
    return this._winRef.get();
  },

  get _headerElement() {
    return this._headerElementRef.get();
  },

  get _domainElement() {
    return this._domainElementRef.get();
  },

  get _titleElement() {
    return this._titleElementRef.get();
  },

  get _creditsElement() {
    return this._creditsElementRef.get();
  },

  get _contentElement() {
    return this._contentElementRef.get();
  },

  get _toolbarElement() {
    return this._toolbarElementRef.get();
  },

  get _messageElement() {
    return this._messageElementRef.get();
  },

  get _isToolbarVertical() {
    if (this._toolbarVertical !== undefined) {
      return this._toolbarVertical;
    }
    return this._toolbarVertical = Services.prefs.getBoolPref("reader.toolbar.vertical");
  },

  receiveMessage: function (message) {
    switch (message.name) {
      case "Reader:Added": {
        
        if (message.data.url == this._article.url) {
          if (this._isReadingListItem != 1) {
            this._isReadingListItem = 1;
            this._updateToggleButton();
          }
        }
        break;
      }
      case "Reader:Removed": {
        if (message.data.url == this._article.url) {
          if (this._isReadingListItem != 0) {
            this._isReadingListItem = 0;
            this._updateToggleButton();
          }
        }
        break;
      }

      
      
      case "Sidebar:VisibilityChange": {
        let data = message.data;
        this._updateListButtonStyle(data.isOpen && data.commandID === READINGLIST_COMMAND_ID);
        break;
      }

      
      case "ReadingList:VisibilityStatus": {
        this._updateListButtonStyle(message.data.isOpen);
        break;
      }
    }
  },

  handleEvent: function Reader_handleEvent(aEvent) {
    if (!aEvent.isTrusted)
      return;

    switch (aEvent.type) {
      case "click":
        let target = aEvent.target;
        while (target && target.id != "reader-popup")
          target = target.parentNode;
        if (!target)
          this._toggleToolbarVisibility();
        break;
      case "scroll":
        let isScrollingUp = this._scrollOffset > aEvent.pageY;
        this._setToolbarVisibility(isScrollingUp);
        this._scrollOffset = aEvent.pageY;
        break;
      case "resize":
        this._updateImageMargins();
        break;

      case "devicelight":
        this._handleDeviceLight(aEvent.value);
        break;

      case "visibilitychange":
        this._handleVisibilityChange();
        break;

      case "unload":
        this._mm.removeMessageListener("Reader:Added", this);
        this._mm.removeMessageListener("Reader:Removed", this);
        this._mm.removeMessageListener("Sidebar:VisibilityChange", this);
        this._mm.removeMessageListener("ReadingList:VisibilityStatus", this);
        this._windowUnloaded = true;
        break;
    }
  },

  _updateToggleButton: function Reader_updateToggleButton() {
    let button = this._doc.getElementById("toggle-button");

    if (this._isReadingListItem == 1) {
      button.classList.add("on");
      button.setAttribute("title", gStrings.GetStringFromName("aboutReader.toolbar.removeFromReadingList"));
    } else {
      button.classList.remove("on");
      button.setAttribute("title", gStrings.GetStringFromName("aboutReader.toolbar.addToReadingList"));
    }
    this._updateFooter();
  },

  _requestReadingListStatus: function Reader_requestReadingListStatus() {
    let handleListStatusData = (message) => {
      this._mm.removeMessageListener("Reader:ListStatusData", handleListStatusData);

      let args = message.data;
      if (args.url == this._article.url) {
        if (this._isReadingListItem != args.inReadingList) {
          let isInitialStateChange = (this._isReadingListItem == -1);
          this._isReadingListItem = args.inReadingList;
          this._updateToggleButton();

          
          if (isInitialStateChange) {
            
            this._win.setTimeout(() => this._setToolbarVisibility(true), 500);
          }
        }
      }
    };

    this._mm.addMessageListener("Reader:ListStatusData", handleListStatusData);
    this._mm.sendAsyncMessage("Reader:ListStatusRequest", { url: this._article.url });
  },

  _onReaderClose: function Reader_onToggle() {
    this._win.location.href = this._getOriginalUrl();
  },

  _onReaderToggle: function Reader_onToggle(aMethod) {
    if (!this._article)
      return;

    if (this._isReadingListItem == 0) {
      this._mm.sendAsyncMessage("Reader:AddToList", { article: this._article });
      UITelemetry.addEvent("save.1", aMethod, null, "reader");
    } else {
      this._mm.sendAsyncMessage("Reader:RemoveFromList", { url: this._article.url });
      UITelemetry.addEvent("unsave.1", aMethod, null, "reader");
    }
  },

  _onShare: function Reader_onShare() {
    if (!this._article)
      return;

    this._mm.sendAsyncMessage("Reader:Share", {
      url: this._article.url,
      title: this._article.title
    });
    UITelemetry.addEvent("share.1", "list", null);
  },

  



  _showListIntro: function() {
    this._mm.sendAsyncMessage("ReadingList:ShowIntro");
  },

  



  _onList: function() {
    this._mm.sendAsyncMessage("ReadingList:ToggleVisibility");
  },

  



  _updateListButton: function() {
    this._mm.sendAsyncMessage("ReadingList:GetVisibility");
  },

  






  _updateListButtonStyle: function(isVisible) {
    let classes = this._doc.getElementById("list-button").classList;
    if (isVisible) {
      classes.add("on");
      
      this._setButtonTip("list-button", "aboutReader.toolbar.closeReadingList");
    } else {
      classes.remove("on");
      
      this._setButtonTip("list-button", "aboutReader.toolbar.openReadingList");
    }
  },

  _setFontSize: function Reader_setFontSize(newFontSize) {
    let containerClasses = this._doc.getElementById("container").classList;

    if (this._fontSize > 0)
      containerClasses.remove("font-size" + this._fontSize);

    this._fontSize = newFontSize;
    containerClasses.add("font-size" + this._fontSize);

    this._mm.sendAsyncMessage("Reader:SetIntPref", {
      name: "reader.font_size",
      value: this._fontSize
    });
  },

  _setupFontSizeButtons: function() {
    const FONT_SIZE_MIN = 1;
    const FONT_SIZE_MAX = 9;

    
    let sampleText = this._doc.getElementById("font-size-sample");
    sampleText.textContent = gStrings.GetStringFromName("aboutReader.fontTypeSample");

    let currentSize = Services.prefs.getIntPref("reader.font_size");
    currentSize = Math.max(FONT_SIZE_MIN, Math.min(FONT_SIZE_MAX, currentSize));

    let plusButton = this._doc.getElementById("font-size-plus");
    let minusButton = this._doc.getElementById("font-size-minus");

    function updateControls() {
      if (currentSize === FONT_SIZE_MIN) {
        minusButton.setAttribute("disabled", true);
      } else {
        minusButton.removeAttribute("disabled");
      }
      if (currentSize === FONT_SIZE_MAX) {
        plusButton.setAttribute("disabled", true);
      } else {
        plusButton.removeAttribute("disabled");
      }
    }

    updateControls();
    this._setFontSize(currentSize);

    plusButton.addEventListener("click", (event) => {
      if (!event.isTrusted) {
        return;
      }
      event.stopPropagation();

      if (currentSize >= FONT_SIZE_MAX) {
        return;
      }

      currentSize++;
      updateControls();
      this._setFontSize(currentSize);
    }, true);

    minusButton.addEventListener("click", (event) => {
      if (!event.isTrusted) {
        return;
      }
      event.stopPropagation();

      if (currentSize <= FONT_SIZE_MIN) {
        return;
      }

      currentSize--;
      updateControls();
      this._setFontSize(currentSize);
    }, true);
  },

  _updateFooter: function RupdateFooter() {
    let footer = this._doc.getElementById("reader-footer");
    if (!this._article || this._isReadingListItem == 0 ||
        footer.getAttribute("readinglist-enabled") != "true") {
      footer.style.display = "none";
      return;
    }
    footer.style.display = null;
  },

  _handleDeviceLight: function Reader_handleDeviceLight(newLux) {
    
    let luxValuesSize = 10;
    
    this._luxValues.unshift(newLux);
    
    this._totalLux += newLux;

    
    if (this._luxValues.length < luxValuesSize) {
      
      if (this._luxValues.length == 1) {
        this._updateColorScheme(newLux);
      }
      return;
    }
    
    let averageLuxValue = this._totalLux/luxValuesSize;

    this._updateColorScheme(averageLuxValue);
    
    let oldLux = this._luxValues.pop();
    
    this._totalLux -= oldLux;
  },

  _handleVisibilityChange: function Reader_handleVisibilityChange() {
    
    if (this._doc.visibilityState === "visible") {
      this._updateListButton();
    }

    let colorScheme = Services.prefs.getCharPref("reader.color_scheme");
    if (colorScheme != "auto") {
      return;
    }

    
    this._enableAmbientLighting(!this._doc.hidden);
  },

  
  _enableAmbientLighting: function Reader_enableAmbientLighting(enable) {
    if (enable) {
      this._win.addEventListener("devicelight", this, false);
      this._luxValues = [];
      this._totalLux = 0;
    } else {
      this._win.removeEventListener("devicelight", this, false);
      delete this._luxValues;
      delete this._totalLux;
    }
  },

  _updateColorScheme: function Reader_updateColorScheme(luxValue) {
    
    let upperBoundDark = 50;
    
    let lowerBoundLight = 10;
    
    let colorChangeThreshold = 20;

    
    if ((this._colorScheme === "dark" && luxValue < upperBoundDark) ||
        (this._colorScheme === "light" && luxValue > lowerBoundLight))
      return;

    if (luxValue < colorChangeThreshold)
      this._setColorScheme("dark");
    else
      this._setColorScheme("light");
  },

  _setColorScheme: function Reader_setColorScheme(newColorScheme) {
    
    if (this._colorScheme === newColorScheme || newColorScheme === "auto")
      return;

    let bodyClasses = this._doc.body.classList;

    if (this._colorScheme)
      bodyClasses.remove(this._colorScheme);

    this._colorScheme = newColorScheme;
    bodyClasses.add(this._colorScheme);
  },

  
  
  _setColorSchemePref: function Reader_setColorSchemePref(colorSchemePref) {
    this._enableAmbientLighting(colorSchemePref === "auto");
    this._setColorScheme(colorSchemePref);

    this._mm.sendAsyncMessage("Reader:SetCharPref", {
      name: "reader.color_scheme",
      value: colorSchemePref
    });
  },

  _setFontType: function Reader_setFontType(newFontType) {
    if (this._fontType === newFontType)
      return;

    let bodyClasses = this._doc.body.classList;

    if (this._fontType)
      bodyClasses.remove(this._fontType);

    this._fontType = newFontType;
    bodyClasses.add(this._fontType);

    this._mm.sendAsyncMessage("Reader:SetCharPref", {
      name: "reader.font_type",
      value: this._fontType
    });
  },

  _getToolbarVisibility: function Reader_getToolbarVisibility() {
    return this._toolbarElement.hasAttribute("visible");
  },

  _setToolbarVisibility: function Reader_setToolbarVisibility(visible) {
    let dropdown = this._doc.getElementById("style-dropdown");
    dropdown.classList.remove("open");

    if (this._getToolbarVisibility() === visible) {
      return;
    }

    if (visible) {
      this._toolbarElement.setAttribute("visible", true);
    } else {
      this._toolbarElement.removeAttribute("visible");
    }
    this._setSystemUIVisibility(visible);

    if (!visible) {
      this._mm.sendAsyncMessage("Reader:ToolbarHidden");
    }
    this._updateFooter();
  },

  _toggleToolbarVisibility: function Reader_toggleToolbarVisibility() {
    this._setToolbarVisibility(!this._getToolbarVisibility());
  },

  _setSystemUIVisibility: function Reader_setSystemUIVisibility(visible) {
    this._mm.sendAsyncMessage("Reader:SystemUIVisibility", { visible: visible });
  },

  _loadArticle: Task.async(function* () {
    let url = this._getOriginalUrl();
    this._showProgressDelayed();

    let article;
    if (this._articlePromise) {
      article = yield this._articlePromise;
    } else {
      article = yield this._getArticle(url);
    }

    if (this._windowUnloaded) {
      return;
    }

    if (article && article.url == url) {
      this._showContent(article);
    } else if (this._articlePromise) {
      
      this._showError();
    } else {
      
      
      this._win.location.href = url;
    }
  }),

  _getArticle: function(url) {
    return new Promise((resolve, reject) => {
      let listener = (message) => {
        this._mm.removeMessageListener("Reader:ArticleData", listener);
        resolve(message.data.article);
      };
      this._mm.addMessageListener("Reader:ArticleData", listener);
      this._mm.sendAsyncMessage("Reader:ArticleGet", { url: url });
    });
  },

  _requestFavicon: function Reader_requestFavicon() {
    let handleFaviconReturn = (message) => {
      this._mm.removeMessageListener("Reader:FaviconReturn", handleFaviconReturn);
      this._loadFavicon(message.data.url, message.data.faviconUrl);
    };

    this._mm.addMessageListener("Reader:FaviconReturn", handleFaviconReturn);
    this._mm.sendAsyncMessage("Reader:FaviconRequest", { url: this._article.url });
  },

  _loadFavicon: function Reader_loadFavicon(url, faviconUrl) {
    if (this._article.url !== url)
      return;

    let doc = this._doc;

    let link = doc.createElement('link');
    link.rel = 'shortcut icon';
    link.href = faviconUrl;

    doc.getElementsByTagName('head')[0].appendChild(link);
  },

  _updateImageMargins: function Reader_updateImageMargins() {
    let windowWidth = this._win.innerWidth;
    let bodyWidth = this._doc.body.clientWidth;

    let setImageMargins = function(img) {
      
      if (img.naturalWidth >= windowWidth) {
        img.setAttribute("moz-reader-full-width", true);
      } else {
        img.removeAttribute("moz-reader-full-width");
      }

      
      if (img.naturalWidth >= bodyWidth/2) {
        img.setAttribute("moz-reader-center", true);
      } else {
        img.removeAttribute("moz-reader-center");
      }
    }

    let imgs = this._doc.querySelectorAll(this._BLOCK_IMAGES_SELECTOR);
    for (let i = imgs.length; --i >= 0;) {
      let img = imgs[i];

      if (img.naturalWidth > 0) {
        setImageMargins(img);
      } else {
        img.onload = function() {
          setImageMargins(img);
        }
      }
    }
  },

  _maybeSetTextDirection: function Read_maybeSetTextDirection(article){
    if(!article.dir)
      return;

    
    this._contentElement.setAttribute("dir", article.dir);
    this._headerElement.setAttribute("dir", article.dir);
  },

  _showError: function() {
    this._headerElement.style.display = "none";
    this._contentElement.style.display = "none";

    let errorMessage = gStrings.GetStringFromName("aboutReader.loadError");
    this._messageElement.textContent = errorMessage;
    this._messageElement.style.display = "block";

    this._doc.title = errorMessage;

    this._error = true;
  },

  
  _stripHost: function Reader_stripHost(host) {
    if (!host)
      return host;

    let start = 0;

    if (host.startsWith("www."))
      start = 4;
    else if (host.startsWith("m."))
      start = 2;
    else if (host.startsWith("mobile."))
      start = 7;

    return host.substring(start);
  },

  _showContent: function Reader_showContent(article) {
    this._messageElement.style.display = "none";

    this._article = article;

    this._domainElement.href = article.url;
    let articleUri = Services.io.newURI(article.url, null, null);
    this._domainElement.innerHTML = this._stripHost(articleUri.host);

    this._creditsElement.innerHTML = article.byline;

    this._titleElement.textContent = article.title;
    this._doc.title = article.title;

    this._headerElement.style.display = "block";

    let parserUtils = Cc["@mozilla.org/parserutils;1"].getService(Ci.nsIParserUtils);
    let contentFragment = parserUtils.parseFragment(article.content,
      Ci.nsIParserUtils.SanitizerDropForms | Ci.nsIParserUtils.SanitizerAllowStyle,
      false, articleUri, this._contentElement);
    this._contentElement.innerHTML = "";
    this._contentElement.appendChild(contentFragment);
    this._maybeSetTextDirection(article);

    this._contentElement.style.display = "block";
    this._updateImageMargins();
    this._requestReadingListStatus();

    this._showListIntro();
    this._requestFavicon();
    this._doc.body.classList.add("loaded");
  },

  _hideContent: function Reader_hideContent() {
    this._headerElement.style.display = "none";
    this._contentElement.style.display = "none";
  },

  _showProgressDelayed: function Reader_showProgressDelayed() {
    this._win.setTimeout(function() {
      
      
      
      if (this._article || this._windowUnloaded || this._error) {
        return;
      }

      this._headerElement.style.display = "none";
      this._contentElement.style.display = "none";

      this._messageElement.textContent = gStrings.GetStringFromName("aboutReader.loading");
      this._messageElement.style.display = "block";
    }.bind(this), 300);
  },

  


  _getOriginalUrl: function(win) {
    let url = win ? win.location.href : this._win.location.href;
    let searchParams = new URLSearchParams(url.split("?")[1]);
    if (!searchParams.has("url")) {
      Cu.reportError("Error finding original URL for about:reader URL: " + url);
      return url;
    }
    return decodeURIComponent(searchParams.get("url"));
  },

  _setupSegmentedButton: function Reader_setupSegmentedButton(id, options, initialValue, callback) {
    let doc = this._doc;
    let segmentedButton = doc.getElementById(id);

    for (let i = 0; i < options.length; i++) {
      let option = options[i];

      let item = doc.createElement("button");

      
      let span = doc.createElement("span");
      span.textContent = option.name;
      item.appendChild(span);

      if (option.itemClass !== undefined)
        item.classList.add(option.itemClass);

      if (option.description !== undefined) {
        let description = doc.createElement("div");
        description.textContent = option.description;
        item.appendChild(description);
      }

      segmentedButton.appendChild(item);

      item.addEventListener("click", function(aEvent) {
        if (!aEvent.isTrusted)
          return;

        aEvent.stopPropagation();

        
        
        UITelemetry.addEvent("action.1", "button", null, id);

        let items = segmentedButton.children;
        for (let j = items.length - 1; j >= 0; j--) {
          items[j].classList.remove("selected");
        }

        item.classList.add("selected");
        callback(option.value);
      }.bind(this), true);

      if (option.value === initialValue)
        item.classList.add("selected");
    }
  },

  _setupButton: function(id, callback, titleEntity, textEntity) {
    this._setButtonTip(id, titleEntity);

    let button = this._doc.getElementById(id);
    if (textEntity)
      button.textContent = gStrings.GetStringFromName(textEntity);
    button.removeAttribute("hidden");
    button.addEventListener("click", function(aEvent) {
      if (!aEvent.isTrusted)
        return;

      aEvent.stopPropagation();
      callback();
    }, true);
  },

  




  _setButtonTip: function(id, titleEntity) {
    let button = this._doc.getElementById(id);
    button.setAttribute("title", gStrings.GetStringFromName(titleEntity));
  },

  _setupStyleDropdown: function Reader_setupStyleDropdown() {
    let doc = this._doc;
    let win = this._win;

    let dropdown = doc.getElementById("style-dropdown");
    let dropdownToggle = dropdown.querySelector(".dropdown-toggle");
    let dropdownPopup = dropdown.querySelector(".dropdown-popup");

    
    
    function updatePopupPosition() {
      let toggleHeight = dropdownToggle.offsetHeight;
      let toggleTop = dropdownToggle.offsetTop;
      let popupTop = toggleTop - toggleHeight / 2;
      dropdownPopup.style.top = popupTop + "px";
    }

    if (this._isToolbarVertical) {
      win.addEventListener("resize", event => {
        if (!event.isTrusted)
          return;

        
        win.setTimeout(updatePopupPosition, 0);
      }, true);
    }

    dropdownToggle.setAttribute("title", gStrings.GetStringFromName("aboutReader.toolbar.typeControls"));
    dropdownToggle.addEventListener("click", event => {
      if (!event.isTrusted)
        return;

      event.stopPropagation();

      if (dropdown.classList.contains("open")) {
        dropdown.classList.remove("open");
      } else {
        dropdown.classList.add("open");
        if (this._isToolbarVertical) {
          updatePopupPosition();
        }
      }
    }, true);
  },
};
