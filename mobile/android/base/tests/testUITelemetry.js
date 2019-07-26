




Components.utils.import("resource://gre/modules/Services.jsm");

function do_check_array_eq(a1, a2) {
  do_check_eq(a1.length, a2.length);
  for (let i = 0; i < a1.length; ++i) {
    do_check_eq(a1[i], a2[i]);
  }
}

add_test(function test_telemetry_events() {
  let bridge = Components.classes["@mozilla.org/android/bridge;1"]
                         .getService(Components.interfaces.nsIAndroidBridge);
  let obsXPCOM = bridge.browserApp.getUITelemetryObserver();
  do_check_true(!!obsXPCOM);

  let obs = obsXPCOM.wrappedJSObject;
  do_check_true(!!obs);

  let measurements = obs.getUIMeasurements();

  let expected = [
    ["event", "enone", "method0", [], null],
    ["event", "efoo", "method1", ["foo"], null],
    ["event", "efoo", "method2", ["foo"], null],
    ["event", "efoobar", "method3", ["foo", "bar"], "foobarextras"],
    ["session", "foo", "reasonfoo"],
    ["event", "ebar", "method4", ["bar"], "barextras"],
    ["session", "bar", "reasonbar"],
    ["event", "enone", "method5", [], null],
  ];

  do_check_eq(expected.length, measurements.length);

  for (let i = 0; i < measurements.length; ++i) {
    let m = measurements[i];

    let type = m[0];
    if (type == "event") {
      let [type, action, method, sessions, extras] = expected[i];
      do_check_eq(m.action, action);
      do_check_eq(m.method, method);
      do_check_array_eq(m.sessions, sessions);
      do_check_eq(m.extras, extras);
      continue;
    }

    if (type == "session") {
      let [type, name, reason] = expected[i];
      do_check_eq(m.name, name);
      do_check_eq(m.reason, method);
      continue;
    }
  }

  run_next_test();
});

run_next_test();
