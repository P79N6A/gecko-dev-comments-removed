




"use strict";

module.metadata = {
  "stability": "unstable"
};



function singularify(text) {
  return text[text.length - 1] === "s" ? text.substr(0, text.length - 1) : text;
}






function getInitializerName(category) {
  return "init" + singularify(category);
}






















function on(element, type, listener, capture) {
  
  capture = capture || false;
  element.addEventListener(type, listener, capture);
}
exports.on = on;























function once(element, type, listener, capture) {
  on(element, type, function selfRemovableListener(event) {
    removeListener(element, type, selfRemovableListener, capture);
    listener.apply(this, arguments);
  }, capture);
}
exports.once = once;























function removeListener(element, type, listener, capture) {
  element.removeEventListener(type, listener, capture);
}
exports.removeListener = removeListener;





















function emit(element, type, { category, initializer, settings }) {
  category = category || "UIEvents";
  initializer = initializer || getInitializerName(category);
  let document = element.ownerDocument;
  let event = document.createEvent(category);
  event[initializer].apply(event, [type].concat(settings));
  element.dispatchEvent(event);
};
exports.emit = emit;
