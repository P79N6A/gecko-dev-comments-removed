


"use strict";

const { Tool } = require("dev/toolbox");
const { Class } = require("sdk/core/heritage");
const { onEnable, onDisable } = require("dev/theme/hooks");
const { Theme, LightTheme } = require("dev/theme");







const MyTheme = Theme({
  name: "mytheme",
  label: "My Light Theme",
  styles: [LightTheme, "./theme.css"],

  onEnable: function(window, oldTheme) {
    console.log("myTheme.onEnable; method override " +
      window.location.href);
  },
  onDisable: function(window, newTheme) {
    console.log("myTheme.onDisable; method override " +
      window.location.href);
  },
});



const mytheme = new Tool({
  name: "My Tool",
  themes: { mytheme: MyTheme }
});
