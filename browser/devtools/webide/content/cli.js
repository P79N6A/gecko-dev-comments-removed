


















































window.handleCommandline = function(cmdline) {
  console.log("External query", cmdline);
  let params = new Map();
  for (let token of cmdline.split("&")) {
    token = token.split("=");
    params.set(token[0], token[1]);
  }
  if (params.has("actions")) {
    return UI.busyUntil(Task.spawn(function* () {
      let actions = params.get("actions").split(",");
      for (let action of actions) {
        if (action in CliActions) {
          console.log("External query - running action", action);
          yield CliActions[action].call(window, params);
        } else {
          console.log("External query - unknown action", action);
        }
      }
    }), "Computing command line");
  } else {
    return promise.reject("No actions provided");
  }
}

let CliActions = {
  addPackagedApp: function(params) {
    return Task.spawn(function* () {
      let location = params.get("location");
      if (!location) {
        throw new Error("No location parameter");
      }

      yield AppProjects.load();

      
      let directory = new FileUtils.File(location);
      if (AppProjects.get(directory.path)) {
        
        return;
      }

      yield Cmds.importPackagedApp(location);
    })
  },
  addHostedApp: function(params) {
    return Task.spawn(function* () {
      let location = params.get("location");
      if (!location) {
        throw new Error("No location parameter");
      }
      yield AppProjects.load();
      if (AppProjects.get(location)) {
        
        return;
      }
      yield Cmds.importHostedApp(location);
    })
  },
  debug: function(params) {
    return Task.spawn(function* () {

      let appID = params.get("appID");

      if (appID) {
        let appToSelect;
        for (let i = 0; i < AppManager.webAppsStore.object.all.length; i++) {
          let app = AppManager.webAppsStore.object.all[i];
          if (app.manifestURL == appID) {
            appToSelect = app;
            break;
          }
        }
        if (!appToSelect) {
          throw new Error("App not found on device");
        }
        AppManager.selectedProject = {
          type: "runtimeApp",
          app: appToSelect,
          icon: appToSelect.iconURL,
          name: appToSelect.name
        };
      }

      UI.closeToolbox();

      yield Cmds.toggleToolbox();
    });
  },
  connectToRuntime: function(params) {
    return Task.spawn(function* () {

      let type = params.get("runtimeType");
      if (type != "usb" && type != "simulator") {
        return promise.reject("Unkown runtime type");
      }

      yield Cmds.disconnectRuntime();

      if (AppManager.runtimeList[type].length == 0) {
        let deferred = promise.defer();
        function onRuntimeListUpdate(event, what) {
          if (AppManager.runtimeList[type].length > 0) {
            deferred.resolve();
          }
        }

        let timeout = setTimeout(deferred.resolve, 3000);
        AppManager.on("app-manager-update", onRuntimeListUpdate);
        yield deferred.promise;

        AppManager.off("app-manager-update", onRuntimeListUpdate);
        clearTimeout(timeout);
      }

      let runtime;
      let runtimeID = params.get("runtimeID");

      if (runtimeID) {
        for (let r of AppManager.runtimeList[type]) {
          if (r.getID() == runtimeID) {
            runtime = r;
            break;
          }
        }
      } else {
        let list = AppManager.runtimeList[type];
        runtime = list[list.length - 1];
      }

      if (!runtime) {
        return promise.reject("Can't find any runtime to connect to");
      }

      let deferred = promise.defer();
      
      
      AppManager.webAppsStore.once("store-ready", deferred.resolve);
      UI.connectToRuntime(runtime).then(null, deferred.reject);
      return deferred.promise;
    })
  },
  play: function(params) {
    return Task.spawn(function* () {
      yield Cmds.play();
    })
  },
}
