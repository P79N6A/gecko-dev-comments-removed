



'use strict';

const DEVELOPER_HUD_LOG_PREFIX = 'DeveloperHUD';

XPCOMUtils.defineLazyGetter(this, 'devtools', function() {
  const {devtools} = Cu.import('resource://gre/modules/devtools/Loader.jsm', {});
  return devtools;
});

XPCOMUtils.defineLazyGetter(this, 'DebuggerClient', function() {
  return Cu.import('resource://gre/modules/devtools/dbg-client.jsm', {}).DebuggerClient;
});

XPCOMUtils.defineLazyGetter(this, 'WebConsoleUtils', function() {
  return devtools.require('devtools/toolkit/webconsole/utils').Utils;
});

XPCOMUtils.defineLazyGetter(this, 'EventLoopLagFront', function() {
  return devtools.require('devtools/server/actors/eventlooplag').EventLoopLagFront;
});

XPCOMUtils.defineLazyGetter(this, 'MemoryFront', function() {
  return devtools.require('devtools/server/actors/memory').MemoryFront;
});

Cu.import('resource://gre/modules/AppFrames.jsm');






let developerHUD = {

  _targets: new Map(),
  _client: null,
  _conn: null,
  _watchers: [],
  _logging: true,

  






  registerWatcher: function dwp_registerWatcher(watcher) {
    this._watchers.unshift(watcher);
  },

  init: function dwp_init() {
    if (this._client)
      return;

    if (!DebuggerServer.initialized) {
      RemoteDebugger.initServer();
    }

    
    
    
    
    
    
    let transport = DebuggerServer.connectPipe();
    this._conn = transport._serverConnection;
    this._client = new DebuggerClient(transport);

    for (let w of this._watchers) {
      if (w.init) {
        w.init(this._client);
      }
    }

    AppFrames.addObserver(this);

    for (let frame of AppFrames.list()) {
      this.trackFrame(frame);
    }

    SettingsListener.observe('hud.logging', this._logging, enabled => {
      this._logging = enabled;
    });
  },

  uninit: function dwp_uninit() {
    if (!this._client)
      return;

    for (let frame of this._targets.keys()) {
      this.untrackFrame(frame);
    }

    AppFrames.removeObserver(this);

    this._client.close();
    delete this._client;
  },

  



  trackFrame: function dwp_trackFrame(frame) {
    if (this._targets.has(frame))
      return;

    DebuggerServer.connectToChild(this._conn, frame).then(actor => {
      let target = new Target(frame, actor);
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

      target.destroy();
      this._targets.delete(frame);
    }
  },

  onAppFrameCreated: function (frame, isFirstAppFrame) {
    this.trackFrame(frame);
  },

  onAppFrameDestroyed: function (frame, isLastAppFrame) {
    this.untrackFrame(frame);
  },

  log: function dwp_log(message) {
    if (this._logging) {
      dump(DEVELOPER_HUD_LOG_PREFIX + ': ' + message + '\n');
    }
  }

};







function Target(frame, actor) {
  this.frame = frame;
  this.actor = actor;
  this.metrics = new Map();
}

Target.prototype = {

  


  register: function target_register(metric) {
    this.metrics.set(metric, 0);
  },

  



  update: function target_update(metric, message) {
    if (!metric.name) {
      throw new Error('Missing metric.name');
    }

    if (!metric.value) {
      metric.value = 0;
    }

    let metrics = this.metrics;
    if (metrics) {
      metrics.set(metric.name, metric.value);
    }

    let data = {
      metrics: [], 
      manifest: this.frame.appManifestURL,
      metric: metric,
      message: message
    };

    
    if (metrics && metrics.size > 0) {
      for (let name of metrics.keys()) {
        data.metrics.push({name: name, value: metrics.get(name)});
      }
    }

    if (message) {
      developerHUD.log('[' + data.manifest + '] ' + data.message);
    }
    this._send(data);
  },

  



  bump: function target_bump(metric, message) {
    metric.value = (this.metrics.get(metric.name) || 0) + 1;
    this.update(metric, message);
  },

  



  clear: function target_clear(metric) {
    metric.value = 0;
    this.update(metric);
  },

  



  destroy: function target_destroy() {
    delete this.metrics;
    this._send({});
  },

  _send: function target_send(data) {
    let frame = this.frame;

    let systemapp = document.querySelector('#systemapp');
    if (this.frame === systemapp) {
      frame = getContentWindow();
    }

    shell.sendEvent(frame, 'developer-hud-update', Cu.cloneInto(data, frame));
  }

};






let consoleWatcher = {

  _client: null,
  _targets: new Map(),
  _watching: {
    reflows: false,
    warnings: false,
    errors: false,
    security: false
  },
  _security: [
    'Mixed Content Blocker',
    'Mixed Content Message',
    'CSP',
    'Invalid HSTS Headers',
    'Insecure Password Field',
    'SSL',
    'CORS'
  ],

  init: function cw_init(client) {
    this._client = client;
    this.consoleListener = this.consoleListener.bind(this);

    let watching = this._watching;

    for (let key in watching) {
      let metric = key;
      SettingsListener.observe('hud.' + metric, watching[metric], watch => {
        
        if (watching[metric] = watch) {
          return;
        }

        
        for (let target of this._targets.values()) {
          target.clear({name: metric});
        }
      });
    }

    client.addListener('logMessage', this.consoleListener);
    client.addListener('pageError', this.consoleListener);
    client.addListener('consoleAPICall', this.consoleListener);
    client.addListener('reflowActivity', this.consoleListener);
  },

  trackTarget: function cw_trackTarget(target) {
    target.register('reflows');
    target.register('warnings');
    target.register('errors');
    target.register('security');

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

  consoleListener: function cw_consoleListener(type, packet) {
    let target = this._targets.get(packet.from);
    let metric = {};
    let output = '';

    switch (packet.type) {

      case 'pageError':
        let pageError = packet.pageError;

        if (pageError.warning || pageError.strict) {
          metric.name = 'warnings';
          output += 'warning (';
        } else {
          metric.name = 'errors';
          output += 'error (';
        }

        if (this._security.indexOf(pageError.category) > -1) {
          metric.name = 'security';
        }

        let {errorMessage, sourceName, category, lineNumber, columnNumber} = pageError;
        output += category + '): "' + (errorMessage.initial || errorMessage) +
          '" in ' + sourceName + ':' + lineNumber + ':' + columnNumber;
        break;

      case 'consoleAPICall':
        switch (packet.message.level) {

          case 'error':
            metric.name = 'errors';
            output += 'error (console)';
            break;

          case 'warn':
            metric.name = 'warnings';
            output += 'warning (console)';
            break;

          default:
            return;
        }
        break;

      case 'reflowActivity':
        metric.name = 'reflows';

        let {start, end, sourceURL, interruptible} = packet;
        metric.interruptible = interruptible;
        let duration = Math.round((end - start) * 100) / 100;
        output += 'reflow: ' + duration + 'ms';
        if (sourceURL) {
          output += ' ' + this.formatSourceURL(packet);
        }
        break;

      default:
        return;
    }

    if (!this._watching[metric.name]) {
      return;
    }

    target.bump(metric, output);
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
        target.clear({name: 'jank'});
      }
    }
  },

  trackTarget: function(target) {
    target.register('jank');

    let front = new EventLoopLagFront(this._client, target.actor);
    this._fronts.set(target, front);

    front.on('event-loop-lag', time => {
      target.update({name: 'jank', value: time}, 'jank: ' + time + 'ms');
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
    uss: false,
    appmemory: false,
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
        this.update();
      });
    }
  },

  update: function mw_update() {
    let watching = this._watching;
    let active = watching.memory || watching.uss;

    if (this._active) {
      for (let target of this._fronts.keys()) {
        if (!watching.appmemory) target.clear({name: 'memory'});
        if (!watching.uss) target.clear({name: 'uss'});
        if (!active) clearTimeout(this._timers.get(target));
      }
    } else if (active) {
      for (let target of this._fronts.keys()) {
        this.measure(target);
      }
    }
    this._active = active;
  },

  measure: function mw_measure(target) {
    let watch = this._watching;
    let front = this._fronts.get(target);

    if (watch.uss) {
      front.residentUnique().then(value => {
        target.update({name: 'uss', value: value});
      }, err => {
        console.error(err);
      });
    }

    if (watch.appmemory) {
      front.measure().then(data => {
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
        

        target.update({name: 'memory', value: total});
      }, err => {
        console.error(err);
      });
    }

    let timer = setTimeout(() => this.measure(target), 500);
    this._timers.set(target, timer);
  },

  trackTarget: function mw_trackTarget(target) {
    target.register('uss');
    target.register('memory');
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
