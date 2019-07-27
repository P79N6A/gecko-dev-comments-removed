




config = {
    "log_name": "staging_beta",

    "tools_repo_url": "https://hg.mozilla.org/build/tools",
    "tools_repo_revision": "default",
    "from_repo_url": "ssh://hg.mozilla.org/releases/mozilla-aurora",
    "to_repo_url": "ssh://hg.mozilla.org/users/stage-ffxbld/mozilla-beta",

    "base_tag": "FIREFOX_BETA_%(major_version)s_BASE",
    "end_tag": "FIREFOX_BETA_%(major_version)s_END",

    "migration_behavior": "aurora_to_beta",
}
