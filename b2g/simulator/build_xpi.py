















import sys, os, re, subprocess
from mozbuild.preprocessor import Preprocessor
from mozbuild.base import MozbuildObject
from mozbuild.util import ensureParentDir
from zipfile import ZipFile
from distutils.version import LooseVersion

ftp_root_path = "/pub/mozilla.org/labs/fxos-simulator"
UPDATE_LINK = "https://ftp.mozilla.org" + ftp_root_path + "/%(update_path)s/%(xpi_name)s"
UPDATE_URL = "https://ftp.mozilla.org" + ftp_root_path + "/%(update_path)s/update.rdf"
XPI_NAME = "fxos-simulator-%(version)s-%(platform)s.xpi"

class GaiaBuilder(object):
    def __init__(self, build, gaia_path):
        self.build = build
        self.gaia_path = gaia_path

    def clean(self):
        self.build._run_make(target="clean", directory=self.gaia_path)

    def profile(self, env):
        self.build._run_make(target="profile", directory=self.gaia_path, num_jobs=1, silent=False, append_env=env)

    def override_prefs(self, srcfile):
        
        
        with open(os.path.join(self.gaia_path, "profile", "user.js"), "a") as userJs:
            userJs.write(open(srcfile).read())

def process_package_overload(src, dst, version, app_buildid):
    ensureParentDir(dst)
    
    
    
    
    
    defines = {
        "NUM_VERSION": version,
        "SLASH_VERSION": version.replace(".", "_"),
        "FULL_VERSION": ("%s.%s" % (version, app_buildid[:8]))
    }
    pp = Preprocessor(defines=defines)
    pp.do_filter("substitution")
    with open(dst, "w") as output:
        with open(src, "r") as input:
            pp.processFile(input=input, output=output)

def add_dir_to_zip(zip, top, pathInZip, blacklist=()):
    zf = ZipFile(zip, "a")
    for dirpath, subdirs, files in os.walk(top):
        dir_relpath = os.path.relpath(dirpath, top)
        if dir_relpath.startswith(blacklist):
            continue
        zf.write(dirpath, os.path.join(pathInZip, dir_relpath))
        for filename in files:
            relpath = os.path.join(dir_relpath, filename)
            if relpath in blacklist:
                continue
            zf.write(os.path.join(dirpath, filename),
                     os.path.join(pathInZip, relpath))
    zf.close()

def main(platform):
    build = MozbuildObject.from_environment()
    topsrcdir = build.topsrcdir
    distdir = build.distdir

    srcdir = os.path.join(topsrcdir, "b2g", "simulator")

    app_buildid = open(os.path.join(build.topobjdir, "config", "buildid")).read().strip()

    
    
    
    b2g_version = build.config_environment.defines["MOZ_B2G_VERSION"].replace('"', '')
    version = ".".join(str(n) for n in LooseVersion(b2g_version).version[0:2])

    
    
    
    
    
    gaia_path = build.config_environment.substs["GAIADIR"]
    builder = GaiaBuilder(build, gaia_path)
    builder.clean()
    env = {
      "NOFTU": "1",
      "GAIA_APP_TARGET": "production",
      "SETTINGS_PATH": os.path.join(srcdir, "custom-settings.json")
    }
    builder.profile(env)
    builder.override_prefs(os.path.join(srcdir, "custom-prefs.js"))

    
    manifest_overload = os.path.join(build.topobjdir, "b2g", "simulator", "package-overload.json")
    process_package_overload(os.path.join(srcdir, "package-overload.json.in"),
                             manifest_overload,
                             version,
                             app_buildid)

    
    xpi_name = XPI_NAME % {"version": version, "platform": platform}
    xpi_path = os.path.join(distdir, xpi_name)

    update_path = "%s/%s" % (version, platform)
    update_link = UPDATE_LINK % {"update_path": update_path, "xpi_name": xpi_name}
    update_url = UPDATE_URL % {"update_path": update_path}
    subprocess.check_call([
      build.virtualenv_manager.python_path, os.path.join(topsrcdir, "addon-sdk", "source", "bin", "cfx"), "xpi", \
      "--pkgdir", srcdir, \
      "--manifest-overload", manifest_overload, \
      "--strip-sdk", \
      "--update-link", update_link, \
      "--update-url", update_url, \
      "--static-args", "{\"label\": \"Firefox OS %s\"}" % version, \
      "--output-file", xpi_path \
    ])

    
    add_dir_to_zip(xpi_path, os.path.join(distdir, "b2g"), "b2g", ("gaia", "B2G.app/Contents/MacOS/gaia"))
    
    add_dir_to_zip(xpi_path, os.path.join(gaia_path, "profile"), "profile")
    
    
    add_dir_to_zip(xpi_path, os.path.join(srcdir, "defaults"), "defaults")

if __name__ == '__main__':
    if 2 != len(sys.argv):
        print("""Usage:
  python {0} MOZ_PKG_PLATFORM
""".format(sys.argv[0]))
        sys.exit(1)
    main(*sys.argv[1:])

