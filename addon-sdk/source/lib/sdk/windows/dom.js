


'use strict';

const { Trait } = require('../deprecated/traits');
const { isWindowPrivate, getWindowTitle } = require('../window/utils');
const { deprecateUsage } = require('../util/deprecate');

module.metadata = {
  "stability": "unstable"
};

const WindowDom = Trait.compose({
  _window: Trait.required,
  get title() {
    return getWindowTitle(this._window);
  },
  close: function close() {
    let window = this._window;
    if (window) window.close();
    return this._public;
  },
  activate: function activate() {
    let window = this._window;
    if (window) window.focus();
    return this._public;
  },
  get isPrivateBrowsing() {
    deprecateUsage('`browserWindow.isPrivateBrowsing` is deprecated, please ' +
                   'consider using ' +
                   '`require("sdk/private-browsing").isPrivate(browserWindow)` ' +
                   'instead.');
    return isWindowPrivate(this._window);
  }
});
exports.WindowDom = WindowDom;
