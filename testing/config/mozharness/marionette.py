



config = {
    "marionette_desktop_options": [
        "--type=%(type)s",
        "--log-raw=%(raw_log_file)s",
        "--binary=%(binary)s",
        "--address=%(address)s",
        "--symbols-path=%(symbols_path)s",
    ],
    "marionette_emulator_options": [
        "--type=%(type)s",
        "--log-raw=%(raw_log_file)s",
        "--logcat-dir=%(logcat_dir)s",
        "--emulator=%(emulator)s",
        "--homedir=%(homedir)s",
        "--symbols-path=%(symbols_path)s",
    ],
    "webapi_emulator_options": [
        "--type=%(type)s",
        "--log-raw=%(raw_log_file)s",
        "--symbols-path=%(symbols_path)s",
        "--logcat-dir=%(logcat_dir)s",
        "--emulator=%(emulator)s",
        "--homedir=%(homedir)s",
    ],
    
    "webapi_desktop_options": [
    ],
    "gaiatest_emulator_options": [
        "--restart",
        "--timeout=%(timeout)s",
        "--type=%(type)s",
        "--testvars=%(testvars)s",
        "--profile=%(profile)s",
        "--symbols-path=%(symbols_path)s",
        "--xml-output=%(xml_output)s",
        "--html-output=%(html_output)s",
        "--log-raw=%(raw_log_file)s",
        "--logcat-dir=%(logcat_dir)s",
        "--emulator=%(emulator)s",
        "--homedir=%(homedir)s",
    ],
    "gaiatest_desktop_options": [
        "--restart",
        "--timeout=%(timeout)s",
        "--type=%(type)s",
        "--testvars=%(testvars)s",
        "--profile=%(profile)s",
        "--symbols-path=%(symbols_path)s",
        "--gecko-log=%(gecko_log)s",
        "--xml-output=%(xml_output)s",
        "--html-output=%(html_output)s",
        "--log-raw=%(raw_log_file)s",
        "--binary=%(binary)s",
        "--address=%(address)s",
        "--total-chunks=%(total_chunks)s",
        "--this-chunk=%(this_chunk)s",
    ],
}
