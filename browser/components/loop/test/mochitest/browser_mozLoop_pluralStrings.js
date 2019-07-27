







"use strict";

Components.utils.import("resource://gre/modules/Promise.jsm", this);

add_task(loadLoopPanel);

add_task(function* test_mozLoop_pluralStrings() {
  Assert.ok(gMozLoopAPI, "mozLoop should exist");

  var strings = JSON.parse(gMozLoopAPI.getStrings("import_contacts_success_message"));
  Assert.equal(gMozLoopAPI.getPluralForm(1, strings.textContent),
               "{{total}} contact was successfully imported.");
  Assert.equal(gMozLoopAPI.getPluralForm(3, strings.textContent),
               "{{total}} contacts were successfully imported.");
});
