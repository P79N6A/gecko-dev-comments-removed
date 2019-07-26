











const kBrowserFormZoomLevelMin = 0.8;
const kBrowserFormZoomLevelMax = 2.0;


const kPrefFormHelperEnabled = "formhelper.enabled";
const kPrefFormHelperMode = "formhelper.mode";
const kPrefFormHelperZoom = "formhelper.autozoom";
const kPrefFormHelperZoomCaret = "formhelper.autozoom.caret";

var FormHelperUI = {
  _debugEvents: false,
  _currentBrowser: null,
  _currentElement: null,
  _currentCaretRect: null,
  _currentElementRect: null,

  type: "form",

  get enabled() {
    return Services.prefs.getBoolPref(kPrefFormHelperEnabled);
  },

  init: function formHelperInit() {
    
    messageManager.addMessageListener("FormAssist:Show", this);
    messageManager.addMessageListener("FormAssist:Hide", this);
    messageManager.addMessageListener("FormAssist:Update", this);
    messageManager.addMessageListener("FormAssist:Resize", this);
    messageManager.addMessageListener("FormAssist:AutoComplete", this);
    messageManager.addMessageListener("FormAssist:ValidationMessage", this);

    
    let tabs = Elements.tabList;
    tabs.addEventListener("TabSelect", this, true);
    tabs.addEventListener("TabClose", this, true);
    Elements.browsers.addEventListener("URLChanged", this, true);
    Elements.browsers.addEventListener("SizeChanged", this, true);

    
    Elements.browsers.addEventListener("PanBegin", this, false);
    Elements.browsers.addEventListener("PanFinished", this, false);

    
    
    let mode = Services.prefs.getIntPref(kPrefFormHelperMode);
    let state = (mode == 2) ? false : !!mode;
    Services.prefs.setBoolPref(kPrefFormHelperEnabled, state);
  },

  



  show: function formHelperShow(aElement, aHasPrevious, aHasNext) {
    
    
    
    if (aElement.editable &&
        (MetroUtils.immersive && !ContentAreaObserver.isKeyboardOpened &&
         !InputSourceHelper.isPrecise)) {
      this._waitForKeyboard(aElement, aHasPrevious, aHasNext);
      return;
    }

    this._currentBrowser = Browser.selectedBrowser;
    this._currentCaretRect = null;

    let lastElement = this._currentElement || null;

    this._currentElement = {
      id: aElement.id,
      name: aElement.name,
      title: aElement.title,
      value: aElement.value,
      maxLength: aElement.maxLength,
      type: aElement.type,
      choices: aElement.choices,
      isAutocomplete: aElement.isAutocomplete,
      validationMessage: aElement.validationMessage,
      list: aElement.list,
      rect: aElement.rect
    };

    this._zoom(Rect.fromRect(aElement.rect), Rect.fromRect(aElement.caretRect));
    this._updateContainerForSelect(lastElement, this._currentElement);
    this._updatePopupsFor(this._currentElement);

    
    this._currentBrowser.scrollSync = false;
  },

  hide: function formHelperHide() {
    SelectHelperUI.hide();
    AutofillMenuUI.hide();

    
    if (this._currentBrowser)
      this._currentBrowser.scrollSync = true;

    
    this._currentElementRect = null;
    this._currentCaretRect = null;

    this._updateContainerForSelect(this._currentElement, null);
    if (this._currentBrowser)
      this._currentBrowser.messageManager.sendAsyncMessage("FormAssist:Closed", { });
  },

  



  handleEvent: function formHelperHandleEvent(aEvent) {
    if (this._debugEvents) Util.dumpLn(aEvent.type);

    if (!this._open)
      return;

    switch (aEvent.type) {
      case "TabSelect":
      case "TabClose":
      case "PanBegin":
      case "SizeChanged":
        this.hide();
        break;

      case "URLChanged":
        if (aEvent.detail && aEvent.target == getBrowser())
          this.hide();
        break;
    }
  },

  receiveMessage: function formHelperReceiveMessage(aMessage) {
    if (this._debugEvents) Util.dumpLn(aMessage.name);
    let allowedMessages = ["FormAssist:Show", "FormAssist:Hide",
                           "FormAssist:AutoComplete", "FormAssist:ValidationMessage"];
    if (!this._open && allowedMessages.indexOf(aMessage.name) == -1)
      return;

    let json = aMessage.json;
    switch (aMessage.name) {
      case "FormAssist:Show":
        
        
        
        if (this.enabled) {
          this.show(json.current, json.hasPrevious, json.hasNext)
        } else if (json.current.choices) {
          SelectHelperUI.show(json.current.choices, json.current.title, json.current.rect);
        } else {
          this._currentElementRect = Rect.fromRect(json.current.rect);
          this._currentBrowser = getBrowser();
          this._updatePopupsFor(json.current);
        }
        break;

      case "FormAssist:Hide":
        if (this.enabled) {
          this.hide();
        }
        break;

      case "FormAssist:Resize":
        if (!ContentAreaObserver.isKeyboardOpened)
          return;

        let element = json.current;
        this._zoom(Rect.fromRect(element.rect), Rect.fromRect(element.caretRect));
        break;

      case "FormAssist:ValidationMessage":
        this._updatePopupsFor(json.current, { fromInput: true });
        break;

      case "FormAssist:AutoComplete":
        this._updatePopupsFor(json.current, { fromInput: true });
        break;

       case "FormAssist:Update":
        if (!ContentAreaObserver.isKeyboardOpened)
          return;
        
        break;
    }
  },

  doAutoComplete: function formHelperDoAutoComplete(aData) {
    this._currentBrowser.messageManager.sendAsyncMessage("FormAssist:AutoComplete",
      { value: aData });
  },

  get _open() {
    
    return true;
  },

  



  _updatePopupsFor: function _formHelperUpdatePopupsFor(aElement, options) {
    options = options || {};

    let fromInput = 'fromInput' in options && options.fromInput;

    
    
    
    
    
    
    
    
    let noPopupsShown = fromInput ?
                        (!this._updateSuggestionsFor(aElement) &&
                         !this._updateFormValidationFor(aElement)) :
                        (!this._updateFormValidationFor(aElement) &&
                         !this._updateSuggestionsFor(aElement));

    if (noPopupsShown) {
      AutofillMenuUI.hide();
    }
  },

  


  _updateSuggestionsFor: function _formHelperUpdateSuggestionsFor(aElement) {
    let suggestions = this._getAutocompleteSuggestions(aElement);
    if (!suggestions.length)
      return false;

    
    
    








    
    AutofillMenuUI.show(this._currentElementRect, suggestions);
    return true;
  },

  _updateFormValidationFor: function _formHelperUpdateFormValidationFor(aElement) {
    if (!aElement.validationMessage)
      return false;
    



















    return false;
  },

  


  _getAutocompleteSuggestions: function _formHelperGetAutocompleteSuggestions(aElement) {
    if (!aElement.isAutocomplete) {
      return [];
    }

    let suggestions = [];

    let autocompleteService = Cc["@mozilla.org/satchel/form-autocomplete;1"].getService(Ci.nsIFormAutoComplete);
    let results = autocompleteService.autoCompleteSearch(aElement.name || aElement.id, aElement.value, aElement, null);
    if (results.matchCount > 0) {
      for (let i = 0; i < results.matchCount; i++) {
        let value = results.getValueAt(i);

        
        if (value == aElement.value)
          continue;

        suggestions.push({ "label": value, "value": value});
      }
    }

    
    
    let options = aElement.list;
    for (let i = 0; i < options.length; i++)
      suggestions.push(options[i]);

    return suggestions;
  },

  


  _updateContainerForSelect: function _formHelperUpdateContainerForSelect(aLastElement, aCurrentElement) {
    let lastHasChoices = aLastElement && (aLastElement.choices != null);
    let currentHasChoices = aCurrentElement && (aCurrentElement.choices != null);

    if (currentHasChoices)
      SelectHelperUI.show(aCurrentElement.choices, aCurrentElement.title, aCurrentElement.rect);
    else if (lastHasChoices)
      SelectHelperUI.hide();
  },

  


  _zoom: function _formHelperZoom(aElementRect, aCaretRect) {
    let browser = getBrowser();
    let zoomRect = Rect.fromRect(browser.getBoundingClientRect());

    this._currentElementRect = aElementRect;

    
    let autozoomEnabled = Services.prefs.getBoolPref(kPrefFormHelperZoom);
    if (aElementRect && Browser.selectedTab.allowZoom && autozoomEnabled) {
      this._currentElementRect = aElementRect;

      
      let zoomLevel = Browser.selectedTab.clampZoomLevel(this._getZoomLevelForRect(aElementRect));

      zoomRect = Browser._getZoomRectForPoint(aElementRect.center().x, aElementRect.y, zoomLevel);
      AnimatedZoom.animateTo(zoomRect);
    } else if (aElementRect && !Browser.selectedTab.allowZoom && autozoomEnabled) {
      this._currentElementRect = aElementRect;

      
      
      zoomRect = Browser._getZoomRectForPoint(aElementRect.center().x, aElementRect.y, browser.scale);
      AnimatedZoom.animateTo(zoomRect);
    }

    this._ensureCaretVisible(aCaretRect);
  },

  _ensureCaretVisible: function _ensureCaretVisible(aCaretRect) {
    if (!aCaretRect || !Services.prefs.getBoolPref(kPrefFormHelperZoomCaret))
      return;

    
    
    if (AnimatedZoom.isZooming()) {
      let self = this;
      this._waitForZoom(function() {
        self._ensureCaretVisible(aCaretRect);
      });
      return;
    }

    let browser = getBrowser();
    let zoomRect = Rect.fromRect(browser.getBoundingClientRect());

    this._currentCaretRect = aCaretRect;
    let caretRect = aCaretRect.clone().scale(browser.scale, browser.scale);

    let scroll = browser.getRootView().getPosition();
    zoomRect = new Rect(scroll.x, scroll.y, zoomRect.width, zoomRect.height);
    if (zoomRect.contains(caretRect))
      return;

    let [deltaX, deltaY] = this._getOffsetForCaret(caretRect, zoomRect);
    if (deltaX != 0 || deltaY != 0) {
      let view = browser.getRootView();
      view.scrollBy(deltaX, deltaY);
    }
  },

  _waitForZoom: function _formHelperWaitForZoom(aCallback) {
    let currentElement = this._currentElement;
    let self = this;
    window.addEventListener("AnimatedZoomEnd", function() {
      window.removeEventListener("AnimatedZoomEnd", arguments.callee, true);
      
      if (self._currentElement != currentElement)
        return;

      aCallback();
    }, true);
  },

  _getZoomLevelForRect: function _getZoomLevelForRect(aRect) {
    const margin = 30;
    let zoomLevel = getBrowser().getBoundingClientRect().width / (aRect.width + margin);

    
    let defaultZoomLevel = Browser.selectedTab.getDefaultZoomLevel();
    return Util.clamp(zoomLevel, (defaultZoomLevel * kBrowserFormZoomLevelMin),
                                 (defaultZoomLevel * kBrowserFormZoomLevelMax));
  },

  _getOffsetForCaret: function _formHelperGetOffsetForCaret(aCaretRect, aRect) {
    
    let deltaX = 0;
    if (aCaretRect.right > aRect.right)
      deltaX = aCaretRect.right - aRect.right;
    if (aCaretRect.left < aRect.left)
      deltaX = aCaretRect.left - aRect.left;

    
    let deltaY = 0;
    if (aCaretRect.bottom > aRect.bottom)
      deltaY = aCaretRect.bottom - aRect.bottom;
    if (aCaretRect.top < aRect.top)
      deltaY = aCaretRect.top - aRect.top;

    return [deltaX, deltaY];
  },

  _waitForKeyboard: function formHelperWaitForKeyboard(aElement, aHasPrevious, aHasNext) {
    let self = this;
    window.addEventListener("KeyboardChanged", function(aEvent) {
      window.removeEventListener("KeyboardChanged", arguments.callee, false);

      if (AnimatedZoom.isZooming()) {
        self._waitForZoom(function() {
          self.show(aElement, aHasPrevious, aHasNext);
        });
        return;
      }

      self.show(aElement, aHasPrevious, aHasNext);
    }, false);
  }
};

