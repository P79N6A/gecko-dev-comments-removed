
function run_test() {
  Components.utils.import("resource:///modules/AppsUtils.jsm");

  let manifest1 = {
    launch_path: "other.html"
  };

  let manifest2 = {
    start_url: "start.html",
    launch_path: "other.html"
  };

  var helper = new ManifestHelper(manifest1, "http://foo.com",
    "http://foo.com/manifest.json");
  var path = helper.fullLaunchPath();
  do_check_true(path == "http://foo.com/other.html");

  helper = new ManifestHelper(manifest2, "http://foo.com",
    "http://foo.com/manifest.json");
  path = helper.fullLaunchPath();
  do_check_true(path == "http://foo.com/start.html");
}
