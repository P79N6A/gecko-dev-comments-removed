








































let Cu = Components.utils;
Cu.import("resource://gre/modules/DownloadUtils.jsm");

function run_test()
{
  do_check_eq(DownloadUtils.getDownloadStatus(1000, null, null, null) + "",
              DownloadUtils.getDownloadStatus(1000) + "");
  do_check_eq(DownloadUtils.getDownloadStatus(1000, null, null) + "",
              DownloadUtils.getDownloadStatus(1000, null) + "");

  do_check_eq(DownloadUtils.getTransferTotal(1000, null) + "",
              DownloadUtils.getTransferTotal(1000) + "");

  do_check_eq(DownloadUtils.getTimeLeft(1000, null) + "",
              DownloadUtils.getTimeLeft(1000) + "");
}
