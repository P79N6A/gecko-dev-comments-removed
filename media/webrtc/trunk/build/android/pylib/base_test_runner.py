



import contextlib
import httplib
import logging
import os
import tempfile
import time

import android_commands
from chrome_test_server_spawner import SpawningServer
import constants
from flag_changer import FlagChanger
from forwarder import Forwarder
import lighttpd_server
import ports
from valgrind_tools import CreateTool




NET_TEST_SERVER_PORT_INFO_FILE = '/data/local/tmp/net-test-server-ports'


class BaseTestRunner(object):
  """Base class for running tests on a single device.

  A subclass should implement RunTests() with no parameter, so that calling
  the Run() method will set up tests, run them and tear them down.
  """

  def __init__(self, device, tool, shard_index, build_type):
    """
      Args:
        device: Tests will run on the device of this ID.
        shard_index: Index number of the shard on which the test suite will run.
        build_type: 'Release' or 'Debug'.
    """
    self.device = device
    self.adb = android_commands.AndroidCommands(device=device)
    self.tool = CreateTool(tool, self.adb)
    
    
    
    
    self.adb.SynchronizeDateTime()
    self._http_server = None
    self._forwarder = None
    self._forwarder_device_port = 8000
    self.forwarder_base_url = ('http://localhost:%d' %
        self._forwarder_device_port)
    self.flags = FlagChanger(self.adb)
    self.shard_index = shard_index
    self.flags.AddFlags(['--disable-fre'])
    self._spawning_server = None
    self._spawner_forwarder = None
    
    
    
    self.test_server_spawner_port = 0
    self.test_server_port = 0
    self.build_type = build_type

  def _PushTestServerPortInfoToDevice(self):
    """Pushes the latest port information to device."""
    self.adb.SetFileContents(NET_TEST_SERVER_PORT_INFO_FILE,
                             '%d:%d' % (self.test_server_spawner_port,
                                        self.test_server_port))

  def Run(self):
    """Calls subclass functions to set up tests, run them and tear them down.

    Returns:
      Test results returned from RunTests().
    """
    if not self.HasTests():
      return True
    self.SetUp()
    try:
      return self.RunTests()
    finally:
      self.TearDown()

  def SetUp(self):
    """Called before tests run."""
    pass

  def HasTests(self):
    """Whether the test suite has tests to run."""
    return True

  def RunTests(self):
    """Runs the tests. Need to be overridden."""
    raise NotImplementedError

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
          os.path.join(constants.CHROME_DIR, p),
          os.path.join(dest_dir, p))

  def LinkSdCardPathsToTempDir(self, paths):
    """Link |paths| which are under sdcard to /data/local/tmp.

    For example, the test data '/sdcard/my_data' will be linked to
    '/data/local/tmp/my_data'.

    Args:
      paths: A list of files and directories relative to /sdcard.
    """
    links = set()
    for path in paths:
      link_name = os.path.dirname(path)
      assert link_name, 'Linked paths must be in a subdir of /sdcard/.'
      link_name = link_name.split('/')[0]
      if link_name not in links:
        mapped_device_path = '/data/local/tmp/' + link_name
        
        
        self.adb.RunShellCommand('rm -r %s' %  mapped_device_path)
        self.adb.RunShellCommand(
            'ln -s /sdcard/%s %s' % (link_name, mapped_device_path))
        links.add(link_name)

  def LaunchTestHttpServer(self, document_root, port=None,
                           extra_config_contents=None):
    """Launches an HTTP server to serve HTTP tests.

    Args:
      document_root: Document root of the HTTP server.
      port: port on which we want to the http server bind.
      extra_config_contents: Extra config contents for the HTTP server.
    """
    self._http_server = lighttpd_server.LighttpdServer(
        document_root, port=port, extra_config_contents=extra_config_contents)
    if self._http_server.StartupHttpServer():
      logging.info('http server started: http://localhost:%s',
                   self._http_server.port)
    else:
      logging.critical('Failed to start http server')
    self.StartForwarderForHttpServer()

  def StartForwarder(self, port_pairs):
    """Starts TCP traffic forwarding for the given |port_pairs|.

    Args:
      host_port_pairs: A list of (device_port, local_port) tuples to forward.
    """
    
    
    
    self.adb.KillAll('forwarder')
    if self._forwarder:
      self._forwarder.Close()
    self._forwarder = Forwarder(
        self.adb, port_pairs, self.tool, '127.0.0.1', self.build_type)

  def StartForwarderForHttpServer(self):
    """Starts a forwarder for the HTTP server.

    The forwarder forwards HTTP requests and responses between host and device.
    """
    self.StartForwarder([(self._forwarder_device_port, self._http_server.port)])

  def RestartHttpServerForwarderIfNecessary(self):
    """Restarts the forwarder if it's not open."""
    
    
    
    
    if not ports.IsDevicePortUsed(self.adb,
        self._forwarder_device_port):
      self.StartForwarderForHttpServer()

  def ShutdownHelperToolsForTestSuite(self):
    """Shuts down the server and the forwarder."""
    
    
    
    if self._forwarder or self._spawner_forwarder:
      
      
      self.adb.KillAll('forwarder')
      if self._forwarder:
        self._forwarder.Close()
      if self._spawner_forwarder:
        self._spawner_forwarder.Close()
    if self._http_server:
      self._http_server.ShutdownHttpServer()
    if self._spawning_server:
      self._spawning_server.Stop()
    self.flags.Restore()

  def LaunchChromeTestServerSpawner(self):
    """Launches test server spawner."""
    server_ready = False
    error_msgs = []
    
    for i in xrange(0, 3):
      
      
      self.test_server_spawner_port = ports.AllocateTestServerPort()
      self._spawning_server = SpawningServer(self.test_server_spawner_port,
                                             self.test_server_port)
      self._spawning_server.Start()
      server_ready, error_msg = ports.IsHttpServerConnectable(
          '127.0.0.1', self.test_server_spawner_port, path='/ping',
          expected_read='ready')
      if server_ready:
        break
      else:
        error_msgs.append(error_msg)
      self._spawning_server.Stop()
      
      time.sleep(2)
    if not server_ready:
      logging.error(';'.join(error_msgs))
      raise Exception('Can not start the test spawner server.')
    self._PushTestServerPortInfoToDevice()
    self._spawner_forwarder = Forwarder(
        self.adb,
        [(self.test_server_spawner_port, self.test_server_spawner_port)],
        self.tool, '127.0.0.1', self.build_type)
