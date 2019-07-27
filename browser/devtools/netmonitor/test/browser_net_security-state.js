


"use strict";






add_task(function* () {
  const EXPECTED_SECURITY_STATES = {
    "test1.example.com": "security-state-insecure",
    "example.com": "security-state-secure",
    "nocert.example.com": "security-state-broken",
    "rc4.example.com": "security-state-weak",
  };

  yield new promise(resolve => {
    SpecialPowers.pushPrefEnv({"set": [
      ["security.tls.insecure_fallback_hosts", "rc4.example.com"]
    ]}, resolve);
  });

  let [tab, debuggee, monitor] = yield initNetMonitor(CUSTOM_GET_URL);
  let { $, EVENTS, NetMonitorView } = monitor.panelWin;
  let { RequestsMenu } = NetMonitorView;
  RequestsMenu.lazyUpdate = false;

  yield performRequests();

  for (let item of RequestsMenu.items) {
    let domain = $(".requests-menu-domain", item.target).value;

    info("Found a request to " + domain);
    ok(domain in EXPECTED_SECURITY_STATES, "Domain " + domain + " was expected.");

    let classes = $(".requests-security-state-icon", item.target).classList;
    let expectedClass = EXPECTED_SECURITY_STATES[domain];

    info("Classes of security state icon are: " + classes);
    info("Security state icon is expected to contain class: " + expectedClass);
    ok(classes.contains(expectedClass), "Icon contained the correct class name.");
  }

  yield teardown(monitor);

  






  function* performRequests() {
    
    
    let done = waitForSecurityBrokenNetworkEvent();

    info("Requesting a resource that has a certificate problem.");
    debuggee.performRequests(1, "https://nocert.example.com");

    
    
    info("Waiting for request to complete.");
    yield done;

    
    
    done = waitForNetworkEvents(monitor, 1);
    info("Requesting a resource over HTTP.");
    debuggee.performRequests(1, "http://test1.example.com" + CORS_SJS_PATH);
    yield done;

    done = waitForNetworkEvents(monitor, 1);
    info("Requesting a resource over HTTPS.");
    debuggee.performRequests(1, "https://example.com" + CORS_SJS_PATH);
    yield done;

    done = waitForNetworkEvents(monitor, 1);
    info("Requesting a resource over HTTPS with RC4.");
    debuggee.performRequests(1, "https://rc4.example.com" + CORS_SJS_PATH);
    yield done;

    is(RequestsMenu.itemCount, 4, "Four events logged.");
  }

  



  function waitForSecurityBrokenNetworkEvent() {
    let awaitedEvents = [
      "UPDATING_REQUEST_HEADERS",
      "RECEIVED_REQUEST_HEADERS",
      "UPDATING_REQUEST_COOKIES",
      "RECEIVED_REQUEST_COOKIES",
      "STARTED_RECEIVING_RESPONSE",
      "UPDATING_RESPONSE_CONTENT",
      "RECEIVED_RESPONSE_CONTENT",
      "UPDATING_EVENT_TIMINGS",
      "RECEIVED_EVENT_TIMINGS",
    ];

    let promises = awaitedEvents.map((event) => {
      return monitor.panelWin.once(EVENTS[event]);
    });

    return Promise.all(promises);
  }
});
