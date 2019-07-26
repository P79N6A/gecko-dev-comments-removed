



var Cc = SpecialPowers.Cc;
var Ci = SpecialPowers.Ci;
var Cr = SpecialPowers.Cr;


var FAKE_ENABLED = true;














function createHTML(meta) {
  var test = document.getElementById('test');

  
  var elem = document.createElement('meta');
  elem.setAttribute('charset', 'utf-8');
  document.head.appendChild(elem);

  var title = document.createElement('title');
  title.textContent = meta.title;
  document.head.appendChild(title);

  
  var anchor = document.createElement('a');
  anchor.setAttribute('target', '_blank');

  if (meta.bug) {
    anchor.setAttribute('href', 'https://bugzilla.mozilla.org/show_bug.cgi?id=' + meta.bug);
  }

  anchor.textContent = meta.title;
  document.body.insertBefore(anchor, test);

  var display = document.createElement('p');
  display.setAttribute('id', 'display');
  document.body.insertBefore(display, test);

  var content = document.createElement('div');
  content.setAttribute('id', 'content');
  content.style.display = meta.visible ? 'block' : "none";
  document.body.appendChild(content);
}












function createMediaElement(type, label) {
  var id = label + '_' + type;
  var element = document.getElementById(id);

  
  if (element)
    return element;

  element = document.createElement(type === 'audio' ? 'audio' : 'video');
  element.setAttribute('id', id);
  element.setAttribute('height', 100);
  element.setAttribute('width', 150);
  element.setAttribute('controls', 'controls');
  document.getElementById('content').appendChild(element);

  return element;
}













function getUserMedia(constraints, onSuccess, onError) {
  constraints["fake"] = FAKE_ENABLED;

  info("Call getUserMedia for " + JSON.stringify(constraints));
  navigator.mozGetUserMedia(constraints, onSuccess, onError);
}










function runTest(aCallback) {
  SimpleTest.waitForExplicitFinish();
  SpecialPowers.pushPrefEnv({'set': [
    ['media.peerconnection.enabled', true],
    ['media.navigator.permission.disabled', true]]
  }, function () {
    try {
      aCallback();
    }
    catch (err) {
      unexpectedCallbackAndFinish(new Error)(err);
    }
  });
}











function checkMediaStreamTracksByType(constraints, type, mediaStreamTracks) {
  if(constraints[type]) {
    is(mediaStreamTracks.length, 1, 'One ' + type + ' track shall be present');

    if(mediaStreamTracks.length) {
      is(mediaStreamTracks[0].kind, type, 'Track kind should be ' + type);
      ok(mediaStreamTracks[0].id, 'Track id should be defined');
    }
  } else {
    is(mediaStreamTracks.length, 0, 'No ' + type + ' tracks shall be present');
  }
}









function checkMediaStreamTracks(constraints, mediaStream) {
  checkMediaStreamTracksByType(constraints, 'audio',
    mediaStream.getAudioTracks());
  checkMediaStreamTracksByType(constraints, 'video',
    mediaStream.getVideoTracks());
}










function unexpectedCallbackAndFinish(error) {
  



  return function(aObj) {
    var where = error.fileName + ":" + error.lineNumber;
    if (aObj && aObj.name && aObj.message) {
      ok(false, "Unexpected error callback from " + where + " with name = '" +
                aObj.name + "', message = '" + aObj.message + "'");
    } else {
      ok(false, "Unexpected error callback from " + where + " with " + aObj);
    }
    SimpleTest.finish();
  }
}







function unexpectedEventAndFinish(description, eventName) {
  return function () {
    var e = new Error();
    ok(false, "Unexpected event '" + eventName + "' fired for " + description +
       " " + e.stack.split("\n")[1]);
    SimpleTest.finish();
  }
}
