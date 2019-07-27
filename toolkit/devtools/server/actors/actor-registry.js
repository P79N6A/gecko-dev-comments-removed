



"use strict";

const protocol = require("devtools/server/protocol");
const { method, custom, Arg, Option, RetVal } = protocol;

const { Cu, CC, components } = require("chrome");
const { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});
const Services = require("Services");
const { DebuggerServer } = require("devtools/server/main");
const { XPCOMUtils } = require("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil", "resource://gre/modules/NetUtil.jsm");





const ActorActor = protocol.ActorClass({
  typeName: "actorActor",

  initialize: function (conn, options) {
    protocol.Actor.prototype.initialize.call(this, conn);

    this.options = options;
  },

  unregister: method(function () {
    if (this.options.tab) {
      DebuggerServer.removeTabActor(this.options);
    }

    if (this.options.global) {
      DebuggerServer.removeGlobalActor(this.options);
    }
  }, {
    request: {},
    response: {}
  })
});

const ActorActorFront = protocol.FrontClass(ActorActor, {
  initialize: function (client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
  }
});

exports.ActorActorFront = ActorActorFront;





const ActorRegistryActor = protocol.ActorClass({
  typeName: "actorRegistry",

  initialize: function (conn) {
    protocol.Actor.prototype.initialize.call(this, conn);
  },

  registerActor: method(function (sourceText, fileName, options) {
    const principal = CC("@mozilla.org/systemprincipal;1", "nsIPrincipal")();
    const sandbox = Cu.Sandbox(principal);
    const exports = sandbox.exports = {};
    sandbox.require = require;

    Cu.evalInSandbox(sourceText, sandbox, "1.8", fileName, 1);

    let { prefix, constructor, type } = options;

    if (type.global) {
      DebuggerServer.addGlobalActor({
        constructorName: constructor,
        constructorFun: sandbox[constructor]
      }, prefix);
    }

    if (type.tab) {
      DebuggerServer.addTabActor({
        constructorName: constructor,
        constructorFun: sandbox[constructor]
      }, prefix);
    }

    return ActorActor(this.conn, {
      name: constructor,
      tab: type.tab,
      global: type.global
    });
  }, {
    request: {
      sourceText: Arg(0, "string"),
      filename: Arg(1, "string"),
      options: Arg(2, "json")
    },

    response: {
      actorActor: RetVal("actorActor")
    }
  })
});

exports.ActorRegistryActor = ActorRegistryActor;

function request(uri) {
  return new Promise((resolve, reject) => {
    try {
      uri = Services.io.newURI(uri, null, null);
    } catch (e) {
      reject(e);
    }

    if (uri.scheme != "resource") {
      reject(new Error(
        "Can only register actors whose URI scheme is 'resource'."));
    }

    NetUtil.asyncFetch(uri, (stream, status, req) => {
      if (!components.isSuccessCode(status)) {
        reject(new Error("Request failed with status code = "
                         + status
                         + " after NetUtil.asyncFetch for url = "
                         + uri));
        return;
      }

      let source = NetUtil.readInputStreamToString(stream, stream.available());
      stream.close();
      resolve(source);
    });
  });
}

const ActorRegistryFront = protocol.FrontClass(ActorRegistryActor, {
  initialize: function (client, form) {
    protocol.Front.prototype.initialize.call(this, client,
      { actor: form.actorRegistryActor });

    this.manage(this);
  },

  registerActor: custom(function (uri, options) {
    return request(uri, options)
      .then(sourceText => {
        return this._registerActor(sourceText, uri, options);
      });
  }, {
    impl: "_registerActor"
  })
});
exports.ActorRegistryFront = ActorRegistryFront;
