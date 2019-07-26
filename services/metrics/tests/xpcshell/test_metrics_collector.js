


"use strict";

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/services/metrics/collector.jsm");
Cu.import("resource://gre/modules/services/metrics/dataprovider.jsm");
Cu.import("resource://testing-common/services/metrics/mocks.jsm");


function run_test() {
  run_next_test();
};

add_test(function test_constructor() {
  let collector = new MetricsCollector();

  run_next_test();
});

add_test(function test_register_provider() {
  let collector = new MetricsCollector();
  let dummy = new DummyProvider();

  collector.registerProvider(dummy);
  do_check_eq(collector._providers.length, 1);
  collector.registerProvider(dummy);
  do_check_eq(collector._providers.length, 1);

  let failed = false;
  try {
    collector.registerProvider({});
  } catch (ex) {
    do_check_true(ex.message.startsWith("argument must be a MetricsProvider"));
    failed = true;
  } finally {
    do_check_true(failed);
    failed = false;
  }

  run_next_test();
});

add_test(function test_collect_constant_measurements() {
  let collector = new MetricsCollector();
  let provider = new DummyProvider();
  collector.registerProvider(provider);

  do_check_eq(provider.collectConstantCount, 0);

  collector.collectConstantMeasurements().then(function onResult() {
    do_check_eq(provider.collectConstantCount, 1);
    do_check_eq(collector.collectionResults.size, 1);
    do_check_true(collector.collectionResults.has("DummyProvider"));

    let result = collector.collectionResults.get("DummyProvider");
    do_check_true(result instanceof MetricsCollectionResult);

    do_check_true(collector._providers[0].constantsCollected);

    run_next_test();
  });
});

add_test(function test_collect_constant_onetime() {
  let collector = new MetricsCollector();
  let provider = new DummyProvider();
  collector.registerProvider(provider);

  collector.collectConstantMeasurements().then(function onResult() {
    do_check_eq(provider.collectConstantCount, 1);

    collector.collectConstantMeasurements().then(function onResult() {
      do_check_eq(provider.collectConstantCount, 1);

      run_next_test();
    });
  });
});

add_test(function test_collect_multiple() {
  let collector = new MetricsCollector();

  for (let i = 0; i < 10; i++) {
    collector.registerProvider(new DummyProvider("provider" + i));
  }

  do_check_eq(collector._providers.length, 10);

  collector.collectConstantMeasurements().then(function onResult(innerCollector) {
    do_check_eq(collector, innerCollector);
    do_check_eq(collector.collectionResults.size, 10);

    run_next_test();
  });
});

add_test(function test_collect_aggregate() {
  let collector = new MetricsCollector();

  let dummy1 = new DummyProvider();
  dummy1.constantMeasurementName = "measurement1";

  let dummy2 = new DummyProvider();
  dummy2.constantMeasurementName = "measurement2";

  collector.registerProvider(dummy1);
  collector.registerProvider(dummy2);
  do_check_eq(collector._providers.length, 2);

  collector.collectConstantMeasurements().then(function onResult() {
    do_check_eq(collector.collectionResults.size, 1);

    let measurements = collector.collectionResults.get("DummyProvider").measurements;
    do_check_eq(measurements.size, 2);
    do_check_true(measurements.has("measurement1"));
    do_check_true(measurements.has("measurement2"));

    run_next_test();
  });
});

