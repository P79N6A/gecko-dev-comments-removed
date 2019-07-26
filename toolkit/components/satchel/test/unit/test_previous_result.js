



let aaaListener = {
  onSearchResult: function(search, result) {
    do_check_eq(result.searchString, "aaa");
    do_test_finished();
  }
};

let aaListener = {
  onSearchResult: function(search, result) {
    do_check_eq(result.searchString, "aa");
    search.startSearch("aaa", "", result, aaaListener);
  }
};

function run_test()
{
  do_test_pending();
  let search = Cc['@mozilla.org/autocomplete/search;1?name=form-history'].
               getService(Components.interfaces.nsIAutoCompleteSearch);
  search.startSearch("aa", "", null, aaListener);
}
