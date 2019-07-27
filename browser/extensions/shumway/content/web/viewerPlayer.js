















var release = true;
var SHUMWAY_ROOT = "resource://shumway/";

var viewerPlayerglobalInfo = {
  abcs: SHUMWAY_ROOT + "playerglobal/playerglobal.abcs",
  catalog: SHUMWAY_ROOT + "playerglobal/playerglobal.json"
};

var builtinPath = SHUMWAY_ROOT + "libs/builtin.abc";

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
  var objectParams = flashParams.objectParams;
  var movieUrl = flashParams.url;

  Shumway.frameRateOption.value = flashParams.turboMode ? 60 : -1;
  Shumway.AVM2.Verifier.enabled.value = compilerSettings.verifier;

  Shumway.createAVM2(builtinPath, viewerPlayerglobalInfo, sysMode, appMode, function (avm2) {
    function runSWF(file, buffer, baseUrl) {
      var player = new Shumway.Player.Window.WindowPlayer(window, window.parent);
      player.defaultStageColor = flashParams.bgcolor;
      player.movieParams = flashParams.movieParams;
      player.stageAlign = (objectParams && (objectParams.salign || objectParams.align)) || '';
      player.stageScale = (objectParams && objectParams.scale) || 'showall';
      player.displayParameters = flashParams.displayParameters;

      Shumway.ExternalInterfaceService.instance = player.createExternalInterfaceService();

      player.pageUrl = baseUrl;
      player.load(file, buffer);
    }
    Shumway.FileLoadingService.instance.setBaseUrl(baseUrl);
    if (asyncLoading) {
      runSWF(movieUrl, undefined, baseUrl);
    } else {
      new Shumway.BinaryFileReader(movieUrl).readAll(null, function(buffer, error) {
        if (!buffer) {
          throw "Unable to open the file " + movieUrl + ": " + error;
        }
        runSWF(movieUrl, buffer, baseUrl);
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

  Shumway.ClipboardService.instance = {
    setClipboard: function (data) {
      window.parent.postMessage({
        callback: 'setClipboard',
        data: data
      }, '*');
    }
  };

  Shumway.FileLoadingService.instance = {
    baseUrl: null,
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
        },
        close: function () {
          if (Shumway.FileLoadingService.instance.sessions[sessionId]) {
            
          }
        }
      };
    },
    setBaseUrl: function (url) {
      Shumway.FileLoadingService.instance.baseUrl = url;
    },
    resolveUrl: function (url) {
      return new URL(url, Shumway.FileLoadingService.instance.baseUrl).href;
    },
    navigateTo: function (url, target) {
      window.parent.postMessage({
        callback: 'navigateTo',
        data: {
          url: this.resolveUrl(url),
          target: target
        }
      }, '*');
    }
  };

  
  if (parent.createSpecialInflate) {
    window.SpecialInflate = function () {
      return parent.createSpecialInflate();
    };
  }

  
  if (parent.createRtmpXHR) {
    window.createRtmpSocket = parent.createRtmpSocket;
    window.createRtmpXHR = parent.createRtmpXHR;
  }
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
      window.parent.postMessage({
        callback: 'started'
      }, '*');
      break;
  }
}, true);
