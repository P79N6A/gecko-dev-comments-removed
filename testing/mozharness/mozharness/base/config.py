





"""Generic config parsing and dumping, the way I remember it from scripts
gone by.

The config should be built from script-level defaults, overlaid by
config-file defaults, overlaid by command line options.

  (For buildbot-analogues that would be factory-level defaults,
   builder-level defaults, and build request/scheduler settings.)

The config should then be locked (set to read-only, to prevent runtime
alterations).  Afterwards we should dump the config to a file that is
uploaded with the build, and can be used to debug or replicate the build
at a later time.

TODO:

* check_required_settings or something -- run at init, assert that
  these settings are set.
"""

from copy import deepcopy
from optparse import OptionParser, Option, OptionGroup
import os
import sys
import urllib2
import socket
import time
try:
    import simplejson as json
except ImportError:
    import json

from mozharness.base.log import DEBUG, INFO, WARNING, ERROR, CRITICAL, FATAL



class ExtendedOptionParser(OptionParser):
    """OptionParser, but with ExtendOption as the option_class.
    """
    def __init__(self, **kwargs):
        kwargs['option_class'] = ExtendOption
        OptionParser.__init__(self, **kwargs)


class ExtendOption(Option):
    """from http://docs.python.org/library/optparse.html?highlight=optparse#adding-new-actions"""
    ACTIONS = Option.ACTIONS + ("extend",)
    STORE_ACTIONS = Option.STORE_ACTIONS + ("extend",)
    TYPED_ACTIONS = Option.TYPED_ACTIONS + ("extend",)
    ALWAYS_TYPED_ACTIONS = Option.ALWAYS_TYPED_ACTIONS + ("extend",)

    def take_action(self, action, dest, opt, value, values, parser):
        if action == "extend":
            lvalue = value.split(",")
            values.ensure_value(dest, []).extend(lvalue)
        else:
            Option.take_action(
                self, action, dest, opt, value, values, parser)


def make_immutable(item):
    if isinstance(item, list) or isinstance(item, tuple):
        result = LockedTuple(item)
    elif isinstance(item, dict):
        result = ReadOnlyDict(item)
        result.lock()
    else:
        result = item
    return result


class LockedTuple(tuple):
    def __new__(cls, items):
        return tuple.__new__(cls, (make_immutable(x) for x in items))
    def __deepcopy__(self, memo):
        return [deepcopy(elem, memo) for elem in self]



class ReadOnlyDict(dict):
    def __init__(self, dictionary):
        self._lock = False
        self.update(dictionary.copy())

    def _check_lock(self):
        assert not self._lock, "ReadOnlyDict is locked!"

    def lock(self):
        for (k, v) in self.items():
            self[k] = make_immutable(v)
        self._lock = True

    def __setitem__(self, *args):
        self._check_lock()
        return dict.__setitem__(self, *args)

    def __delitem__(self, *args):
        self._check_lock()
        return dict.__delitem__(self, *args)

    def clear(self, *args):
        self._check_lock()
        return dict.clear(self, *args)

    def pop(self, *args):
        self._check_lock()
        return dict.pop(self, *args)

    def popitem(self, *args):
        self._check_lock()
        return dict.popitem(self, *args)

    def setdefault(self, *args):
        self._check_lock()
        return dict.setdefault(self, *args)

    def update(self, *args):
        self._check_lock()
        dict.update(self, *args)

    def __deepcopy__(self, memo):
        cls = self.__class__
        result = cls.__new__(cls)
        memo[id(self)] = result
        for k, v in self.__dict__.items():
            setattr(result, k, deepcopy(v, memo))
        result._lock = False
        for k, v in self.items():
            result[k] = deepcopy(v, memo)
        return result


def parse_config_file(file_name, quiet=False, search_path=None,
                      config_dict_name="config"):
    """Read a config file and return a dictionary.
    """
    file_path = None
    if os.path.exists(file_name):
        file_path = file_name
    else:
        if not search_path:
            search_path = ['.', os.path.join(sys.path[0], '..', 'configs'),
                           os.path.join(sys.path[0], '..', '..', 'configs')]
        for path in search_path:
            if os.path.exists(os.path.join(path, file_name)):
                file_path = os.path.join(path, file_name)
                break
        else:
            raise IOError("Can't find %s in %s!" % (file_name, search_path))
    if file_name.endswith('.py'):
        global_dict = {}
        local_dict = {}
        execfile(file_path, global_dict, local_dict)
        config = local_dict[config_dict_name]
    elif file_name.endswith('.json'):
        fh = open(file_path)
        config = {}
        json_config = json.load(fh)
        config = dict(json_config)
        fh.close()
    else:
        raise RuntimeError("Unknown config file type %s!" % file_name)
    
    return config


def download_config_file(url, file_name):
    n = 0
    attempts = 5
    sleeptime = 60
    max_sleeptime = 5 * 60
    while True:
        if n >= attempts:
            print "Failed to download from url %s after %d attempts, quiting..." % (url, attempts)
            raise SystemError(-1)
        try:
            contents = urllib2.urlopen(url, timeout=30).read()
            break
        except urllib2.URLError, e:
            print "Error downloading from url %s: %s" % (url, str(e))
        except socket.timeout, e:
            print "Time out accessing %s: %s" % (url, str(e))
        except socket.error, e:
            print "Socket error when accessing %s: %s" % (url, str(e))
        print "Sleeping %d seconds before retrying" % sleeptime
        time.sleep(sleeptime)
        sleeptime = sleeptime * 2
        if sleeptime > max_sleeptime:
            sleeptime = max_sleeptime
        n += 1

    try:
        f = open(file_name, 'w')
        f.write(contents)
        f.close()
    except IOError, e:
        print "Error writing downloaded contents to file %s: %s" % (file_name, str(e))
        raise SystemError(-1)



class BaseConfig(object):
    """Basic config setting/getting.
    """
    def __init__(self, config=None, initial_config_file=None, config_options=None,
                 all_actions=None, default_actions=None,
                 volatile_config=None, option_args=None,
                 require_config_file=False,
                 append_env_variables_from_configs=False,
                 usage="usage: %prog [options]"):
        self._config = {}
        self.all_cfg_files_and_dicts = []
        self.actions = []
        self.config_lock = False
        self.require_config_file = require_config_file
        
        self.append_env_variables_from_configs = append_env_variables_from_configs

        if all_actions:
            self.all_actions = all_actions[:]
        else:
            self.all_actions = ['clobber', 'build']
        if default_actions:
            self.default_actions = default_actions[:]
        else:
            self.default_actions = self.all_actions[:]
        if volatile_config is None:
            self.volatile_config = {
                'actions': None,
                'add_actions': None,
                'no_actions': None,
            }
        else:
            self.volatile_config = deepcopy(volatile_config)

        if config:
            self.set_config(config)
        if initial_config_file:
            initial_config = parse_config_file(initial_config_file)
            self.all_cfg_files_and_dicts.append(
                (initial_config_file, initial_config)
            )
            self.set_config(initial_config)
            
            
            
            
            
            
            
            
            
            if option_args is None:
                option_args=['dummy_mozharness_script_with_no_command_line_options.py']
        if config_options is None:
            config_options = []
        self._create_config_parser(config_options, usage)
        
        self.parse_args(args=option_args)

    def get_read_only_config(self):
        return ReadOnlyDict(self._config)

    def _create_config_parser(self, config_options, usage):
        self.config_parser = ExtendedOptionParser(usage=usage)
        self.config_parser.add_option(
            "--work-dir", action="store", dest="work_dir",
            type="string", default="build",
            help="Specify the work_dir (subdir of base_work_dir)"
        )
        self.config_parser.add_option(
            "--base-work-dir", action="store", dest="base_work_dir",
            type="string", default=os.getcwd(),
            help="Specify the absolute path of the parent of the working directory"
        )
        self.config_parser.add_option(
            "-c", "--config-file", "--cfg", action="extend", dest="config_files",
            type="string", help="Specify a config file; can be repeated"
        )
        self.config_parser.add_option(
            "-C", "--opt-config-file", "--opt-cfg", action="extend",
            dest="opt_config_files", type="string", default=[],
            help="Specify an optional config file, like --config-file but with no "
                 "error if the file is missing; can be repeated"
        )
        self.config_parser.add_option(
            "--dump-config", action="store_true",
            dest="dump_config",
            help="List and dump the config generated from this run to "
                 "a JSON file."
        )
        self.config_parser.add_option(
            "--dump-config-hierarchy", action="store_true",
            dest="dump_config_hierarchy",
            help="Like --dump-config but will list and dump which config "
                 "files were used making up the config and specify their own "
                 "keys/values that were not overwritten by another cfg -- "
                 "held the highest hierarchy."
        )

        
        log_option_group = OptionGroup(self.config_parser, "Logging")
        log_option_group.add_option(
            "--log-level", action="store",
            type="choice", dest="log_level", default=INFO,
            choices=[DEBUG, INFO, WARNING, ERROR, CRITICAL, FATAL],
            help="Set log level (debug|info|warning|error|critical|fatal)"
        )
        log_option_group.add_option(
            "-q", "--quiet", action="store_false", dest="log_to_console",
            default=True, help="Don't log to the console"
        )
        log_option_group.add_option(
            "--append-to-log", action="store_true",
            dest="append_to_log", default=False,
            help="Append to the log"
        )
        log_option_group.add_option(
            "--multi-log", action="store_const", const="multi",
            dest="log_type", help="Log using MultiFileLogger"
        )
        log_option_group.add_option(
            "--simple-log", action="store_const", const="simple",
            dest="log_type", help="Log using SimpleFileLogger"
        )
        self.config_parser.add_option_group(log_option_group)

        
        action_option_group = OptionGroup(
            self.config_parser, "Actions",
            "Use these options to list or enable/disable actions."
        )
        action_option_group.add_option(
            "--list-actions", action="store_true",
            dest="list_actions",
            help="List all available actions, then exit"
        )
        action_option_group.add_option(
            "--add-action", action="extend",
            dest="add_actions", metavar="ACTIONS",
            help="Add action %s to the list of actions" % self.all_actions
        )
        action_option_group.add_option(
            "--no-action", action="extend",
            dest="no_actions", metavar="ACTIONS",
            help="Don't perform action"
        )
        for action in self.all_actions:
            action_option_group.add_option(
                "--%s" % action, action="append_const",
                dest="actions", const=action,
                help="Add %s to the limited list of actions" % action
            )
            action_option_group.add_option(
                "--no-%s" % action, action="append_const",
                dest="no_actions", const=action,
                help="Remove %s from the list of actions to perform" % action
            )
        self.config_parser.add_option_group(action_option_group)
        
        
        if config_options:
            for option in config_options:
                self.config_parser.add_option(*option[0], **option[1])

        
        config_options = self._config.get('config_options', None)
        if config_options:
            for option in config_options:
                self.config_parser.add_option(*option[0], **option[1])

    def set_config(self, config, overwrite=False):
        """This is probably doable some other way."""
        if self._config and not overwrite:
            self._config.update(config)
        else:
            self._config = config
        return self._config

    def get_actions(self):
        return self.actions

    def verify_actions(self, action_list, quiet=False):
        for action in action_list:
            if action not in self.all_actions:
                if not quiet:
                    print("Invalid action %s not in %s!" % (action,
                                                            self.all_actions))
                raise SystemExit(-1)
        return action_list

    def verify_actions_order(self, action_list):
        try:
            indexes = [ self.all_actions.index(elt) for elt in action_list ]
            sorted_indexes = sorted(indexes)
            for i in range(len(indexes)):
                if indexes[i] != sorted_indexes[i]:
                    print(("Action %s comes in different order in %s\n" +
                           "than in %s") % (action_list[i], action_list, self.all_actions))
                    raise SystemExit(-1)
        except ValueError as e:
            print("Invalid action found: " + str(e))
            raise SystemExit(-1)

    def list_actions(self):
        print "Actions available:"
        for a in self.all_actions:
            print "    " + ("*" if a in self.default_actions else " "), a
        raise SystemExit(0)

    def get_cfgs_from_files(self, all_config_files, options):
        """Returns the configuration derived from the list of configuration
        files.  The result is represented as a list of `(filename,
        config_dict)` tuples; they will be combined with keys in later
        dictionaries taking precedence over earlier.

        `all_config_files` is all files specified with `--config-file` and
        `--opt-config-file`; `options` is the argparse options object giving
        access to any other command-line options.

        This function is also responsible for downloading any configuration
        files specified by URL.  It uses ``parse_config_file`` in this module
        to parse individual files.

        This method can be overridden in a subclass to add extra logic to the
        way that self.config is made up.  See
        `mozharness.mozilla.building.buildbase.BuildingConfig` for an example.
        """
        all_cfg_files_and_dicts = []
        for cf in all_config_files:
            try:
                if '://' in cf:  
                    file_name = os.path.basename(cf)
                    file_path = os.path.join(os.getcwd(), file_name)
                    download_config_file(cf, file_path)
                    all_cfg_files_and_dicts.append(
                        (file_path, parse_config_file(file_path))
                    )
                else:
                    all_cfg_files_and_dicts.append((cf, parse_config_file(cf)))
            except Exception:
                if cf in options.opt_config_files:
                    print(
                        "WARNING: optional config file not found %s" % cf
                    )
                else:
                    raise
        return all_cfg_files_and_dicts

    def parse_args(self, args=None):
        """Parse command line arguments in a generic way.
        Return the parser object after adding the basic options, so
        child objects can manipulate it.
        """
        self.command_line = ' '.join(sys.argv)
        if args is None:
            args = sys.argv[1:]
        (options, args) = self.config_parser.parse_args(args)

        defaults = self.config_parser.defaults.copy()

        if not options.config_files:
            if self.require_config_file:
                if options.list_actions:
                    self.list_actions()
                print("Required config file not set! (use --config-file option)")
                raise SystemExit(-1)
        else:
            
            
            
            
            self.all_cfg_files_and_dicts.extend(self.get_cfgs_from_files(
                
                options.config_files + options.opt_config_files, options=options
            ))
            config = {}
            if self.append_env_variables_from_configs:
                
                
                for i, (c_file, c_dict) in enumerate(self.all_cfg_files_and_dicts):
                    for v in c_dict.keys():
                        if v == 'env' and v in config:
                            config[v].update(c_dict[v])
                        else:
                            config[v] = c_dict[v]
            else:
                for i, (c_file, c_dict) in enumerate(self.all_cfg_files_and_dicts):
                    config.update(c_dict)
            
            
            
            
            
            self.set_config(config)
        for key in defaults.keys():
            value = getattr(options, key)
            if value is None:
                continue
            
            if key in defaults and value == defaults[key] and key in self._config:
                continue
            self._config[key] = value

        
        
        
        for key in self.volatile_config.keys():
            if self._config.get(key) is not None:
                self.volatile_config[key] = self._config[key]
                del(self._config[key])

        """Actions.

        Seems a little complex, but the logic goes:

        First, if default_actions is specified in the config, set our
        default actions even if the script specifies other default actions.

        Without any other action-specific options, run with default actions.

        If we specify --ACTION or --only-ACTION once or multiple times,
        we want to override the default_actions list with the one(s) we list.

        Otherwise, if we specify --add-action ACTION, we want to add an
        action to the list.

        Finally, if we specify --no-ACTION, remove that from the list of
        actions to perform.
        """
        if self._config.get('default_actions'):
            default_actions = self.verify_actions(self._config['default_actions'])
            self.default_actions = default_actions
        self.verify_actions_order(self.default_actions)
        if options.list_actions:
            self.list_actions()
        self.actions = self.default_actions[:]
        if self.volatile_config['actions']:
            actions = self.verify_actions(self.volatile_config['actions'])
            self.actions = actions
        elif self.volatile_config['add_actions']:
            actions = self.verify_actions(self.volatile_config['add_actions'])
            self.actions.extend(actions)
        if self.volatile_config['no_actions']:
            actions = self.verify_actions(self.volatile_config['no_actions'])
            for action in actions:
                if action in self.actions:
                    self.actions.remove(action)

        
        self._config['volatile_config'] = self.volatile_config

        self.options = options
        self.args = args
        return (self.options, self.args)



if __name__ == '__main__':
    pass
