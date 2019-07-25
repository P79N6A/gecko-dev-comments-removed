







var gTestfile = 'watch-undefined-setter.js';

var BUGNUMBER = 560277;
var summary =
  'Crash [@ JSObject::getParent] or [@ js_WrapWatchedSetter] or ' +
  '[@ js_GetClassPrototype]';

this.watch("x", function() { });
Object.defineProperty(this, "x", { set: undefined });

reportCompare(true, true);
