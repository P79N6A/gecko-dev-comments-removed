"""
In asm code, out-of-bounds heap accesses cause segfaults, which the engine
handles internally. Make GDB ignore them.
"""

import gdb

SIGSEGV = 11


sigaction_buffers = {}

def on_stop(event):
    if isinstance(event, gdb.SignalEvent) and event.stop_signal == 'SIGSEGV':
        
        process = gdb.selected_inferior()
        buf = sigaction_buffers.get(process)
        if buf is None:
            buf = gdb.parse_and_eval("(struct sigaction *) malloc(sizeof(struct sigaction))")
            sigaction_buffers[process] = buf

        
        sigaction_fn = gdb.parse_and_eval('__sigaction')
        sigaction_fn(SIGSEGV, 0, buf)
        AsmJSFaultHandler = gdb.parse_and_eval("AsmJSFaultHandler")
        if buf['__sigaction_handler']['sa_handler'] == AsmJSFaultHandler:
            
            print "js/src/gdb/mozilla/asmjs.py: Allowing AsmJSFaultHandler to run."

            
            
            gdb.execute("continue")

def on_exited(event):
    if event.inferior in sigaction_buffers:
        del sigaction_buffers[event.inferior]

def install():
    gdb.events.stop.connect(on_stop)
    gdb.events.exited.connect(on_exited)
