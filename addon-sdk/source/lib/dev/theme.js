



"use strict";

module.metadata = {
  "stability": "experimental"
};

const { Class } = require("../sdk/core/heritage");
const { EventTarget } = require("../sdk/event/target");
const { Disposable, setup, dispose } = require("../sdk/core/disposable");
const { contract, validate } = require("../sdk/util/contract");
const { id: addonID } = require("../sdk/self");
const { onEnable, onDisable } = require("dev/theme/hooks");
const { isString, instanceOf, isFunction } = require("sdk/lang/type");
const { add } = require("sdk/util/array");
const { data } = require("../sdk/self");
const { isLocalURL } = require("../sdk/url");

const makeID = name =>
  ("dev-theme-" + addonID + (name ? "-" + name : "")).
  split(/[ . /]/).join("-").
  replace(/[^A-Za-z0-9_\-]/g, "");

const Theme = Class({
  extends: Disposable,
  implements: [EventTarget],

  initialize: function(options) {
    this.name = options.name;
    this.label = options.label;
    this.styles = options.styles;

    
    this.onEnable = options.onEnable;
    this.onDisable = options.onDisable;
  },
  get id() {
    return makeID(this.name || this.label);
  },
  setup: function() {
    
  },
  getStyles: function() {
    if (!this.styles) {
      return [];
    }

    if (isString(this.styles)) {
      if (isLocalURL(this.styles)) {
        return [data.url(this.styles)];
      }
    }

    let result = [];
    for (let style of this.styles) {
      if (isString(style)) {
        if (isLocalURL(style)) {
          style = data.url(style);
        }
        add(result, style);
      } else if (instanceOf(style, Theme)) {
        result = result.concat(style.getStyles());
      }
    }
    return result;
  },
  getClassList: function() {
    let result = [];
    for (let style of this.styles) {
      if (instanceOf(style, Theme)) {
        result = result.concat(style.getClassList());
      }
    }

    if (this.name) {
      add(result, this.name);
    }

    return result;
  }
});

exports.Theme = Theme;



setup.define(Theme, (theme) => {
  theme.classList = [];
  theme.setup();
});

dispose.define(Theme, function(theme) {
  theme.dispose();
});



validate.define(Theme, contract({
  label: {
    is: ["string"],
    msg: "The `option.label` must be a provided"
  },
}));



onEnable.define(Theme, (theme, {window, oldTheme}) => {
  if (isFunction(theme.onEnable)) {
    theme.onEnable(window, oldTheme);
  }
});

onDisable.define(Theme, (theme, {window, newTheme}) => {
  if (isFunction(theme.onDisable)) {
    theme.onDisable(window, newTheme);
  }
});



const LightTheme = Theme({
  name: "theme-light",
  styles: "chrome://browser/skin/devtools/light-theme.css",
});

const DarkTheme = Theme({
  name: "theme-dark",
  styles: "chrome://browser/skin/devtools/dark-theme.css",
});

exports.LightTheme = LightTheme;
exports.DarkTheme = DarkTheme;
