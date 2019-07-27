



config = {
    "jsreftest_options": [
        "--adbpath=%(adbpath)s", "--b2gpath=%(b2gpath)s", "--emulator=%(emulator)s",
        "--emulator-res=800x1000", "--logdir=%(logcat_dir)s",
        "--remote-webserver=%(remote_webserver)s", "--ignore-window-size",
        "--xre-path=%(xre_path)s", "--symbols-path=%(symbols_path)s", "--busybox=%(busybox)s",
        "--total-chunks=%(total_chunks)s", "--this-chunk=%(this_chunk)s",
        "--extra-profile-file=jsreftest/tests/user.js",
        "jsreftest/tests/jstests.list",
    ],

    "mochitest_options": [
        "--adbpath=%(adbpath)s", "--b2gpath=%(b2gpath)s", "--console-level=INFO",
        "--emulator=%(emulator)s", "--logdir=%(logcat_dir)s",
        "--remote-webserver=%(remote_webserver)s", "%(test_manifest)s",
        "--xre-path=%(xre_path)s", "--symbols-path=%(symbols_path)s", "--busybox=%(busybox)s",
        "--total-chunks=%(total_chunks)s", "--this-chunk=%(this_chunk)s",
        "--quiet", "--log-raw=%(raw_log_file)s",
        "--certificate-path=%(certificate_path)s",
        "--test-path=%(test_path)s",
    ],

    "reftest_options": [
        "--adbpath=%(adbpath)s", "--b2gpath=%(b2gpath)s", "--emulator=%(emulator)s",
        "--emulator-res=800x1000", "--logdir=%(logcat_dir)s",
        "--remote-webserver=%(remote_webserver)s", "--ignore-window-size",
        "--xre-path=%(xre_path)s", "--symbols-path=%(symbols_path)s", "--busybox=%(busybox)s",
        "--total-chunks=%(total_chunks)s", "--this-chunk=%(this_chunk)s", "--enable-oop",
        "tests/layout/reftests/reftest.list",
    ],

    "crashtest_options": [
        "--adbpath=%(adbpath)s", "--b2gpath=%(b2gpath)s", "--emulator=%(emulator)s",
        "--emulator-res=800x1000", "--logdir=%(logcat_dir)s",
        "--remote-webserver=%(remote_webserver)s", "--ignore-window-size",
        "--xre-path=%(xre_path)s", "--symbols-path=%(symbols_path)s", "--busybox=%(busybox)s",
        "--total-chunks=%(total_chunks)s", "--this-chunk=%(this_chunk)s",
        "tests/testing/crashtest/crashtests.list",
    ],

    "xpcshell_options": [
        "--adbpath=%(adbpath)s", "--b2gpath=%(b2gpath)s", "--emulator=%(emulator)s",
        "--logdir=%(logcat_dir)s", "--manifest=tests/xpcshell.ini", "--use-device-libs",
        "--testing-modules-dir=%(modules_dir)s", "--symbols-path=%(symbols_path)s",
        "--busybox=%(busybox)s", "--total-chunks=%(total_chunks)s", "--this-chunk=%(this_chunk)s",
    ],

    "cppunittest_options": [
        "--dm_trans=adb",
        "--symbols-path=%(symbols_path)s",
        "--xre-path=%(xre_path)s",
        "--addEnv", "LD_LIBRARY_PATH=/vendor/lib:/system/lib:/system/b2g",
        "--with-b2g-emulator=%(b2gpath)s",
        "--skip-manifest=b2g_cppunittest_manifest.txt",
        "."
    ],
}
