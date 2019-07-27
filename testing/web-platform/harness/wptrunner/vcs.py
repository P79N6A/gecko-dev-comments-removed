



import subprocess
from functools import partial


def vcs(bin_name):
    def inner(command, *args, **kwargs):
        repo = kwargs.pop("repo", None)
        if kwargs:
            raise TypeError, kwargs

        args = list(args)

        proc_kwargs = {}
        if repo is not None:
            proc_kwargs["cwd"] = repo

        command_line = [bin_name, command] + args
        print " ".join(command_line)
        try:
            return subprocess.check_output(command_line, **proc_kwargs)
        except subprocess.CalledProcessError as e:
            print proc_kwargs
            print e.output
            raise
    return inner

git = vcs("git")
hg = vcs("hg")


def bind_to_repo(vcs_func, repo):
    return partial(vcs_func, repo=repo)


def is_git_root(path):
    try:
        rv = git("rev-parse", "--show-cdup", repo=path)
    except subprocess.CalledProcessError:
        return False
    print rv
    return rv == "\n"
