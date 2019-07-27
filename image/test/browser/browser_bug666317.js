waitForExplicitFinish();

let pageSource =
  '<html><body>' +
    '<img id="testImg" src="' + TESTROOT + 'big.png">' +
  '</body></html>';

let oldDiscardingPref, oldTab, newTab;
let prefBranch = Cc["@mozilla.org/preferences-service;1"]
                   .getService(Ci.nsIPrefService)
                   .getBranch('image.mem.');

var gWaitingForDiscard = false;
var gScriptedObserver;
var gClonedRequest;

function ImageDiscardObserver(callback) {
  this.discard = function onDiscard(request)
  {
    if (!gWaitingForDiscard) {
      return;
    }

    this.synchronous = false;
    callback();
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
          .addEventListener("pageshow", step2);
}

function step2() {
  
  var observer = new ImageDiscardObserver(() => runAfterAsyncEvents(step5));
  gScriptedObserver = Cc["@mozilla.org/image/tools;1"]
                        .getService(Ci.imgITools)
                        .createScriptedObserver(observer);

  
  var request = currentRequest();
  gClonedRequest = request.clone(gScriptedObserver);

  
  forceDecodeImg();

  
  
  runAfterAsyncEvents(step3);
}

function step3() {
  ok(isImgDecoded(), 'Image should initially be decoded.');

  
  
  gBrowser.selectedTab = oldTab;

  
  runAfterAsyncEvents(step4);
}

function step4() {
  gWaitingForDiscard = true;

  var os = Cc["@mozilla.org/observer-service;1"]
             .getService(Ci.nsIObserverService);
  os.notifyObservers(null, 'memory-pressure', 'heap-minimize');

  
  
}

function step5() {
  ok(true, 'Image should be discarded.');

  
  gBrowser.removeTab(newTab);
  prefBranch.setBoolPref('discardable', oldDiscardingPref);

  gClonedRequest.cancelAndForgetObserver(0);

  finish();
}
