


"use strict";

const { id } = require("sdk/self");
const { getAddonByID } = require("sdk/addon/manager");

exports["test getAddonByID"] = function*(assert) {
  let addon = yield getAddonByID(id);
  assert.equal(addon.id, id, "getAddonByID works");
}
