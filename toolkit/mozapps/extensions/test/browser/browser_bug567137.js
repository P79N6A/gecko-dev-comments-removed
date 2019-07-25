





function test() {
  waitForExplicitFinish();

  open_manager(null, function(aWindow) {
    let utils = new CategoryUtilities(aWindow);

    
    utils.openType("plugin", function() {

      
      close_manager(aWindow, function() {
        open_manager(null, function(aWindow) {
          utils = new CategoryUtilities(aWindow);

          is(utils.selectedCategory, "plugin", "Should have shown the plugins category");

          
          utils.openType("extension", function() {

            
            close_manager(aWindow, function() {
              open_manager(null, function(aWindow) {
                utils = new CategoryUtilities(aWindow);

                is(utils.selectedCategory, "extension", "Should have shown the extensions category");
                close_manager(aWindow, finish);
              });
            });
          });
        });
      });
    });
  });
}
