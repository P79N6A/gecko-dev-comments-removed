




var dir = do_get_profile();
dir.append("temp");
dir.create(dir.DIRECTORY_TYPE, -1);
var path = dir.path + "/";


var file = dir.clone();
file.append("test_file");
file.create(file.NORMAL_FILE_TYPE, -1);
file = dir.clone();
file.append("other_file");
file.create(file.NORMAL_FILE_TYPE, -1);
dir.append("test_dir");
dir.create(dir.DIRECTORY_TYPE, -1);

var gListener = {
  onSearchResult: function(aSearch, aResult) {
    
    do_check_eq(aResult.searchString, "test");
    
    do_check_eq(aResult.searchResult, aResult.RESULT_SUCCESS);
    
    do_check_eq(aResult.matchCount, 2);
    
    do_check_eq(aResult.getValueAt(0), "test_dir");
    
    do_check_eq(aResult.getStyleAt(0), "directory");
    
    do_check_eq(aResult.getValueAt(1), "test_file");
    
    do_check_eq(aResult.getStyleAt(1), "file");
  }
};

function run_test()
{
  Components.classes["@mozilla.org/autocomplete/search;1?name=file"]
            .getService(Components.interfaces.nsIAutoCompleteSearch)
            .startSearch("test", path, null, gListener);
}
