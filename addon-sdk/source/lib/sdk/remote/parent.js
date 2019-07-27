


"use strict";

const { Cu, Ci, Cc } = require('chrome');
const runtime = require('../system/runtime');

const MAIN_PROCESS = Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;

if (runtime.processType != MAIN_PROCESS) {
  throw new Error('Cannot use sdk/remote/parent in a child process.');
}

const { Class } = require('../core/heritage');
const { Namespace } = require('../core/namespace');
const { Disposable } = require('../core/disposable');
const { omit } = require('../util/object');
const { when } = require('../system/unload');
const { EventTarget } = require('../event/target');
const { emit } = require('../event/core');
const system = require('../system/events');
const { EventParent } = require('./utils');
const options = require('@loader/options');
const loaderModule = require('toolkit/loader');
const { getTabForBrowser } = require('../tabs/utils');


let moduleResolve;
if (options.isNative) {
  moduleResolve = (id, requirer) => loaderModule.nodeResolve(id, requirer, { rootURI: options.rootURI });
}
else {
  moduleResolve = loaderModule.resolve;
}

let pathMapping = Object.keys(options.paths)
                        .sort((a, b) => b.length - a.length)
                        .map(p => [p, options.paths[p]]);


let { getNewLoaderID } = require('../../framescript/FrameScriptManager.jsm');
let PATH = options.paths[''];

const childOptions = omit(options, ['modules', 'globals']);
childOptions.modules = {};

try {
  childOptions.modules["@l10n/data"] = require("@l10n/data");
}
catch (e) {
  
}
const loaderID = getNewLoaderID();
childOptions.loaderID = loaderID;

const ppmm = Cc['@mozilla.org/parentprocessmessagemanager;1'].
             getService(Ci.nsIMessageBroadcaster);
const gmm = Cc['@mozilla.org/globalmessagemanager;1'].
            getService(Ci.nsIMessageBroadcaster);

const ns = Namespace();

let processMap = new Map();

function processMessageReceived({ target, data }) {
  if (data.loaderID != loaderID)
    return;
  let [event, ...args] = data.args;
  emit(this.port, event, this, ...args);
}




const Process = Class({
  implements: [ Disposable ],
  extends: EventTarget,
  setup: function(id, messageManager, isRemote) {
    ns(this).id = id;
    ns(this).isRemote = isRemote;
    ns(this).messageManager = messageManager;
    ns(this).messageReceived = processMessageReceived.bind(this);
    this.destroy = this.destroy.bind(this);
    ns(this).messageManager.addMessageListener('sdk/remote/process/message', ns(this).messageReceived);
    ns(this).messageManager.addMessageListener('child-process-shutdown', this.destroy);

    this.port = new EventTarget();
    this.port.emit = (...args) => {
      ns(this).messageManager.sendAsyncMessage('sdk/remote/process/message', {
        loaderID,
        args
      });
    };

    
    for (let module of remoteModules.values())
      this.port.emit('sdk/remote/require', module);

    processMap.set(ns(this).id, this);
    processes.attachItem(this);
  },

  dispose: function() {
    emit(this, 'detach', this);
    processMap.delete(ns(this).id);
    ns(this).messageManager.removeMessageListener('sdk/remote/process/message', ns(this).messageReceived);
    ns(this).messageManager.removeMessageListener('child-process-shutdown', this.destroy);
    ns(this).messageManager = null;
  },

  
  get isRemote() {
    return ns(this).isRemote;
  }
});



const Processes = Class({
  implements: [ EventParent ],
  extends: EventTarget,
  initialize: function() {
    EventParent.prototype.initialize.call(this);

    this.port = new EventTarget();
    this.port.emit = (...args) => {
      ppmm.broadcastAsyncMessage('sdk/remote/process/message', {
        loaderID,
        args
      });
    };
  },

  getById: function(id) {
    return processMap.get(id);
  }
});
let processes = exports.processes = new Processes();

let frameMap = new Map();

function frameMessageReceived({ target, data }) {
  if (data.loaderID != loaderID)
    return;
  let [event, ...args] = data.args;
  emit(this.port, event, this, ...args);
}

function setFrameProcess(frame, process) {
  ns(frame).process = process;
  frames.attachItem(frame);
}



const Frame = Class({
  implements: [ Disposable ],
  extends: EventTarget,
  setup: function(id, node) {
    ns(this).id = id;
    ns(this).node = node;

    let frameLoader = node.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader;
    ns(this).messageManager = frameLoader.messageManager;

    ns(this).messageReceived = frameMessageReceived.bind(this);
    ns(this).messageManager.addMessageListener('sdk/remote/frame/message', ns(this).messageReceived);

    this.port = new EventTarget();
    this.port.emit = (...args) => {
      ns(this).messageManager.sendAsyncMessage('sdk/remote/frame/message', {
        loaderID,
        args
      });
    };

    frameMap.set(ns(this).messageManager, this);
  },

  dispose: function() {
    emit(this, 'detach', this);
    ns(this).messageManager.removeMessageListener('sdk/remote/frame/message', ns(this).messageReceived);
    ns(this).messageManager = null;

    frameMap.delete(ns(this).messageManager);
  },

  
  get frameElement() {
    return ns(this).node;
  },

  
  get process() {
    return ns(this).process;
  },

  
  get isTab() {
    let tab = getTabForBrowser(ns(this).node);
    return !!tab;
  }
});

function managerDisconnected({ subject: manager }) {
  let frame = frameMap.get(manager);
  if (frame)
    frame.destroy();
}
system.on('message-manager-disconnect', managerDisconnected);



const FrameList = Class({
  implements: [ EventParent ],
  extends: EventTarget,
  initialize: function() {
    EventParent.prototype.initialize.call(this);

    this.port = new EventTarget();
    this.port.emit = (...args) => {
      gmm.broadcastAsyncMessage('sdk/remote/frame/message', {
        loaderID,
        args
      });
    };
  },

  
  getFrameForBrowser: function(browser) {
    for (let frame of this) {
      if (frame.frameElement == browser)
        return frame;
    }
    return null;
  }
});
let frames = exports.frames = new FrameList();


ppmm.broadcastAsyncMessage('sdk/remote/process/load', {
  modulePath: PATH,
  loaderID,
  options: childOptions,
  reason: "broadcast"
});


function processLoaderStarted({ target, data }) {
  if (data.loaderID != loaderID)
    return;

  if (processMap.has(data.processID)) {
    console.error("Saw the same process load the same loader twice. This is a bug in the SDK.");
    return;
  }

  let process = new Process(data.processID, target, data.isRemote);

  if (pendingFrames.has(data.processID)) {
    for (let frame of pendingFrames.get(data.processID))
      setFrameProcess(frame, process);
    pendingFrames.delete(data.processID);
  }
}


function processStarted({ target, data: { modulePath } }) {
  if (modulePath != PATH)
    return;

  
  target.sendAsyncMessage('sdk/remote/process/load', {
    modulePath,
    loaderID,
    options: childOptions,
    reason: "response"
  });
}

let pendingFrames = new Map();


function frameAttached({ target, data }) {
  if (data.loaderID != loaderID)
    return;

  let frame = new Frame(data.frameID, target);

  let process = processMap.get(data.processID);
  if (process) {
    setFrameProcess(frame, process);
    return;
  }

  
  
  
  if (!pendingFrames.has(data.processID))
    pendingFrames.set(data.processID, [frame]);
  else
    pendingFrames.get(data.processID).push(frame);
}


ppmm.addMessageListener('sdk/remote/process/attach', processLoaderStarted);
ppmm.addMessageListener('sdk/remote/process/start', processStarted);
gmm.addMessageListener('sdk/remote/frame/attach', frameAttached);

when(reason => {
  ppmm.removeMessageListener('sdk/remote/process/attach', processLoaderStarted);
  ppmm.removeMessageListener('sdk/remote/process/start', processStarted);
  gmm.removeMessageListener('sdk/remote/frame/attach', frameAttached);

  ppmm.broadcastAsyncMessage('sdk/remote/process/unload', { loaderID, reason });
});

let remoteModules = new Set();




function remoteRequire(id, module = null) {
  
  if (module)
    id = moduleResolve(id, module.id);
  let uri = loaderModule.resolveURI(id, pathMapping);

  
  if (remoteModules.has(uri))
    return;

  remoteModules.add(uri);
  processes.port.emit('sdk/remote/require', uri);
}
exports.remoteRequire = remoteRequire;
