



"use strict";








this.EXPORTED_SYMBOLS = [ "Node", "HTMLElement", "setTimeout", "clearTimeout" ];





this.Node = Components.interfaces.nsIDOMNode;
this.HTMLElement = Components.interfaces.nsIDOMHTMLElement;




let Timer = Components.utils.import("resource://gre/modules/Timer.jsm", {});
this.setTimeout = Timer.setTimeout;
this.clearTimeout = Timer.clearTimeout;
