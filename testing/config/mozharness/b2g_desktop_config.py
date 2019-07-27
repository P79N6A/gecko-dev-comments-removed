



config = {
    "mochitest_options": [
        "--console-level=INFO", "%(test_manifest)s",
        "--total-chunks=%(total_chunks)s", "--this-chunk=%(this_chunk)s",
        "--profile=%(gaia_profile)s", "--app=%(application)s", "--desktop",
        "--utility-path=%(utility_path)s", "--certificate-path=%(cert_path)s",
        "--symbols-path=%(symbols_path)s", "--browser-arg=%(browser_arg)s",
        "--quiet", "--log-raw=%(raw_log_file)s"
    ],

    "reftest_options": [
        "--desktop", "--profile=%(gaia_profile)s", "--appname=%(application)s",
        "--total-chunks=%(total_chunks)s", "--this-chunk=%(this_chunk)s",
        "--browser-arg=%(browser_arg)s", "--symbols-path=%(symbols_path)s",
        "%(test_manifest)s"
    ]
}
