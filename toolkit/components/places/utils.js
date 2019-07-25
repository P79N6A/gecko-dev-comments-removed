















var EXPORTED_SYMBOLS = ["PlacesUtils"];

Components.utils.reportError('An add-on is importing utils.js module, this file is deprecated, PlacesUtils.jsm should be used instead. Please notify add-on author of this problem.');

Components.utils.import("resource://gre/modules/PlacesUtils.jsm");
