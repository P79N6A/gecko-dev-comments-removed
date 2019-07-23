









































let Cu = Components.utils;
Cu.import("resource://gre/modules/DownloadUtils.jsm");

function run_test()
{
  
  let downloadTimes = {};
  for each (let time in [1, 30, 60, 3456, 9999])
    downloadTimes[time] = DownloadUtils.getTimeLeft(time)[0];

  
  
  let lastSec = 314;
  for (let [time, text] in Iterator(downloadTimes))
    do_check_eq(DownloadUtils.getTimeLeft(time, lastSec)[0], text);
}
