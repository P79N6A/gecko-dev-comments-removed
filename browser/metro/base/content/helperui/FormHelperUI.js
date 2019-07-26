









const kBrowserFormZoomLevelMin = 0.8;
const kBrowserFormZoomLevelMax = 2.0;

var FormHelperUI = {
  _debugEvents: false,
  _currentBrowser: null,
  _currentElement: null,
  _currentCaretRect: null,
  _currentElementRect: null,
  _open: false,

  type: "form",

  init: function formHelperInit() {
    
    messageManager.addMessageListener("FormAssist:Show", this);
    messageManager.addMessageListener("FormAssist:Hide", this);
    messageManager.addMessageListener("FormAssist:Update", this);
    messageManager.addMessageListener("FormAssist:AutoComplete", this);

    
    let tabs = Elements.tabList;
    tabs.addEventListener("TabSelect", this, true);
    tabs.addEventListener("TabClose", this, true);
    Elements.browsers.addEventListener("URLChanged", this, true);
    Elements.browsers.addEventListener("SizeChanged", this, true);

    
    Elements.browsers.addEventListener("PanBegin", this, false);
    Elements.browsers.addEventListener("PanFinished", this, false);
  },

  



  show: function formHelperShow(aElement) {
    
    
    if (!InputSourceHelper.isPrecise && aElement.editable &&
        ContentAreaObserver.isKeyboardTransitioning) {
      this._waitForKeyboard(aElement);
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
      list: aElement.list,
      rect: aElement.rect
    };

    this._updateContainerForSelect(lastElement, this._currentElement);
    this._updatePopupsFor(this._currentElement);

    
    this._currentBrowser.scrollSync = false;

    this._open = true;
  },

  hide: function formHelperHide() {
    this._open = false;

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

  _onShowRequest: function _onShowRequest(aJsonMsg) {
    if (aJsonMsg.current.choices) {
      
      
      SelectHelperUI.show(aJsonMsg.current.choices, aJsonMsg.current.title,
                          aJsonMsg.current.rect);
    } else {
      this._currentBrowser = getBrowser();
      this._currentElementRect =
        Rect.fromRect(this._currentBrowser.rectBrowserToClient(aJsonMsg.current.rect));
      this.show(aJsonMsg.current);
    }
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
    let json = aMessage.json;

    switch (aMessage.name) {
      case "FormAssist:Show":
        this._onShowRequest(json);
        break;

      case "FormAssist:Hide":
        this.hide();
        break;

      case "FormAssist:AutoComplete":
        this.show(json.current);
        break;
    }
  },

  doAutoComplete: function formHelperDoAutoComplete(aData) {
    this._currentBrowser.messageManager.sendAsyncMessage("FormAssist:AutoComplete",
      { value: aData });
  },

  _updatePopupsFor: function _formHelperUpdatePopupsFor(aElement) {
    if (!this._updateSuggestionsFor(aElement)) {
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

  _waitForKeyboard: function formHelperWaitForKeyboard(aElement) {
    let self = this;
    window.addEventListener("KeyboardChanged", function(aEvent) {
      window.removeEventListener("KeyboardChanged", arguments.callee, false);
      self._currentBrowser.messageManager.sendAsyncMessage("FormAssist:Update", {});
    }, false);
  }
};

