














































let ac = Cc["@mozilla.org/autocomplete/search;1?name=history"].
         getService(Ci.nsIAutoCompleteSearch);




function test_stopSearch()
{
  try {
    ac.stopSearch();
  }
  catch (e) {
    do_throw("we should not have caught anything!");
  }
}




let tests = [
  test_stopSearch,
];
function run_test()
{
  tests.forEach(function(test) test());
}
