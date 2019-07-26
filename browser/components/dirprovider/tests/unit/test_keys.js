



function test_bookmarkhtml() {
  let bmarks = gProfD.clone();
  bmarks.append("bookmarks.html");

  let tbmarks = gDirSvc.get("BMarks", Ci.nsIFile);
  do_check_true(bmarks.equals(tbmarks));
}

function run_test() {
  [test_bookmarkhtml
  ].forEach(function(f) {
    do_test_pending();
    print("Running test: " + f.name);
    f();
    do_test_finished();
  });
}
