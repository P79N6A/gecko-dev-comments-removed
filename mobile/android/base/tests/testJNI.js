




Components.utils.import("resource://gre/modules/ctypes.jsm");
Components.utils.import("resource://gre/modules/JNI.jsm");

add_task(function test_JNI() {
  let iconSize = -1;

  let jni = null;
  try {
    jni = new JNI();
    let cls = jni.findClass("org/mozilla/gecko/GeckoAppShell");
    let method = jni.getStaticMethodID(cls, "getPreferredIconSize", "()I");
    iconSize = jni.callStaticIntMethod(cls, method);
  } finally {
    if (jni != null) {
      jni.close();
    }
  }

  do_check_neq(iconSize, -1);
});

run_next_test();
