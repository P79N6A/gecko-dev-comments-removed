
config = {
    
    "reftest_options": [
        "--desktop", "--profile=%(gaia_profile)s",
        "--appname=%(application)s", "%(test_manifest)s"
    ],
    "run_file_names": {
        "reftest": "runreftestb2g.py",
    },
}
