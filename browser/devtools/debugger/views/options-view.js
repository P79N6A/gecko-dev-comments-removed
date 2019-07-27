


"use strict";



const POPUP_HIDDEN_DELAY = 100; 




function OptionsView(DebuggerController, DebuggerView) {
  dumpn("OptionsView was instantiated");

  this.DebuggerController = DebuggerController;
  this.DebuggerView = DebuggerView;

  this._toggleAutoPrettyPrint = this._toggleAutoPrettyPrint.bind(this);
  this._togglePauseOnExceptions = this._togglePauseOnExceptions.bind(this);
  this._toggleIgnoreCaughtExceptions = this._toggleIgnoreCaughtExceptions.bind(this);
  this._toggleShowPanesOnStartup = this._toggleShowPanesOnStartup.bind(this);
  this._toggleShowVariablesOnlyEnum = this._toggleShowVariablesOnlyEnum.bind(this);
  this._toggleShowVariablesFilterBox = this._toggleShowVariablesFilterBox.bind(this);
  this._toggleShowOriginalSource = this._toggleShowOriginalSource.bind(this);
  this._toggleAutoBlackBox = this._toggleAutoBlackBox.bind(this);
}

OptionsView.prototype = {
  


  initialize: function() {
    dumpn("Initializing the OptionsView");

    this._button = document.getElementById("debugger-options");
    this._autoPrettyPrint = document.getElementById("auto-pretty-print");
    this._pauseOnExceptionsItem = document.getElementById("pause-on-exceptions");
    this._ignoreCaughtExceptionsItem = document.getElementById("ignore-caught-exceptions");
    this._showPanesOnStartupItem = document.getElementById("show-panes-on-startup");
    this._showVariablesOnlyEnumItem = document.getElementById("show-vars-only-enum");
    this._showVariablesFilterBoxItem = document.getElementById("show-vars-filter-box");
    this._showOriginalSourceItem = document.getElementById("show-original-source");
    this._autoBlackBoxItem = document.getElementById("auto-black-box");

    this._autoPrettyPrint.setAttribute("checked", Prefs.autoPrettyPrint);
    this._pauseOnExceptionsItem.setAttribute("checked", Prefs.pauseOnExceptions);
    this._ignoreCaughtExceptionsItem.setAttribute("checked", Prefs.ignoreCaughtExceptions);
    this._showPanesOnStartupItem.setAttribute("checked", Prefs.panesVisibleOnStartup);
    this._showVariablesOnlyEnumItem.setAttribute("checked", Prefs.variablesOnlyEnumVisible);
    this._showVariablesFilterBoxItem.setAttribute("checked", Prefs.variablesSearchboxVisible);
    this._showOriginalSourceItem.setAttribute("checked", Prefs.sourceMapsEnabled);
    this._autoBlackBoxItem.setAttribute("checked", Prefs.autoBlackBox);

    this._addCommands();
  },

  


  destroy: function() {
    dumpn("Destroying the OptionsView");
    
  },

  


  _addCommands: function() {
    XULUtils.addCommands(document.getElementById('debuggerCommands'), {
      toggleAutoPrettyPrint: () => this._toggleAutoPrettyPrint(),
      togglePauseOnExceptions: () => this._togglePauseOnExceptions(),
      toggleIgnoreCaughtExceptions: () => this._toggleIgnoreCaughtExceptions(),
      toggleShowPanesOnStartup: () => this._toggleShowPanesOnStartup(),
      toggleShowOnlyEnum: () => this._toggleShowVariablesOnlyEnum(),
      toggleShowVariablesFilterBox: () => this._toggleShowVariablesFilterBox(),
      toggleShowOriginalSource: () => this._toggleShowOriginalSource(),
      toggleAutoBlackBox: () =>  this._toggleAutoBlackBox()
    });
  },

  


  _onPopupShowing: function() {
    this._button.setAttribute("open", "true");
    window.emit(EVENTS.OPTIONS_POPUP_SHOWING);
  },

  


  _onPopupHiding: function() {
    this._button.removeAttribute("open");
  },

  


  _onPopupHidden: function() {
    window.emit(EVENTS.OPTIONS_POPUP_HIDDEN);
  },

  


  _toggleAutoPrettyPrint: function(){
    Prefs.autoPrettyPrint =
      this._autoPrettyPrint.getAttribute("checked") == "true";
  },

  


  _togglePauseOnExceptions: function() {
    Prefs.pauseOnExceptions =
      this._pauseOnExceptionsItem.getAttribute("checked") == "true";

    this.DebuggerController.activeThread.pauseOnExceptions(
      Prefs.pauseOnExceptions,
      Prefs.ignoreCaughtExceptions);
  },

  _toggleIgnoreCaughtExceptions: function() {
    Prefs.ignoreCaughtExceptions =
      this._ignoreCaughtExceptionsItem.getAttribute("checked") == "true";

    this.DebuggerController.activeThread.pauseOnExceptions(
      Prefs.pauseOnExceptions,
      Prefs.ignoreCaughtExceptions);
  },

  


  _toggleShowPanesOnStartup: function() {
    Prefs.panesVisibleOnStartup =
      this._showPanesOnStartupItem.getAttribute("checked") == "true";
  },

  


  _toggleShowVariablesOnlyEnum: function() {
    let pref = Prefs.variablesOnlyEnumVisible =
      this._showVariablesOnlyEnumItem.getAttribute("checked") == "true";

    this.DebuggerView.Variables.onlyEnumVisible = pref;
  },

  


  _toggleShowVariablesFilterBox: function() {
    let pref = Prefs.variablesSearchboxVisible =
      this._showVariablesFilterBoxItem.getAttribute("checked") == "true";

    this.DebuggerView.Variables.searchEnabled = pref;
  },

  


  _toggleShowOriginalSource: function() {
    let pref = Prefs.sourceMapsEnabled =
      this._showOriginalSourceItem.getAttribute("checked") == "true";

    
    window.once(EVENTS.OPTIONS_POPUP_HIDDEN, () => {
      
      window.setTimeout(() => {
        this.DebuggerController.reconfigureThread({
          useSourceMaps: pref,
          autoBlackBox: Prefs.autoBlackBox
        });
      }, POPUP_HIDDEN_DELAY);
    });
  },

  



  _toggleAutoBlackBox: function() {
    let pref = Prefs.autoBlackBox =
      this._autoBlackBoxItem.getAttribute("checked") == "true";

    
    window.once(EVENTS.OPTIONS_POPUP_HIDDEN, () => {
      
      window.setTimeout(() => {
        this.DebuggerController.reconfigureThread({
          useSourceMaps: Prefs.sourceMapsEnabled,
          autoBlackBox: pref
        });
      }, POPUP_HIDDEN_DELAY);
    });
  },

  _button: null,
  _pauseOnExceptionsItem: null,
  _showPanesOnStartupItem: null,
  _showVariablesOnlyEnumItem: null,
  _showVariablesFilterBoxItem: null,
  _showOriginalSourceItem: null,
  _autoBlackBoxItem: null
};

DebuggerView.Options = new OptionsView(DebuggerController, DebuggerView);
