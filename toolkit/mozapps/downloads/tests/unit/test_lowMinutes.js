






































Components.utils.import("resource://gre/modules/DownloadUtils.jsm");










let _ = function(some, debug, text, to) print(Array.slice(arguments).join(" "));

_("Make an array of time lefts and expected string to be shown for that time");
let expectedTimes = [
  [1.1, "A few seconds remaining", "under 4sec -> few"],
  [2.5, "A few seconds remaining", "under 4sec -> few"],
  [3.9, "A few seconds remaining", "under 4sec -> few"],
  [5.3, "5 seconds remaining", "truncate seconds"],
  [1.1 * 60, "1 minute, 6 seconds remaining", "under 4min -> show sec"],
  [2.5 * 60, "2 minutes, 30 seconds remaining", "under 4min -> show sec"],
  [3.9 * 60, "3 minutes, 54 seconds remaining", "under 4min -> show sec"],
  [5.3 * 60, "5 minutes remaining", "over 4min -> only show min"],
  [1.1 * 3600, "1 hour, 6 minutes remaining", "over 1hr -> show min/sec"],
  [2.5 * 3600, "2 hours, 30 minutes remaining", "over 1hr -> show min/sec"],
  [3.9 * 3600, "3 hours, 54 minutes remaining", "over 1hr -> show min/sec"],
  [5.3 * 3600, "5 hours, 18 minutes remaining", "over 1hr -> show min/sec"],
];
_(expectedTimes.join("\n"));

function run_test()
{
  expectedTimes.forEach(function([time, expectStatus, comment]) {
    _("Running test with time", time);
    _("Test comment:", comment);
    let [status, last] = DownloadUtils.getTimeLeft(time);

    _("Got status:", status, "last:", last);
    _("Expecting..", expectStatus);
    do_check_eq(status, expectStatus);

    _();
  });
}
