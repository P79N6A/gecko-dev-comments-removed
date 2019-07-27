


"use strict";















exports["test can't delete exported property"] = function(assert) {
  let hotkeys = require("sdk/hotkeys");
  let { Hotkey } = hotkeys;

  try { delete hotkeys.Hotkey; } catch(e) {}
  assert.equal(hotkeys.Hotkey, Hotkey, "exports can't be deleted");
};

exports["test can't override exported property"] = function(assert) {
  let hotkeys = require("sdk/hotkeys");
  let { Hotkey } = hotkeys;

  try { hotkeys.Hotkey = Object } catch(e) {}
  assert.equal(hotkeys.Hotkey, Hotkey, "exports can't be overriden");
};

require("sdk/test").run(exports);
