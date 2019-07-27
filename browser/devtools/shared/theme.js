



"use strict";






const { Ci, Cu } = require("chrome");
const { NetUtil } = Cu.import("resource://gre/modules/NetUtil.jsm", {});
loader.lazyRequireGetter(this, "Services");
loader.lazyImporter(this, "gDevTools", "resource:///modules/devtools/gDevTools.jsm");

const themeURIs = {
  light: "chrome://browser/skin/devtools/light-theme.css",
  dark: "chrome://browser/skin/devtools/dark-theme.css"
}

const cachedThemes = {};




function readURI (uri) {
  let stream = NetUtil.newChannel({
    uri: NetUtil.newURI(uri, "UTF-8"),
    loadUsingSystemPrincipal: true}
  ).open();

  let count = stream.available();
  let data = NetUtil.readInputStreamToString(stream, count, { charset: "UTF-8" });
  stream.close();
  return data;
}





function getThemeFile (name) {
  
  let themeFile = cachedThemes[name] || readURI(themeURIs[name]).match(/--theme-.*: .*;/g).join("\n");

  
  if (!cachedThemes[name]) {
    cachedThemes[name] = themeFile;
  }

  return themeFile;
}





const getTheme = exports.getTheme = () => Services.prefs.getCharPref("devtools.theme");







const getColor = exports.getColor = (type, theme) => {
  let themeName = theme || getTheme();

  
  if (!themeURIs[themeName]) {
    themeName = "light";
  }

  let themeFile = getThemeFile(themeName);
  let match;

  
  return (match = themeFile.match(new RegExp("--theme-" + type + ": (.*);"))) ? match[1] : null;
};






const setTheme = exports.setTheme = (newTheme) => {
  let oldTheme = getTheme();

  Services.prefs.setCharPref("devtools.theme", newTheme);
  gDevTools.emit("pref-changed", {
    pref: "devtools.theme",
    newValue: newTheme,
    oldValue: oldTheme
  });
};
