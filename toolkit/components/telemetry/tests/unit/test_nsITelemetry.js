


const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const INT_MAX = 0x7FFFFFFF;

const Telemetry = Cc["@mozilla.org/base/telemetry;1"].getService(Ci.nsITelemetry);
Cu.import("resource://gre/modules/Services.jsm");

function test_histogram(histogram_type, name, min, max, bucket_count) {
  var h = Telemetry.newHistogram(name, min, max, bucket_count, histogram_type);
  
  var r = h.snapshot().ranges;
  var sum = 0;
  for(var i=0;i<r.length;i++) {
    var v = r[i];
    sum += v;
    h.add(v);
  }
  var s = h.snapshot();
  
  do_check_eq(sum, h.snapshot().sum);;
  
  for each(var i in s.counts) {
    do_check_eq(i, 1);
  }
  var hgrams = Telemetry.histogramSnapshots
  gh = hgrams[name]
  do_check_eq(gh.histogram_type, histogram_type);

  do_check_eq(gh.min, min)
  do_check_eq(gh.max, max)

  
  h.add(false);
  h.add(true);
  var s = h.snapshot().counts;
  do_check_eq(s[0], 2)
  do_check_eq(s[1], 2)
}

function expect_fail(f) {
  let failed = false;
  try {
    f();
    failed = false;
  } catch (e) {
    failed = true;
  }
  do_check_true(failed);
}

function expect_success(f) {
  let succeeded = false;
  try {
    f();
    succeeded = true;
  } catch (e) {
    succeeded = false;
  }
  do_check_true(succeeded);
}

function test_boolean_histogram()
{
  var h = Telemetry.newHistogram("test::boolean histogram", 99,1,4, Telemetry.HISTOGRAM_BOOLEAN);
  var r = h.snapshot().ranges;
  
  do_check_eq(uneval(r), uneval([0, 1, 2]))
  var sum = 0
  for(var i=0;i<r.length;i++) {
    var v = r[i];
    sum += v;
    h.add(v);
  }
  h.add(true);
  h.add(false);
  var s = h.snapshot();
  do_check_eq(s.histogram_type, Telemetry.HISTOGRAM_BOOLEAN);
  
  do_check_eq(s.counts[2], 0);
  do_check_eq(s.sum, 3);
  do_check_eq(s.counts[0], 2);
}

function test_getHistogramById() {
  try {
    Telemetry.getHistogramById("nonexistent");
    do_throw("This can't happen");
  } catch (e) {
    
  }
  var h = Telemetry.getHistogramById("CYCLE_COLLECTOR");
  var s = h.snapshot();
  do_check_eq(s.histogram_type, Telemetry.HISTOGRAM_EXPONENTIAL);
  do_check_eq(s.min, 1);
  do_check_eq(s.max, 10000);
}

function test_getSlowSQL() {
  var slow = Telemetry.slowSQL;
  do_check_true(("mainThread" in slow) && ("otherThreads" in slow));
}

function test_addons() {
  var addon_id = "testing-addon";
  var fake_addon_id = "fake-addon";
  var name1 = "testing-histogram1";
  var register = Telemetry.registerAddonHistogram;
  expect_success(function ()
                 register(addon_id, name1, 1, 5, 6, Telemetry.HISTOGRAM_LINEAR));
  
  expect_fail(function ()
	      register(addon_id, name1, 1, 5, 6, Telemetry.HISTOGRAM_LINEAR));
  
  expect_fail(function () Telemetry.getAddonHistogram(fake_addon_id, name1));

  
  var h1 = Telemetry.getAddonHistogram(addon_id, name1);
  h1.add(1);
  h1.add(3);
  var s1 = h1.snapshot();
  do_check_eq(s1.histogram_type, Telemetry.HISTOGRAM_LINEAR);
  do_check_eq(s1.min, 1);
  do_check_eq(s1.max, 5);
  do_check_eq(s1.counts[1], 1);
  do_check_eq(s1.counts[3], 1);

  var name2 = "testing-histogram2";
  expect_success(function ()
                 register(addon_id, name2, 2, 4, 4, Telemetry.HISTOGRAM_LINEAR));

  var h2 = Telemetry.getAddonHistogram(addon_id, name2);
  h2.add(2);
  h2.add(3);
  var s2 = h2.snapshot();
  do_check_eq(s2.histogram_type, Telemetry.HISTOGRAM_LINEAR);
  do_check_eq(s2.min, 2);
  do_check_eq(s2.max, 4);
  do_check_eq(s2.counts[1], 1);
  do_check_eq(s2.counts[2], 1);

  
  
  var extra_addon = "testing-extra-addon";
  expect_success(function ()
		 register(extra_addon, name1, 0, 1, 2, Telemetry.HISTOGRAM_BOOLEAN));

  
  snapshots = Telemetry.addonHistogramSnapshots;
  do_check_true(addon_id in snapshots)
  do_check_true(extra_addon in snapshots);

  
  do_check_true(name1 in snapshots[addon_id]);
  do_check_true(name2 in snapshots[addon_id]);
  var s1_alt = snapshots[addon_id][name1];
  var s2_alt = snapshots[addon_id][name2];
  do_check_eq(s1_alt.min, s1.min);
  do_check_eq(s1_alt.max, s1.max);
  do_check_eq(s1_alt.histogram_type, s1.histogram_type);
  do_check_eq(s2_alt.min, s2.min);
  do_check_eq(s2_alt.max, s2.max);
  do_check_eq(s2_alt.histogram_type, s2.histogram_type);

  
  do_check_false(name1 in snapshots[extra_addon]);

  
  Telemetry.unregisterAddonHistograms(addon_id);
  snapshots = Telemetry.addonHistogramSnapshots;
  do_check_false(addon_id in snapshots);
  
  do_check_true(extra_addon in snapshots);
}


function test_privateMode() {
  var h = Telemetry.newHistogram("test::private_mode_boolean", 1,2,3, Telemetry.HISTOGRAM_BOOLEAN);
  var orig = h.snapshot();
  Telemetry.canRecord = false;
  h.add(1);
  do_check_eq(uneval(orig), uneval(h.snapshot()));
  Telemetry.canRecord = true;
  h.add(1);
  do_check_neq(uneval(orig), uneval(h.snapshot()));
}

function generateUUID() {
  let str = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator).generateUUID().toString();
  
  return str.substring(1, str.length - 1);
}


function test_loadSave()
{
  let dirService = Cc["@mozilla.org/file/directory_service;1"]
                    .getService(Ci.nsIProperties);
  let tmpDir = dirService.get("TmpD", Ci.nsILocalFile);
  let tmpFile = tmpDir.clone();
  tmpFile.append("saved-histograms.dat");
  if (tmpFile.exists()) {
    tmpFile.remove(true);
  }

  let saveFinished = false;
  let loadFinished = false;
  let uuid = generateUUID();
  let loadCallback = function(data) {
    do_check_true(data != null);
    do_check_eq(uuid, data.uuid);
    loadFinished = true;
    do_test_finished();
  };
  let saveCallback = function(success) {
    do_check_true(success);
    Telemetry.loadHistograms(tmpFile, loadCallback);
    saveFinished = true;
  };
  do_test_pending();
  Telemetry.saveHistograms(tmpFile, uuid, saveCallback, false);
  do_register_cleanup(function () do_check_true(saveFinished));
  do_register_cleanup(function () do_check_true(loadFinished));
  do_register_cleanup(function () tmpFile.remove(true));
}

function run_test()
{
  let kinds = [Telemetry.HISTOGRAM_EXPONENTIAL, Telemetry.HISTOGRAM_LINEAR]
  for each (let histogram_type in kinds) {
    let [min, max, bucket_count] = [1, INT_MAX - 1, 10]
    test_histogram(histogram_type, "test::"+histogram_type, min, max, bucket_count);
    
    const nh = Telemetry.newHistogram;
    expect_fail(function () nh("test::min", 0, max, bucket_count, histogram_type));
    expect_fail(function () nh("test::bucket_count", min, max, 1, histogram_type));
  }

  test_boolean_histogram();
  test_getHistogramById();
  test_getSlowSQL();
  test_privateMode();
  test_addons();
  test_loadSave();
}
