







































"use strict";

const Cu = Components.utils;

Cu.import("resource:///modules/devtools/TiltMath.jsm");

let EXPORTED_SYMBOLS = ["TiltVisualizerStyle"];
let rgba = TiltMath.hex2rgba;




let TiltVisualizerStyle = {

  canvas: {
    background: "-moz-linear-gradient(top, #454545 0%, #000 100%)",
  },

  nodes: {
    highlight: {
      defaultFill: rgba("#555"),
      defaultStroke: rgba("#000"),
      defaultStrokeWeight: 1
    },

    html: rgba("#8880"),
    body: rgba("#fff0"),
    h1: rgba("#e667af"),
    h2: rgba("#c667af"),
    h3: rgba("#a667af"),
    h4: rgba("#8667af"),
    h5: rgba("#8647af"),
    h6: rgba("#8627af"),
    div: rgba("#5dc8cd"),
    span: rgba("#67e46f"),
    table: rgba("#ff0700"),
    tr: rgba("#ff4540"),
    td: rgba("#ff7673"),
    ul: rgba("#4671d5"),
    li: rgba("#6c8cd5"),
    p: rgba("#aaa"),
    a: rgba("#123eab"),
    img: rgba("#ffb473"),
    iframe: rgba("#85004b")
  }
};
