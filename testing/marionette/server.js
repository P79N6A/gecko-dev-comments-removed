



"use strict";

const {Constructor: CC, classes: Cc, interfaces: Ci, utils: Cu} = Components;

const loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].getService(Ci.mozIJSSubScriptLoader);
const ServerSocket = CC("@mozilla.org/network/server-socket;1", "nsIServerSocket", "initSpecialConnection");

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Services.jsm");

Cu.import("chrome://marionette/content/dispatcher.js");
Cu.import("chrome://marionette/content/driver.js");
Cu.import("chrome://marionette/content/elements.js");
Cu.import("chrome://marionette/content/simpletest.js");


loader.loadSubScript("resource://gre/modules/devtools/transport/transport.js");


let events = {};
loader.loadSubScript("chrome://marionette/content/EventUtils.js", events);
loader.loadSubScript("chrome://marionette/content/ChromeUtils.js", events);
loader.loadSubScript("chrome://marionette/content/frame-manager.js");

const logger = Log.repository.getLogger("Marionette");

this.EXPORTED_SYMBOLS = ["MarionetteServer"];
const SPECIAL_POWERS_PREF = "security.turn_off_all_security_so_that_viruses_can_take_over_this_computer";
const CONTENT_LISTENER_PREF = "marionette.contentListener";














this.MarionetteServer = function(port, forceLocal) {
  this.port = port;
  this.forceLocal = forceLocal;
  this.conns = {};
  this.nextConnId = 0;
  this.alive = false;
};











MarionetteServer.prototype.driverFactory = function(emulator) {
  let appName = isMulet() ? "B2G" : Services.appinfo.name;
  let qemu = "0";
  let device = null;
  let bypassOffline = false;

  try {
    Cu.import("resource://gre/modules/systemlibs.js");
    qemu = libcutils.property_get("ro.kernel.qemu");
    logger.debug("B2G emulator: " + (qemu == "1" ? "yes" : "no"));
    device = libcutils.property_get("ro.product.device");
    logger.debug("Device detected is " + device);
    bypassOffline = (qemu == "1" || device == "panda");
  } catch (e) {}

  if (qemu == "1") {
    device = "qemu";
  }
  if (!device) {
    device = "desktop";
  }

  Services.prefs.setBoolPref(CONTENT_LISTENER_PREF, false);

  if (bypassOffline) {
    logger.debug("Bypassing offline status");
    Services.prefs.setBoolPref("network.gonk.manage-offline-status", false);
    Services.io.manageOfflineStatus = false;
    Services.io.offline = false;
  }

  return new GeckoDriver(appName, device, emulator);
};

MarionetteServer.prototype.start = function() {
  if (this.alive) {
    return;
  }
  let flags = Ci.nsIServerSocket.KeepWhenOffline;
  if (this.forceLocal) {
    flags |= Ci.nsIServerSocket.LoopbackOnly;
  }
  this.listener = new ServerSocket(this.port, flags, 0);
  this.listener.asyncListen(this);
  this.alive = true;
};

MarionetteServer.prototype.stop = function() {
  if (!this.alive) {
    return;
  }
  this.closeListener();
  this.alive = false;
};

MarionetteServer.prototype.closeListener = function() {
  this.listener.close();
  this.listener = null;
};

MarionetteServer.prototype.onSocketAccepted = function(
    serverSocket, clientSocket) {
  let input = clientSocket.openInputStream(0, 0, 0);
  let output = clientSocket.openOutputStream(0, 0, 0);
  let transport = new DebuggerTransport(input, output);
  let connId = "conn" + this.nextConnId++;

  let stopSignal = () => this.stop();
  let dispatcher = new Dispatcher(connId, transport, this.driverFactory, stopSignal);
  dispatcher.onclose = this.onConnectionClosed.bind(this);
  this.conns[connId] = dispatcher;

  logger.info(`Accepted connection ${connId} from ${clientSocket.host}:${clientSocket.port}`);

  
  dispatcher.sayHello();
  transport.ready();
};

MarionetteServer.prototype.onConnectionClosed = function(conn) {
  let id = conn.id;
  delete this.conns[id];
  logger.info(`Closed connection ${id}`);
};

function isMulet() {
  try {
    return Services.prefs.getBoolPref("b2g.is_mulet");
  } catch (e) {
    return false;
  }
}
