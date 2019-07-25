






































import logging
import mozinfo
import os
import select
import signal
import subprocess
import sys
import threading
import time
from Queue import Queue
from datetime import datetime, timedelta

__all__ = ['ProcessHandlerMixin', 'ProcessHandler']

if mozinfo.isWin:
    import ctypes, ctypes.wintypes, msvcrt
    from ctypes import sizeof, addressof, c_ulong, byref, POINTER, WinError
    import winprocess
    from qijo import JobObjectAssociateCompletionPortInformation, JOBOBJECT_ASSOCIATE_COMPLETION_PORT

class ProcessHandlerMixin(object):
    """Class which represents a process to be executed."""

    class Process(subprocess.Popen):
        """
        Represents our view of a subprocess.
        It adds a kill() method which allows it to be stopped explicitly.
        """

        MAX_IOCOMPLETION_PORT_NOTIFICATION_DELAY = 180
        MAX_PROCESS_KILL_DELAY = 30

        def __init__(self,
                     args,
                     bufsize=0,
                     executable=None,
                     stdin=None,
                     stdout=None,
                     stderr=None,
                     preexec_fn=None,
                     close_fds=False,
                     shell=False,
                     cwd=None,
                     env=None,
                     universal_newlines=False,
                     startupinfo=None,
                     creationflags=0,
                     ignore_children=False):

            
            self._ignore_children = ignore_children

            if not self._ignore_children and not mozinfo.isWin:
                
                
                
                
                def setpgidfn():
                    os.setpgid(0, 0)
                preexec_fn = setpgidfn

            try:
                subprocess.Popen.__init__(self, args, bufsize, executable,
                                          stdin, stdout, stderr,
                                          preexec_fn, close_fds,
                                          shell, cwd, env,
                                          universal_newlines, startupinfo, creationflags)
            except OSError, e:
                print >> sys.stderr, args
                raise

        def __del__(self, _maxint=sys.maxint):
            if mozinfo.isWin:
                if self._handle:
                    self._internal_poll(_deadstate=_maxint)
                if self._handle or self._job or self._io_port:
                    self._cleanup()
            else:
                subprocess.Popen.__del__(self)

        def kill(self):
            self.returncode = 0
            if mozinfo.isWin:
                if not self._ignore_children and self._handle and self._job:
                    winprocess.TerminateJobObject(self._job, winprocess.ERROR_CONTROL_C_EXIT)
                    self.returncode = winprocess.GetExitCodeProcess(self._handle)
                elif self._handle:
                    try:
                        winprocess.TerminateProcess(self._handle, winprocess.ERROR_CONTROL_C_EXIT)
                    except:
                        raise OSError("Could not terminate process")
                    finally:
                        self.returncode = winprocess.GetExitCodeProcess(self._handle)
                        self._cleanup()
                else:
                    pass
            else:
                if not self._ignore_children:
                    try:
                        os.killpg(self.pid, signal.SIGKILL)
                    except BaseException, e:
                        if getattr(e, "errno", None) != 3:
                            
                            print >> sys.stderr, "Could not kill process, could not find pid: %s" % self.pid
                    finally:
                        
                        if self.returncode is None:
                            self.returncode = subprocess.Popen._internal_poll(self)

                else:
                    os.kill(self.pid, signal.SIGKILL)
                    if self.returncode is None:
                        self.returncode = subprocess.Popen._internal_poll(self)

            self._cleanup()
            return self.returncode

        def wait(self):
            """ Popen.wait
                Called to wait for a running process to shut down and return
                its exit code
                Returns the main process's exit code
            """
            
            self.returncode = self._wait()
            self._cleanup()
            return self.returncode

        """ Private Members of Process class """

        if mozinfo.isWin:
            
            def _execute_child(self, args, executable, preexec_fn, close_fds,
                               cwd, env, universal_newlines, startupinfo,
                               creationflags, shell,
                               p2cread, p2cwrite,
                               c2pread, c2pwrite,
                               errread, errwrite):
                if not isinstance(args, basestring):
                    args = subprocess.list2cmdline(args)

                
                creationflags |= winprocess.CREATE_NEW_PROCESS_GROUP

                if startupinfo is None:
                    startupinfo = winprocess.STARTUPINFO()

                if None not in (p2cread, c2pwrite, errwrite):
                    startupinfo.dwFlags |= winprocess.STARTF_USESTDHANDLES
                    startupinfo.hStdInput = int(p2cread)
                    startupinfo.hStdOutput = int(c2pwrite)
                    startupinfo.hStdError = int(errwrite)
                if shell:
                    startupinfo.dwFlags |= winprocess.STARTF_USESHOWWINDOW
                    startupinfo.wShowWindow = winprocess.SW_HIDE
                    comspec = os.environ.get("COMSPEC", "cmd.exe")
                    args = comspec + " /c " + args

                
                canCreateJob = winprocess.CanCreateJobObject()

                
                if not canCreateJob and not self._ignore_children:
                    
                    
                    print >> sys.stderr, "ProcessManager UNABLE to use job objects to manage child processes"

                
                creationflags |= winprocess.CREATE_SUSPENDED
                creationflags |= winprocess.CREATE_UNICODE_ENVIRONMENT
                if canCreateJob:
                    creationflags |= winprocess.CREATE_BREAKAWAY_FROM_JOB
                else:
                    
                    
                    print "ProcessManager NOT managing child processes"

                
                hp, ht, pid, tid = winprocess.CreateProcess(
                    executable, args,
                    None, None, 
                    1, 
                    creationflags,
                    winprocess.EnvironmentBlock(env),
                    cwd, startupinfo)
                self._child_created = True
                self._handle = hp
                self._thread = ht
                self.pid = pid
                self.tid = tid

                if canCreateJob:
                    try:
                        
                        
                        
                        self._io_port = winprocess.CreateIoCompletionPort()
                        self._job = winprocess.CreateJobObject()

                        
                        joacp = JOBOBJECT_ASSOCIATE_COMPLETION_PORT(winprocess.COMPKEY_JOBOBJECT,
                                                                    self._io_port)
                        winprocess.SetInformationJobObject(self._job,
                                                          JobObjectAssociateCompletionPortInformation,
                                                          addressof(joacp),
                                                          sizeof(joacp)
                                                          )

                        
                        winprocess.AssignProcessToJobObject(self._job, int(hp))

                        
                        
                        self._process_events = Queue()

                        
                        self._procmgrthread = threading.Thread(target = self._procmgr)
                    except:
                        print >> sys.stderr, """Exception trying to use job objects;
falling back to not using job objects for managing child processes"""
                        
                        self._cleanup_job_io_port()
                else:
                    self._job = None

                winprocess.ResumeThread(int(ht))
                if self._procmgrthread:
                    self._procmgrthread.start()
                ht.Close()

                for i in (p2cread, c2pwrite, errwrite):
                    if i is not None:
                        i.Close()

            
            
            def _procmgr(self):
                if not (self._io_port) or not (self._job):
                    return

                try:
                    self._poll_iocompletion_port()
                except KeyboardInterrupt:
                    raise KeyboardInterrupt

            def _poll_iocompletion_port(self):
                
                self._spawned_procs = {}
                countdowntokill = 0

                while True:
                    msgid = c_ulong(0)
                    compkey = c_ulong(0)
                    pid = c_ulong(0)
                    portstatus = winprocess.GetQueuedCompletionStatus(self._io_port,
                                                                      byref(msgid),
                                                                      byref(compkey),
                                                                      byref(pid),
                                                                      5000)

                    
                    
                    if countdowntokill != 0:
                        diff = datetime.now() - countdowntokill
                        
                        
                        
                        
                        
                        if diff.seconds > self.MAX_IOCOMPLETION_PORT_NOTIFICATION_DELAY:
                            print >> sys.stderr, "Parent process exited without \
                                                  killing children, attempting to kill children"
                            self.kill()
                            self._process_events.put({self.pid: 'FINISHED'})

                    if not portstatus:
                        
                        errcode = winprocess.GetLastError()
                        if errcode == winprocess.ERROR_ABANDONED_WAIT_0:
                            
                            print >> sys.stderr, "IO Completion Port unexpectedly closed"
                            break
                        elif errcode == winprocess.WAIT_TIMEOUT:
                            
                            continue
                        else:
                            print >> sys.stderr, "Error Code %s trying to query IO Completion Port, exiting" % errcode
                            raise WinError(errcode)
                            break

                    if compkey.value == winprocess.COMPKEY_TERMINATE.value:
                        
                        break

                    
                    if compkey.value == winprocess.COMPKEY_JOBOBJECT.value:
                        if msgid.value == winprocess.JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO:
                            
                            
                            self._process_events.put({self.pid: 'FINISHED'})
                            break
                        elif msgid.value == winprocess.JOB_OBJECT_MSG_NEW_PROCESS:
                            
                            
                            
                            if pid.value != self.pid:
                                self._spawned_procs[pid.value] = 1
                        elif msgid.value == winprocess.JOB_OBJECT_MSG_EXIT_PROCESS:
                            
                            if pid.value == self.pid and len(self._spawned_procs) > 0:
                                
                                countdowntokill = datetime.now()
                            elif pid.value in self._spawned_procs:
                                
                                del(self._spawned_procs[pid.value])
                        elif msgid.value == winprocess.JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS:
                            
                            if pid.value == self.pid and len(self._spawned_procs) > 0:
                                
                                countdowntokill = datetime.now()
                            elif pid.value in self._spawned_procs:
                                
                                del self._spawned_procs[pid.value]
                        else:
                            
                            pass

            def _wait(self):

                
                if self._handle:
                    self.returncode = winprocess.GetExitCodeProcess(self._handle)
                else:
                    
                    return self.returncode

                if self._job and self._procmgrthread.is_alive():
                    
                    
                    
                    
                    
                    
                    try:
                        
                        
                        item = self._process_events.get(timeout=self.MAX_IOCOMPLETION_PORT_NOTIFICATION_DELAY +
                                                                self.MAX_PROCESS_KILL_DELAY)
                        if item[self.pid] == 'FINISHED':
                            self._process_events.task_done()
                    except:
                        raise OSError("IO Completion Port failed to signal process shutdown")
                    finally:
                        
                        self.returncode = winprocess.GetExitCodeProcess(self._handle)
                        self._cleanup()

                else:
                    
                    

                    
                    if self.returncode != winprocess.STILL_ACTIVE:
                        self._cleanup()
                        return self.returncode

                    rc = None
                    if self._handle:
                        rc = winprocess.WaitForSingleObject(self._handle, -1)

                    if rc == winprocess.WAIT_TIMEOUT:
                        
                        print "Timed out waiting for process to close, attempting TerminateProcess"
                        self.kill()
                    elif rc == winprocess.WAIT_OBJECT_0:
                        
                        print "Single process terminated successfully"
                        self.returncode = winprocess.GetExitCodeProcess(self._handle)
                    else:
                        
                        rc = winprocess.GetLastError()
                        if rc:
                            raise WinError(rc)

                    self._cleanup()
                    return self.returncode

            def _cleanup_job_io_port(self):
                """ Do the job and IO port cleanup separately because there are
                    cases where we want to clean these without killing _handle
                    (i.e. if we fail to create the job object in the first place)
                """
                if self._job and self._job != winprocess.INVALID_HANDLE_VALUE:
                    self._job.Close()
                    self._job = None
                else:
                    
                    
                    self._job = None

                if self._io_port and self._io_port != winprocess.INVALID_HANDLE_VALUE:
                    self._io_port.Close()
                    self._io_port = None
                else:
                    self._io_port = None

                if self._procmgrthread:
                    self._procmgrthread = None

            def _cleanup(self):
                self._cleanup_job_io_port()
                if self._thread and self._thread != winprocess.INVALID_HANDLE_VALUE:
                    self._thread.Close()
                    self._thread = None
                else:
                    self._thread = None

                if self._handle and self._handle != winprocess.INVALID_HANDLE_VALUE:
                    self._handle.Close()
                    self._handle = None
                else:
                    self._handle = None

        elif mozinfo.isMac or mozinfo.isUnix:

            def _wait(self):
                """ Haven't found any reason to differentiate between these platforms
                    so they all use the same wait callback.  If it is necessary to
                    craft different styles of wait, then a new _wait method
                    could be easily implemented.
                """

                if not self._ignore_children:
                    try:
                        
                        return os.waitpid(self.pid, 0)[1]
                    except OSError, e:
                        if getattr(e, "errno", None) != 10:
                            
                            
                            print >> sys.stderr, "Encountered error waiting for pid to close: %s" % e
                            raise
                        return 0

                else:
                    
                    subprocess.Popen.wait(self)
                    return self.returncode

            def _cleanup(self):
                pass

        else:
            
            print >> sys.stderr, "Unrecognized platform, process groups may not be managed properly"

            def _wait(self):
                self.returncode = subprocess.Popen.wait(self)
                return self.returncode

            def _cleanup(self):
                pass

    def __init__(self,
                 cmd,
                 args=None,
                 cwd=None,
                 env=os.environ.copy(),
                 ignore_children = False,
                 processOutputLine=(),
                 onTimeout=(),
                 onFinish=(),
                 **kwargs):
        """
        cmd = Command to run
        args = array of arguments (defaults to None)
        cwd = working directory for cmd (defaults to None)
        env = environment to use for the process (defaults to os.environ)
        ignore_children = when True, causes system to ignore child processes,
        defaults to False (which tracks child processes)
        processOutputLine = handlers to process the output line
        onTimeout = handlers for timeout event
        kwargs = keyword args to pass directly into Popen

        NOTE: Child processes will be tracked by default.  If for any reason
        we are unable to track child processes and ignore_children is set to False,
        then we will fall back to only tracking the root process.  The fallback
        will be logged.
        """
        self.cmd = cmd
        self.args = args
        self.cwd = cwd
        self.env = env
        self.didTimeout = False
        self._ignore_children = ignore_children
        self.keywordargs = kwargs

        
        self.processOutputLineHandlers = list(processOutputLine)
        self.onTimeoutHandlers = list(onTimeout)
        self.onFinishHandlers = list(onFinish)

        
        
        if not isinstance(self.cmd, list):
            self.cmd = [self.cmd]

        if self.args:
            self.cmd = self.cmd + self.args

    @property
    def timedOut(self):
        """True if the process has timed out."""
        return self.didTimeout

    @property
    def commandline(self):
        """the string value of the command line"""
        return subprocess.list2cmdline([self.cmd] + self.args)

    def run(self):
        """Starts the process.  waitForFinish must be called to allow the
           process to complete.
        """
        self.didTimeout = False
        self.startTime = datetime.now()
        self.proc = self.Process(self.cmd,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT,
                                 cwd=self.cwd,
                                 env=self.env,
                                 ignore_children = self._ignore_children,
                                 **self.keywordargs)

    def kill(self):
        """
          Kills the managed process and if you created the process with
          'ignore_children=False' (the default) then it will also
          also kill all child processes spawned by it.
          If you specified 'ignore_children=True' when creating the process,
          only the root process will be killed.

          Note that this does not manage any state, save any output etc,
          it immediately kills the process.
        """
        return self.proc.kill()

    def readWithTimeout(self, f, timeout):
        """
          Try to read a line of output from the file object |f|.
          |f| must be a  pipe, like the |stdout| member of a subprocess.Popen
          object created with stdout=PIPE. If no output
          is received within |timeout| seconds, return a blank line.
          Returns a tuple (line, did_timeout), where |did_timeout| is True
          if the read timed out, and False otherwise.

          Calls a private member because this is a different function based on
          the OS
        """
        return self._readWithTimeout(f, timeout)

    def processOutputLine(self, line):
        """Called for each line of output that a process sends to stdout/stderr.
        """
        for handler in self.processOutputLineHandlers:
            handler(line)

    def onTimeout(self):
        """Called when a process times out."""
        for handler in self.onTimeoutHandlers:
            handler()

    def onFinish(self):
        """Called when a process finishes without a timeout."""
        for handler in self.onFinishHandlers:
            handler()

    def waitForFinish(self, timeout=None, outputTimeout=None):
        """
        Handle process output until the process terminates or times out.

        If timeout is not None, the process will be allowed to continue for
        that number of seconds before being killed.

        If outputTimeout is not None, the process will be allowed to continue
        for that number of seconds without producing any output before
        being killed.
        """

        if not hasattr(self, 'proc'):
            self.run()

        self.didTimeout = False
        logsource = self.proc.stdout

        lineReadTimeout = None
        if timeout:
            lineReadTimeout = timeout - (datetime.now() - self.startTime).seconds
        elif outputTimeout:
            lineReadTimeout = outputTimeout

        (line, self.didTimeout) = self.readWithTimeout(logsource, lineReadTimeout)
        while line != "" and not self.didTimeout:
            self.processOutputLine(line.rstrip())
            if timeout:
                lineReadTimeout = timeout - (datetime.now() - self.startTime).seconds
            (line, self.didTimeout) = self.readWithTimeout(logsource, lineReadTimeout)


        if self.didTimeout:
            self.proc.kill()
            self.onTimeout()
        else:
            self.onFinish()

        status = self.proc.wait()
        return status


    

    if mozinfo.isWin:
        
        PeekNamedPipe = ctypes.windll.kernel32.PeekNamedPipe
        GetLastError = ctypes.windll.kernel32.GetLastError

        def _readWithTimeout(self, f, timeout):
            if timeout is None:
                
                return (f.readline(), False)
            x = msvcrt.get_osfhandle(f.fileno())
            l = ctypes.c_long()
            done = time.time() + timeout
            while time.time() < done:
                if self.PeekNamedPipe(x, None, 0, None, ctypes.byref(l), None) == 0:
                    err = self.GetLastError()
                    if err == 38 or err == 109: 
                        return ('', False)
                    else:
                        raise OSError("readWithTimeout got error: %d", err)
                if l.value > 0:
                    
                    
                    return (f.readline(), False)
                time.sleep(0.01)
            return ('', True)

    else:
        
        def _readWithTimeout(self, f, timeout):
            try:
                (r, w, e) = select.select([f], [], [], timeout)
            except:
                
                return ('', True)

            if len(r) == 0:
                return ('', True)
            return (f.readline(), False)





def print_output(line):
    print line

class StoreOutput(object):
    """accumulate stdout"""

    def __init__(self):
        self.output = []

    def __call__(self, line):
        self.output.append(line)

class LogOutput(object):
    """pass output to a file"""

    def __init__(self, filename):
        self.filename = filename
        self.file = None

    def __call__(self, line):
        if self.file is None:
            self.file = file(self.filename, 'a')
        self.file.write(line + '\n')
        self.file.flush()

    def __del__(self):
        if self.file is not None:
            self.file.close()



class ProcessHandler(ProcessHandlerMixin):

    def __init__(self, cmd, logfile=None, storeOutput=True, **kwargs):
        """
        If storeOutput=True, the output produced by the process will be saved
        as self.output.

        If logfile is not None, the output produced by the process will be
        appended to the given file.
        """

        kwargs.setdefault('processOutputLine', []).append(print_output)

        if logfile:
            logoutput = LogOutput(logfile)
            kwargs['processOutputLine'].append(logoutput)

        self.output = None
        if storeOutput:
            storeoutput = StoreOutput()
            self.output = storeoutput.output
            kwargs['processOutputLine'].append(storeoutput)

        ProcessHandlerMixin.__init__(self, cmd, **kwargs)
