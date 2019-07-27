import os

BUILD_DIR = "mozilla-central"


REPO_PATH = "mozilla-central"



L10N_REPO_PATH = "l10n-central"

OBJDIR = "objdir-droid"

ANDROID_DIR = "mobile/android"


MOZCONFIG = os.path.join(os.getcwd(), "mozconfig")

config = {
    "work_dir": ".",
    "log_name": "multilocale",
    "objdir": OBJDIR,
    "locales_file": "%s/%s/locales/maemo-locales" % (BUILD_DIR, ANDROID_DIR),
    "locales_dir": "%s/locales" % ANDROID_DIR,
    "ignore_locales": ["en-US", "multi"],
    "repos": [{
        "repo": "https://hg.mozilla.org/%s" % REPO_PATH,
        "tag": "default",
        "dest": BUILD_DIR,
    }],
    "l10n_repos": [{
        "repo": "https://hg.mozilla.org/build/compare-locales",
        "tag": "RELEASE_AUTOMATION"
    }],
    "hg_l10n_base": "https://hg.mozilla.org/%s" % L10N_REPO_PATH,
    "hg_l10n_tag": "default",
    "l10n_dir": "l10n",
    "merge_locales": True,
    "mozilla_dir": BUILD_DIR,
    "mozconfig": MOZCONFIG,
    "default_actions": [
        "pull-locale-source",
        "build",
        "package-en-US",
        "backup-objdir",
        "restore-objdir",
        "add-locales",
        "package-multi",
        "summary",
    ],
}
