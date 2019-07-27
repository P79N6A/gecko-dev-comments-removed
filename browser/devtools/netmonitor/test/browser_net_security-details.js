


"use strict";





add_task(function* () {
  let [tab, debuggee, monitor] = yield initNetMonitor(CUSTOM_GET_URL);
  let { $, EVENTS, NetMonitorView } = monitor.panelWin;
  let { RequestsMenu, NetworkDetails } = NetMonitorView;
  RequestsMenu.lazyUpdate = false;

  info("Performing a secure request.");
  debuggee.performRequests(1, "https://example.com" + CORS_SJS_PATH);

  yield waitForNetworkEvents(monitor, 1);

  info("Selecting the request.");
  RequestsMenu.selectedIndex = 0;

  info("Waiting for details pane to be updated.");
  yield monitor.panelWin.once(EVENTS.TAB_UPDATED);

  info("Selecting security tab.");
  NetworkDetails.widget.selectedIndex = 5;

  info("Waiting for security tab to be updated.");
  yield monitor.panelWin.once(EVENTS.TAB_UPDATED);

  let errorbox = $("#security-error");
  let infobox = $("#security-information");

  is(errorbox.hidden, true, "Error box is hidden.");
  is(infobox.hidden, false, "Information box visible.");

  

  
  
  let protocol = $("#security-protocol-version-value").value;
  ok(protocol.startsWith("TLS"), "The protocol " + protocol + " seems valid.");

  
  
  
  let suite = $("#security-ciphersuite-value").value;
  ok(suite.startsWith("TLS_"), "The suite " + suite + " seems valid.");

  
  checkLabel("#security-info-host-header", "Host example.com:");
  checkLabel("#security-http-strict-transport-security-value", "Disabled");
  checkLabel("#security-public-key-pinning-value", "Disabled");

  
  checkLabel("#security-cert-subject-cn", "example.com");
  checkLabel("#security-cert-subject-o", "<Not Available>");
  checkLabel("#security-cert-subject-ou", "<Not Available>");

  checkLabel("#security-cert-issuer-cn", "Temporary Certificate Authority");
  checkLabel("#security-cert-issuer-o", "Mozilla Testing");
  checkLabel("#security-cert-issuer-ou", "<Not Available>");

  
  
  checkLabelNotEmpty("#security-cert-validity-begins");
  checkLabelNotEmpty("#security-cert-validity-expires");

  checkLabelNotEmpty("#security-cert-sha1-fingerprint");
  checkLabelNotEmpty("#security-cert-sha256-fingerprint");
  yield teardown(monitor);

  



  function checkLabel(selector, expected) {
    info("Checking label " + selector);

    let element = $(selector);

    ok(element, "Selector matched an element.");
    is(element.value, expected, "Label has the expected value.");
  }

  


  function checkLabelNotEmpty(selector) {
    info("Checking that label " + selector + " is non-empty.");

    let element = $(selector);

    ok(element, "Selector matched an element.");
    isnot(element.value, "", "Label was not empty.");
  }
});
