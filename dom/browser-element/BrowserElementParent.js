



"use strict";

const {utils: Cu, interfaces: Ci} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/BrowserElementParent.jsm");





this.NSGetFactory = XPCOMUtils.generateNSGetFactory([BrowserElementParent]);
