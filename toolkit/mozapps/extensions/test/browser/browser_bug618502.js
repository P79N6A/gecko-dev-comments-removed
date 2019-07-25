






var gCategoryUtilities;

function test() {
  waitForExplicitFinish();
  
  run_next_test();
}

function end_test() {
  finish();
}

add_test(function() {
  open_manager("addons://detail/foo", function(aManager) {
    gCategoryUtilities = new CategoryUtilities(aManager);
    is(gCategoryUtilities.selectedCategory, "discover", "Should fall back to the discovery pane");

    close_manager(aManager, run_next_test);
  });
});



add_test(function() {
  new MockProvider().createAddons([{
    id: "addon1@tests.mozilla.org",
    name: "addon 1",
    version: "1.0"
  }]);

  open_manager("addons://detail/addon1@tests.mozilla.org", function(aManager) {
    gCategoryUtilities = new CategoryUtilities(aManager);
    is(gCategoryUtilities.selectedCategory, "extension", "Should have selected the right category");

    close_manager(aManager, run_next_test);
  });
});
