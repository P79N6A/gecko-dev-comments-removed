




































var RELATIVE_ROOT = '../../shared-modules';
var MODULE_REQUIRES = ['SearchAPI'];

const gDelay = 0;

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  search = new SearchAPI.searchBar(controller);
}




var testMultipleEngines = function()
{
  var engines = search.visibleEngines;
  var suggestions = [ ];

  
  for (var ii = 0; ii < 2; ii++) {
    search.selectedEngine = engines[ii].name;
    suggestions[ii] = search.getSuggestions("Moz");
    search.clear();
  }

  
  var difference = false;
  var maxIndex = Math.max(suggestions[0].length, suggestions[1].length);
  for (ii = 0; ii < maxIndex; ii++) {
    if (suggestions[0][ii] != suggestions[1][ii]) {
      difference = true;
      break;
    }
  }

  controller.assertJS("subject.suggestionsDifferent == true",
                      {suggestionsDifferent: difference});
}
