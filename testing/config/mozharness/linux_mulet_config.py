
config = {
    
    "reftest_options": [
        "--mulet",
        "--profile=%(gaia_profile)s",
        "--appname=%(application)s",
        "--total-chunks=%(total_chunks)s",
        "--this-chunk=%(this_chunk)s",
        "--symbols-path=%(symbols_path)s",
        "--enable-oop",
        "%(test_manifest)s"
    ],
    "run_file_names": {
        "reftest": "runreftestb2g.py",
    },
}
