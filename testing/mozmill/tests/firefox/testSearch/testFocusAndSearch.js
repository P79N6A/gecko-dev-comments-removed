




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['SearchAPI'];

const gDelay = 0;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  search = new SearchAPI.searchBar(controller);
}

var teardownTest = function()
{
  search.clear();
}




var testClickAndSearch = function()
{
  search.focus({type: "click"});
  search.search({text: "Firefox", action: "returnKey"});
}




var testShortcutAndSearch = function()
{
  search.focus({type: "shortcut"});
  search.search({text: "Mozilla", action: "goButton"});
}






