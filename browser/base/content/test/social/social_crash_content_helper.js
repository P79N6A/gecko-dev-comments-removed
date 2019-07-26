




Cu.import("resource://gre/modules/ctypes.jsm");
let crash = function() { 
  let zero = new ctypes.intptr_t(8);
  let badptr = ctypes.cast(zero, ctypes.PointerType(ctypes.int32_t));
  badptr.contents
};


TestHelper = {
  init: function() {
    addMessageListener("social-test:crash", this);
  },

  receiveMessage: function(msg) {
    switch (msg.name) {
      case "social-test:crash":
        privateNoteIntentionalCrash();
        crash();
      break;
    }
  },
}

TestHelper.init();
