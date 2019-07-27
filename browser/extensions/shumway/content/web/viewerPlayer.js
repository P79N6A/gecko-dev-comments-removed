















var release = true;
var SHUMWAY_ROOT = "resource://shumway/";

var viewerPlayerglobalInfo = {
  abcs: SHUMWAY_ROOT + "playerglobal/playerglobal.abcs",
  catalog: SHUMWAY_ROOT + "playerglobal/playerglobal.json"
};

var avm2Root = SHUMWAY_ROOT + "avm2/";
var builtinPath = avm2Root + "generated/builtin/builtin.abc";
var avm1Path = avm2Root + "generated/avm1lib/avm1lib.abc";

window.print = function(msg) {
  console.log(msg);
};

function runSwfPlayer(flashParams) {
  var EXECUTION_MODE = Shumway.AVM2.Runtime.ExecutionMode;

  var compilerSettings = flashParams.compilerSettings;
  var sysMode = compilerSettings.sysCompiler ? EXECUTION_MODE.COMPILE : EXECUTION_MODE.INTERPRET;
  var appMode = compilerSettings.appCompiler ? EXECUTION_MODE.COMPILE : EXECUTION_MODE.INTERPRET;
  var asyncLoading = true;
  var baseUrl = flashParams.baseUrl;
  var movieParams = flashParams.movieParams;
  var objectParams = flashParams.objectParams;
  var movieUrl = flashParams.url;

  Shumway.frameRateOption.value = flashParams.turboMode ? 60 : -1;
  Shumway.AVM2.Verifier.enabled.value = compilerSettings.verifier;

  Shumway.createAVM2(builtinPath, viewerPlayerglobalInfo, avm1Path, sysMode, appMode, function (avm2) {
    function runSWF(file) {
      var player = new Shumway.Player.Window.WindowPlayer(window, window.parent);

      Shumway.ExternalInterfaceService.instance = player.createExternalInterfaceService();

      player.load(file);
    }
    file = Shumway.FileLoadingService.instance.setBaseUrl(baseUrl);
    if (asyncLoading) {
      runSWF(movieUrl);
    } else {
      new Shumway.BinaryFileReader(movieUrl).readAll(null, function(buffer, error) {
        if (!buffer) {
          throw "Unable to open the file " + file + ": " + error;
        }
        runSWF(movieUrl, buffer);
      });
    }
  });
}

var LOADER_WORKER_PATH = SHUMWAY_ROOT + 'web/worker.js';

function setupServices() {
  Shumway.Telemetry.instance = {
    reportTelemetry: function (data) {
      window.parent.postMessage({
        callback: 'reportTelemetry',
        data: data
      }, '*');
    }
  };

  Shumway.FileLoadingService.instance = {
    get baseUrl() {
      return movieUrl;
    },
    nextSessionId: 1, 
    sessions: [],
    createSession: function () {
      var sessionId = this.nextSessionId++;
      return this.sessions[sessionId] = {
        open: function (request) {
          var self = this;
          var path = Shumway.FileLoadingService.instance.resolveUrl(request.url);
          console.log('Session #' + sessionId + ': loading ' + path);
          window.parent.postMessage({
            callback: 'loadFileRequest',
            data: {url: path, method: request.method,
              mimeType: request.mimeType, postData: request.data,
              checkPolicyFile: request.checkPolicyFile, sessionId: sessionId}
          }, '*');
        },
        notify: function (args) {
          switch (args.topic) {
            case "open":
              this.onopen();
              break;
            case "close":
              this.onclose();
              Shumway.FileLoadingService.instance.sessions[sessionId] = null;
              console.log('Session #' + sessionId + ': closed');
              break;
            case "error":
              this.onerror && this.onerror(args.error);
              break;
            case "progress":
              console.log('Session #' + sessionId + ': loaded ' + args.loaded + '/' + args.total);
              this.onprogress && this.onprogress(args.array, {bytesLoaded: args.loaded, bytesTotal: args.total});
              break;
          }
        }
      };
    },
    setBaseUrl: function (url) {
      var baseUrl;
      if (typeof URL !== 'undefined') {
        baseUrl = new URL(url, document.location.href).href;
      } else {
        var a = document.createElement('a');
        a.href = url || '#';
        a.setAttribute('style', 'display: none;');
        document.body.appendChild(a);
        baseUrl = a.href;
        document.body.removeChild(a);
      }
      Shumway.FileLoadingService.instance.baseUrl = baseUrl;
      return baseUrl;
    },
    resolveUrl: function (url) {
      if (url.indexOf('://') >= 0) return url;

      var base = Shumway.FileLoadingService.instance.baseUrl;
      base = base.lastIndexOf('/') >= 0 ? base.substring(0, base.lastIndexOf('/') + 1) : '';
      if (url.indexOf('/') === 0) {
        var m = /^[^:]+:\/\/[^\/]+/.exec(base);
        if (m) base = m[0];
      }
      return base + url;
    }
  };
}

window.addEventListener('message', function onWindowMessage(e) {
  var data = e.data;
  if (typeof data !== 'object' || data === null) {
    return;
  }
  switch (data.type) {
    case "loadFileResponse":
      var args = data.args;
      var session = Shumway.FileLoadingService.instance.sessions[args.sessionId];
      if (session) {
        session.notify(args);
      }
      break;
    case "runSwf":
      if (data.settings) {
        Shumway.Settings.setSettings(data.settings);
      }
      setupServices();
      runSwfPlayer(data.flashParams);
      document.body.style.backgroundColor = 'green';
      break;
  }
}, true);
