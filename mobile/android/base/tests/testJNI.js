




Components.utils.import("resource://gre/modules/ctypes.jsm");
Components.utils.import("resource://gre/modules/JNI.jsm");

add_task(function test_JNI() {
  var jenv = null;
  try {
    jenv = JNI.GetForThread();

    
    var geckoAppShell = JNI.LoadClass(jenv, "org.mozilla.gecko.GeckoAppShell", {
      static_methods: [
        { name: "getPreferredIconSize", sig: "()I" },
        { name: "getContext", sig: "()Landroid/content/Context;" },
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

    
    
    JNI.LoadClass(jenv, "android.content.Context", {
      methods: [
        { name: "getClass", sig: "()Ljava/lang/Class;" },
      ],
    });
    JNI.LoadClass(jenv, "java.lang.Class", {
      methods: [
        { name: "getName", sig: "()Ljava/lang/String;" },
      ],
    });
    do_check_eq("org.mozilla.gecko.BrowserApp", JNI.ReadString(jenv, geckoAppShell.getContext().getClass().getName()));
  } finally {
    if (jenv) {
      JNI.UnloadClasses(jenv);
    }
  }
});

run_next_test();
