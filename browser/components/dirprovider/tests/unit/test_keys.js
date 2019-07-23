



































function test_usr_micsum() {
  let mdir = gProfD.clone();
  mdir.append("microsummary-generators");

  let tmdir = gDirSvc.get("UsrMicsumGens", Ci.nsIFile);
  do_check_true(tmdir.equals(mdir));

  if (!tmdir.exists())
    tmdir.create(Ci.nsIFile.DIRECTORY_TYPE, 0777);

  do_check_true(tmdir.isWritable());

  let tfile = writeTestFile(tmdir, "usrmicsum");
  do_check_true(tfile.exists());

  mdir.append(tfile.leafName);
  do_check_true(mdir.exists());
}

function test_app_micsum() {
  let mdir = gDirSvc.get("XCurProcD", Ci.nsIFile);
  mdir.append("microsummary-generators");

  let tmdir = gDirSvc.get("MicsumGens", Ci.nsIFile);
  do_check_true(tmdir.equals(mdir));
}

function test_bookmarkhtml() {
  let bmarks = gProfD.clone();
  bmarks.append("bookmarks.html");

  let tbmarks = gDirSvc.get("BMarks", Ci.nsIFile);
  do_check_true(bmarks.equals(tbmarks));
}

function test_prefoverride() {
  let dir = gDirSvc.get("DefRt", Ci.nsIFile);
  dir.append("existing-profile-defaults.js");

  let tdir = gDirSvc.get("ExistingPrefOverride", Ci.nsIFile);
  do_check_true(dir.equals(tdir));
}

function run_test() {
  [test_usr_micsum,
   test_app_micsum,
   test_bookmarkhtml,
   test_prefoverride
  ].forEach(function(f) {
    do_test_pending();
    print("Running test: " + f.name);
    f();
    do_test_finished();
  });
}
