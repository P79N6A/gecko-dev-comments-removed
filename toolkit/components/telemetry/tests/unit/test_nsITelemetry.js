


const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const INT_MAX = 0x7FFFFFFF;

const Telemetry = Cc["@mozilla.org/base/telemetry;1"].getService(Ci.nsITelemetry);
Cu.import("resource://gre/modules/Services.jsm", this);

function test_expired_histogram() {
  var histogram_id = "FOOBAR";
  var test_expired_id = "TELEMETRY_TEST_EXPIRED";
  var clone_id = "ExpiredClone";
  var dummy = Telemetry.newHistogram(histogram_id, "28.0a1", 1, 2, 3, Telemetry.HISTOGRAM_EXPONENTIAL);
  var dummy_clone = Telemetry.histogramFrom(clone_id, test_expired_id);
  var rh = Telemetry.registeredHistograms([]);

  dummy.add(1);
  dummy_clone.add(1);

  do_check_eq(Telemetry.histogramSnapshots["__expired__"], undefined);
  do_check_eq(Telemetry.histogramSnapshots[histogram_id], undefined);
  do_check_eq(Telemetry.histogramSnapshots[test_expired_id], undefined);
  do_check_eq(Telemetry.histogramSnapshots[clone_id], undefined);
  do_check_eq(rh[test_expired_id], undefined);
}

function test_histogram(histogram_type, name, min, max, bucket_count) {
  var h = Telemetry.newHistogram(name, "never", min, max, bucket_count, histogram_type);
  var r = h.snapshot().ranges;
  var sum = 0;
  var log_sum = 0;
  var log_sum_squares = 0;
  for(var i=0;i<r.length;i++) {
    var v = r[i];
    sum += v;
    if (histogram_type == Telemetry.HISTOGRAM_EXPONENTIAL) {
      var log_v = Math.log(1+v);
      log_sum += log_v;
      log_sum_squares += log_v*log_v;
    }
    h.add(v);
  }
  var s = h.snapshot();
  
  do_check_eq(sum, s.sum);
  if (histogram_type == Telemetry.HISTOGRAM_EXPONENTIAL) {
    
    
    
    do_check_eq(Math.floor(log_sum), Math.floor(s.log_sum));
    do_check_eq(Math.floor(log_sum_squares), Math.floor(s.log_sum_squares));
    do_check_false("sum_squares_lo" in s);
    do_check_false("sum_squares_hi" in s);
  } else {
    
    
    do_check_neq(s.sum_squares_lo + s.sum_squares_hi, 0);
    do_check_false("log_sum" in s);
    do_check_false("log_sum_squares" in s);
  }

  
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

  
  h.clear();
  var s = h.snapshot();
  for each(var i in s.counts) {
    do_check_eq(i, 0);
  }
  do_check_eq(s.sum, 0);
  if (histogram_type == Telemetry.HISTOGRAM_EXPONENTIAL) {
    do_check_eq(s.log_sum, 0);
    do_check_eq(s.log_sum_squares, 0);
  } else {
    do_check_eq(s.sum_squares_lo, 0);
    do_check_eq(s.sum_squares_hi, 0);
  }

  h.add(0);
  h.add(1);
  var c = h.snapshot().counts;
  do_check_eq(c[0], 1);
  do_check_eq(c[1], 1);
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
  var h = Telemetry.newHistogram("test::boolean histogram", "never", 99,1,4, Telemetry.HISTOGRAM_BOOLEAN);
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

function test_flag_histogram()
{
  var h = Telemetry.newHistogram("test::flag histogram", "never", 130, 4, 5, Telemetry.HISTOGRAM_FLAG);
  var r = h.snapshot().ranges;
  
  do_check_eq(uneval(r), uneval([0, 1, 2]))
  
  var c = h.snapshot().counts;
  var s = h.snapshot().sum;
  do_check_eq(uneval(c), uneval([1, 0, 0]));
  do_check_eq(s, 0);
  
  h.add(1);
  var c2 = h.snapshot().counts;
  var s2 = h.snapshot().sum;
  do_check_eq(uneval(c2), uneval([0, 1, 0]));
  do_check_eq(s2, 1);
  
  h.add(1);
  var c3 = h.snapshot().counts;
  var s3 = h.snapshot().sum;
  do_check_eq(uneval(c3), uneval([0, 1, 0]));
  do_check_eq(s3, 1);
  do_check_eq(h.snapshot().histogram_type, Telemetry.HISTOGRAM_FLAG);
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

function compareHistograms(h1, h2) {
  let s1 = h1.snapshot();
  let s2 = h2.snapshot();

  do_check_eq(s1.histogram_type, s2.histogram_type);
  do_check_eq(s1.min, s2.min);
  do_check_eq(s1.max, s2.max);
  do_check_eq(s1.sum, s2.sum);
  if (s1.histogram_type == Telemetry.HISTOGRAM_EXPONENTIAL) {
    do_check_eq(s1.log_sum, s2.log_sum);
    do_check_eq(s1.log_sum_squares, s2.log_sum_squares);
  } else {
    do_check_eq(s1.sum_squares_lo, s2.sum_squares_lo);
    do_check_eq(s1.sum_squares_hi, s2.sum_squares_hi);
  }

  do_check_eq(s1.counts.length, s2.counts.length);
  for (let i = 0; i < s1.counts.length; i++)
    do_check_eq(s1.counts[i], s2.counts[i]);

  do_check_eq(s1.ranges.length, s2.ranges.length);
  for (let i = 0; i < s1.ranges.length; i++)
    do_check_eq(s1.ranges[i], s2.ranges[i]);
}

function test_histogramFrom() {
  
  let names = [
      "CYCLE_COLLECTOR",      
      "GC_REASON_2",          
      "GC_RESET",             
      "TELEMETRY_TEST_FLAG"   
  ];

  for each (let name in names) {
    let [min, max, bucket_count] = [1, INT_MAX - 1, 10]
    let original = Telemetry.getHistogramById(name);
    let clone = Telemetry.histogramFrom("clone" + name, name);
    compareHistograms(original, clone);
  }

  
  let testFlag = Telemetry.getHistogramById("TELEMETRY_TEST_FLAG");
  testFlag.add(1);
  let clone = Telemetry.histogramFrom("FlagClone", "TELEMETRY_TEST_FLAG");
  compareHistograms(testFlag, clone);
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
  
  var snapshots = Telemetry.addonHistogramSnapshots;
  do_check_false(name1 in snapshots[addon_id]);
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

  
  var flag_addon = "testing-flag-addon";
  var flag_histogram = "flag-histogram";
  expect_success(function() 
                 register(flag_addon, flag_histogram, 0, 1, 2, Telemetry.HISTOGRAM_FLAG))
  expect_success(function()
		 register(flag_addon, name2, 2, 4, 4, Telemetry.HISTOGRAM_LINEAR));

  
  snapshots = Telemetry.addonHistogramSnapshots;
  do_check_true(addon_id in snapshots)
  do_check_true(extra_addon in snapshots);
  do_check_true(flag_addon in snapshots);

  
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

  
  do_check_true(flag_histogram in snapshots[flag_addon]);
  do_check_false(name2 in snapshots[flag_addon]);

  
  Telemetry.unregisterAddonHistograms(addon_id);
  snapshots = Telemetry.addonHistogramSnapshots;
  do_check_false(addon_id in snapshots);
  
  do_check_true(extra_addon in snapshots);
}


function test_privateMode() {
  var h = Telemetry.newHistogram("test::private_mode_boolean", "never", 1,2,3, Telemetry.HISTOGRAM_BOOLEAN);
  var orig = h.snapshot();
  Telemetry.canRecord = false;
  h.add(1);
  do_check_eq(uneval(orig), uneval(h.snapshot()));
  Telemetry.canRecord = true;
  h.add(1);
  do_check_neq(uneval(orig), uneval(h.snapshot()));
}



function test_extended_stats() {
  var h = Telemetry.getHistogramById("GRADIENT_DURATION");
  var s = h.snapshot();
  do_check_eq(s.sum, 0);
  do_check_eq(s.log_sum, 0);
  do_check_eq(s.log_sum_squares, 0);
  h.add(1);
  s = h.snapshot();
  do_check_eq(s.sum, 1);
  do_check_eq(s.log_sum, 0);
  do_check_eq(s.log_sum_squares, 0);
}

function generateUUID() {
  let str = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator).generateUUID().toString();
  
  return str.substring(1, str.length - 1);
}

function run_test()
{
  let kinds = [Telemetry.HISTOGRAM_EXPONENTIAL, Telemetry.HISTOGRAM_LINEAR]
  for each (let histogram_type in kinds) {
    let [min, max, bucket_count] = [1, INT_MAX - 1, 10]
    test_histogram(histogram_type, "test::"+histogram_type, min, max, bucket_count);

    const nh = Telemetry.newHistogram;
    expect_fail(function () nh("test::min", "never", 0, max, bucket_count, histogram_type));
    expect_fail(function () nh("test::bucket_count", "never", min, max, 1, histogram_type));
  }

  
  
  let h = Telemetry.getHistogramById("NEWTAB_PAGE_PINNED_SITES_COUNT");
  do_check_false("NEWTAB_PAGE_PINNED_SITES_COUNT" in Telemetry.histogramSnapshots);

  test_boolean_histogram();
  test_flag_histogram();
  test_getHistogramById();
  test_histogramFrom();
  test_getSlowSQL();
  test_privateMode();
  test_addons();
  test_extended_stats();
  test_expired_histogram();
}
