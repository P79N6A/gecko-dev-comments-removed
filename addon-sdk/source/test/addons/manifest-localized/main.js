


"use strict";

const { id } = require('sdk/self');
const { getAddonByID } = require('sdk/addon/manager');

exports["test add-on manifest was localized"] = function*(assert) {
  let addon = yield getAddonByID(id);
  assert.equal(addon.name, "title-en", "title was translated");
  assert.equal(addon.description, "description-en", "description was translated");
  assert.equal(addon.creator, "author-en", "author was translated");
  assert.equal(addon.homepageURL, "homepage-en", "homepage was translated");
};

require("sdk/test/runner").runTestsFromModule(module);
