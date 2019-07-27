







let gWhitelistedHosts = [
  
  "test1.example.org",
  
  "test2.example.org",
  "sub1.test2.example.org",
  "sub2.test2.example.org"
];


let gNotWhitelistedHosts = [
  
  "sub1.test1.example.org",
  
  "mochi.test:8888"
];


const PREF_UNPREFIXING_SERVICE =
  "layout.css.unprefixing-service.enabled";
const PREF_INCLUDE_TEST_DOMAINS =
  "layout.css.unprefixing-service.include-test-domains";


let gCounter = 0;
function getIncreasingCounter() {
  return gCounter++;
}




function testHost(aHost, aExpectEnabled) {
  
  let url = window.location.protocol; 
  url += "//";
  url += aHost;

  
  const re = /(.*\/).*/;
  url += window.location.pathname.replace(re, "$1");
  url += IFRAME_TESTFILE;
  
  
  url += "?" + getIncreasingCounter();
  
  
  url += (aExpectEnabled ? "#expectEnabled" : "#expectDisabled");

  let iframe = document.getElementById("testIframe");
  iframe.contentWindow.location = url;
  
  
}










function registerPostMessageListener(aTestCompleteCallback) {
  let receiveMessage = function(event) {
    if (event.data.type === "is") {
      is(event.data.actual, event.data.expected, event.data.desc);
    } else if (event.data.type === "ok") {
      ok(event.data.condition, event.data.desc);
    } else if (event.data.type === "testComplete") {
      aTestCompleteCallback();
    } else {
      ok(false, "unrecognized data in postMessage call");
    }
  };

  window.addEventListener("message", receiveMessage, false);
}
