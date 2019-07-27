const KEYSYSTEM_TYPE = "org.w3.clearkey";

function bail(message)
{
  return function(err) {
    ok(false, message);
    if (err) {
      info(err);
    }
    SimpleTest.finish();
  }
}

function ArrayBufferToString(arr)
{
  var str = '';
  var view = new Uint8Array(arr);
  for (var i = 0; i < view.length; i++) {
    str += String.fromCharCode(view[i]);
  }
  return str;
}

function StringToArrayBuffer(str)
{
  var arr = new ArrayBuffer(str.length);
  var view = new Uint8Array(arr);
  for (var i = 0; i < str.length; i++) {
    view[i] = str.charCodeAt(i);
  }
  return arr;
}

function Base64ToHex(str)
{
  var bin = window.atob(str.replace(/-/g, "+").replace(/_/g, "/"));
  var res = "";
  for (var i = 0; i < bin.length; i++) {
    res += ("0" + bin.charCodeAt(i).toString(16)).substr(-2);
  }
  return res;
}

function HexToBase64(hex)
{
  var bin = "";
  for (var i = 0; i < hex.length; i += 2) {
    bin += String.fromCharCode(parseInt(hex.substr(i, 2), 16));
  }
  return window.btoa(bin).replace(/=/g, "").replace(/\+/g, "-").replace(/\//g, "_");
}

function TimeStamp(token) {
  function pad(x) {
    return (x < 10) ? "0" + x : x;
  }
  var now = new Date();
  var ms = now.getMilliseconds();
  var time = "[" +
             pad(now.getHours()) + ":" +
             pad(now.getMinutes()) + ":" +
             pad(now.getSeconds()) + "." +
             ms +
             "]" +
             (ms < 10 ? "  " : (ms < 100 ? " " : ""));
  return token ? (time + " " + token) : time;
}

function Log(token, msg) {
  info(TimeStamp(token) + " " + msg);
}

function UpdateSessionFunc(test, token) {
  return function(ev) {
    var msgStr = ArrayBufferToString(ev.message);
    var msg = JSON.parse(msgStr);

    Log(token, "got message from CDM: " + msgStr);
    is(msg.type, test.sessionType, TimeStamp(token) + " key session type should match");
    ok(msg.kids, TimeStamp(token) + " message event should contain key ID array");

    var outKeys = [];

    for (var i = 0; i < msg.kids.length; i++) {
      var id64 = msg.kids[i];
      var idHex = Base64ToHex(msg.kids[i]).toLowerCase();
      var key = test.keys[idHex];

      if (key) {
        Log(token, "found key " + key + " for key id " + idHex);
        outKeys.push({
          "kty":"oct",
          "alg":"A128KW",
          "kid":id64,
          "k":HexToBase64(key)
        });
      } else {
        bail(token + " couldn't find key for key id " + idHex);
      }
    }

    var update = JSON.stringify({
      "keys" : outKeys,
      "type" : msg.type
    });
    Log(token, "sending update message to CDM: " + update);

    ev.target.update(StringToArrayBuffer(update)).then(function() {
      Log(token, "MediaKeySession update ok!");
    }, bail(token + " MediaKeySession update failed"));
  }
}

function PlayFragmented(test, elem, token)
{
  return new Promise(function(resolve, reject) {
    var ms = new MediaSource();
    elem.src = URL.createObjectURL(ms);

    var sb;
    var curFragment = 0;

    function addNextFragment() {
      if (curFragment >= test.fragments.length) {
        Log(token, "addNextFragment() end of stream");
        ms.endOfStream();
        resolve();
        return;
      }

      var fragmentFile = test.fragments[curFragment++];

      var req = new XMLHttpRequest();
      req.open("GET", fragmentFile);
      req.responseType = "arraybuffer";

      req.addEventListener("load", function() {
        Log(token, "fetch of " + fragmentFile + " complete, appending");
        sb.appendBuffer(new Uint8Array(req.response));
      });

      req.addEventListener("error", function(){info(token + " error fetching " + fragmentFile);});
      req.addEventListener("abort", function(){info(token + " aborted fetching " + fragmentFile);});

      Log(token, "addNextFragment() fetching next fragment " + fragmentFile);
      req.send(null);
    }

    ms.addEventListener("sourceopen", function () {
      Log(token, "sourceopen");
      sb = ms.addSourceBuffer(test.type);
      sb.addEventListener("updateend", addNextFragment);

      addNextFragment();
    })
  });
}




function LoadTest(test, elem, token)
{
  if (test.fragments) {
    return PlayFragmented(test, elem, token);
  }

  
  return new Promise(function(resolve, reject) {
    elem.src = test.name;
    resolve();
  });
}

function SetupEME(test, token, params)
{
  var v = document.createElement("video");

  
  [ "canplay", "canplaythrough", "ended", "error", "loadeddata",
    "loadedmetadata", "loadstart", "pause", "play", "playing", "progress",
    "stalled", "suspend", "waiting",
  ].forEach(function (e) {
    v.addEventListener(e, function(event) {
      Log(token, "" + e);
    }, false);
  });

  var onSetKeysFail = (params && params.onSetKeysFail)
    ? params.onSetKeysFail
    : bail(token + " Failed to set MediaKeys on <video> element");

  v.addEventListener("encrypted", function(ev) {
    Log(token, "got encrypted event");
    MediaKeys.create(KEYSYSTEM_TYPE).then(function(mediaKeys) {
      Log(token, "created MediaKeys object ok");
      mediaKeys.sessions = [];
      return v.setMediaKeys(mediaKeys);
    }, bail("failed to create MediaKeys object")).then(function() {
      Log(token, "set MediaKeys on <video> element ok");

      var session = v.mediaKeys.createSession(test.sessionType);
      if (params && params.onsessioncreated) {
        params.onsessioncreated(session);
      }
      session.addEventListener("message", UpdateSessionFunc(test, token));
      session.generateRequest(ev.initDataType, ev.initData).then(function() {
      }, bail(token + " Failed to initialise MediaKeySession"));

    }, onSetKeysFail);
  });

  return v;
}
