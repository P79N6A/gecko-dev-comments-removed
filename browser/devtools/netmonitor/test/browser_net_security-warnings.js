


"use strict";





const TEST_CASES = [
  {
    desc: "no warnings",
    uri: "https://example.com" + CORS_SJS_PATH,
    warnCipher: false,
  },
  {
    desc: "cipher warning",
    uri: "https://rc4.example.com" + CORS_SJS_PATH,
    warnCipher: true,
  },
];

add_task(function* () {
  let [tab, debuggee, monitor] = yield initNetMonitor(CUSTOM_GET_URL);
  let { $, EVENTS, NetMonitorView } = monitor.panelWin;
  let { RequestsMenu, NetworkDetails } = NetMonitorView;
  RequestsMenu.lazyUpdate = false;

  info("Enabling RC4 for the test.");
  yield new promise(resolve => {
    SpecialPowers.pushPrefEnv({"set": [
      ["security.tls.insecure_fallback_hosts", "rc4.example.com"]
    ]}, resolve);
  });

  let cipher = $("#security-warning-cipher");

  for (let test of TEST_CASES) {
    info("Testing site with " + test.desc);

    info("Performing request to " + test.uri);
    debuggee.performRequests(1, test.uri);
    yield waitForNetworkEvents(monitor, 1);

    info("Selecting the request.");
    RequestsMenu.selectedIndex = 0;

    info("Waiting for details pane to be updated.");
    yield monitor.panelWin.once(EVENTS.TAB_UPDATED);

    if (NetworkDetails.widget.selectedIndex !== 5) {
      info("Selecting security tab.");
      NetworkDetails.widget.selectedIndex = 5;

      info("Waiting for details pane to be updated.");
      yield monitor.panelWin.once(EVENTS.TAB_UPDATED);
    }

    is(cipher.hidden, !test.warnCipher, "Cipher suite warning is hidden.");

    RequestsMenu.clear();

  }

  yield teardown(monitor);

});
