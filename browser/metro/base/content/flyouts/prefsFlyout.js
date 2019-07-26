



'use strict';

Components.utils.import("resource://gre/modules/Services.jsm");

let PrefsFlyout = {
  _isInitialized: false,
  _hasShown: false,
  init: function pv_init() {
    if (this._isInitialized) {
      Cu.reportError("Attempting to re-initialize PreferencesPanelView");
      return;
    }

    Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
    this._isInitialized = true;
    let self = this;

    this._elements = {};
    [
      ['prefsFlyout', 'prefs-flyoutpanel'],
      ['dntNoPref',   'prefs-dnt-nopref'],
    ].forEach(function(aElement) {
      let [name, id] = aElement;
      XPCOMUtils.defineLazyGetter(self._elements, name, function() {
        return document.getElementById(id);
      });
    });

    this._topmostElement = this._elements.prefsFlyout;
  },

  _show: function() {
    if (!this._hasShown) {
      SanitizeUI.init();
      this._hasShown = true;
    }

    this._elements.prefsFlyout.show();
  },

  onDNTPreferenceChanged: function onDNTPreferenceChanged() {
    let selected = this._elements.dntNoPref.selected;

    
    Services.prefs.setBoolPref("privacy.donottrackheader.enabled", !selected);
  }
};
