


"use strict";

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/Metrics.jsm");
Cu.import("resource://testing-common/services/metrics/mocks.jsm");

function run_test() {
  run_next_test();
};

add_task(function test_constructor() {
  let storage = yield Metrics.Storage("constructor");
  let manager = new Metrics.ProviderManager(storage);

  yield storage.close();
});

add_task(function test_register_provider() {
  let storage = yield Metrics.Storage("register_provider");

  let manager = new Metrics.ProviderManager(storage);
  let dummy = new DummyProvider();

  yield manager.registerProvider(dummy);
  do_check_eq(manager._providers.size, 1);
  yield manager.registerProvider(dummy);
  do_check_eq(manager._providers.size, 1);
  do_check_eq(manager.getProvider(dummy.name), dummy);

  let failed = false;
  try {
    manager.registerProvider({});
  } catch (ex) {
    do_check_true(ex.message.startsWith("Argument must be a Provider"));
    failed = true;
  } finally {
    do_check_true(failed);
    failed = false;
  }

  manager.unregisterProvider(dummy.name);
  do_check_eq(manager._providers.size, 0);
  do_check_null(manager.getProvider(dummy.name));

  yield storage.close();
});

add_task(function test_collect_constant_data() {
  let storage = yield Metrics.Storage("collect_constant_data");
  let errorCount = 0;
  let manager= new Metrics.ProviderManager(storage);
  manager.onProviderError = function () { errorCount++; }
  let provider = new DummyProvider();
  yield manager.registerProvider(provider);

  do_check_eq(provider.collectConstantCount, 0);

  yield manager.collectConstantData();
  do_check_eq(provider.collectConstantCount, 1);

  do_check_true(manager._providers.get("DummyProvider").constantsCollected);

  yield storage.close();
  do_check_eq(errorCount, 0);
});

add_task(function test_collect_constant_throws() {
  let storage = yield Metrics.Storage("collect_constant_throws");
  let manager = new Metrics.ProviderManager(storage);
  let errors = [];
  manager.onProviderError = function (error) { errors.push(error); };

  let provider = new DummyProvider();
  provider.throwDuringCollectConstantData = "Fake error during collect";
  yield manager.registerProvider(provider);

  yield manager.collectConstantData();
  do_check_eq(errors.length, 1);
  do_check_true(errors[0].contains(provider.throwDuringCollectConstantData));

  yield storage.close();
});

add_task(function test_collect_constant_populate_throws() {
  let storage = yield Metrics.Storage("collect_constant_populate_throws");
  let manager = new Metrics.ProviderManager(storage);
  let errors = [];
  manager.onProviderError = function (error) { errors.push(error); };

  let provider = new DummyProvider();
  provider.throwDuringConstantPopulate = "Fake error during constant populate";
  yield manager.registerProvider(provider);

  yield manager.collectConstantData();

  do_check_eq(errors.length, 1);
  do_check_true(errors[0].contains(provider.throwDuringConstantPopulate));
  do_check_false(manager._providers.get(provider.name).constantsCollected);

  yield storage.close();
});

add_task(function test_collect_constant_onetime() {
  let storage = yield Metrics.Storage("collect_constant_onetime");
  let manager = new Metrics.ProviderManager(storage);
  let provider = new DummyProvider();
  yield manager.registerProvider(provider);

  yield manager.collectConstantData();
  do_check_eq(provider.collectConstantCount, 1);

  yield manager.collectConstantData();
  do_check_eq(provider.collectConstantCount, 1);

  yield storage.close();
});

add_task(function test_collect_multiple() {
  let storage = yield Metrics.Storage("collect_multiple");
  let manager = new Metrics.ProviderManager(storage);

  for (let i = 0; i < 10; i++) {
    yield manager.registerProvider(new DummyProvider("provider" + i));
  }

  do_check_eq(manager._providers.size, 10);

  yield manager.collectConstantData();

  yield storage.close();
});

add_task(function test_collect_daily() {
  let storage = yield Metrics.Storage("collect_daily");
  let manager = new Metrics.ProviderManager(storage);

  let provider1 = new DummyProvider("DP1");
  let provider2 = new DummyProvider("DP2");

  yield manager.registerProvider(provider1);
  yield manager.registerProvider(provider2);

  yield manager.collectDailyData();
  do_check_eq(provider1.collectDailyCount, 1);
  do_check_eq(provider2.collectDailyCount, 1);

  yield manager.collectDailyData();
  do_check_eq(provider1.collectDailyCount, 2);
  do_check_eq(provider2.collectDailyCount, 2);

  yield storage.close();
});

