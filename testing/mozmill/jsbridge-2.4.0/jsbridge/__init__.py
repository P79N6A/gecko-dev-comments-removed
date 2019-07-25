





































import socket
import os
import copy
import asyncore

from time import sleep
from network import Bridge, BackChannel, create_network
from jsobjects import JSObject

import mozrunner

settings_env = 'JSBRIDGE_SETTINGS_FILE'

parent = os.path.abspath(os.path.dirname(__file__))
extension_path = os.path.join(parent, 'extension')
xpi_path = os.path.join(parent, 'xpi')

window_string = "Components.classes['@mozilla.org/appshell/window-mediator;1'].getService(Components.interfaces.nsIWindowMediator).getMostRecentWindow('')"

wait_to_create_timeout = 60

def wait_and_create_network(host, port, timeout=wait_to_create_timeout):
    ttl = 0
    while ttl < timeout:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((host, port))
            s.close()
            break
        except socket.error:
            pass
        sleep(.25)
        ttl += .25
    if ttl == timeout:
        raise Exception("Sorry, cannot connect to jsbridge extension, port %s" % port)
    
    back_channel, bridge = create_network(host, port)
    sleep(.5)
    
    while back_channel.registered is False:
        back_channel.close()
        bridge.close()
        asyncore.socket_map = {}
        sleep(1)
        back_channel, bridge = create_network(host, port)
    
    return back_channel, bridge

class CLI(mozrunner.CLI):
    """Command line interface."""
    
    module = "jsbridge"

    parser_options = copy.copy(mozrunner.CLI.parser_options)
    parser_options[('-D', '--debug',)] = dict(dest="debug", 
                                             action="store_true",
                                             help="Install debugging addons.", 
                                             metavar="JSBRIDGE_DEBUG",
                                             default=False )
    parser_options[('-s', '--shell',)] = dict(dest="shell", 
                                             action="store_true",
                                             help="Start a Python shell",
                                             metavar="JSBRIDGE_SHELL",
                                             default=False )
    parser_options[('-u', '--usecode',)] = dict(dest="usecode", action="store_true",
                                               help="Use code module instead of iPython",
                                               default=False)
    parser_options[('-P', '--port')] = dict(dest="port", default="24242",
                                            help="TCP port to run jsbridge on.")

    debug_addons = []
    if os.path.exists(xpi_path):
        debug_addons = [os.path.join(xpi_path, x) for x
                        in os.listdir(xpi_path)]

    def get_profile(self, *args, **kwargs):
        if self.options.debug:
            kwargs.setdefault('preferences', 
                              {}).update({'extensions.checkCompatibility':False})
        profile = mozrunner.CLI.get_profile(self, *args, **kwargs)
        profile.install_addon(extension_path)
        if self.options.debug:
            for addon in self.debug_addons:
                profile.install_addon(addon)
        return profile
        
    def get_runner(self, *args, **kwargs):
        runner = super(CLI, self).get_runner(*args, **kwargs)
        if self.options.debug:
            runner.cmdargs.append('-jsconsole')
        runner.cmdargs += ['-jsbridge', self.options.port]
        return runner
        
    def run(self):
        runner = self.create_runner()
        runner.start()
        self.start_jsbridge_network()
        if self.options.shell:
            self.start_shell(runner)
        else:
            try:
                runner.wait()
            except KeyboardInterrupt:
                runner.stop()
                
        runner.profile.cleanup()
    
    def start_shell(self, runner):
        try:
            import IPython
        except:
            IPython = None
        jsobj = JSObject(self.bridge, window_string)
        
        if IPython is None or self.options.usecode:
            import code
            code.interact(local={"jsobj":jsobj, 
                                 "getBrowserWindow":lambda : getBrowserWindow(self.bridge),
                                 "back_channel":self.back_channel,
                                 })
        else:
            from IPython.Shell import IPShellEmbed
            ipshell = IPShellEmbed([])
            ipshell(local_ns={"jsobj":jsobj, 
                              "getBrowserWindow":lambda : getBrowserWindow(self.bridge),
                              "back_channel":self.back_channel,
                              })
        runner.stop()
        
    def start_jsbridge_network(self, timeout=10):
        port = int(self.options.port)
        host = '127.0.0.1'
        self.back_channel, self.bridge = wait_and_create_network(host, port, timeout)

def cli():
    CLI().run()

def getBrowserWindow(bridge):
    return JSObject(bridge, "Components.classes['@mozilla.org/appshell/window-mediator;1'].getService(Components.interfaces.nsIWindowMediator).getMostRecentWindow('')")
    






