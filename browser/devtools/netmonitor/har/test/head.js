

"use strict";

let { Services } = Cu.import("resource://gre/modules/Services.jsm", {});


let netMonitorHead = "chrome://mochitests/content/browser/browser/devtools/netmonitor/test/head.js";
Services.scriptloader.loadSubScript(netMonitorHead, this);


const HAR_EXAMPLE_URL = "http://example.com/browser/browser/devtools/netmonitor/har/test/";
