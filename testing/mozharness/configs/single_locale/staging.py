config = {
    "upload_env": {
        "UPLOAD_USER": "ffxbld",
        
        
        
        "UPLOAD_SSH_KEY": "%(ssh_key_dir)s/stage-ffxbld_rsa",
        "UPLOAD_HOST": "dev-stage01.srv.releng.scl3.mozilla.com",
        "POST_UPLOAD_CMD": "post_upload.py -b %(branch)s-l10n -p firefox -i %(buildid)s --release-to-latest --release-to-dated",
        "UPLOAD_TO_TEMP": "1"
    },
    'taskcluster_index': 'index.garbage.staging',
}
