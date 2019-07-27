


"use strict";

exports["test compatibility"] = function(assert) {
  assert.throws(() => require("self"),
                /^Module `self` is not found/,
                "sdk/self -> self");
};
