


"use strict";




let ToolbarView = {
  


  initialize: Task.async(function *() {
    this._onPrefChanged = this._onPrefChanged.bind(this);

    this.optionsView = new OptionsView({
      branchName: BRANCH_NAME,
      menupopup: $("#performance-options-menupopup")
    });

    yield this.optionsView.initialize();
    this.optionsView.on("pref-changed", this._onPrefChanged);
  }),

  


  destroy: function () {
    this.optionsView.off("pref-changed", this._onPrefChanged);
    this.optionsView.destroy();
  },

  



  _onPrefChanged: function (_, prefName) {
    let value = Services.prefs.getBoolPref(BRANCH_NAME + prefName);
    this.emit(EVENTS.PREF_CHANGED, prefName, value);
  },

  toString: () => "[object ToolbarView]"
};

EventEmitter.decorate(ToolbarView);
