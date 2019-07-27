



config = {
    "reftest_options": [
        "--appname=%(binary_path)s", "--utility-path=tests/bin",
        "--extra-profile-file=tests/bin/plugins", "--symbols-path=%(symbols_path)s"
    ],
    "mochitest_options": [
        "--appname=%(binary_path)s", "--utility-path=tests/bin",
        "--extra-profile-file=tests/bin/plugins", "--symbols-path=%(symbols_path)s",
        "--certificate-path=tests/certs", "--autorun", "--close-when-done",
        "--console-level=INFO", "--setpref=webgl.force-enabled=true",
        "--quiet", "--log-raw=%(raw_log_file)s",
        "--use-test-media-devices"
    ],
    "webapprt_options": [
        "--app=%(app_path)s", "--utility-path=tests/bin",
        "--extra-profile-file=tests/bin/plugins", "--symbols-path=%(symbols_path)s",
        "--certificate-path=tests/certs", "--autorun", "--close-when-done",
        "--console-level=INFO", "--testing-modules-dir=tests/modules",
        "--quiet"
    ],
    "xpcshell_options": [
        "--symbols-path=%(symbols_path)s",
        "--test-plugin-path=%(test_plugin_path)s"
    ],
    "cppunittest_options": [
        "--symbols-path=%(symbols_path)s",
        "--xre-path=%(abs_app_dir)s"
    ],
    "jittest_options": [
        "tests/bin/js",
        "--no-slow",
        "--no-progress",
        "--tinderbox",
        "--tbpl"
    ],
    "mozbase_options": [
        "-b", "%(binary_path)s"
    ],
}
