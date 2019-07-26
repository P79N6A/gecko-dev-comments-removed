


"use strict";

const { windows, isBrowser, isInteractive, isDocumentLoaded,
        getOuterId } = require("../window/utils");
const { InputPort } = require("./system");
const { lift, merges, foldp, keepIf, start, Input } = require("../event/utils");
const { patch } = require("diffpatcher/index");
const { Sequence, seq, filter, object, pairs } = require("../util/sequence");







const opened = seq(function*() {
  const items = windows("navigator:browser", {includePrivates: true});
  for (let item of items) {
      yield [getOuterId(item), item];
  }
});
const interactive = filter(([_, window]) => isInteractive(window), opened);
const loaded = filter(([_, window]) => isDocumentLoaded(window), opened);


const Update = window => window && object([getOuterId(window), window]);
const Delete = window => window && object([getOuterId(window), null]);



const LastClosed = lift(Delete,
                        keepIf(isBrowser, null,
                               new InputPort({topic: "domwindowclosed"})));
exports.LastClosed = LastClosed;

const windowFor = document => document && document.defaultView;


const InteractiveDoc = new InputPort({topic: "chrome-document-interactive"});
const InteractiveWin = lift(windowFor, InteractiveDoc);
const LastInteractive = lift(Update, keepIf(isBrowser, null, InteractiveWin));
exports.LastInteractive = LastInteractive;


const LoadedDoc = new InputPort({topic: "chrome-document-loaded"});
const LoadedWin = lift(windowFor, LoadedDoc);
const LastLoaded = lift(Update, keepIf(isBrowser, null, LoadedWin));
exports.LastLoaded = LastLoaded;


const initialize = input => {
  if (!input.initialized) {
    input.value = object(...input.value);
    Input.start(input);
    input.initialized = true;
  }
};



const Interactive = foldp(patch, interactive, merges([LastInteractive,
                                                      LastClosed]));
Interactive[start] = initialize;
exports.Interactive = Interactive;



const Loaded = foldp(patch, loaded, merges([LastLoaded, LastClosed]));
Loaded[start] = initialize;
exports.Loaded = Loaded;
