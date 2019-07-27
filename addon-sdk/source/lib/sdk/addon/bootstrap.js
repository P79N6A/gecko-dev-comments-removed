


"use strict";

const { Cu } = require("chrome");
const { NetUtil } = require("resource://gre/modules/NetUtil.jsm");
const { Task: { spawn } } = require("resource://gre/modules/Task.jsm");
const { readURI } = require("sdk/net/url");
const { mount, unmount } = require("sdk/uri/resource");
const { setTimeout } = require("sdk/timers");
const { Loader, Require, Module, main, unload } = require("toolkit/loader");
const prefs = require("sdk/preferences/service");



const Startup = Cu.import("resource://gre/modules/sdk/system/Startup.js", {});

const REASON = [ "unknown", "startup", "shutdown", "enable", "disable",
                 "install", "uninstall", "upgrade", "downgrade" ];

const UUID_PATTERN = /^\{([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})\}$/;


const readDomain = id =>
  
  
  
  
  
  id.lastIndexOf("@") === 0 ? id.substr(1).toLowerCase() :
  id.toLowerCase().
     replace(/@/g, "-at-").
     replace(/\./g, "-dot-").
     replace(UUID_PATTERN, "$1");

const readPaths = id => {
  const base = `extensions.modules.${id}.path.`;
  const domain = readDomain(id);
  return prefs.keys(base).reduce((paths, key) => {
    const value = prefs.get(key);
    const name = key.replace(base, "");
    const path = name.split(".").join("/");
    const prefix = path.length ? `${path}/` : path;
    const uri = value.endsWith("/") ? value : `${value}/`;
    const root = `extensions.modules.${domain}.commonjs.path.${name}`;

    mount(root, uri);

    paths[prefix] = `resource://${root}/`;
    return paths;
  }, {});
};

const Bootstrap = function(mountURI) {
  this.mountURI = mountURI;
  this.install = this.install.bind(this);
  this.uninstall = this.uninstall.bind(this);
  this.startup = this.startup.bind(this);
  this.shutdown = this.shutdown.bind(this);
};
Bootstrap.prototype = {
  constructor: Bootstrap,
  mount(domain, rootURI) {
    mount(domain, rootURI);
    this.domain = domain;
  },
  unmount() {
    if (this.domain) {
      unmount(this.domain);
      this.domain = null;
    }
  },
  install(addon, reason) {
  },
  uninstall(addon, reason) {
    const {id} = addon;

    prefs.reset(`extensions.${id}.sdk.domain`);
    prefs.reset(`extensions.${id}.sdk.version`);
    prefs.reset(`extensions.${id}.sdk.rootURI`);
    prefs.reset(`extensions.${id}.sdk.baseURI`);
    prefs.reset(`extensions.${id}.sdk.load.reason`);

  },
  startup(addon, reasonCode) {
    const { id, version, resourceURI: {spec: addonURI} } = addon;
    const rootURI = this.mountURI || addonURI;
    const reason = REASON[reasonCode];

    spawn(function*() {
      const metadata = JSON.parse(yield readURI(`${rootURI}package.json`));
      const domain = readDomain(id);
      const baseURI = `resource://${domain}/`;

      this.mount(domain, rootURI);

      prefs.set(`extensions.${id}.sdk.domain`, domain);
      prefs.set(`extensions.${id}.sdk.version`, version);
      prefs.set(`extensions.${id}.sdk.rootURI`, rootURI);
      prefs.set(`extensions.${id}.sdk.baseURI`, baseURI);
      prefs.set(`extensions.${id}.sdk.load.reason`, reason);

      const command = prefs.get(`extensions.${id}.sdk.load.command`);

      const loader = Loader({
        id,
        isNative: true,
        checkCompatibility: true,
        prefixURI: baseURI,
        rootURI: baseURI,
        name: metadata.name,
        paths: Object.assign({
          "": "resource://gre/modules/commonjs/",
          "devtools/": "resource://gre/modules/devtools/",
          "./": baseURI
        }, readPaths(id)),
        manifest: metadata,
        metadata: metadata,
        modules: {
          "@test/options": {}
        },
        noQuit: prefs.get(`extensions.${id}.sdk.test.no-quit`, false)
      });
      this.loader = loader;

      const module = Module("package.json", `${baseURI}package.json`);
      const require = Require(loader, module);
      const main = command === "test" ? "sdk/test/runner" : null;
      const prefsURI = `${baseURI}defaults/preferences/prefs.js`;

      const { startup } = require("sdk/addon/runner");
      startup(reason, {loader, main, prefsURI});
    }.bind(this)).catch(error => {
      console.error(`Failed to start ${id} addon`, error);
      throw error;
    });
  },
  shutdown(addon, code) {
    const { loader, domain } = this;

    this.unmount();
    this.unload(REASON[code]);
  },
  unload(reason) {
    const {loader} = this;
    if (loader) {
      this.loader = null;
      unload(loader, reason);
      setTimeout(() => {
        for (let uri of Object.keys(loader.sandboxes)) {
          Cu.nukeSandbox(loader.sandboxes[uri]);
          delete loader.sandboxes[uri];
          delete loader.modules[uri];
        }
      }, 1000);
    }
  }
};
exports.Bootstrap = Bootstrap;
