





































function run_test() {
  let dir = gProfD.clone();
  let tfile = writeTestFile(dir, "bookmarkfile.test");
  gPrefSvc.setCharPref("browser.bookmarks.file", tfile.path);

  let bmarks = gDirSvc.get("BMarks", Ci.nsIFile);
  do_check_true(tfile.equals(bmarks));
}
