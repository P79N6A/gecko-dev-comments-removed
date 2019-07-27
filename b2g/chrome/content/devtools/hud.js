



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

Cu.import('resource://gre/modules/Frames.jsm');

let _telemetryDebug = true;

function telemetryDebug(...args) {
  if (_telemetryDebug) {
    args.unshift('[AdvancedTelemetry]');
    console.log(...args);
  }
}






let developerHUD = {

  _targets: new Map(),
  _client: null,
  _conn: null,
  _watchers: [],
  _logging: true,
  _telemetry: false,

  






  registerWatcher: function dwp_registerWatcher(watcher) {
    this._watchers.unshift(watcher);
  },

  init: function dwp_init() {
    if (this._client) {
      return;
    }

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

    Frames.addObserver(this);

    let appFrames = Frames.list().filter(frame => frame.getAttribute('mozapp'));
    for (let frame of appFrames) {
      this.trackFrame(frame);
    }

    SettingsListener.observe('hud.logging', this._logging, enabled => {
      this._logging = enabled;
    });

    SettingsListener.observe('debug.performance_data.advanced_telemetry', this._telemetry, enabled => {
      this._telemetry = enabled;
    });
  },

  uninit: function dwp_uninit() {
    if (!this._client) {
      return;
    }

    for (let frame of this._targets.keys()) {
      this.untrackFrame(frame);
    }

    Frames.removeObserver(this);

    this._client.close();
    delete this._client;
  },

  



  trackFrame: function dwp_trackFrame(frame) {
    if (this._targets.has(frame)) {
      return;
    }

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

  onFrameCreated: function (frame, isFirstAppFrame) {
    let mozapp = frame.getAttribute('mozapp');
    if (!mozapp) {
      return;
    }
    this.trackFrame(frame);
  },

  onFrameDestroyed: function (frame, isLastAppFrame) {
    let mozapp = frame.getAttribute('mozapp');
    if (!mozapp) {
      return;
    }
    this.untrackFrame(frame);
  },

  log: function dwp_log(message) {
    if (this._logging) {
      dump(DEVELOPER_HUD_LOG_PREFIX + ': ' + message + '\n');
    }
  }

};







function Target(frame, actor) {
  this._frame = frame;
  this.actor = actor;
  this.metrics = new Map();
}

Target.prototype = {

  get frame() {
    let frame = this._frame;
    let systemapp = document.querySelector('#systemapp');

    return (frame === systemapp ? getContentWindow() : frame);
  },

  get manifest() {
    return this._frame.appManifestURL;
  },

  


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
      manifest: this.manifest,
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

    shell.sendEvent(frame, 'developer-hud-update', Cu.cloneInto(data, frame));
    this._sendTelemetryEvent(data.metric);
  },

  _sendTelemetryEvent: function target_sendTelemetryEvent(metric) {
    if (!developerHUD._telemetry || !metric || metric.skipTelemetry) {
      return;
    }

    if (!this.appName) {
      let manifest = this.manifest;
      if (!manifest) {
        return;
      }
      let start = manifest.indexOf('/') + 2;
      let end = manifest.indexOf('.', start);
      this.appName = manifest.substring(start, end).toLowerCase();
    }

    metric.appName = this.appName;

    let data = { metric: metric };
    let frame = this.frame;

    telemetryDebug('sending advanced-telemetry-update with this data: ' + JSON.stringify(data));
    shell.sendEvent(frame, 'advanced-telemetry-update', Cu.cloneInto(data, frame));
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
    'Invalid HPKP Headers',
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
          output += 'Warning (';
        } else {
          metric.name = 'errors';
          output += 'Error (';
        }

        if (this._security.indexOf(pageError.category) > -1) {
          metric.name = 'security';

          
          
          target._sendTelemetryEvent({
            name: 'security',
            value: pageError.category,
          });

          
          
          
          metric.skipTelemetry = true;
        }

        let {errorMessage, sourceName, category, lineNumber, columnNumber} = pageError;
        output += category + '): "' + (errorMessage.initial || errorMessage) +
          '" in ' + sourceName + ':' + lineNumber + ':' + columnNumber;
        break;

      case 'consoleAPICall':
        switch (packet.message.level) {

          case 'error':
            metric.name = 'errors';
            output += 'Error (console)';
            break;

          case 'warn':
            metric.name = 'warnings';
            output += 'Warning (console)';
            break;

          case 'info':
            this.handleTelemetryMessage(target, packet);

            
            
            
            
            metric.name = 'info';
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
        output += 'Reflow: ' + duration + 'ms';
        if (sourceURL) {
          output += ' ' + this.formatSourceURL(packet);
        }

        
        target._sendTelemetryEvent({name: 'reflow-duration', value: Math.round(duration)});
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
  },

  handleTelemetryMessage:
    function cw_handleTelemetryMessage(target, packet) {

    if (!developerHUD._telemetry) {
      return;
    }

    
    
    let separator = '|';
    let logContent = packet.message.arguments.toString();

    if (logContent.indexOf('telemetry') < 0) {
      return;
    }

    let telemetryData = logContent.split(separator);

    
    let TELEMETRY_IDENTIFIER_IDX = 0;
    let NAME_IDX = 1;
    let VALUE_IDX = 2;
    let CONTEXT_IDX = 3;

    if (telemetryData[TELEMETRY_IDENTIFIER_IDX] != 'telemetry' ||
        telemetryData.length < 3 || telemetryData.length > 4) {
      return;
    }

    let metric = {
      name: telemetryData[NAME_IDX],
      value: telemetryData[VALUE_IDX]
    };

    
    
    if (telemetryData.length === 4) {
      metric.context = telemetryData[CONTEXT_IDX];
    }

    target._sendTelemetryEvent(metric);
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
      target.update({name: 'jank', value: time}, 'Jank: ' + time + 'ms');
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
    let active = watching.appmemory || watching.uss;

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
    let format = this.formatMemory;

    if (watch.uss) {
      front.residentUnique().then(value => {
        target.update({name: 'uss', value: value}, 'USS: ' + format(value));
      }, err => {
        console.error(err);
      });
    }

    if (watch.appmemory) {
      front.measure().then(data => {
        let total = 0;
        let details = [];

        function item(name, condition, value) {
          if (!condition) {
            return;
          }

          let v = parseInt(value);
          total += v;
          details.push(name + ': ' + format(v));
        }

        item('JS objects', watch.jsobjects, data.jsObjectsSize);
        item('JS strings', watch.jsstrings, data.jsStringsSize);
        item('JS other', watch.jsother, data.jsOtherSize);
        item('DOM', watch.dom, data.domSize);
        item('Style', watch.style, data.styleSize);
        item('Other', watch.other, data.otherSize);
        

        target.update({name: 'memory', value: total},
          'App Memory: ' + format(total) + ' (' + details.join(', ') + ')');
      }, err => {
        console.error(err);
      });
    }

    let timer = setTimeout(() => this.measure(target), 800);
    this._timers.set(target, timer);
  },

  formatMemory: function mw_formatMemory(bytes) {
    var prefix = ['','K','M','G','T','P','E','Z','Y'];
    var i = 0;
    for (; bytes > 1024 && i < prefix.length; ++i) {
      bytes /= 1024;
    }
    return (Math.round(bytes * 100) / 100) + ' ' + prefix[i] + 'B';
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
