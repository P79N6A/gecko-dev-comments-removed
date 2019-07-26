



'use strict';

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

this.EXPORTED_SYMBOLS = ['AccessFu'];

Cu.import('resource://gre/modules/Services.jsm');

Cu.import('resource://gre/modules/accessibility/Utils.jsm');

const ACCESSFU_DISABLE = 0;
const ACCESSFU_ENABLE = 1;
const ACCESSFU_AUTO = 2;

this.AccessFu = {
  




  attach: function attach(aWindow) {
    Utils.init(aWindow);

    try {
      Cc['@mozilla.org/android/bridge;1'].
        getService(Ci.nsIAndroidBridge).handleGeckoMessage(
          JSON.stringify({ type: 'Accessibility:Ready' }));
      Services.obs.addObserver(this, 'Accessibility:Settings', false);
    } catch (x) {
      
      if (Utils.MozBuildApp === 'b2g') {
        aWindow.addEventListener('ContentStart', this, false);
      }
    }

    this._activatePref = new PrefCache(
      'accessibility.accessfu.activate', this._enableOrDisable.bind(this));

    this._enableOrDisable();
  },

  


  detach: function detach() {
    
    if (this._enabled) {
      this._disable();
    }
    if (Utils.MozBuildApp === 'mobile/android') {
      Services.obs.removeObserver(this, 'Accessibility:Settings');
    } else if (Utils.MozBuildApp === 'b2g') {
      Utils.win.shell.contentBrowser.contentWindow.removeEventListener(
        'mozContentEvent', this);
      Utils.win.removeEventListener('ContentStart', this);
    }
    delete this._activatePref;
    Utils.uninit();
  },

  



  _enable: function _enable() {
    if (this._enabled)
      return;
    this._enabled = true;

    Cu.import('resource://gre/modules/accessibility/Utils.jsm');
    Cu.import('resource://gre/modules/accessibility/TouchAdapter.jsm');
    Cu.import('resource://gre/modules/accessibility/Presentation.jsm');

    Logger.info('enable');

    for each (let mm in Utils.AllMessageManagers) {
      this._addMessageListeners(mm);
      this._loadFrameScript(mm);
    }

    
    let stylesheetURL = 'chrome://global/content/accessibility/AccessFu.css';
    let stylesheet = Utils.win.document.createProcessingInstruction(
      'xml-stylesheet', 'href="' + stylesheetURL + '" type="text/css"');
    Utils.win.document.insertBefore(stylesheet, Utils.win.document.firstChild);
    this.stylesheet = Cu.getWeakReference(stylesheet);


    
    this._quicknavModesPref =
      new PrefCache(
        'accessibility.accessfu.quicknav_modes',
        (aName, aValue) => {
          this.Input.quickNavMode.updateModes(aValue);
        }, true);

    
    this._notifyOutputPref =
      new PrefCache('accessibility.accessfu.notify_output');


    this.Input.start();
    Output.start();
    TouchAdapter.start();

    Services.obs.addObserver(this, 'remote-browser-frame-shown', false);
    Services.obs.addObserver(this, 'in-process-browser-or-app-frame-shown', false);
    Services.obs.addObserver(this, 'Accessibility:NextObject', false);
    Services.obs.addObserver(this, 'Accessibility:PreviousObject', false);
    Services.obs.addObserver(this, 'Accessibility:Focus', false);
    Services.obs.addObserver(this, 'Accessibility:ActivateObject', false);
    Services.obs.addObserver(this, 'Accessibility:MoveCaret', false);
    Utils.win.addEventListener('TabOpen', this);
    Utils.win.addEventListener('TabClose', this);
    Utils.win.addEventListener('TabSelect', this);

    if (this.readyCallback) {
      this.readyCallback();
      delete this.readyCallback;
    }
  },

  


  _disable: function _disable() {
    if (!this._enabled)
      return;

    this._enabled = false;

    Logger.info('disable');

    Utils.win.document.removeChild(this.stylesheet.get());

    for each (let mm in Utils.AllMessageManagers) {
      mm.sendAsyncMessage('AccessFu:Stop');
      this._removeMessageListeners(mm);
    }

    this.Input.stop();
    Output.stop();
    TouchAdapter.stop();

    Utils.win.removeEventListener('TabOpen', this);
    Utils.win.removeEventListener('TabClose', this);
    Utils.win.removeEventListener('TabSelect', this);

    Services.obs.removeObserver(this, 'remote-browser-frame-shown');
    Services.obs.removeObserver(this, 'in-process-browser-or-app-frame-shown');
    Services.obs.removeObserver(this, 'Accessibility:NextObject');
    Services.obs.removeObserver(this, 'Accessibility:PreviousObject');
    Services.obs.removeObserver(this, 'Accessibility:Focus');
    Services.obs.removeObserver(this, 'Accessibility:ActivateObject');
    Services.obs.removeObserver(this, 'Accessibility:MoveCaret');

    if (this.doneCallback) {
      this.doneCallback();
      delete this.doneCallback;
    }
  },

  _enableOrDisable: function _enableOrDisable() {
    try {
      let activatePref = this._activatePref.value;
      if (activatePref == ACCESSFU_ENABLE ||
          this._systemPref && activatePref == ACCESSFU_AUTO)
        this._enable();
      else
        this._disable();
    } catch (x) {
      dump('Error ' + x.message + ' ' + x.fileName + ':' + x.lineNumber);
    }
  },

  receiveMessage: function receiveMessage(aMessage) {
    if (Logger.logLevel >= Logger.DEBUG)
      Logger.debug('Recieved', aMessage.name, JSON.stringify(aMessage.json));

    switch (aMessage.name) {
      case 'AccessFu:Ready':
        let mm = Utils.getMessageManager(aMessage.target);
        if (this._enabled) {
          mm.sendAsyncMessage('AccessFu:Start',
                              {method: 'start', buildApp: Utils.MozBuildApp});
        }
        break;
      case 'AccessFu:Present':
        this._output(aMessage.json, aMessage.target);
        break;
      case 'AccessFu:Input':
        this.Input.setEditState(aMessage.json);
        break;
      case 'AccessFu:ActivateContextMenu':
        this.Input.activateContextMenu(aMessage.json);
        break;
    }
  },

  _output: function _output(aPresentationData, aBrowser) {
    for each (let presenter in aPresentationData) {
      if (!presenter)
        continue;

      try {
        Output[presenter.type](presenter.details, aBrowser);
      } catch (x) {
        Logger.logException(x);
      }
    }

    if (this._notifyOutputPref.value) {
      Services.obs.notifyObservers(null, 'accessfu-output',
                                   JSON.stringify(aPresentationData));
    }
  },

  _loadFrameScript: function _loadFrameScript(aMessageManager) {
    if (this._processedMessageManagers.indexOf(aMessageManager) < 0) {
      aMessageManager.loadFrameScript(
        'chrome://global/content/accessibility/content-script.js', true);
      this._processedMessageManagers.push(aMessageManager);
    } else if (this._enabled) {
      
      
      aMessageManager.sendAsyncMessage('AccessFu:Start',
        {method: 'start', buildApp: Utils.MozBuildApp});
    }
  },

  _addMessageListeners: function _addMessageListeners(aMessageManager) {
    aMessageManager.addMessageListener('AccessFu:Present', this);
    aMessageManager.addMessageListener('AccessFu:Input', this);
    aMessageManager.addMessageListener('AccessFu:Ready', this);
    aMessageManager.addMessageListener('AccessFu:ActivateContextMenu', this);
  },

  _removeMessageListeners: function _removeMessageListeners(aMessageManager) {
    aMessageManager.removeMessageListener('AccessFu:Present', this);
    aMessageManager.removeMessageListener('AccessFu:Input', this);
    aMessageManager.removeMessageListener('AccessFu:Ready', this);
    aMessageManager.removeMessageListener('AccessFu:ActivateContextMenu', this);
  },

  _handleMessageManager: function _handleMessageManager(aMessageManager) {
    if (this._enabled) {
      this._addMessageListeners(aMessageManager);
    }
    this._loadFrameScript(aMessageManager);
  },

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case 'Accessibility:Settings':
        this._systemPref = JSON.parse(aData).enabled;
        this._enableOrDisable();
        break;
      case 'Accessibility:NextObject':
        this.Input.moveCursor('moveNext', 'Simple', 'gesture');
        break;
      case 'Accessibility:PreviousObject':
        this.Input.moveCursor('movePrevious', 'Simple', 'gesture');
        break;
      case 'Accessibility:ActivateObject':
        this.Input.activateCurrent(JSON.parse(aData));
        break;
      case 'Accessibility:Focus':
        this._focused = JSON.parse(aData);
        if (this._focused) {
          this.showCurrent(true);
        }
        break;
      case 'Accessibility:MoveCaret':
        this.Input.moveCaret(JSON.parse(aData));
        break;
      case 'remote-browser-frame-shown':
      case 'in-process-browser-or-app-frame-shown':
      {
        let mm = aSubject.QueryInterface(Ci.nsIFrameLoader).messageManager;
        this._handleMessageManager(mm);
        break;
      }
    }
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case 'ContentStart':
      {
        Utils.win.shell.contentBrowser.contentWindow.addEventListener(
          'mozContentEvent', this, false, true);
        break;
      }
      case 'mozContentEvent':
      {
        if (aEvent.detail.type == 'accessibility-screenreader') {
          this._systemPref = aEvent.detail.enabled;
          this._enableOrDisable();
        }
        break;
      }
      case 'TabOpen':
      {
        let mm = Utils.getMessageManager(aEvent.target);
        this._handleMessageManager(mm);
        break;
      }
      case 'TabClose':
      {
        let mm = Utils.getMessageManager(aEvent.target);
        let mmIndex = this._processedMessageManagers.indexOf(mm);
        if (mmIndex > -1) {
          this._removeMessageListeners(mm);
          this._processedMessageManagers.splice(mmIndex, 1);
        }
        break;
      }
      case 'TabSelect':
      {
        if (this._focused) {
          
          
          
          Utils.win.setTimeout(
            function () {
              this.showCurrent(false);
            }.bind(this), 500);
        }
        break;
      }
    }
  },

  showCurrent: function showCurrent(aMove) {
    let mm = Utils.getMessageManager(Utils.CurrentBrowser);
    mm.sendAsyncMessage('AccessFu:ShowCurrent', { move: aMove });
  },

  announce: function announce(aAnnouncement) {
    this._output(Presentation.announce(aAnnouncement),
                 Utils.CurrentBrowser);
  },

  
  _enabled: false,

  
  _focused: false,

  
  
  _processedMessageManagers: [],

  









  adjustContentBounds: function(aJsonBounds, aBrowser, aToCSSPixels, aFromDevicePixels) {
    let bounds = new Rect(aJsonBounds.left, aJsonBounds.top,
                          aJsonBounds.right - aJsonBounds.left,
                          aJsonBounds.bottom - aJsonBounds.top);
    let win = Utils.win;
    let dpr = win.devicePixelRatio;
    let vp = Utils.getViewport(win);
    let offset = { left: -win.mozInnerScreenX, top: -win.mozInnerScreenY };

    if (!aBrowser.contentWindow) {
      
      
      let clientRect = aBrowser.getBoundingClientRect();
      let win = aBrowser.ownerDocument.defaultView;
      offset.left += clientRect.left + win.mozInnerScreenX;
      offset.top += clientRect.top + win.mozInnerScreenY;
    }

    
    
    
    
    if (!aFromDevicePixels && vp) {
      bounds = bounds.scale(vp.zoom / dpr, vp.zoom / dpr);
    }

    
    
    bounds = bounds.translate(offset.left * dpr, offset.top * dpr);

    
    
    if (aToCSSPixels) {
      bounds = bounds.scale(1 / dpr, 1 / dpr);
    }

    return bounds.expandToIntegers();
  }
};

var Output = {
  brailleState: {
    startOffset: 0,
    endOffset: 0,
    text: '',
    selectionStart: 0,
    selectionEnd: 0,

    init: function init(aOutput) {
      if (aOutput && 'output' in aOutput) {
        this.startOffset = aOutput.startOffset;
        this.endOffset = aOutput.endOffset;
        
        
        this.text = aOutput.output + ' ';
        this.selectionStart = typeof aOutput.selectionStart === 'number' ?
                              aOutput.selectionStart : this.selectionStart;
        this.selectionEnd = typeof aOutput.selectionEnd === 'number' ?
                            aOutput.selectionEnd : this.selectionEnd;

        return { text: this.text,
                 selectionStart: this.selectionStart,
                 selectionEnd: this.selectionEnd };
      }

      return null;
    },

    adjustText: function adjustText(aText) {
      let newBraille = [];
      let braille = {};

      let prefix = this.text.substring(0, this.startOffset).trim();
      if (prefix) {
        prefix += ' ';
        newBraille.push(prefix);
      }

      newBraille.push(aText);

      let suffix = this.text.substring(this.endOffset).trim();
      if (suffix) {
        suffix = ' ' + suffix;
        newBraille.push(suffix);
      }

      this.startOffset = braille.startOffset = prefix.length;
      this.text = braille.text = newBraille.join('') + ' ';
      this.endOffset = braille.endOffset = braille.text.length - suffix.length;
      braille.selectionStart = this.selectionStart;
      braille.selectionEnd = this.selectionEnd;

      return braille;
    },

    adjustSelection: function adjustSelection(aSelection) {
      let braille = {};

      braille.startOffset = this.startOffset;
      braille.endOffset = this.endOffset;
      braille.text = this.text;
      this.selectionStart = braille.selectionStart = aSelection.selectionStart + this.startOffset;
      this.selectionEnd = braille.selectionEnd = aSelection.selectionEnd + this.startOffset;

      return braille;
    }
  },

  start: function start() {
    Cu.import('resource://gre/modules/Geometry.jsm');
  },

  stop: function stop() {
    if (this.highlightBox) {
      Utils.win.document.documentElement.removeChild(this.highlightBox.get());
      delete this.highlightBox;
    }

    if (this.announceBox) {
      Utils.win.document.documentElement.removeChild(this.announceBox.get());
      delete this.announceBox;
    }
  },

  Speech: function Speech(aDetails, aBrowser) {
    for each (let action in aDetails.actions)
      Logger.info('tts.' + action.method, '"' + action.data + '"', JSON.stringify(action.options));
  },

  Visual: function Visual(aDetails, aBrowser) {
    switch (aDetails.method) {
      case 'showBounds':
      {
        let highlightBox = null;
        if (!this.highlightBox) {
          
          highlightBox = Utils.win.document.
            createElementNS('http://www.w3.org/1999/xhtml', 'div');
          Utils.win.document.documentElement.appendChild(highlightBox);
          highlightBox.id = 'virtual-cursor-box';

          
          let inset = Utils.win.document.
            createElementNS('http://www.w3.org/1999/xhtml', 'div');
          inset.id = 'virtual-cursor-inset';

          highlightBox.appendChild(inset);
          this.highlightBox = Cu.getWeakReference(highlightBox);
        } else {
          highlightBox = this.highlightBox.get();
        }

        let padding = aDetails.padding;
        let r = AccessFu.adjustContentBounds(aDetails.bounds, aBrowser, true);

        
        highlightBox.style.display = 'none';
        highlightBox.style.top = (r.top - padding) + 'px';
        highlightBox.style.left = (r.left - padding) + 'px';
        highlightBox.style.width = (r.width + padding*2) + 'px';
        highlightBox.style.height = (r.height + padding*2) + 'px';
        highlightBox.style.display = 'block';

        break;
      }
      case 'hideBounds':
      {
        let highlightBox = this.highlightBox ? this.highlightBox.get() : null;
        if (highlightBox)
          highlightBox.style.display = 'none';
        break;
      }
      case 'showAnnouncement':
      {
        let announceBox = this.announceBox ? this.announceBox.get() : null;
        if (!announceBox) {
          announceBox = Utils.win.document.
            createElementNS('http://www.w3.org/1999/xhtml', 'div');
          announceBox.id = 'announce-box';
          Utils.win.document.documentElement.appendChild(announceBox);
          this.announceBox = Cu.getWeakReference(announceBox);
        }

        announceBox.innerHTML = '<div>' + aDetails.text + '</div>';
        announceBox.classList.add('showing');

        if (this._announceHideTimeout)
          Utils.win.clearTimeout(this._announceHideTimeout);

        if (aDetails.duration > 0)
          this._announceHideTimeout = Utils.win.setTimeout(
            function () {
              announceBox.classList.remove('showing');
              this._announceHideTimeout = 0;
            }.bind(this), aDetails.duration);
        break;
      }
      case 'hideAnnouncement':
      {
        let announceBox = this.announceBox ? this.announceBox.get() : null;
        if (announceBox)
          announceBox.classList.remove('showing');
        break;
      }
    }
  },

  Android: function Android(aDetails, aBrowser) {
    const ANDROID_VIEW_TEXT_CHANGED = 0x10;
    const ANDROID_VIEW_TEXT_SELECTION_CHANGED = 0x2000;

    if (!this._bridge)
      this._bridge = Cc['@mozilla.org/android/bridge;1'].getService(Ci.nsIAndroidBridge);

    for each (let androidEvent in aDetails) {
      androidEvent.type = 'Accessibility:Event';
      if (androidEvent.bounds)
        androidEvent.bounds = AccessFu.adjustContentBounds(androidEvent.bounds, aBrowser);

      switch(androidEvent.eventType) {
        case ANDROID_VIEW_TEXT_CHANGED:
          androidEvent.brailleOutput = this.brailleState.adjustText(androidEvent.text);
          break;
        case ANDROID_VIEW_TEXT_SELECTION_CHANGED:
          androidEvent.brailleOutput = this.brailleState.adjustSelection(androidEvent.brailleOutput);
          break;
        default:
          androidEvent.brailleOutput = this.brailleState.init(androidEvent.brailleOutput);
          break;
      }
      this._bridge.handleGeckoMessage(JSON.stringify(androidEvent));
    }
  },

  Haptic: function Haptic(aDetails, aBrowser) {
    Utils.win.navigator.vibrate(aDetails.pattern);
  },

  Braille: function Braille(aDetails, aBrowser) {
    Logger.debug('Braille output: ' + aDetails.text);
  }
};

var Input = {
  editState: {},

  start: function start() {
    
    
    if (Utils.MozBuildApp != 'browser') {
      Utils.win.document.addEventListener('keypress', this, true);
    }
    Utils.win.addEventListener('mozAccessFuGesture', this, true);
  },

  stop: function stop() {
    if (Utils.MozBuildApp != 'browser') {
      Utils.win.document.removeEventListener('keypress', this, true);
    }
    Utils.win.removeEventListener('mozAccessFuGesture', this, true);
  },

  handleEvent: function Input_handleEvent(aEvent) {
    try {
      switch (aEvent.type) {
      case 'keypress':
        this._handleKeypress(aEvent);
        break;
      case 'mozAccessFuGesture':
        this._handleGesture(aEvent.detail);
        break;
      }
    } catch (x) {
      Logger.logException(x);
    }
  },

  _handleGesture: function _handleGesture(aGesture) {
    let gestureName = aGesture.type + aGesture.touches.length;
    Logger.info('Gesture', aGesture.type,
                '(fingers: ' + aGesture.touches.length + ')');

    switch (gestureName) {
      case 'dwell1':
      case 'explore1':
        this.moveToPoint('SimpleTouch', aGesture.x, aGesture.y);
        break;
      case 'doubletap1':
        this.activateCurrent();
        break;
      case 'doubletaphold1':
        this.sendContextMenuMessage();
        break;
      case 'swiperight1':
        this.moveCursor('moveNext', 'Simple', 'gestures');
        break;
      case 'swipeleft1':
        this.moveCursor('movePrevious', 'Simple', 'gesture');
        break;
      case 'swiperight2':
        this.scroll(-1, true);
        break;
      case 'swipedown2':
        this.scroll(-1);
        break;
      case 'swipeleft2':
        this.scroll(1, true);
        break;
      case 'swipeup2':
        this.scroll(1);
        break;
      case 'explore2':
        Utils.CurrentBrowser.contentWindow.scrollBy(
          -aGesture.deltaX, -aGesture.deltaY);
        break;
      case 'swiperight3':
        this.moveCursor('moveNext', this.quickNavMode.current, 'gesture');
        break;
      case 'swipeleft3':
        this.moveCursor('movePrevious', this.quickNavMode.current, 'gesture');
        break;
      case 'swipedown3':
        this.quickNavMode.next();
        AccessFu.announce('quicknav_' + this.quickNavMode.current);
        break;
      case 'swipeup3':
        this.quickNavMode.previous();
        AccessFu.announce('quicknav_' + this.quickNavMode.current);
        break;
    }
  },

  _handleKeypress: function _handleKeypress(aEvent) {
    let target = aEvent.target;

    
    if (aEvent.ctrlKey || aEvent.altKey || aEvent.metaKey)
      return;

    switch (aEvent.keyCode) {
      case 0:
        
        
        
        if (this.editState.editing)
          return;

        let key = String.fromCharCode(aEvent.charCode);
        try {
          let [methodName, rule] = this.keyMap[key];
          this.moveCursor(methodName, rule, 'keyboard');
        } catch (x) {
          return;
        }
        break;
      case aEvent.DOM_VK_RIGHT:
        if (this.editState.editing) {
          if (!this.editState.atEnd)
            
            
            return;
          else
            target.blur();
        }
        this.moveCursor(aEvent.shiftKey ? 'moveLast' : 'moveNext', 'Simple', 'keyboard');
        break;
      case aEvent.DOM_VK_LEFT:
        if (this.editState.editing) {
          if (!this.editState.atStart)
            
            
            return;
          else
            target.blur();
        }
        this.moveCursor(aEvent.shiftKey ? 'moveFirst' : 'movePrevious', 'Simple', 'keyboard');
        break;
      case aEvent.DOM_VK_UP:
        if (this.editState.multiline) {
          if (!this.editState.atStart)
            
            return;
          else
            target.blur();
        }

        if (Utils.MozBuildApp == 'mobile/android')
          
          Cc['@mozilla.org/android/bridge;1'].
            getService(Ci.nsIAndroidBridge).handleGeckoMessage(
              JSON.stringify({ type: 'ToggleChrome:Focus' }));
        break;
      case aEvent.DOM_VK_RETURN:
      case aEvent.DOM_VK_ENTER:
        if (this.editState.editing)
          return;
        this.activateCurrent();
        break;
    default:
      return;
    }

    aEvent.preventDefault();
    aEvent.stopPropagation();
  },

  moveToPoint: function moveToPoint(aRule, aX, aY) {
    let mm = Utils.getMessageManager(Utils.CurrentBrowser);
    mm.sendAsyncMessage('AccessFu:MoveToPoint', {rule: aRule,
                                                 x: aX, y: aY,
                                                 origin: 'top'});
  },

  moveCursor: function moveCursor(aAction, aRule, aInputType) {
    let mm = Utils.getMessageManager(Utils.CurrentBrowser);
    mm.sendAsyncMessage('AccessFu:MoveCursor',
                        {action: aAction, rule: aRule,
                         origin: 'top', inputType: aInputType});
  },

  moveCaret: function moveCaret(aDetails) {
    if (!this.editState.editing) {
      return;
    }

    aDetails.atStart = this.editState.atStart;
    aDetails.atEnd = this.editState.atEnd;

    let mm = Utils.getMessageManager(Utils.CurrentBrowser);
    mm.sendAsyncMessage('AccessFu:MoveCaret', aDetails);
  },

  activateCurrent: function activateCurrent(aData) {
    let mm = Utils.getMessageManager(Utils.CurrentBrowser);
    let offset = aData && typeof aData.keyIndex === 'number' ?
                 aData.keyIndex - Output.brailleState.startOffset : -1;

    mm.sendAsyncMessage('AccessFu:Activate', {offset: offset});
  },

  sendContextMenuMessage: function sendContextMenuMessage() {
    let mm = Utils.getMessageManager(Utils.CurrentBrowser);
    mm.sendAsyncMessage('AccessFu:ContextMenu', {});
  },

  activateContextMenu: function activateContextMenu(aMessage) {
    if (Utils.MozBuildApp === 'mobile/android') {
      let p = AccessFu.adjustContentBounds(aMessage.bounds, Utils.CurrentBrowser,
                                           true, true).center();
      Services.obs.notifyObservers(null, 'Gesture:LongPress',
                                   JSON.stringify({x: p.x, y: p.y}));
    }
  },

  setEditState: function setEditState(aEditState) {
    this.editState = aEditState;
  },

  scroll: function scroll(aPage, aHorizontal) {
    let mm = Utils.getMessageManager(Utils.CurrentBrowser);
    mm.sendAsyncMessage('AccessFu:Scroll', {page: aPage, horizontal: aHorizontal, origin: 'top'});
  },

  get keyMap() {
    delete this.keyMap;
    this.keyMap = {
      a: ['moveNext', 'Anchor'],
      A: ['movePrevious', 'Anchor'],
      b: ['moveNext', 'Button'],
      B: ['movePrevious', 'Button'],
      c: ['moveNext', 'Combobox'],
      C: ['movePrevious', 'Combobox'],
      d: ['moveNext', 'Landmark'],
      D: ['movePrevious', 'Landmark'],
      e: ['moveNext', 'Entry'],
      E: ['movePrevious', 'Entry'],
      f: ['moveNext', 'FormElement'],
      F: ['movePrevious', 'FormElement'],
      g: ['moveNext', 'Graphic'],
      G: ['movePrevious', 'Graphic'],
      h: ['moveNext', 'Heading'],
      H: ['movePrevious', 'Heading'],
      i: ['moveNext', 'ListItem'],
      I: ['movePrevious', 'ListItem'],
      k: ['moveNext', 'Link'],
      K: ['movePrevious', 'Link'],
      l: ['moveNext', 'List'],
      L: ['movePrevious', 'List'],
      p: ['moveNext', 'PageTab'],
      P: ['movePrevious', 'PageTab'],
      r: ['moveNext', 'RadioButton'],
      R: ['movePrevious', 'RadioButton'],
      s: ['moveNext', 'Separator'],
      S: ['movePrevious', 'Separator'],
      t: ['moveNext', 'Table'],
      T: ['movePrevious', 'Table'],
      x: ['moveNext', 'Checkbox'],
      X: ['movePrevious', 'Checkbox']
    };

    return this.keyMap;
  },

  quickNavMode: {
    get current() {
      return this.modes[this._currentIndex];
    },

    previous: function quickNavMode_previous() {
      if (--this._currentIndex < 0)
        this._currentIndex = this.modes.length - 1;
    },

    next: function quickNavMode_next() {
      if (++this._currentIndex >= this.modes.length)
        this._currentIndex = 0;
    },

    updateModes: function updateModes(aModes) {
      if (aModes) {
        this.modes = aModes.split(',');
      } else {
        this.modes = [];
      }
    },

    _currentIndex: -1
  }
};
AccessFu.Input = Input;
