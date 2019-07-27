



"use strict";


const NOTIFICATIONS = [
    "ActionBar:UpdateState",
    "TextSelection:Action",
    "TextSelection:End",
];

const DEFER_INIT_DELAY_MS = 50; 
const PHONE_REGEX = /^\+?[0-9\s,-.\(\)*#pw]{1,30}$/; 






var ActionBarHandler = {
  
  START_TOUCH_ERROR: {
    NO_CONTENT_WINDOW: "No valid content Window found.",
    NONE: "",
  },

  _selectionID: null, 
  _actionBarActions: null, 

  


  observe: function(subject, topic, data) {
    switch (topic) {

      
      case "ActionBar:OpenNew": {
        
        this._uninit(false);
        this._init(data);
        break;
      }

      
      case "ActionBar:Close": {
        if (this._selectionID === data) {
          this._uninit(false);
        }
        break;
      }

      
      case "ActionBar:UpdateState": {
        this._sendActionBarActions();
        break;
      }

      
      case "TextSelection:Action": {
        for (let type in this.actions) {
          let action = this.actions[type];
          if (action.id == data) {
            action.action(this._targetElement, this._contentWindow);
            break;
          }
        }
        break;
      }

      
      case "TextSelection:Get": {
        Messaging.sendRequest({
          type: "TextSelection:Data",
          requestId: data,
          text: this._getSelectedText(),
        });
        break;
      }

      
      case "TextSelection:End": {
        
        if (this._selectionID === JSON.parse(data).selectionID) {
          this._uninit();
        }
        break;
      }
    }
  },

  


  _init: function(actionBarID) {
    let [element, win] = this._getSelectionTargets();
    if (!win) {
      return this.START_TOUCH_ERROR.NO_CONTENT_WINDOW;
    }

    
    this._selectionID = actionBarID;
    [this._targetElement, this._contentWindow] = [element, win];

    
    NOTIFICATIONS.forEach(notification => {
      Services.obs.addObserver(this, notification, false);
    });

    
    Messaging.sendRequest({
      type: "TextSelection:ActionbarInit",
      selectionID: this._selectionID,
    });
    this._sendActionBarActions(true);

    return this.START_TOUCH_ERROR.NONE;
  },

  



  _getSelectionTargets: function() {
    let [element, win] = [Services.focus.focusedElement, Services.focus.focusedWindow];
    if (!element) {
      
      return [null, win];
    }

    
    if (((element instanceof HTMLInputElement) && element.mozIsTextField(false)) ||
        (element instanceof HTMLTextAreaElement) ||
        element.isContentEditable) {
      return [element, win];
    }

    
    return [null, win];
  },

  




  _uninit: function(clearSelection = true) {
    
    if (!this._selectionID) {
      return;
    }

    
    NOTIFICATIONS.forEach(notification => {
      Services.obs.removeObserver(this, notification);
    });

    
    Messaging.sendRequest({
      type: "TextSelection:ActionbarUninit",
    });

    
    
    
    this._selectionID = null;

    
    
    
    if (clearSelection) {
      this._clearSelection();
    }
  },

  




  _clearSelection: function(element = this._targetElement, win = this._contentWindow) {
    
    if (element) {
      let imeSupport = this._getEditor(element, win).QueryInterface(Ci.nsIEditorIMESupport);
      if (imeSupport.composing) {
        imeSupport.forceCompositionEnd();
      }
      element.blur();
    }

    
    if (!element || element.isContentEditable) {
      this._getSelection().removeAllRanges();
    }
  },

  








  _sendActionBarActions: function(sendAlways) {
    let actions = this._getActionBarActions();

    if (sendAlways || actions !== this._actionBarActions) {
      Messaging.sendRequest({
        type: "TextSelection:ActionbarStatus",
        actions: actions,
      });
    }

    this._actionBarActions = actions;
  },

  


  _getActionBarActions: function(element = this._targetElement, win = this._contentWindow) {
    let actions = [];

    for (let type in this.actions) {
      let action = this.actions[type];
      if (action.selector.matches(element, win)) {
        let a = {
          id: action.id,
          label: this._getActionValue(action, "label", "", element),
          icon: this._getActionValue(action, "icon", "drawable://ic_status_logo", element),
          order: this._getActionValue(action, "order", 0, element),
          showAsAction: this._getActionValue(action, "showAsAction", true, element),
        };
        actions.push(a);
      }
    }
    actions.sort((a, b) => b.order - a.order);

    return actions;
  },

  




  _getActionValue: function(obj, name, defaultValue, element) {
    if (!(name in obj))
      return defaultValue;

    if (typeof obj[name] == "function")
      return obj[name](element);

    return obj[name];
  },

  


  actions: {

    SELECT_ALL: {
      id: "selectall_action",
      label: Strings.browser.GetStringFromName("contextmenu.selectAll"),
      icon: "drawable://ab_select_all",
      order: 5,

      selector: {
        matches: function(element, win) {
          
          
          return (element) ? element.textLength != 0 : true;
        },
      },

      action: function(element, win) {
        
        
        if (element) {
          
          
          let imeSupport = ActionBarHandler._getEditor(element, win).
            QueryInterface(Ci.nsIEditorIMESupport);
          if (imeSupport.composing) {
            element.blur();
            element.focus();
          }
        }

        
        ActionBarHandler._getSelectAllController(element, win).selectAll();
        ActionBarHandler._getSelectionController(element, win).
          selectionCaretsVisibility = true;

        UITelemetry.addEvent("action.1", "actionbar", null, "select_all");
      },
    },

    CUT: {
      id: "cut_action",
      label: Strings.browser.GetStringFromName("contextmenu.cut"),
      icon: "drawable://ab_cut",
      order: 4,

      selector: {
        matches: function(element, win) {
          
          if (!element) {
            return false;
          }
          
          if (element instanceof Ci.nsIDOMHTMLInputElement &&
              !element.mozIsTextField(true)) {
            return false;
          }
          
          if (element.disabled || element.readOnly) {
            return false;
          }
          
          return (ActionBarHandler._getSelectedText().length > 0);
        },
      },

      action: function(element, win) {
        
        let selectedText = ActionBarHandler._getSelectedText();
        let clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"].
          getService(Ci.nsIClipboardHelper);
        clipboard.copyString(selectedText, win.document);

        let msg = Strings.browser.GetStringFromName("selectionHelper.textCopied");
        NativeWindow.toast.show(msg, "short");

        
        ActionBarHandler._getSelection(element, win).deleteFromDocument();

        ActionBarHandler._uninit();
        UITelemetry.addEvent("action.1", "actionbar", null, "cut");
      },
    },

    COPY: {
      id: "copy_action",
      label: Strings.browser.GetStringFromName("contextmenu.copy"),
      icon: "drawable://ab_copy",
      order: 3,

      selector: {
        matches: function(element, win) {
          
          if (element instanceof Ci.nsIDOMHTMLInputElement &&
              !element.mozIsTextField(true)) {
            return false;
          }
          
          return (ActionBarHandler._getSelectedText().length > 0);
        },
      },

      action: function(element, win) {
        let selectedText = ActionBarHandler._getSelectedText();
        let clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"].
          getService(Ci.nsIClipboardHelper);
        clipboard.copyString(selectedText, win.document);

        let msg = Strings.browser.GetStringFromName("selectionHelper.textCopied");
        NativeWindow.toast.show(msg, "short");

        ActionBarHandler._uninit();
        UITelemetry.addEvent("action.1", "actionbar", null, "copy");
      },
    },

    PASTE: {
      id: "paste_action",
      label: Strings.browser.GetStringFromName("contextmenu.paste"),
      icon: "drawable://ab_paste",
      order: 2,

      selector: {
        matches: function(element, win) {
          
          if (!element) {
            return false;
          }
          
          if (element.disabled || element.readOnly) {
            return false;
          }
          
          let flavors = ["text/unicode"];
          return Services.clipboard.hasDataMatchingFlavors(flavors, flavors.length,
            Ci.nsIClipboard.kGlobalClipboard);
        },
      },

      action: function(element, win) {
        
        ActionBarHandler._getEditor(element, win).
          paste(Ci.nsIClipboard.kGlobalClipboard);
        ActionBarHandler._uninit();
        UITelemetry.addEvent("action.1", "actionbar", null, "paste");
      },
    },

    CALL: {
      id: "call_action",
      label: Strings.browser.GetStringFromName("contextmenu.call"),
      icon: "drawable://phone",
      order: 1,

      selector: {
        matches: function(element, win) {
          return (ActionBarHandler._getSelectedPhoneNumber() != null);
        },
      },

      action: function(element, win) {
        BrowserApp.loadURI("tel:" +
          ActionBarHandler._getSelectedPhoneNumber());

        ActionBarHandler._uninit();
        UITelemetry.addEvent("action.1", "actionbar", null, "call");
      },
    },

    SEARCH: {
      id: "search_action",
      label: Strings.browser.formatStringFromName("contextmenu.search",
        [Services.search.defaultEngine.name], 1),
      icon: "drawable://ab_search",
      order: 1,

      selector: {
        matches: function(element, win) {
          
          return (ActionBarHandler._getSelectedText().length > 0);
        },  
      },

      action: function(element, win) {
        let selectedText = ActionBarHandler._getSelectedText();
        ActionBarHandler._uninit();

        
        
        let searchSubmission = Services.search.defaultEngine.getSubmission(selectedText);
        let parent = BrowserApp.selectedTab;
        let isPrivate = PrivateBrowsingUtils.isBrowserPrivate(parent.browser);
        BrowserApp.addTab(searchSubmission.uri.spec,
          { parentId: parent.id,
            selected: true,
            isPrivate: isPrivate,
          }
        );

        UITelemetry.addEvent("action.1", "actionbar", null, "search");
      },
    },

    SEARCH_ADD: {
      id: "search_add_action",
      label: Strings.browser.GetStringFromName("contextmenu.addSearchEngine2"),
      icon: "drawable://ab_add_search_engine",
      order: 0,

      selector: {
        matches: function(element, win) {
          if(!(element instanceof HTMLInputElement)) {
            return false;
          }
          let form = element.form;
          if (!form || element.type == "password") {
            return false;
          }
          let method = form.method.toUpperCase();
          return (method == "GET" || method == "") ||
                 (form.enctype != "text/plain") && (form.enctype != "multipart/form-data");
        },
      },

      action: function(element, win) {
        UITelemetry.addEvent("action.1", "actionbar", null, "add_search_engine");
        SearchEngines.addEngine(element);
      },
    },

    SHARE: {
      id: "share_action",
      label: Strings.browser.GetStringFromName("contextmenu.share"),
      icon: "drawable://ic_menu_share",
      order: 0,

      selector: {
        matches: function(element, win) {
          if (!ParentalControls.isAllowed(ParentalControls.SHARE)) {
            return false;
          }
          
          return (ActionBarHandler._getSelectedText().length > 0);
        },
      },

      action: function(element, win) {
        Messaging.sendRequest({
          type: "Share:Text",
          text: ActionBarHandler._getSelectedText(),
        });

        ActionBarHandler._uninit();
        UITelemetry.addEvent("action.1", "actionbar", null, "share");
      },
    },
  },

  


  get _idService() {
    delete this._idService;
    return this._idService = Cc["@mozilla.org/uuid-generator;1"].
      getService(Ci.nsIUUIDGenerator);
  },

  



  get _targetElement() {
    if (this._targetElementRef)
      return this._targetElementRef.get();
    return null;
  },

  set _targetElement(element) {
    this._targetElementRef = Cu.getWeakReference(element);
  },

  



  get _contentWindow() {
    if (this._contentWindowRef)
      return this._contentWindowRef.get();
    return null;
  },

  set _contentWindow(aContentWindow) {
    this._contentWindowRef = Cu.getWeakReference(aContentWindow);
  },

  



  _getSelectedText: function() {
    
    
    if (!this._selectionID) {
      return "";
    }

    let selection = this._getSelection();

    
    if (this._targetElement instanceof Ci.nsIDOMHTMLTextAreaElement) {
      let flags = Ci.nsIDocumentEncoder.OutputPreformatted |
        Ci.nsIDocumentEncoder.OutputRaw;
      return selection.QueryInterface(Ci.nsISelectionPrivate).
        toStringWithFormat("text/plain", flags, 0);
    }

    
    return selection.toString().trim();
  },

  



  _getSelection: function(element = this._targetElement, win = this._contentWindow) {
    return (element instanceof Ci.nsIDOMNSEditableElement) ?
      this._getEditor(element).selection :
      win.getSelection();
  },

  


  _getEditor: function(element = this._targetElement, win = this._contentWindow) {
    if (element instanceof Ci.nsIDOMNSEditableElement) {
      return element.QueryInterface(Ci.nsIDOMNSEditableElement).editor;
    }

    return win.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIWebNavigation).
               QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIEditingSession).
               getEditorForWindow(win);
  },

  


  _getSelectionController: function(element = this._targetElement, win = this._contentWindow) {
    if (element instanceof Ci.nsIDOMNSEditableElement) {
      return this._getEditor(element, win).selectionController;
    }

    return win.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIWebNavigation).
               QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsISelectionDisplay).
               QueryInterface(Ci.nsISelectionController);
  },

  


  _getSelectAllController: function(element = this._targetElement, win = this._contentWindow) {
    let editor = this._getEditor(element, win);
    return (editor) ?
      editor : this._getSelectionController(element, win);
  },

  


  _getSelectedPhoneNumber: function() {
    let selectedText = this._getSelectedText().trim();
    return this._isPhoneNumber(selectedText) ?
      selectedText : null;
  },

  _isPhoneNumber: function(selectedText) {
    return (PHONE_REGEX.test(selectedText));
  },
};
