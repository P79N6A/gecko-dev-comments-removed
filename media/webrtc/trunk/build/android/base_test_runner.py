



import logging
import os

import android_commands
from chrome_test_server_spawner import SpawningServer
from flag_changer import FlagChanger
import lighttpd_server
import run_tests_helper

FORWARDER_PATH = '/data/local/tmp/forwarder'

TEST_SERVER_SPAWNER_PORT = 8001
TEST_SERVER_PORT = 8002
TEST_SYNC_SERVER_PORT = 8003


class BaseTestRunner(object):
  """Base class for running tests on a single device."""

  def __init__(self, device):
    """
      Args:
        device: Tests will run on the device of this ID.
    """
    self.device = device
    self.adb = android_commands.AndroidCommands(device=device)
    
    
    
    
    self.adb.SynchronizeDateTime()
    self._http_server = None
    self._forwarder = None
    self._spawning_server = None
    self._spawner_forwarder = None
    self._forwarder_device_port = 8000
    self.forwarder_base_url = ('http://localhost:%d' %
        self._forwarder_device_port)
    self.flags = FlagChanger(self.adb)

  def RunTests(self):
    
    
    
    raise NotImplementedError

  def SetUp(self):
    """Called before tests run."""
    pass

  def TearDown(self):
    """Called when tests finish running."""
    self.ShutdownHelperToolsForTestSuite()

  def CopyTestData(self, test_data_paths, dest_dir):
    """Copies |test_data_paths| list of files/directories to |dest_dir|.

    Args:
      test_data_paths: A list of files or directories relative to |dest_dir|
          which should be copied to the device. The paths must exist in
          |CHROME_DIR|.
      dest_dir: Absolute path to copy to on the device.
    """
    for p in test_data_paths:
      self.adb.PushIfNeeded(
          os.path.join(run_tests_helper.CHROME_DIR, p),
          os.path.join(dest_dir, p))

  def LaunchTestHttpServer(self, document_root, extra_config_contents=None):
    """Launches an HTTP server to serve HTTP tests.

    Args:
      document_root: Document root of the HTTP server.
      extra_config_contents: Extra config contents for the HTTP server.
    """
    self._http_server = lighttpd_server.LighttpdServer(
        document_root, extra_config_contents=extra_config_contents)
    if self._http_server.StartupHttpServer():
      logging.info('http server started: http://localhost:%s',
                   self._http_server.port)
    else:
      logging.critical('Failed to start http server')
    
    self.adb.EnableAdbRoot()
    self.StartForwarderForHttpServer()

  def StartForwarderForHttpServer(self):
    """Starts a forwarder for the HTTP server.

    The forwarder forwards HTTP requests and responses between host and device.
    """
    
    
    
    
    
    
    
    
    self._forwarder = run_tests_helper.ForwardDevicePorts(
        self.adb, [(self._forwarder_device_port, self._http_server.port)])

  def RestartHttpServerForwarderIfNecessary(self):
    """Restarts the forwarder if it's not open."""
    
    
    
    
    if not run_tests_helper.IsDevicePortUsed(self.adb,
        self._forwarder_device_port):
      self.StartForwarderForHttpServer()

  def ShutdownHelperToolsForTestSuite(self):
    """Shuts down the server and the forwarder."""
    
    
    
    if self._forwarder or self._spawner_forwarder:
      
      
      self.adb.KillAll('forwarder')
      if self._forwarder:
        self._forwarder.kill()
      if self._spawner_forwarder:
        self._spawner_forwarder.kill()
    if self._http_server:
      self._http_server.ShutdownHttpServer()
    if self._spawning_server:
      self._spawning_server.Stop()
    self.flags.Restore()

  def LaunchChromeTestServerSpawner(self):
    """Launches test server spawner."""
    self._spawning_server = SpawningServer(TEST_SERVER_SPAWNER_PORT,
                                          TEST_SERVER_PORT)
    self._spawning_server.Start()
    
    
    
    self._spawner_forwarder = run_tests_helper.ForwardDevicePorts(
        self.adb, [(TEST_SERVER_SPAWNER_PORT, TEST_SERVER_SPAWNER_PORT),
                   (TEST_SERVER_PORT, TEST_SERVER_PORT)])
