



import os

import mozcrash
from mozprocess import ProcessHandler
from mozprofile import FirefoxProfile, Preferences
from mozprofile.permissions import ServerLocations
from mozrunner import FirefoxRunner
import mozcrash
from mozcrash import mozcrash

from .base import get_free_port, Browser, ExecutorBrowser, require_arg, cmd_arg
from ..executors import executor_kwargs as base_executor_kwargs
from ..executors.executormarionette import MarionetteTestharnessExecutor, MarionetteReftestExecutor, required_files

here = os.path.join(os.path.split(__file__)[0])

__wptrunner__ = {"product": "firefox",
                 "check_args": "check_args",
                 "browser": "FirefoxBrowser",
                 "executor": {"testharness": "MarionetteTestharnessExecutor",
                              "reftest": "MarionetteReftestExecutor"},
                 "browser_kwargs": "browser_kwargs",
                 "executor_kwargs": "executor_kwargs",
                 "env_options": "env_options"}


def check_args(**kwargs):
    require_arg(kwargs, "binary")


def browser_kwargs(**kwargs):
    return {"binary": kwargs["binary"],
            "prefs_root": kwargs["prefs_root"],
            "debug_args": kwargs["debug_args"],
            "interactive": kwargs["interactive"],
            "symbols_path":kwargs["symbols_path"],
            "stackwalk_binary":kwargs["stackwalk_binary"]}


def executor_kwargs(http_server_url, **kwargs):
    executor_kwargs = base_executor_kwargs(http_server_url, **kwargs)
    executor_kwargs["close_after_done"] = True
    executor_kwargs["http_server_override"] = "http://web-platform.test:8000"
    return executor_kwargs


def env_options():
    return {"host": "localhost",
            "bind_hostname": "true",
            "required_files": required_files}


class FirefoxBrowser(Browser):
    used_ports = set()

    def __init__(self, logger, binary, prefs_root, debug_args=None, interactive=None,
                 symbols_path=None, stackwalk_binary=None):
        Browser.__init__(self, logger)
        self.binary = binary
        self.prefs_root = prefs_root
        self.marionette_port = None
        self.used_ports.add(self.marionette_port)
        self.runner = None
        self.debug_args = debug_args
        self.interactive = interactive
        self.profile = None
        self.symbols_path = symbols_path
        self.stackwalk_binary = stackwalk_binary

    def start(self):
        self.marionette_port = get_free_port(2828, exclude=self.used_ports)

        env = os.environ.copy()
        env["MOZ_CRASHREPORTER"] = "1"
        env["MOZ_CRASHREPORTER_SHUTDOWN"] = "1"
        env["MOZ_CRASHREPORTER_NO_REPORT"] = "1"
        env["MOZ_DISABLE_NONLOCAL_CONNECTIONS"] = "1"

        locations = ServerLocations(filename=os.path.join(here, "server-locations.txt"))

        preferences = self.load_prefs()

        self.profile = FirefoxProfile(locations=locations, proxy=True, preferences=preferences)
        self.profile.set_preferences({"marionette.defaultPrefs.enabled": True,
                                      "marionette.defaultPrefs.port": self.marionette_port,
                                      "dom.disable_open_during_load": False})

        self.runner = FirefoxRunner(profile=self.profile,
                                    binary=self.binary,
                                    cmdargs=[cmd_arg("marionette"), "about:blank"],
                                    env=env,
                                    process_class=ProcessHandler,
                                    process_args={"processOutputLine": [self.on_output]})

        self.logger.debug("Starting Firefox")
        self.runner.start(debug_args=self.debug_args, interactive=self.interactive)
        self.logger.debug("Firefox Started")

    def load_prefs(self):
        prefs_path = os.path.join(self.prefs_root, "prefs_general.js")
        if os.path.exists(prefs_path):
            preferences = Preferences.read_prefs(prefs_path)
        else:
            self.logger.warning("Failed to find base prefs file in %s" % prefs_path)
            preferences = []

        return preferences

    def stop(self):
        self.logger.debug("Stopping browser")
        if self.runner is not None:
            try:
                self.runner.stop()
            except OSError:
                
                pass

    def pid(self):
        if self.runner.process_handler is None:
            return None

        try:
            return self.runner.process_handler.pid
        except AttributeError:
            return None

    def on_output(self, line):
        """Write a line of output from the firefox process to the log"""
        self.logger.process_output(self.pid(),
                                   line.decode("utf8", "replace"),
                                   command=" ".join(self.runner.command))

    def is_alive(self):
        return self.runner.is_running()

    def cleanup(self):
        self.stop()

    def executor_browser(self):
        assert self.marionette_port is not None
        return ExecutorBrowser, {"marionette_port": self.marionette_port}

    def log_crash(self, logger, process, test):
        dump_dir = os.path.join(self.profile.profile, "minidumps")
        mozcrash.log_crashes(logger, dump_dir, symbols_path=self.symbols_path,
                             stackwalk_binary=self.stackwalk_binary,
                             process=process, test=test)
