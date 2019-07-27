



'use strict';

this.EXPORTED_SYMBOLS = ['Keyboard'];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import('resource://gre/modules/Services.jsm');
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
  "@mozilla.org/parentprocessmessagemanager;1", "nsIMessageBroadcaster");

XPCOMUtils.defineLazyModuleGetter(this, "SystemAppProxy",
                                  "resource://gre/modules/SystemAppProxy.jsm");

XPCOMUtils.defineLazyGetter(this, "appsService", function() {
  return Cc["@mozilla.org/AppsService;1"].getService(Ci.nsIAppsService);
});

let Utils = {
  getMMFromMessage: function u_getMMFromMessage(msg) {
    let mm;
    try {
      mm = msg.target.QueryInterface(Ci.nsIFrameLoaderOwner)
                     .frameLoader.messageManager;
    } catch(e) {
      mm = msg.target;
    }

    return mm;
  },
  checkPermissionForMM: function u_checkPermissionForMM(mm, permName) {
    let testing = false;
    try {
      testing = Services.prefs.getBoolPref("dom.mozInputMethod.testing");
    } catch (e) { }

    if (testing) {
      return true;
    }

    return mm.assertPermission(permName);
  }
};

this.Keyboard = {
  _formMM: null,      
  _keyboardMM: null,  
  _keyboardID: -1,    
  _nextKeyboardID: 0, 
  _systemMessageName: [
    'SetValue', 'RemoveFocus', 'SetSelectedOption', 'SetSelectedOptions'
  ],

  _messageNames: [
    'RemoveFocus',
    'SetSelectionRange', 'ReplaceSurroundingText', 'ShowInputMethodPicker',
    'SwitchToNextInputMethod', 'HideInputMethod',
    'GetText', 'SendKey', 'GetContext',
    'SetComposition', 'EndComposition',
    'Register', 'Unregister'
  ],

  get formMM() {
    if (this._formMM && !Cu.isDeadWrapper(this._formMM))
      return this._formMM;

    return null;
  },

  set formMM(mm) {
    this._formMM = mm;
  },

  sendToForm: function(name, data) {
    try {
      this.formMM.sendAsyncMessage(name, data);
    } catch(e) { }
  },

  sendToKeyboard: function(name, data) {
    try {
      this._keyboardMM.sendAsyncMessage(name, data);
    } catch(e) { }
  },

  init: function keyboardInit() {
    Services.obs.addObserver(this, 'inprocess-browser-shown', false);
    Services.obs.addObserver(this, 'remote-browser-shown', false);
    Services.obs.addObserver(this, 'oop-frameloader-crashed', false);
    Services.obs.addObserver(this, 'message-manager-close', false);

    for (let name of this._messageNames) {
      ppmm.addMessageListener('Keyboard:' + name, this);
    }

    for (let name of this._systemMessageName) {
      ppmm.addMessageListener('System:' + name, this);
    }

    this.inputRegistryGlue = new InputRegistryGlue();
  },

  observe: function keyboardObserve(subject, topic, data) {
    let frameLoader = null;
    let mm = null;

    if (topic == 'message-manager-close') {
      mm = subject;
    } else {
      frameLoader = subject.QueryInterface(Ci.nsIFrameLoader);
      mm = frameLoader.messageManager;
    }

    if (topic == 'oop-frameloader-crashed' ||
	topic == 'message-manager-close') {
      if (this.formMM == mm) {
        
        
        this.sendToKeyboard('Keyboard:FocusChange', { 'type': 'blur' });
        
        SystemAppProxy.dispatchEvent({
          type: 'inputmethod-contextchange',
          inputType: 'blur'
        });
      }
    } else {
      
      if (!frameLoader.ownerIsBrowserOrAppFrame) {
        return;
      }
      this.initFormsFrameScript(mm);
    }
  },

  initFormsFrameScript: function(mm) {
    mm.addMessageListener('Forms:Input', this);
    mm.addMessageListener('Forms:SelectionChange', this);
    mm.addMessageListener('Forms:GetText:Result:OK', this);
    mm.addMessageListener('Forms:GetText:Result:Error', this);
    mm.addMessageListener('Forms:SetSelectionRange:Result:OK', this);
    mm.addMessageListener('Forms:SetSelectionRange:Result:Error', this);
    mm.addMessageListener('Forms:ReplaceSurroundingText:Result:OK', this);
    mm.addMessageListener('Forms:ReplaceSurroundingText:Result:Error', this);
    mm.addMessageListener('Forms:SendKey:Result:OK', this);
    mm.addMessageListener('Forms:SendKey:Result:Error', this);
    mm.addMessageListener('Forms:SequenceError', this);
    mm.addMessageListener('Forms:GetContext:Result:OK', this);
    mm.addMessageListener('Forms:SetComposition:Result:OK', this);
    mm.addMessageListener('Forms:EndComposition:Result:OK', this);
  },

  receiveMessage: function keyboardReceiveMessage(msg) {
    
    
    let mm;
    let isKeyboardRegistration = msg.name == "Keyboard:Register" ||
                                 msg.name == "Keyboard:Unregister";
    if (msg.name.indexOf("Keyboard:") === 0 ||
        msg.name.indexOf("System:") === 0) {
      if (!this.formMM && !isKeyboardRegistration) {
        return;
      }

      mm = Utils.getMMFromMessage(msg);

      
      if (!mm) {
        dump("!! No message manager found for " + msg.name);
        return;
      }

      let perm = (msg.name.indexOf("Keyboard:") === 0) ? "input"
                                                       : "input-manage";

      if (!isKeyboardRegistration && !Utils.checkPermissionForMM(mm, perm)) {
        dump("Keyboard message " + msg.name +
        " from a content process with no '" + perm + "' privileges.");
        return;
      }
    }

    
    
    
    let kbID = null;
    if ('kbID' in msg.data) {
      kbID = msg.data.kbID;
    }

    if (0 === msg.name.indexOf('Keyboard:') &&
        ('Keyboard:Register' !== msg.name && this._keyboardID !== kbID)
       ) {
      return;
    }

    switch (msg.name) {
      case 'Forms:Input':
        this.handleFocusChange(msg);
        break;
      case 'Forms:SelectionChange':
      case 'Forms:GetText:Result:OK':
      case 'Forms:GetText:Result:Error':
      case 'Forms:SetSelectionRange:Result:OK':
      case 'Forms:ReplaceSurroundingText:Result:OK':
      case 'Forms:SendKey:Result:OK':
      case 'Forms:SendKey:Result:Error':
      case 'Forms:SequenceError':
      case 'Forms:GetContext:Result:OK':
      case 'Forms:SetComposition:Result:OK':
      case 'Forms:EndComposition:Result:OK':
      case 'Forms:SetSelectionRange:Result:Error':
      case 'Forms:ReplaceSurroundingText:Result:Error':
        let name = msg.name.replace(/^Forms/, 'Keyboard');
        this.forwardEvent(name, msg);
        break;

      case 'System:SetValue':
        this.setValue(msg);
        break;
      case 'Keyboard:RemoveFocus':
      case 'System:RemoveFocus':
        this.removeFocus();
        break;
      case 'System:SetSelectedOption':
        this.setSelectedOption(msg);
        break;
      case 'System:SetSelectedOptions':
        this.setSelectedOption(msg);
        break;
      case 'Keyboard:SetSelectionRange':
        this.setSelectionRange(msg);
        break;
      case 'Keyboard:ReplaceSurroundingText':
        this.replaceSurroundingText(msg);
        break;
      case 'Keyboard:SwitchToNextInputMethod':
        this.switchToNextInputMethod();
        break;
      case 'Keyboard:ShowInputMethodPicker':
        this.showInputMethodPicker();
        break;
      case 'Keyboard:GetText':
        this.getText(msg);
        break;
      case 'Keyboard:SendKey':
        this.sendKey(msg);
        break;
      case 'Keyboard:GetContext':
        this.getContext(msg);
        break;
      case 'Keyboard:SetComposition':
        this.setComposition(msg);
        break;
      case 'Keyboard:EndComposition':
        this.endComposition(msg);
        break;
      case 'Keyboard:Register':
        this._keyboardMM = mm;
        if (kbID !== null) {
          
          
          this._keyboardID = kbID;
        }else{
          
          this._keyboardID = this._nextKeyboardID;
          this._nextKeyboardID++;
          
          
          return this._keyboardID;
        }
        break;
      case 'Keyboard:Unregister':
        this._keyboardMM = null;
        this._keyboardID = -1;
        break;
    }
  },

  forwardEvent: function keyboardForwardEvent(newEventName, msg) {
    let mm = msg.target.QueryInterface(Ci.nsIFrameLoaderOwner)
                .frameLoader.messageManager;
    if (newEventName === 'Keyboard:FocusChange') {
      if (msg.data.type !== 'blur') { 
        
        
        this.formMM = mm;
      } else { 
        
        
        
        
        if (mm !== this.formMM) {
          return false;
        }

        this.formMM = null;
      }
    }

    this.sendToKeyboard(newEventName, msg.data);
    return true;
  },

  handleFocusChange: function keyboardHandleFocusChange(msg) {
    let isSent = this.forwardEvent('Keyboard:FocusChange', msg);

    if (!isSent) {
      return;
    }

    
    
    SystemAppProxy.dispatchEvent({
      type: 'inputmethod-contextchange',
      inputType: msg.data.type,
      value: msg.data.value,
      choices: JSON.stringify(msg.data.choices),
      min: msg.data.min,
      max: msg.data.max
    });
  },

  setSelectedOption: function keyboardSetSelectedOption(msg) {
    this.sendToForm('Forms:Select:Choice', msg.data);
  },

  setSelectedOptions: function keyboardSetSelectedOptions(msg) {
    this.sendToForm('Forms:Select:Choice', msg.data);
  },

  setSelectionRange: function keyboardSetSelectionRange(msg) {
    this.sendToForm('Forms:SetSelectionRange', msg.data);
  },

  setValue: function keyboardSetValue(msg) {
    this.sendToForm('Forms:Input:Value', msg.data);
  },

  removeFocus: function keyboardRemoveFocus() {
    this.sendToForm('Forms:Select:Blur', {});
  },

  replaceSurroundingText: function keyboardReplaceSurroundingText(msg) {
    this.sendToForm('Forms:ReplaceSurroundingText', msg.data);
  },

  showInputMethodPicker: function keyboardShowInputMethodPicker() {
    SystemAppProxy.dispatchEvent({
      type: "inputmethod-showall"
    });
  },

  switchToNextInputMethod: function keyboardSwitchToNextInputMethod() {
    SystemAppProxy.dispatchEvent({
      type: "inputmethod-next"
    });
  },

  getText: function keyboardGetText(msg) {
    this.sendToForm('Forms:GetText', msg.data);
  },

  sendKey: function keyboardSendKey(msg) {
    this.sendToForm('Forms:Input:SendKey', msg.data);
  },

  getContext: function keyboardGetContext(msg) {
    if (this._layouts) {
      this.sendToKeyboard('Keyboard:LayoutsChange', this._layouts);
    }

    this.sendToForm('Forms:GetContext', msg.data);
  },

  setComposition: function keyboardSetComposition(msg) {
    this.sendToForm('Forms:SetComposition', msg.data);
  },

  endComposition: function keyboardEndComposition(msg) {
    this.sendToForm('Forms:EndComposition', msg.data);
  },

  


  _layouts: null,
  setLayouts: function keyboardSetLayoutCount(layouts) {
    
    
    
    this._layouts = layouts;

    this.sendToKeyboard('Keyboard:LayoutsChange', layouts);
  }
};

function InputRegistryGlue() {
  this._messageId = 0;
  this._msgMap = new Map();

  ppmm.addMessageListener('InputRegistry:Add', this);
  ppmm.addMessageListener('InputRegistry:Remove', this);
};

InputRegistryGlue.prototype.receiveMessage = function(msg) {
  let mm = Utils.getMMFromMessage(msg);

  if (!Utils.checkPermissionForMM(mm, 'input')) {
    dump("InputRegistryGlue message " + msg.name +
      " from a content process with no 'input' privileges.");
    return;
  }

  switch (msg.name) {
    case 'InputRegistry:Add':
      this.addInput(msg, mm);

      break;

    case 'InputRegistry:Remove':
      this.removeInput(msg, mm);

      break;
  }
};

InputRegistryGlue.prototype.addInput = function(msg, mm) {
  let msgId = this._messageId++;
  this._msgMap.set(msgId, {
    mm: mm,
    requestId: msg.data.requestId
  });

  let manifestURL = appsService.getManifestURLByLocalId(msg.data.appId);

  SystemAppProxy.dispatchEvent({
    type: 'inputregistry-add',
    id: msgId,
    manifestURL: manifestURL,
    inputId: msg.data.inputId,
    inputManifest: msg.data.inputManifest
  });
};

InputRegistryGlue.prototype.removeInput = function(msg, mm) {
  let msgId = this._messageId++;
  this._msgMap.set(msgId, {
    mm: mm,
    requestId: msg.data.requestId
  });

  let manifestURL = appsService.getManifestURLByLocalId(msg.data.appId);

  SystemAppProxy.dispatchEvent({
    type: 'inputregistry-remove',
    id: msgId,
    manifestURL: manifestURL,
    inputId: msg.data.inputId
  });
};

InputRegistryGlue.prototype.returnMessage = function(detail) {
  if (!this._msgMap.has(detail.id)) {
    return;
  }

  let { mm, requestId } = this._msgMap.get(detail.id);
  this._msgMap.delete(detail.id);

  if (Cu.isDeadWrapper(mm)) {
    return;
  }

  if (!('error' in detail)) {
    mm.sendAsyncMessage('InputRegistry:Result:OK', {
      requestId: requestId
    });
  } else {
    mm.sendAsyncMessage('InputRegistry:Result:Error', {
      error: detail.error,
      requestId: requestId
    });
  }
};

this.Keyboard.init();
