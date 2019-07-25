





























"""Web Sockets utilities.
"""


import StringIO
import os
import re
import traceback


def get_stack_trace():
    """Get the current stack trace as string.

    This is needed to support Python 2.3.
    TODO: Remove this when we only support Python 2.4 and above.
          Use traceback.format_exc instead.
    """

    out = StringIO.StringIO()
    traceback.print_exc(file=out)
    return out.getvalue()


def prepend_message_to_exception(message, exc):
    """Prepend message to the exception."""

    exc.args = (message + str(exc),)
    return


def __translate_interp(interp, cygwin_path):
    """Translate interp program path for Win32 python to run cygwin program
    (e.g. perl).  Note that it doesn't support path that contains space,
    which is typically true for Unix, where #!-script is written.
    For Win32 python, cygwin_path is a directory of cygwin binaries.

    Args:
      interp: interp command line
      cygwin_path: directory name of cygwin binary, or None
    Returns:
      translated interp command line.
    """
    if not cygwin_path:
        return interp
    m = re.match("^[^ ]*/([^ ]+)( .*)?", interp)
    if m:
        cmd = os.path.join(cygwin_path, m.group(1))
        return cmd + m.group(2)
    return interp


def get_script_interp(script_path, cygwin_path=None):
    """Gets #!-interpreter command line from the script.

    It also fixes command path.  When Cygwin Python is used, e.g. in WebKit,
    it could run "/usr/bin/perl -wT hello.pl".
    When Win32 Python is used, e.g. in Chromium, it couldn't.  So, fix
    "/usr/bin/perl" to "<cygwin_path>\perl.exe".

    Args:
      script_path: pathname of the script
      cygwin_path: directory name of cygwin binary, or None
    Returns:
      #!-interpreter command line, or None if it is not #!-script.
    """
    fp = open(script_path)
    line = fp.readline()
    fp.close()
    m = re.match("^#!(.*)", line)
    if m:
        return __translate_interp(m.group(1), cygwin_path)
    return None

def wrap_popen3_for_win(cygwin_path):
    """Wrap popen3 to support #!-script on Windows.

    Args:
      cygwin_path:  path for cygwin binary if command path is needed to be
                    translated.  None if no translation required.
    """
    __orig_popen3 = os.popen3
    def __wrap_popen3(cmd, mode='t', bufsize=-1):
        cmdline = cmd.split(' ')
        interp = get_script_interp(cmdline[0], cygwin_path)
        if interp:
            cmd = interp + " " + cmd
        return __orig_popen3(cmd, mode, bufsize)
    os.popen3 = __wrap_popen3



