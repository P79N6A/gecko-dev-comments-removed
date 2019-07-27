



"use strict";

var Cc = SpecialPowers.Cc;
var Ci = SpecialPowers.Ci;


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














function realCreateHTML(meta) {
  var test = document.getElementById('test');

  
  var elem = document.createElement('meta');
  elem.setAttribute('charset', 'utf-8');
  document.head.appendChild(elem);

  var title = document.createElement('title');
  title.textContent = meta.title;
  document.head.appendChild(title);

  
  var anchor = document.createElement('a');
  anchor.textContent = meta.title;
  if (meta.bug) {
    anchor.setAttribute('href', 'https://bugzilla.mozilla.org/show_bug.cgi?id=' + meta.bug);
  } else {
    anchor.setAttribute('target', '_blank');
  }

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

  
  if (element) {
    return element;
  }

  element = document.createElement(type === 'audio' ? 'audio' : 'video');
  element.setAttribute('id', id);
  element.setAttribute('height', 100);
  element.setAttribute('width', 150);
  element.setAttribute('controls', 'controls');
  element.setAttribute('autoplay', 'autoplay');
  document.getElementById('content').appendChild(element);

  return element;
}









function getUserMedia(constraints) {
  if (!("fake" in constraints) && FAKE_ENABLED) {
    constraints["fake"] = FAKE_ENABLED;
  }

  info("Call getUserMedia for " + JSON.stringify(constraints));
  return navigator.mediaDevices.getUserMedia(constraints);
}




var setTestOptions;
var testConfigured = new Promise(r => setTestOptions = r);

function setupEnvironment() {
  if (!window.SimpleTest) {
    return Promise.resolve();
  }

  
  SimpleTest.requestFlakyTimeout("WebRTC inherently depends on timeouts");
  window.finish = () => SimpleTest.finish();
  SpecialPowers.pushPrefEnv({
    'set': [
      ['dom.messageChannel.enabled', true],
      ['media.peerconnection.enabled', true],
      ['media.peerconnection.identity.enabled', true],
      ['media.peerconnection.identity.timeout', 12000],
      ['media.peerconnection.default_iceservers', '[]'],
      ['media.navigator.permission.disabled', true],
      ['media.getusermedia.screensharing.enabled', true],
      ['media.getusermedia.screensharing.allowed_domains', "mochi.test"]
    ]
  }, setTestOptions);
}




function run_test(is_initiator) {
  var options = { is_local: is_initiator,
                  is_remote: !is_initiator };

  
  var s = document.createElement("script");
  s.src = "/test.js";
  s.onload = () => setTestOptions(options);
  document.head.appendChild(s);
}

function runTestWhenReady(testFunc) {
  setupEnvironment();
  return Promise.all([scriptsReady, testConfigured]).then(() => {
    try {
      return testConfigured.then(options => testFunc(options));
    } catch (e) {
      ok(false, 'Error executing test: ' + e +
         ((typeof e.stack === 'string') ?
          (' ' + e.stack.split('\n').join(' ... ')) : ''));
    }
  });
}












function checkMediaStreamTracksByType(constraints, type, mediaStreamTracks) {
  if (constraints[type]) {
    is(mediaStreamTracks.length, 1, 'One ' + type + ' track shall be present');

    if (mediaStreamTracks.length) {
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




function wait(time) {
  return new Promise(r => setTimeout(r, time));
}


function waitUntil(func, time) {
  return new Promise(resolve => {
    var interval = setInterval(() => {
      if (func())  {
        clearInterval(interval);
        resolve();
      }
    }, time || 200);
  });
}












function generateErrorCallback(message) {
  var stack = new Error().stack.split("\n");
  stack.shift(); 

  



  return aObj => {
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
    throw new Error("Unexpected callback");
  }
}

var unexpectedEventArrived;
var rejectOnUnexpectedEvent = new Promise((x, reject) => {
  unexpectedEventArrived = reject;
});









function unexpectedEvent(message, eventName) {
  var stack = new Error().stack.split("\n");
  stack.shift(); 

  return e => {
    var details = "Unexpected event '" + eventName + "' fired with message = '" +
        message + "' at: " + JSON.stringify(stack);
    ok(false, details);
    unexpectedEventArrived(new Error(details));
  }
}















function createOneShotEventWrapper(wrapper, obj, event) {
  var onx = 'on' + event;
  var unexpected = unexpectedEvent(wrapper, event);
  wrapper[onx] = unexpected;
  obj[onx] = e => {
    info(wrapper + ': "on' + event + '" event fired');
    e.wrapper = wrapper;
    wrapper[onx](e);
    wrapper[onx] = unexpected;
  };
}













function CommandChain(framework, commandList) {
  this._framework = framework;
  this.commands = commandList || [ ];
}

CommandChain.prototype = {
  



  execute: function () {
    return this.commands.reduce((prev, next, i) => {
      if (typeof next !== 'function' || !next.name) {
        throw new Error('registered non-function' + next);
      }

      return prev.then(() => {
        info('Run step ' + (i + 1) + ': ' + next.name);
        return Promise.race([ next(this._framework), rejectOnUnexpectedEvent ]);
      });
    }, Promise.resolve())
      .catch(e =>
             ok(false, 'Error in test execution: ' + e +
                ((typeof e.stack === 'string') ?
                 (' ' + e.stack.split('\n').join(' ... ')) : '')));
  },

  


  append: function(commands) {
    this.commands = this.commands.concat(commands);
  },

  




  indexOf: function(functionOrName, start) {
    start = start || 0;
    if (typeof functionOrName === 'string') {
      var index = this.commands.slice(start).findIndex(f => f.name === functionOrName);
      if (index !== -1) {
        index += start;
      }
      return index;
    }
    return this.commands.indexOf(functionOrName, start);
  },

  


  insertAfter: function(functionOrName, commands) {
    this._insertHelper(functionOrName, commands, 1);
  },

  


  insertAfterEach: function(functionOrName, commands) {
    this._insertHelper(functionOrName, commands, 1, true);
  },

  


  insertBefore: function(functionOrName, commands, all, start) {
    this._insertHelper(functionOrName, commands, 0, all, start);
  },

  _insertHelper: function(functionOrName, commands, delta, all, start) {
    var index = this.indexOf(functionOrName);
    start = start || 0;
    for (; index !== -1; index = this.indexOf(functionOrName, index)) {
      if (!start) {
        this.commands = [].concat(
          this.commands.slice(0, index + delta),
          commands,
          this.commands.slice(index + delta));
        if (!all) {
          break;
        }
      } else {
        start -= 1;
      }
      index += (commands.length + 1);
    }
  },

  


  remove: function(functionOrName) {
    var index = this.indexOf(functionOrName);
    if (index >= 0) {
      return this.commands.splice(index, 1);
    }
    return [];
  },

  


  removeAfter: function(functionOrName) {
    var index = this.indexOf(functionOrName);
    if (index >= 0) {
      return this.commands.splice(index + 1);
    }
    return [];
  },

  


  removeBefore: function(functionOrName) {
    var index = this.indexOf(functionOrName);
    if (index >= 0) {
      return this.commands.splice(0, index);
    }
    return [];
  },

  


  replace: function(functionOrName, commands) {
    this.insertBefore(functionOrName, commands);
    return this.remove(functionOrName);
  },

  


  replaceAfter: function(functionOrName, commands) {
    var oldCommands = this.removeAfter(functionOrName);
    this.append(commands);
    return oldCommands;
  },

  


  replaceBefore: function(functionOrName, commands) {
    var oldCommands = this.removeBefore(functionOrName);
    this.insertBefore(functionOrName, commands);
    return oldCommands;
  },

  


  filterOut: function (id_match) {
    this.commands = this.commands.filter(c => !id_match.test(c.name));
  },
};


function IsMacOSX10_6orOlder() {
  if (navigator.platform.indexOf("Mac") !== 0) {
    return false;
  }

  var version = Cc["@mozilla.org/system-info;1"]
      .getService(Ci.nsIPropertyBag2)
      .getProperty("version");
  
  
  
  return (parseFloat(version) < 11.0);
}

(function(){
  var el = document.createElement("link");
  el.rel = "stylesheet";
  el.type = "text/css";
  el.href= "/tests/SimpleTest/test.css";
  document.head.appendChild(el);
}());
