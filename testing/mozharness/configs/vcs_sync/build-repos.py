import os
import socket
hostname = socket.gethostname()

build_repos = (
    'autoland',
    'buildapi',
    'buildbot-configs',
    'buildbotcustom',
    'mozharness',
    'opsi-package-sources',
    'partner-repacks',
    'preproduction',
    'puppet',
    'puppet-manifests',
    'rpm-sources',
    'talos',
    'tools',
)

conversion_repos = []
remote_targets = {}

for repo in build_repos:
    conversion_repos.append({
        "repo": "https://hg.mozilla.org/build/%s" % repo,
        "repo_name": "build-%s" % repo,
        "conversion_dir": "build-%s" % repo,
        "targets": [{
            "target_dest": "build-%s-github" % repo,
            "force_push": True
        }],
        "vcs": "hg",
        "mapper": {
            "url": "https://api.pub.build.mozilla.org/mapper",
            "project": "build-%s" % repo,
        },
        "branch_config": {
            "branches": {
                "default": "master",
            },
            "branch_regexes": [
                "^.*$"
            ]
        },








    })
    remote_targets["build-%s-github" % repo] = {
        "repo": "git@github.com:mozilla/build-%s.git" % repo,
        "ssh_key": "~/.ssh/releng-github-id_rsa",
        "vcs": "git",
    }

config = {
    "log_name": "build-repos",
    "log_max_rotate": 99,
    "job_name": "build-repos",
    "env": {
        "PATH": "%(PATH)s:/usr/libexec/git-core",
    },
    "conversion_repos": conversion_repos,
    "remote_targets": remote_targets,
    "virtualenv_modules": [
        "dulwich==0.9.0",
        "ordereddict==1.1",
        "hg-git==0.4.0-moz2",
        "mapper==0.1",
        "mercurial==2.6.3",
        "mozfile==0.9",
        "mozinfo==0.5",
        "mozprocess==0.11",
        "requests==2.2.1",
    ],
    "find_links": [
        "http://pypi.pub.build.mozilla.org/pub"
    ],
    "pip_index": False,

    "default_notify_from": "developer-services+%s@mozilla.org" % hostname,
    "notify_config": [{
        "to": "releng-ops-trial@mozilla.com",
        "failure_only": False,
        "skip_empty_messages": True,
    }],

    
    "vcs_share_base": None,
    "hg_share_base": None,

    
    "hg_options": (
        "--config",
        "web.cacerts=/etc/pki/tls/certs/ca-bundle.crt"
    ),

    "default_actions": [
        'list-repos',
        'create-virtualenv',
        'update-stage-mirror',
        'update-work-mirror',
        'publish-to-mapper',
        'push',
        'combine-mapfiles',
        'notify',
    ],
}
