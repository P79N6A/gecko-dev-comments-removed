


"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

this.EXPORTED_SYMBOLS = ["LoopRooms"];




this.LoopRooms = Object.freeze({

  channelIDs: {
    FxA: "6add272a-d316-477c-8335-f00f73dfde71",
    Guest: "19d3f799-a8f3-4328-9822-b7cd02765832",
  },

  onNotification: function(version, channelID) {
    return;
  },
});
