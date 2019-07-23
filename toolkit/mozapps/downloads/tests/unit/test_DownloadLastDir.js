



































function run_test()
{
  let Cc = Components.classes;
  let Ci = Components.interfaces;
  let Cu = Components.utils;
  Cu.import("resource://gre/modules/DownloadLastDir.jsm");

  do_check_eq(typeof gDownloadLastDir, "object");
  do_check_eq(gDownloadLastDir.file, null);

  let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
               getService(Ci.nsIProperties);
  let tmpDir = dirSvc.get("TmpD", Ci.nsILocalFile);

  gDownloadLastDir.file = tmpDir;
  do_check_eq(gDownloadLastDir.file.path, tmpDir.path);
  do_check_neq(gDownloadLastDir.file, tmpDir);

  gDownloadLastDir.file = 1; 
  do_check_eq(gDownloadLastDir.file, null);
  gDownloadLastDir.file = tmpDir;

  let pb;
  try {
    pb = Cc["@mozilla.org/privatebrowsing;1"].
         getService(Ci.nsIPrivateBrowsingService);
  } catch (e) {
    print("PB service is not available, bail out");
    return;
  }

  pb.privateBrowsingEnabled = true;
  do_check_eq(gDownloadLastDir.file, null);

  pb.privateBrowsingEnabled = false;
  do_check_eq(gDownloadLastDir.file, null);
  pb.privateBrowsingEnabled = true;

  gDownloadLastDir.file = tmpDir;
  do_check_eq(gDownloadLastDir.file.path, tmpDir.path);
  do_check_neq(gDownloadLastDir.file, tmpDir);

  pb.privateBrowsingEnabled = false;
  do_check_eq(gDownloadLastDir.file, null);
}
