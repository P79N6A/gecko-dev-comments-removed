




Components.utils.import("resource://gre/modules/ctypes.jsm");
Components.utils.import("resource://gre/modules/JNI.jsm");

add_task(function test_JNI() {
  var jenv = null;
  try {
    jenv = JNI.GetForThread();

    
    var geckoAppShell = JNI.LoadClass(jenv, "org.mozilla.gecko.GeckoAppShell", {
      static_methods: [
        { name: "getPreferredIconSize", sig: "()I" }
      ],
    });

    let iconSize = -1;
    iconSize = geckoAppShell.getPreferredIconSize();
    do_check_neq(iconSize, -1);

    
    
    
    let jGeckoNetworkManager = JNI.LoadClass(jenv, "org/mozilla/gecko/GeckoNetworkManager", {
      static_methods: [
        { name: "getMNC", sig: "()I" },
        { name: "getMCC", sig: "()I" },
      ],
    });
    do_check_eq(typeof jGeckoNetworkManager.getMNC(), "number");
    do_check_eq(typeof jGeckoNetworkManager.getMCC(), "number");
  } finally {
    if (jenv) {
      JNI.UnloadClasses(jenv);
    }
  }
});

run_next_test();
