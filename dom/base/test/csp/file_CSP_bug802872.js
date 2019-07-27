




function createAllowedEvent() {
  



  var src_event = new EventSource("http://mochi.test:8888/tests/dom/base/test/csp/file_CSP_bug802872.sjs");

  src_event.onmessage = function(e) {
    src_event.close();
    parent.dispatchEvent(new Event('allowedEventSrcCallbackOK'));
  }

  src_event.onerror = function(e) {
    src_event.close();
    parent.dispatchEvent(new Event('allowedEventSrcCallbackFailed'));
  }
}

function createBlockedEvent() {
  



  var src_event = new EventSource("http://example.com/tests/dom/base/test/csp/file_CSP_bug802872.sjs");

  src_event.onmessage = function(e) {
    src_event.close();
    parent.dispatchEvent(new Event('blockedEventSrcCallbackOK'));
  }

  src_event.onerror = function(e) {
    src_event.close();
    parent.dispatchEvent(new Event('blockedEventSrcCallbackFailed'));
  }
}

addLoadEvent(createAllowedEvent);
addLoadEvent(createBlockedEvent);
