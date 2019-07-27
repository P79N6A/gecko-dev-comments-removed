



var Cc = SpecialPowers.Cc;
var Ci = SpecialPowers.Ci;
var Cr = SpecialPowers.Cr;


var FAKE_ENABLED = true;
try {
  var audioDevice = SpecialPowers.getCharPref('media.audio_loopback_dev');
  var videoDevice = SpecialPowers.getCharPref('media.video_loopback_dev');
  dump('TEST DEVICES: Using media devices:\n');
  dump('audio: ' + audioDevice + '\nvideo: ' + videoDevice + '\n');
  FAKE_ENABLED = false;
} catch (e) {
  dump('TEST DEVICES: No test devices found (in media.{audio,video}_loopback_dev, using fake streams.\n');
  FAKE_ENABLED = true;
}














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
  if (!("fake" in constraints) && FAKE_ENABLED) {
    constraints["fake"] = FAKE_ENABLED;
  }

  info("Call getUserMedia for " + JSON.stringify(constraints));
  navigator.mozGetUserMedia(constraints, onSuccess, onError);
}










function runTest(aCallback) {
  if (window.SimpleTest) {
    
    SimpleTest.waitForExplicitFinish();
    SpecialPowers.pushPrefEnv({'set': [
      ['dom.messageChannel.enabled', true],
      ['media.peerconnection.enabled', true],
      ['media.peerconnection.identity.enabled', true],
      ['media.peerconnection.identity.timeout', 12000],
      ['media.navigator.permission.disabled', true],
      ['media.getusermedia.screensharing.enabled', true],
      ['media.getusermedia.screensharing.allowed_domains', "mochi.test"]]
    }, function () {
      try {
        aCallback();
      }
      catch (err) {
        generateErrorCallback()(err);
      }
    });
  } else {
    
    window.run_test = function(is_initiator) {
      var options = {is_local: is_initiator,
                     is_remote: !is_initiator};
      aCallback(options);
    };
    
    var s = document.createElement("script");
    s.src = "/test.js";
    document.head.appendChild(s);
  }
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













function getBlobContent(blob, onSuccess) {
  var reader = new FileReader();

  
  reader.onloadend = function (event) {
    onSuccess(event.target.result);
  };

  reader.readAsText(blob);
}










function generateErrorCallback(message) {
  var stack = new Error().stack.split("\n");
  stack.shift(); 

  



  return function (aObj) {
    if (aObj) {
      if (aObj.name && aObj.message) {
        ok(false, "Unexpected callback for '" + aObj.name +
           "' with message = '" + aObj.message + "' at " +
           JSON.stringify(stack));
      } else {
        ok(false, "Unexpected callback with = '" + aObj +
           "' at: " + JSON.stringify(stack));
      }
    } else {
      ok(false, "Unexpected callback with message = '" + message +
         "' at: " + JSON.stringify(stack));
    }
    SimpleTest.finish();
  }
}









function unexpectedEventAndFinish(message, eventName) {
  var stack = new Error().stack.split("\n");
  stack.shift(); 

  return function () {
    ok(false, "Unexpected event '" + eventName + "' fired with message = '" +
       message + "' at: " + JSON.stringify(stack));
    SimpleTest.finish();
  }
}

function IsMacOSX10_6orOlder() {
    var is106orOlder = false;

    if (navigator.platform.indexOf("Mac") == 0) {
        var version = Cc["@mozilla.org/system-info;1"]
                        .getService(SpecialPowers.Ci.nsIPropertyBag2)
                        .getProperty("version");
        
        
        
        is106orOlder = (parseFloat(version) < 11.0);
    }
    return is106orOlder;
}

