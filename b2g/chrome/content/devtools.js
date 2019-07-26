



'use strict';

const DEVELOPER_HUD_LOG_PREFIX = 'DeveloperHUD';

XPCOMUtils.defineLazyGetter(this, 'devtools', function() {
  const {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
  return devtools;
});

XPCOMUtils.defineLazyGetter(this, 'DebuggerClient', function() {
  return Cu.import('resource://gre/modules/devtools/dbg-client.jsm', {}).DebuggerClient;
});

XPCOMUtils.defineLazyGetter(this, 'WebConsoleUtils', function() {
  return devtools.require("devtools/toolkit/webconsole/utils").Utils;
});

XPCOMUtils.defineLazyGetter(this, 'EventLoopLagFront', function() {
  return devtools.require("devtools/server/actors/eventlooplag").EventLoopLagFront;
});

XPCOMUtils.defineLazyGetter(this, 'MemoryFront', function() {
  return devtools.require("devtools/server/actors/memory").MemoryFront;
});







let developerHUD = {

  _targets: new Map(),
  _frames: new Map(),
  _client: null,
  _webappsActor: null,
  _watchers: [],

  





  registerWatcher: function dwp_registerWatcher(watcher) {
    this._watchers.unshift(watcher);
  },

  init: function dwp_init() {
    if (this._client)
      return;

    if (!DebuggerServer.initialized) {
      RemoteDebugger.start();
    }

    this._client = new DebuggerClient(DebuggerServer.connectPipe());
    this._client.connect((type, traits) => {

      
      this._client.listTabs((res) => {
        this._webappsActor = res.webappsActor;

        for (let w of this._watchers) {
          if (w.init) {
            w.init(this._client);
          }
        }

        Services.obs.addObserver(this, 'remote-browser-shown', false);
        Services.obs.addObserver(this, 'inprocess-browser-shown', false);
        Services.obs.addObserver(this, 'message-manager-disconnect', false);

        let systemapp = document.querySelector('#systemapp');
        this.trackFrame(systemapp);

        let frames = systemapp.contentWindow.document.querySelectorAll('iframe[mozapp]');
        for (let frame of frames) {
          this.trackFrame(frame);
        }
      });
    });
  },

  uninit: function dwp_uninit() {
    if (!this._client)
      return;

    for (let frame of this._targets.keys()) {
      this.untrackFrame(frame);
    }

    Services.obs.removeObserver(this, 'remote-browser-shown');
    Services.obs.removeObserver(this, 'inprocess-browser-shown');
    Services.obs.removeObserver(this, 'message-manager-disconnect');

    this._client.close();
    delete this._client;
  },

  



  trackFrame: function dwp_trackFrame(frame) {
    if (this._targets.has(frame))
      return;

    
    this._client.request({
      to: this._webappsActor,
      type: 'getAppActor',
      manifestURL: frame.appManifestURL
    }, (res) => {
      if (res.error) {
        return;
      }

      let target = new Target(frame, res.actor);
      this._targets.set(frame, target);

      for (let w of this._watchers) {
        w.trackTarget(target);
      }
    });
  },

  untrackFrame: function dwp_untrackFrame(frame) {
    let target = this._targets.get(frame);
    if (target) {
      for (let w of this._watchers) {
        w.untrackTarget(target);
      }

      
      delete target.metrics;
      target.display();

      this._targets.delete(frame);
    }
  },

  observe: function dwp_observe(subject, topic, data) {
    if (!this._client)
      return;

    let frame;

    switch(topic) {

      
      case 'remote-browser-shown':
      case 'inprocess-browser-shown':
        let frameLoader = subject;
        
        frameLoader.QueryInterface(Ci.nsIFrameLoader);
        
        if (!frameLoader.ownerIsBrowserOrAppFrame) {
          return;
        }
        frame = frameLoader.ownerElement;
        if (!frame.appManifestURL) 
          return;
        this.trackFrame(frame);
        this._frames.set(frameLoader.messageManager, frame);
        break;

      
      case 'message-manager-disconnect':
        let mm = subject;
        frame = this._frames.get(mm);
        if (!frame)
          return;
        this.untrackFrame(frame);
        this._frames.delete(mm);
        break;
    }
  },

  log: function dwp_log(message) {
    dump(DEVELOPER_HUD_LOG_PREFIX + ': ' + message + '\n');
  }

};







function Target(frame, actor) {
  this.frame = frame;
  this.actor = actor;
  this.metrics = new Map();
}

Target.prototype = {
  display: function target_display() {
    let data = {
      metrics: []
    };

    let metrics = this.metrics;
    if (metrics && metrics.size > 0) {
      for (let name of metrics.keys()) {
        data.metrics.push({name: name, value: metrics.get(name)});
      }
    }

    shell.sendEvent(this.frame, 'developer-hud-update', Cu.cloneInto(data, this.frame));

    
    return false;
  }

};






let consoleWatcher = {

  _targets: new Map(),
  _watching: {
    reflows: false,
    warnings: false,
    errors: false
  },
  _client: null,

  init: function cw_init(client) {
    this._client = client;
    this.consoleListener = this.consoleListener.bind(this);

    let watching = this._watching;

    for (let key in watching) {
      let metric = key;
      SettingsListener.observe('hud.' + metric, false, watch => {
        
        if (watching[metric] = watch) {
          return;
        }

        
        for (let target of this._targets.values()) {
          target.metrics.set(metric, 0);
          target.display();
        }
      });
    }

    client.addListener('logMessage', this.consoleListener);
    client.addListener('pageError', this.consoleListener);
    client.addListener('consoleAPICall', this.consoleListener);
    client.addListener('reflowActivity', this.consoleListener);
  },

  trackTarget: function cw_trackTarget(target) {
    target.metrics.set('reflows', 0);
    target.metrics.set('warnings', 0);
    target.metrics.set('errors', 0);

    this._client.request({
      to: target.actor.consoleActor,
      type: 'startListeners',
      listeners: ['LogMessage', 'PageError', 'ConsoleAPI', 'ReflowActivity']
    }, (res) => {
      this._targets.set(target.actor.consoleActor, target);
    });
  },

  untrackTarget: function cw_untrackTarget(target) {
    this._client.request({
      to: target.actor.consoleActor,
      type: 'stopListeners',
      listeners: ['LogMessage', 'PageError', 'ConsoleAPI', 'ReflowActivity']
    }, (res) => { });

    this._targets.delete(target.actor.consoleActor);
  },

  bump: function cw_bump(target, metric) {
    if (!this._watching[metric]) {
      return false;
    }

    let metrics = target.metrics;
    metrics.set(metric, metrics.get(metric) + 1);
    return true;
  },

  consoleListener: function cw_consoleListener(type, packet) {
    let target = this._targets.get(packet.from);
    let output = '';

    switch (packet.type) {

      case 'pageError':
        let pageError = packet.pageError;

        if (pageError.warning || pageError.strict) {
          if (!this.bump(target, 'warnings')) {
            return;
          }
          output = 'warning (';
        } else {
          if (!this.bump(target, 'errors')) {
            return;
          }
          output += 'error (';
        }

        let {errorMessage, sourceName, category, lineNumber, columnNumber} = pageError;
        output += category + '): "' + (errorMessage.initial || errorMessage) +
          '" in ' + sourceName + ':' + lineNumber + ':' + columnNumber;
        break;

      case 'consoleAPICall':
        switch (packet.message.level) {

          case 'error':
            if (!this.bump(target, 'errors')) {
              return;
            }
            output = 'error (console)';
            break;

          case 'warn':
            if (!this.bump(target, 'warnings')) {
              return;
            }
            output = 'warning (console)';
            break;

          default:
            return;
        }
        break;

      case 'reflowActivity':
        if (!this.bump(target, 'reflows')) {
          return;
        }

        let {start, end, sourceURL} = packet;
        let duration = Math.round((end - start) * 100) / 100;
        output = 'reflow: ' + duration + 'ms';
        if (sourceURL) {
          output += ' ' + this.formatSourceURL(packet);
        }
        break;
    }

    if (!target.display()) {
      
      developerHUD.log(output);
    }
  },

  formatSourceURL: function cw_formatSourceURL(packet) {
    
    let source = WebConsoleUtils.abbreviateSourceURL(packet.sourceURL);

    
    let {functionName, sourceLine} = packet;
    source = 'in ' + (functionName || '<anonymousFunction>') +
      ', ' + source + ':' + sourceLine;

    return source;
  }
};
developerHUD.registerWatcher(consoleWatcher);


let eventLoopLagWatcher = {
  _client: null,
  _fronts: new Map(),
  _active: false,

  init: function(client) {
    this._client = client;

    SettingsListener.observe('hud.jank', false, this.settingsListener.bind(this));
  },

  settingsListener: function(value) {
    if (this._active == value) {
      return;
    }
    this._active = value;

    
    let fronts = this._fronts;
    for (let target of fronts.keys()) {
      if (value) {
        fronts.get(target).start();
      } else {
        fronts.get(target).stop();
        target.metrics.set('jank', 0);
        target.display();
      }
    }
  },

  trackTarget: function(target) {
    target.metrics.set('jank', 0);

    let front = new EventLoopLagFront(this._client, target.actor);
    this._fronts.set(target, front);

    front.on('event-loop-lag', time => {
      target.metrics.set('jank', time);

      if (!target.display()) {
        developerHUD.log('jank: ' + time + 'ms');
      }
    });

    if (this._active) {
      front.start();
    }
  },

  untrackTarget: function(target) {
    let fronts = this._fronts;
    if (fronts.has(target)) {
      fronts.get(target).destroy();
      fronts.delete(target);
    }
  }
};
developerHUD.registerWatcher(eventLoopLagWatcher);





let memoryWatcher = {

  _client: null,
  _fronts: new Map(),
  _timers: new Map(),
  _watching: {
    jsobjects: false,
    jsstrings: false,
    jsother: false,
    dom: false,
    style: false,
    other: false
  },
  _active: false,

  init: function mw_init(client) {
    this._client = client;
    let watching = this._watching;

    for (let key in watching) {
      let category = key;
      SettingsListener.observe('hud.' + category, false, watch => {
        watching[category] = watch;
      });
    }

    SettingsListener.observe('hud.appmemory', false, enabled => {
      if (this._active = enabled) {
        for (let target of this._fronts.keys()) {
          this.measure(target);
        }
      } else {
        for (let target of this._fronts.keys()) {
          clearTimeout(this._timers.get(target));
          target.metrics.set('memory', 0);
          target.display();
        }
      }
    });
  },

  measure: function mw_measure(target) {

    

    let watch = this._watching;
    let front = this._fronts.get(target);

    front.measure().then((data) => {

      let total = 0;
      if (watch.jsobjects) {
        total += parseInt(data.jsObjectsSize);
      }
      if (watch.jsstrings) {
        total += parseInt(data.jsStringsSize);
      }
      if (watch.jsother) {
        total += parseInt(data.jsOtherSize);
      }
      if (watch.dom) {
        total += parseInt(data.domSize);
      }
      if (watch.style) {
        total += parseInt(data.styleSize);
      }
      if (watch.other) {
        total += parseInt(data.otherSize);
      }
      

      target.metrics.set('memory', total);
      target.display();
      let duration = parseInt(data.jsMilliseconds) + parseInt(data.nonJSMilliseconds);
      let timer = setTimeout(() => this.measure(target), 100 * duration);
      this._timers.set(target, timer);
    }, (err) => {
      console.error(err);
    });
  },

  trackTarget: function mw_trackTarget(target) {
    target.metrics.set('uss', 0);
    target.metrics.set('memory', 0);
    this._fronts.set(target, MemoryFront(this._client, target.actor));
    if (this._active) {
      this.measure(target);
    }
  },

  untrackTarget: function mw_untrackTarget(target) {
    let front = this._fronts.get(target);
    if (front) {
      front.destroy();
      clearTimeout(this._timers.get(target));
      this._fronts.delete(target);
      this._timers.delete(target);
    }
  }
};
developerHUD.registerWatcher(memoryWatcher);
