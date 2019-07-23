

import sys
import xpcom
from xpcom import components
import nsdom



def write( msg, *args):
    tb = document.getElementById("output_box")
    tb.value = tb.value + (msg % args) + "\n"



class EventListener:
    _com_interfaces_ = components.interfaces.nsIDOMEventListener
    def __init__(self, handler, globs = None):
        try:
            self.co = handler.func_code
        except AttributeError:
            self.co = compile(handler, "inline script", "exec")
        self.globals = globs or globals()

    def handleEvent(self, event):
        exec self.co in self.globals


timer_id = None
timer_count = 0

def on_timer(max):
    global timer_count, timer_id
    assert timer_id is not None, "We must have a timer id!"
    change_image() 
    timer_count += 1
    if timer_count >= max:
        write("Stopping the image timer (but clicking it will still change it)")
        window.clearTimeout(timer_id)
        time_id = None



def do_load():
    input = document.getElementById("output_box")
    
    input.value = ""
    write("This is the Python on XUL demo using\nPython %s", sys.version)

    
    button = document.getElementById("but_dialog")
    
    
    button.addEventListener('click', 'write("hello from the click event for the dialog button")', False)

    
    
    button = document.getElementById("some-button")
    button.custom_value = "Python"
    
    def check_expando(event):
        write("The custom value is %s", event.target.custom_value)
        if event.target.custom_value != "Python":
            write("but it is wrong!!!")

    button.addEventListener('click', check_expando, False)

    
    global timer_id
    assert timer_id is None, "Already have a timer - event fired twice??"
    timer_id = window.setInterval(on_timer, 2000, 10)


window.addEventListener('load', do_load, False)

window.addEventListener('load', "dump('hello from string event handler')", False)

window.addEventListener('load', EventListener('dump("hello from an object event handler")'), False)


def on_but_dialog_click():
    write("Button clicked from %s", window.location.href)
    w = window.open("chrome://pyxultest/content/dialog.xul", "myDialog", "chrome")

def do_textbox_keypress(event):
    if event.keyCode==13:
        val = event.target.value
        if val:
            write('You wrote: %s', val)
        else:
            write("You wrote nothing!")
        event.target.value = ""


def change_image():
    image = document.getElementById("image-python");
    import random
    num = random.randrange(64) + 1
    url = "http://www.python.org/pics/PyBanner%03d.gif" % num
    image.src = url

def run_tests():
    
    tests = """
                test_error_eventlistener_function
                test_error_eventlistener_function_noargs
                test_error_eventlistener_object
                test_error_eventlistener_string
                test_error_explicit
                test_error_explicit_no_cancel
                test_error_explicit_string
                test_interval_func
                test_timeout_func
                test_timeout_func_lateness
                test_timeout_string
                test_wrong_event_args
                """.split()
    keep_open = document.getElementById("keep_tests_open").getAttribute("checked")
    for test in tests:
        write("Running test %s", test)
        if keep_open:
            args = (test, "-k")
        else:
            args = (test,)
        window.openDialog("chrome://pyxultest/content/pytester.xul", "testDialog", "modal", *args)
    if keep_open:
        write("Ran all the tests - the windows told you if the tests worked")
    else:
        write("Ran all the tests - if no window stayed open, it worked!")
