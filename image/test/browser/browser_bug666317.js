waitForExplicitFinish();

let pageSource =
  '<html><body>' +
    '<img id="testImg" src="' + TESTROOT + 'big.png">' +
  '</body></html>';

let oldDiscardingPref, oldTab, newTab;
let prefBranch = Cc["@mozilla.org/preferences-service;1"]
                   .getService(Ci.nsIPrefService)
                   .getBranch('image.mem.');

function ImageDiscardObserver(result) {
  this.discard = function onDiscard(request)
  {
    result.wasDiscarded = true;
    this.synchronous = false;
  }

  this.synchronous = true;
}

function currentRequest() {
  let img = gBrowser.getBrowserForTab(newTab).contentWindow
            .document.getElementById('testImg');
  img.QueryInterface(Ci.nsIImageLoadingContent);
  return img.getRequest(Ci.nsIImageLoadingContent.CURRENT_REQUEST);
}

function isImgDecoded() {
  let request = currentRequest();
  return request.imageStatus & Ci.imgIRequest.STATUS_FRAME_COMPLETE ? true : false;
}


function forceDecodeImg() {
  let doc = gBrowser.getBrowserForTab(newTab).contentWindow.document;
  let img = doc.getElementById('testImg');
  let canvas = doc.createElement('canvas');
  let ctx = canvas.getContext('2d');
  ctx.drawImage(img, 0, 0);
}

function runAfterAsyncEvents(aCallback) {
  function handlePostMessage(aEvent) {
    if (aEvent.data == 'next') {
      window.removeEventListener('message', handlePostMessage, false);
      aCallback();
    }
  }

  window.addEventListener('message', handlePostMessage, false);

  
  
  
  
  window.postMessage('next', '*');
}

function test() {
  
  oldDiscardingPref = prefBranch.getBoolPref('discardable');
  prefBranch.setBoolPref('discardable', true);

  
  oldTab = gBrowser.selectedTab;
  newTab = gBrowser.addTab('data:text/html,' + pageSource);
  gBrowser.selectedTab = newTab;

  
  gBrowser.getBrowserForTab(newTab)
          .addEventListener("pageshow", step2 );
}

function step2() {
  
  var result = { wasDiscarded: false };

  
  var observer = new ImageDiscardObserver(result);
  var scriptedObserver = Cc["@mozilla.org/image/tools;1"]
                           .getService(Ci.imgITools)
                           .createScriptedObserver(observer);

  
  var request = currentRequest();
  var clonedRequest = request.clone(scriptedObserver);

  
  forceDecodeImg();

  
  
  runAfterAsyncEvents(() => step3(result, scriptedObserver, clonedRequest));
}

function step3(result, scriptedObserver, clonedRequest) {
  ok(isImgDecoded(), 'Image should initially be decoded.');

  
  
  gBrowser.selectedTab = oldTab;
  var os = Cc["@mozilla.org/observer-service;1"]
             .getService(Ci.nsIObserverService);
  os.notifyObservers(null, 'memory-pressure', 'heap-minimize');

  
  
  runAfterAsyncEvents(() => step4(result, scriptedObserver, clonedRequest));
}

function step4(result, scriptedObserver, clonedRequest) {
  ok(result.wasDiscarded, 'Image should be discarded.');

  
  gBrowser.removeTab(newTab);
  prefBranch.setBoolPref('discardable', oldDiscardingPref);
  clonedRequest.cancelAndForgetObserver(0);
  finish();
}
