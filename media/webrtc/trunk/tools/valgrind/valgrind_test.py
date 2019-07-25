



"""Runs an exe through Valgrind and puts the intermediate files in a
directory.
"""

import datetime
import glob
import logging
import optparse
import os
import re
import shutil
import stat
import subprocess
import sys
import tempfile

import common

import drmemory_analyze
import memcheck_analyze
import tsan_analyze

class BaseTool(object):
  """Abstract class for running Valgrind-, PIN-based and other dynamic
  error detector tools.

  Always subclass this and implement ToolCommand with framework- and
  tool-specific stuff.
  """

  def __init__(self):
    self.temp_dir = tempfile.mkdtemp(prefix="vg_logs_")  
    self.log_dir = self.temp_dir  
    self.option_parser_hooks = []
    
    
    self._env = {
      "G_SLICE" : "always-malloc",
      "NSS_DISABLE_UNLOAD" : "1",
      "NSS_DISABLE_ARENA_FREE_LIST" : "1",
      "GTEST_DEATH_TEST_USE_FORK": "1",
    }

  def ToolName(self):
    raise NotImplementedError, "This method should be implemented " \
                               "in the tool-specific subclass"

  def Analyze(self, check_sanity=False):
    raise NotImplementedError, "This method should be implemented " \
                               "in the tool-specific subclass"

  def RegisterOptionParserHook(self, hook):
    
    self.option_parser_hooks.append(hook)

  def CreateOptionParser(self):
    
    self._parser = optparse.OptionParser("usage: %prog [options] <program to "
                                         "test>")
    self._parser.disable_interspersed_args()
    self._parser.add_option("-t", "--timeout",
                      dest="timeout", metavar="TIMEOUT", default=10000,
                      help="timeout in seconds for the run (default 10000)")
    self._parser.add_option("", "--build_dir",
                            help="the location of the compiler output")
    self._parser.add_option("", "--source_dir",
                            help="path to top of source tree for this build"
                                 "(used to normalize source paths in baseline)")
    self._parser.add_option("", "--gtest_filter", default="",
                            help="which test case to run")
    self._parser.add_option("", "--gtest_repeat",
                            help="how many times to run each test")
    self._parser.add_option("", "--gtest_print_time", action="store_true",
                            default=False,
                            help="show how long each test takes")
    self._parser.add_option("", "--ignore_exit_code", action="store_true",
                            default=False,
                            help="ignore exit code of the test "
                                 "(e.g. test failures)")
    self._parser.add_option("", "--keep_logs", action="store_true",
                            default=False,
                            help="store memory tool logs in the <tool>.logs "
                                 "directory instead of /tmp.\nThis can be "
                                 "useful for tool developers/maintainers.\n"
                                 "Please note that the <tool>.logs directory "
                                 "will be clobbered on tool startup.")

    
    
    
    for hook in self.option_parser_hooks:
      hook(self, self._parser)

  def ParseArgv(self, args):
    self.CreateOptionParser()

    
    
    self._tool_flags = []
    known_args = []

    """ We assume that the first argument not starting with "-" is a program
    name and all the following flags should be passed to the program.
    TODO(timurrrr): customize optparse instead
    """
    while len(args) > 0 and args[0][:1] == "-":
      arg = args[0]
      if (arg == "--"):
        break
      if self._parser.has_option(arg.split("=")[0]):
        known_args += [arg]
      else:
        self._tool_flags += [arg]
      args = args[1:]

    if len(args) > 0:
      known_args += args

    self._options, self._args = self._parser.parse_args(known_args)

    self._timeout = int(self._options.timeout)
    self._source_dir = self._options.source_dir
    if self._options.keep_logs:
      self.log_dir = "%s.logs" % self.ToolName()
      if os.path.exists(self.log_dir):
        shutil.rmtree(self.log_dir)
      os.mkdir(self.log_dir)

    self._ignore_exit_code = self._options.ignore_exit_code
    if self._options.gtest_filter != "":
      self._args.append("--gtest_filter=%s" % self._options.gtest_filter)
    if self._options.gtest_repeat:
      self._args.append("--gtest_repeat=%s" % self._options.gtest_repeat)
    if self._options.gtest_print_time:
      self._args.append("--gtest_print_time")

    return True

  def Setup(self, args):
    return self.ParseArgv(args)

  def ToolCommand(self):
    raise NotImplementedError, "This method should be implemented " \
                               "in the tool-specific subclass"

  def Cleanup(self):
    
    pass

  def Execute(self):
    """ Execute the app to be tested after successful instrumentation.
    Full execution command-line provided by subclassers via proc."""
    logging.info("starting execution...")
    proc = self.ToolCommand()
    for var in self._env:
      common.PutEnvAndLog(var, self._env[var])
    return common.RunSubprocess(proc, self._timeout)

  def RunTestsAndAnalyze(self, check_sanity):
    exec_retcode = self.Execute()
    analyze_retcode = self.Analyze(check_sanity)

    if analyze_retcode:
      logging.error("Analyze failed.")
      logging.info("Search the log for '[ERROR]' to see the error reports.")
      return analyze_retcode

    if exec_retcode:
      if self._ignore_exit_code:
        logging.info("Test execution failed, but the exit code is ignored.")
      else:
        logging.error("Test execution failed.")
        return exec_retcode
    else:
      logging.info("Test execution completed successfully.")

    if not analyze_retcode:
      logging.info("Analysis completed successfully.")

    return 0

  def Main(self, args, check_sanity):
    """Call this to run through the whole process: Setup, Execute, Analyze"""
    start = datetime.datetime.now()
    retcode = -1
    if self.Setup(args):
      retcode = self.RunTestsAndAnalyze(check_sanity)
      shutil.rmtree(self.temp_dir, ignore_errors=True)
      self.Cleanup()
    else:
      logging.error("Setup failed")
    end = datetime.datetime.now()
    seconds = (end - start).seconds
    hours = seconds / 3600
    seconds = seconds % 3600
    minutes = seconds / 60
    seconds = seconds % 60
    logging.info("elapsed time: %02d:%02d:%02d" % (hours, minutes, seconds))
    return retcode

  def Run(self, args, module):
    MODULES_TO_SANITY_CHECK = ["base"]

    
    if self.ToolName() == "tsan" and common.IsMac():
      MODULES_TO_SANITY_CHECK = []

    check_sanity = module in MODULES_TO_SANITY_CHECK
    return self.Main(args, check_sanity)


class ValgrindTool(BaseTool):
  """Abstract class for running Valgrind tools.

  Always subclass this and implement ToolSpecificFlags() and
  ExtendOptionParser() for tool-specific stuff.
  """
  def __init__(self):
    super(ValgrindTool, self).__init__()
    self.RegisterOptionParserHook(ValgrindTool.ExtendOptionParser)

  def UseXML(self):
    
    return True

  def SelfContained(self):
    
    
    return False

  def ExtendOptionParser(self, parser):
    parser.add_option("", "--suppressions", default=[],
                            action="append",
                            help="path to a valgrind suppression file")
    parser.add_option("", "--indirect", action="store_true",
                            default=False,
                            help="set BROWSER_WRAPPER rather than "
                                 "running valgrind directly")
    parser.add_option("", "--indirect_webkit_layout", action="store_true",
                            default=False,
                            help="set --wrapper rather than running valgrind "
                                 "directly.")
    parser.add_option("", "--trace_children", action="store_true",
                            default=False,
                            help="also trace child processes")
    parser.add_option("", "--num-callers",
                            dest="num_callers", default=30,
                            help="number of callers to show in stack traces")
    parser.add_option("", "--generate_dsym", action="store_true",
                          default=False,
                          help="Generate .dSYM file on Mac if needed. Slow!")

  def Setup(self, args):
    if not BaseTool.Setup(self, args):
      return False
    if common.IsMac():
      self.PrepareForTestMac()
    return True

  def PrepareForTestMac(self):
    """Runs dsymutil if needed.

    Valgrind for Mac OS X requires that debugging information be in a .dSYM
    bundle generated by dsymutil.  It is not currently able to chase DWARF
    data into .o files like gdb does, so executables without .dSYM bundles or
    with the Chromium-specific "fake_dsym" bundles generated by
    build/mac/strip_save_dsym won't give source file and line number
    information in valgrind.

    This function will run dsymutil if the .dSYM bundle is missing or if
    it looks like a fake_dsym.  A non-fake dsym that already exists is assumed
    to be up-to-date.
    """
    test_command = self._args[0]
    dsym_bundle = self._args[0] + '.dSYM'
    dsym_file = os.path.join(dsym_bundle, 'Contents', 'Resources', 'DWARF',
                             os.path.basename(test_command))
    dsym_info_plist = os.path.join(dsym_bundle, 'Contents', 'Info.plist')

    needs_dsymutil = True
    saved_test_command = None

    if os.path.exists(dsym_file) and os.path.exists(dsym_info_plist):
      
      dsym_info_plist_contents = open(dsym_info_plist).read()

      if not re.search('^\s*<key>fake_dsym</key>$', dsym_info_plist_contents,
                       re.MULTILINE):
        
        
        needs_dsymutil = False
      else:
        
        
        
        
        
        saved_test_command = test_command + '.stripped'
        os.rename(test_command, saved_test_command)
        shutil.copyfile(dsym_file, test_command)
        shutil.copymode(saved_test_command, test_command)

    if needs_dsymutil:
      if self._options.generate_dsym:
        
        shutil.rmtree(dsym_bundle, True)

        dsymutil_command = ['dsymutil', test_command]

        
        
        
        
        common.RunSubprocess(dsymutil_command)

        if saved_test_command:
          os.rename(saved_test_command, test_command)
      else:
        logging.info("No real .dSYM for test_command.  Line numbers will "
                     "not be shown.  Either tell xcode to generate .dSYM "
                     "file, or use --generate_dsym option to this tool.")

  def ToolCommand(self):
    """Get the valgrind command to run."""
    
    tool_name = self.ToolName()

    
    if self.SelfContained():
      proc = ["valgrind-%s.sh" % tool_name]
    else:
      proc = ["valgrind", "--tool=%s" % tool_name]

    proc += ["--num-callers=%i" % int(self._options.num_callers)]

    if self._options.trace_children:
      proc += ["--trace-children=yes"]

    proc += self.ToolSpecificFlags()
    proc += self._tool_flags

    suppression_count = 0
    for suppression_file in self._options.suppressions:
      if os.path.exists(suppression_file):
        suppression_count += 1
        proc += ["--suppressions=%s" % suppression_file]

    if not suppression_count:
      logging.warning("WARNING: NOT USING SUPPRESSIONS!")

    logfilename = self.log_dir + ("/%s." % tool_name) + "%p"
    if self.UseXML():
      proc += ["--xml=yes", "--xml-file=" + logfilename]
    else:
      proc += ["--log-file=" + logfilename]

    

    
    if self._options.indirect_webkit_layout:
      
      wrapper = self.CreateBrowserWrapper(proc)
      proc = self._args
      proc.append("--wrapper")
      proc.append(wrapper)
      return proc

    if self._options.indirect:
      wrapper = self.CreateBrowserWrapper(proc)
      os.putenv("BROWSER_WRAPPER", wrapper)
      logging.info('export BROWSER_WRAPPER=' + wrapper)
      proc = []
    proc += self._args
    return proc

  def ToolSpecificFlags(self):
    raise NotImplementedError, "This method should be implemented " \
                               "in the tool-specific subclass"

  def CreateBrowserWrapper(self, proc):
    """The program being run invokes Python or something else that can't stand
    to be valgrinded, and also invokes the Chrome browser. In this case, use a
    magic wrapper to only valgrind the Chrome browser. Build the wrapper here.
    Returns the path to the wrapper. It's up to the caller to use the wrapper
    appropriately.
    """
    command = " ".join(proc)
    
    
    command = command.replace("%p", "$$.%p")

    (fd, indirect_fname) = tempfile.mkstemp(dir=self.log_dir,
                                            prefix="browser_wrapper.",
                                            text=True)
    f = os.fdopen(fd, "w")
    f.write('#!/bin/bash\n'
            'echo "Started Valgrind wrapper for this test, PID=$$"\n')

    
    
    f.write('DIR=`dirname $0`\n'
            'FOUND_TESTNAME=0\n'
            'TESTNAME_FILE=$DIR/testcase.$$.name\n'
            'for arg in $@; do\n'
            '  # TODO(timurrrr): this doesn\'t handle "--test-name Test.Name"\n'
            '  if [[ "$arg" =~ --test-name=(.*) ]]; then\n'
            '    echo ${BASH_REMATCH[1]} >$TESTNAME_FILE\n'
            '    FOUND_TESTNAME=1\n'
            '  fi\n'
            'done\n\n')

    f.write('if [ "$FOUND_TESTNAME" = "1" ]; then\n'
            '    %s "$@"\n'
            'else\n' % command)
    
    f.write('    %s "$@" | tee $DIR/test.$$.stdout\n'
            '    EXITCODE=$PIPESTATUS\n'  
            '    head -n 1 $DIR/test.$$.stdout |\n'
            '      grep URL |\n'
            '      sed "s/^.*third_party\/WebKit\/LayoutTests\///" '
                       '>$TESTNAME_FILE\n'
            '    exit $EXITCODE\n'
            'fi\n' % command)

    f.close()
    os.chmod(indirect_fname, stat.S_IRUSR|stat.S_IXUSR)
    return indirect_fname

  def CreateAnalyzer(self):
    raise NotImplementedError, "This method should be implemented " \
                               "in the tool-specific subclass"

  def GetAnalyzeResults(self, check_sanity=False):
    
    filenames = glob.glob(self.log_dir + "/" + self.ToolName() + ".*")

    
    
    
    ppids = set([int(f.split(".")[-2]) \
                for f in filenames if re.search("\.[0-9]+\.[0-9]+$", f)])

    analyzer = self.CreateAnalyzer()
    if len(ppids) == 0:
      
      return analyzer.Report(filenames, None, check_sanity)

    ret = 0
    for ppid in ppids:
      testcase_name = None
      try:
        f = open(self.log_dir + ("/testcase.%d.name" % ppid))
        testcase_name = f.read().strip()
        f.close()
      except IOError:
        pass
      print "====================================================="
      print " Below is the report for valgrind wrapper PID=%d." % ppid
      if testcase_name:
        print " It was used while running the `%s` test." % testcase_name
      else:
        print " You can find the corresponding test"
        print " by searching the above log for 'PID=%d'" % ppid
      sys.stdout.flush()

      ppid_filenames = [f for f in filenames \
                        if re.search("\.%d\.[0-9]+$" % ppid, f)]
      
      assert check_sanity == False
      ret |= analyzer.Report(ppid_filenames, testcase_name)
      print "====================================================="
      sys.stdout.flush()

    if ret != 0:
      print ""
      print "The Valgrind reports are grouped by test names."
      print "Each test has its PID printed in the log when the test was run"
      print "and at the beginning of its Valgrind report."
      print "Hint: you can search for the reports by Ctrl+F -> `=#`"
      sys.stdout.flush()

    return ret



class Memcheck(ValgrindTool):
  """Memcheck
  Dynamic memory error detector for Linux & Mac

  http://valgrind.org/info/tools.html#memcheck
  """

  def __init__(self):
    super(Memcheck, self).__init__()
    self.RegisterOptionParserHook(Memcheck.ExtendOptionParser)

  def ToolName(self):
    return "memcheck"

  def ExtendOptionParser(self, parser):
    parser.add_option("--leak-check", "--leak_check", type="string",
                      default="yes",  
                      help="perform leak checking at the end of the run")
    parser.add_option("", "--show_all_leaks", action="store_true",
                      default=False,
                      help="also show less blatant leaks")
    parser.add_option("", "--track_origins", action="store_true",
                      default=False,
                      help="Show whence uninitialized bytes came. 30% slower.")

  def ToolSpecificFlags(self):
    ret = ["--gen-suppressions=all", "--demangle=no"]
    ret += ["--leak-check=%s" % self._options.leak_check]

    if self._options.show_all_leaks:
      ret += ["--show-reachable=yes"]
    else:
      ret += ["--show-possibly-lost=no"]

    if self._options.track_origins:
      ret += ["--track-origins=yes"]

    
    
    if common.IsMac():
      ret += ["--smc-check=all"]

    return ret

  def CreateAnalyzer(self):
    use_gdb = common.IsMac()
    return memcheck_analyze.MemcheckAnalyzer(self._source_dir,
                                            self._options.show_all_leaks,
                                            use_gdb=use_gdb)

  def Analyze(self, check_sanity=False):
    ret = self.GetAnalyzeResults(check_sanity)

    if ret != 0:
      logging.info("Please see http://dev.chromium.org/developers/how-tos/"
                   "using-valgrind for the info on Memcheck/Valgrind")
    return ret


class PinTool(BaseTool):
  """Abstract class for running PIN tools.

  Always subclass this and implement ToolSpecificFlags() and
  ExtendOptionParser() for tool-specific stuff.
  """
  def PrepareForTest(self):
    pass

  def ToolSpecificFlags(self):
    raise NotImplementedError, "This method should be implemented " \
                               "in the tool-specific subclass"

  def ToolCommand(self):
    """Get the PIN command to run."""

    
    pin_cmd = os.getenv("PIN_COMMAND")
    if not pin_cmd:
      raise RuntimeError, "Please set PIN_COMMAND environment variable " \
                          "with the path to pin.exe"
    proc = pin_cmd.split(" ")

    proc += self.ToolSpecificFlags()

    

    
    
    proc += ["--"]

    proc += self._args
    return proc


class ThreadSanitizerBase(object):
  """ThreadSanitizer
  Dynamic data race detector for Linux, Mac and Windows.

  http://code.google.com/p/data-race-test/wiki/ThreadSanitizer

  Since TSan works on both Valgrind (Linux, Mac) and PIN (Windows), we need
  to have multiple inheritance
  """

  INFO_MESSAGE="Please see http://dev.chromium.org/developers/how-tos/" \
               "using-valgrind/threadsanitizer for the info on " \
               "ThreadSanitizer"

  def __init__(self):
    super(ThreadSanitizerBase, self).__init__()
    self.RegisterOptionParserHook(ThreadSanitizerBase.ExtendOptionParser)

  def ToolName(self):
    return "tsan"

  def UseXML(self):
    return False

  def SelfContained(self):
    return True

  def ExtendOptionParser(self, parser):
    parser.add_option("", "--hybrid", default="no",
                      dest="hybrid",
                      help="Finds more data races, may give false positive "
                      "reports unless the code is annotated")
    parser.add_option("", "--announce-threads", default="yes",
                      dest="announce_threads",
                      help="Show the the stack traces of thread creation")
    parser.add_option("", "--free-is-write", default="no",
                      dest="free_is_write",
                      help="Treat free()/operator delete as memory write. "
                      "This helps finding more data races, but (currently) "
                      "this may give false positive reports on std::string "
                      "internals, see http://code.google.com/p/data-race-test"
                      "/issues/detail?id=40")

  def EvalBoolFlag(self, flag_value):
    if (flag_value in ["1", "true", "yes"]):
      return True
    elif (flag_value in ["0", "false", "no"]):
      return False
    raise RuntimeError, "Can't parse flag value (%s)" % flag_value

  def ToolSpecificFlags(self):
    ret = []

    ignore_files = ["ignores.txt"]
    for platform_suffix in common.PlatformNames():
      ignore_files.append("ignores_%s.txt" % platform_suffix)
    for ignore_file in ignore_files:
      fullname =  os.path.join(self._source_dir,
          "tools", "valgrind", "tsan", ignore_file)
      if os.path.exists(fullname):
        ret += ["--ignore=%s" % fullname]

    
    ret += ["--file-prefix-to-cut=%s/" % self._source_dir]

    
    ret += ["--file-prefix-to-cut=build/src/"]

    
    ret += ["--file-prefix-to-cut=scripts/tsan/tsan/"]

    ret += ["--gen-suppressions=true"]

    if self.EvalBoolFlag(self._options.hybrid):
      ret += ["--hybrid=yes"] 

    if self.EvalBoolFlag(self._options.announce_threads):
      ret += ["--announce-threads"]

    if self.EvalBoolFlag(self._options.free_is_write):
      ret += ["--free-is-write=yes"]
    else:
      ret += ["--free-is-write=no"]


    
    if platform_suffix == 'mac':
      ret += ["--show-pc=yes"]
    ret += ["--show-pid=no"]

    boring_callers = common.BoringCallers(mangled=False, use_re_wildcards=False)
    
    for bc in boring_callers:
      ret += ["--cut_stack_below=%s" % bc]

    return ret


class ThreadSanitizerPosix(ThreadSanitizerBase, ValgrindTool):
  def ToolSpecificFlags(self):
    proc = ThreadSanitizerBase.ToolSpecificFlags(self)
    
    
    proc += ["-v"]
    return proc

  def CreateAnalyzer(self):
    use_gdb = common.IsMac()
    return tsan_analyze.TsanAnalyzer(self._source_dir, use_gdb)

  def Analyze(self, check_sanity=False):
    ret = self.GetAnalyzeResults(check_sanity)

    if ret != 0:
      logging.info(self.INFO_MESSAGE)
    return ret


class ThreadSanitizerWindows(ThreadSanitizerBase, PinTool):

  def __init__(self):
    super(ThreadSanitizerWindows, self).__init__()
    self.RegisterOptionParserHook(ThreadSanitizerWindows.ExtendOptionParser)

  def ExtendOptionParser(self, parser):
    parser.add_option("", "--suppressions", default=[],
                      action="append",
                      help="path to TSan suppression file")


  def ToolSpecificFlags(self):
    proc = ThreadSanitizerBase.ToolSpecificFlags(self)
    
    
    suppression_count = 0
    for suppression_file in self._options.suppressions:
      if os.path.exists(suppression_file):
        suppression_count += 1
        proc += ["--suppressions=%s" % suppression_file]

    if not suppression_count:
      logging.warning("WARNING: NOT USING SUPPRESSIONS!")

    logfilename = self.log_dir + "/tsan.%p"
    proc += ["--log-file=" + logfilename]

    
    

    return proc

  def Analyze(self, check_sanity=False):
    filenames = glob.glob(self.log_dir + "/tsan.*")
    analyzer = tsan_analyze.TsanAnalyzer(self._source_dir)
    ret = analyzer.Report(filenames, None, check_sanity)
    if ret != 0:
      logging.info(self.INFO_MESSAGE)
    return ret


class DrMemory(BaseTool):
  """Dr.Memory
  Dynamic memory error detector for Windows.

  http://dev.chromium.org/developers/how-tos/using-drmemory
  It is not very mature at the moment, some things might not work properly.
  """

  def __init__(self, handle_uninits_and_leaks):
    super(DrMemory, self).__init__()
    self.handle_uninits_and_leaks = handle_uninits_and_leaks
    self.RegisterOptionParserHook(DrMemory.ExtendOptionParser)

  def ToolName(self):
    return "drmemory"

  def ExtendOptionParser(self, parser):
    parser.add_option("", "--suppressions", default=[],
                      action="append",
                      help="path to a drmemory suppression file")
    parser.add_option("", "--follow_python", action="store_true",
                      default=False, dest="follow_python",
                      help="Monitor python child processes.  If off, neither "
                      "python children nor any children of python children "
                      "will be monitored.")
    parser.add_option("", "--indirect", action="store_true",
                      default=False,
                      help="set BROWSER_WRAPPER rather than "
                           "running Dr. Memory directly on the harness")
    parser.add_option("", "--use_debug", action="store_true",
                      default=False, dest="use_debug",
                      help="Run Dr. Memory debug build")
    parser.add_option("", "--trace_children", action="store_true",
                            default=True,
                            help="TODO: default value differs from Valgrind")

  def ToolCommand(self):
    """Get the tool command to run."""
    
    
    
    add_env = {
      "CHROME_ALLOCATOR" : "WINHEAP",
      "JSIMD_FORCEMMX"   : "1",  
    }
    for k,v in add_env.iteritems():
      logging.info("export %s=%s", k, v)
      os.putenv(k, v)

    drmem_cmd = os.getenv("DRMEMORY_COMMAND")
    if not drmem_cmd:
      raise RuntimeError, "Please set DRMEMORY_COMMAND environment variable " \
                          "with the path to drmemory.exe"
    proc = drmem_cmd.split(" ")

    
    
    
    
    
    
    
    drconfig_cmd = [ proc[0].replace("drmemory.exe", "drconfig.exe") ]
    drconfig_cmd += ["-quiet"] 
    run_drconfig = True
    if self._options.follow_python:
      logging.info("Following python children")
      
      query_cmd = drconfig_cmd + ["-isreg", "python.exe"]
      query_proc = subprocess.Popen(query_cmd, stdout=subprocess.PIPE,
                                    shell=True)
      (query_out, query_err) = query_proc.communicate()
      if re.search("exe not registered", query_out):
        run_drconfig = False 
      else:
        drconfig_cmd += ["-unreg", "python.exe"]
    else:
      logging.info("Excluding python children")
      drconfig_cmd += ["-reg", "python.exe", "-norun"]
    if run_drconfig:
      drconfig_retcode = common.RunSubprocess(drconfig_cmd, self._timeout)
      if drconfig_retcode:
        logging.error("Configuring whether to follow python children failed " \
                      "with %d.", drconfig_retcode)
        raise RuntimeError, "Configuring python children failed "

    suppression_count = 0
    supp_files = self._options.suppressions
    if self.handle_uninits_and_leaks:
      supp_files += [s.replace(".txt", "_full.txt") for s in supp_files]
    for suppression_file in supp_files:
      if os.path.exists(suppression_file):
        suppression_count += 1
        proc += ["-suppress", common.NormalizeWindowsPath(suppression_file)]

    if not suppression_count:
      logging.warning("WARNING: NOT USING SUPPRESSIONS!")

    
    

    
    

    if self._options.use_debug:
      proc += ["-debug"]

    proc += ["-logdir", common.NormalizeWindowsPath(self.log_dir)]

    if self._options.build_dir:
      
      
      
      symcache_dir = os.path.join(self._options.build_dir, "drmemory.symcache")
      if not os.path.exists(symcache_dir):
        try:
          os.mkdir(symcache_dir)
        except OSError:
          logging.warning("Can't create symcache dir?")
      if os.path.exists(symcache_dir):
        proc += ["-symcache_dir", common.NormalizeWindowsPath(symcache_dir)]

    
    
    proc += ["-batch", "-no_summary"]

    
    
    

    proc += ["-callstack_max_frames", "40"]

    
    proc += ["-callstack_srcfile_prefix",
             "build\\src,chromium\\src,crt_build\\self_x86"]
    proc += ["-callstack_modname_hide",
             "*.exe,chrome.dll"]

    boring_callers = common.BoringCallers(mangled=False, use_re_wildcards=False)
    
    proc += ["-callstack_truncate_below", ",".join(boring_callers)]

    if not self.handle_uninits_and_leaks:
      proc += ["-no_check_uninitialized", "-no_count_leaks"]

    proc += self._tool_flags

    
    proc += ["--"]

    if self._options.indirect:
      
      wrapper_path = os.path.join(self._source_dir,
                                  "tools", "valgrind", "browser_wrapper_win.py")
      self.CreateBrowserWrapper(" ".join(["python", wrapper_path] + proc))
      proc = []

    
    self._args[0] = common.NormalizeWindowsPath(self._args[0])
    proc += self._args
    return proc

  def CreateBrowserWrapper(self, command):
    os.putenv("BROWSER_WRAPPER", command)

  def Analyze(self, check_sanity=False):
    
    
    
    
    analyzer = drmemory_analyze.DrMemoryAnalyzer()

    ret = 0
    if not self._options.indirect:
      filenames = glob.glob(self.log_dir + "/*/results.txt")

      ret = analyzer.Report(filenames, None, check_sanity)
    else:
      testcases = glob.glob(self.log_dir + "/testcase.*.logs")
      
      
      
      
      ppids = set([f.split(".")[-2] for f in testcases])

      for ppid in ppids:
        testcase_name = None
        try:
          f = open("%s/testcase.%s.name" % (self.log_dir, ppid))
          testcase_name = f.read().strip()
          f.close()
        except IOError:
          pass
        print "====================================================="
        print " Below is the report for drmemory wrapper PID=%s." % ppid
        if testcase_name:
          print " It was used while running the `%s` test." % testcase_name
        else:
          
          print " You can find the corresponding test"
          print " by searching the above log for 'PID=%s'" % ppid
        sys.stdout.flush()
        ppid_filenames = glob.glob("%s/testcase.%s.logs/*/results.txt" %
                                   (self.log_dir, ppid))
        ret |= analyzer.Report(ppid_filenames, testcase_name, False)
        print "====================================================="
        sys.stdout.flush()

    logging.info("Please see http://dev.chromium.org/developers/how-tos/"
                 "using-drmemory for the info on Dr. Memory")
    return ret




class ThreadSanitizerRV1Analyzer(tsan_analyze.TsanAnalyzer):
  """ TsanAnalyzer that saves race reports to a file. """

  TMP_FILE = "rvlog.tmp"

  def __init__(self, source_dir, use_gdb):
    super(ThreadSanitizerRV1Analyzer, self).__init__(source_dir, use_gdb)
    self.out = open(self.TMP_FILE, "w")

  def Report(self, files, testcase, check_sanity=False):
    reports = self.GetReports(files)
    for report in reports:
      print >>self.out, report
    if len(reports) > 0:
      logging.info("RaceVerifier pass 1 of 2, found %i reports" % len(reports))
      return -1
    return 0

  def CloseOutputFile(self):
    self.out.close()


class ThreadSanitizerRV1Mixin(object):
  """RaceVerifier first pass.

  Runs ThreadSanitizer as usual, but hides race reports and collects them in a
  temporary file"""

  def __init__(self):
    super(ThreadSanitizerRV1Mixin, self).__init__()
    self.RegisterOptionParserHook(ThreadSanitizerRV1Mixin.ExtendOptionParser)

  def ExtendOptionParser(self, parser):
    parser.set_defaults(hybrid="yes")

  def CreateAnalyzer(self):
    use_gdb = common.IsMac()
    self.analyzer = ThreadSanitizerRV1Analyzer(self._source_dir, use_gdb)
    return self.analyzer

  def Cleanup(self):
    super(ThreadSanitizerRV1Mixin, self).Cleanup()
    self.analyzer.CloseOutputFile()


class ThreadSanitizerRV2Mixin(object):
  """RaceVerifier second pass."""

  def __init__(self):
    super(ThreadSanitizerRV2Mixin, self).__init__()
    self.RegisterOptionParserHook(ThreadSanitizerRV2Mixin.ExtendOptionParser)

  def ExtendOptionParser(self, parser):
    parser.add_option("", "--race-verifier-sleep-ms",
                            dest="race_verifier_sleep_ms", default=10,
                            help="duration of RaceVerifier delays")

  def ToolSpecificFlags(self):
    proc = super(ThreadSanitizerRV2Mixin, self).ToolSpecificFlags()
    proc += ['--race-verifier=%s' % ThreadSanitizerRV1Analyzer.TMP_FILE,
             '--race-verifier-sleep-ms=%d' %
             int(self._options.race_verifier_sleep_ms)]
    return proc

  def Cleanup(self):
    super(ThreadSanitizerRV2Mixin, self).Cleanup()
    os.unlink(ThreadSanitizerRV1Analyzer.TMP_FILE)


class ThreadSanitizerRV1Posix(ThreadSanitizerRV1Mixin, ThreadSanitizerPosix):
  pass


class ThreadSanitizerRV2Posix(ThreadSanitizerRV2Mixin, ThreadSanitizerPosix):
  pass


class ThreadSanitizerRV1Windows(ThreadSanitizerRV1Mixin,
                                ThreadSanitizerWindows):
  pass


class ThreadSanitizerRV2Windows(ThreadSanitizerRV2Mixin,
                                ThreadSanitizerWindows):
  pass


class RaceVerifier(object):
  """Runs tests under RaceVerifier/Valgrind."""

  MORE_INFO_URL = "http://code.google.com/p/data-race-test/wiki/RaceVerifier"

  def RV1Factory(self):
    if common.IsWindows():
      return ThreadSanitizerRV1Windows()
    else:
      return ThreadSanitizerRV1Posix()

  def RV2Factory(self):
    if common.IsWindows():
      return ThreadSanitizerRV2Windows()
    else:
      return ThreadSanitizerRV2Posix()

  def ToolName(self):
    return "tsan"

  def Main(self, args, check_sanity):
    logging.info("Running a TSan + RaceVerifier test. For more information, " +
                 "see " + self.MORE_INFO_URL)
    cmd1 = self.RV1Factory()
    ret = cmd1.Main(args, check_sanity)
    
    if ret == -1:
      logging.info("Starting pass 2 of 2. Running the same binary in " +
                   "RaceVerifier mode to confirm possible race reports.")
      logging.info("For more information, see " + self.MORE_INFO_URL)
      cmd2 = self.RV2Factory()
      ret = cmd2.Main(args, check_sanity)
    else:
      logging.info("No reports, skipping RaceVerifier second pass")
    logging.info("Please see " + self.MORE_INFO_URL + " for more information " +
                 "on RaceVerifier")
    return ret

  def Run(self, args, module):
   return self.Main(args, False)


class EmbeddedTool(BaseTool):
  """Abstract class for tools embedded directly into the test binary.
  """
  

  def ToolCommand(self):
    
    return self._args


class Asan(EmbeddedTool):
  """AddressSanitizer, a memory error detector.

  More information at
  http://dev.chromium.org/developers/testing/addresssanitizer
  """
  def __init__(self):
    super(Asan, self).__init__()
    self._timeout = 1200
    if common.IsMac():
      self._env["DYLD_NO_PIE"] = "1"


  def ToolName(self):
    return "asan"

  def ToolCommand(self):
    
    
    procs = [os.path.join(self._source_dir, "tools", "valgrind",
                              "asan", "asan_wrapper.sh")]
    procs.extend(self._args)
    return procs

  def Analyze(sels, unused_check_sanity):
    return 0


class TsanGcc(EmbeddedTool):
  """ThreadSanitizer with compile-time instrumentation done using GCC.

  More information at
  code.google.com/p/data-race-test/wiki/GccInstrumentation
  """
  def __init__(self):
    super(TsanGcc, self).__init__()
    self.RegisterOptionParserHook(TsanGcc.ExtendOptionParser)

  def ExtendOptionParser(self, parser):
    parser.add_option("", "--suppressions", default=[],
                      action="append",
                      help="path to TSan suppression file")

  def Setup(self, args):
    if not super(TsanGcc, self).Setup(args):
      return False
    ld_library_paths = []
    for tail in "lib32", "lib64":
      ld_library_paths.append(
          os.path.join(self._source_dir, "third_party",
                       "compiler-tsan", "gcc-current", tail))
    
    self._env["LD_LIBRARY_PATH"] = ":".join(ld_library_paths)

    
    env_options = ["--error-exitcode=1"]
    
    suppression_count = 0
    for suppression_file in self._options.suppressions:
      if os.path.exists(suppression_file):
        suppression_count += 1
        env_options += ["--suppressions=%s" % suppression_file]
    if not suppression_count:
      logging.warning("WARNING: NOT USING SUPPRESSIONS!")

    self._env["TSAN_ARGS"] = " ".join(env_options)
    return True

  def ToolName(self):
    return "tsan"

  def Analyze(self, unused_check_sanity):
    
    
    
    return 0


class ToolFactory:
  def Create(self, tool_name):
    if tool_name == "memcheck":
      return Memcheck()
    if tool_name == "tsan":
      if common.IsWindows():
        return ThreadSanitizerWindows()
      else:
        return ThreadSanitizerPosix()
    if tool_name == "drmemory" or tool_name == "drmemory_light":
      
      
      
      return DrMemory(False)
    if tool_name == "drmemory_full":
      return DrMemory(True)
    if tool_name == "tsan_rv":
      return RaceVerifier()
    if tool_name == "tsan_gcc":
      return TsanGcc()
    if tool_name == "asan":
      return Asan()
    try:
      platform_name = common.PlatformNames()[0]
    except common.NotImplementedError:
      platform_name = sys.platform + "(Unknown)"
    raise RuntimeError, "Unknown tool (tool=%s, platform=%s)" % (tool_name,
                                                                 platform_name)

def CreateTool(tool):
  return ToolFactory().Create(tool)
