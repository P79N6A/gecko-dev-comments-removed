



from __future__ import unicode_literals

import json
import logging
import os
import shutil
import socket
import sys
import threading
import time
import urlparse
from Queue import Empty
from StringIO import StringIO
from abc import ABCMeta, abstractmethod
from collections import defaultdict, OrderedDict
from multiprocessing import Queue

from mozlog.structured import commandline, stdadapter

import manifestexpected
import manifestinclude
import products
import wptcommandline
import wpttest
from testrunner import ManagerGroup

here = os.path.split(__file__)[0]


"""Runner for web-platform-tests

The runner has several design goals:

* Tests should run with no modification from upstream.

* Tests should be regarded as "untrusted" so that errors, timeouts and even
  crashes in the tests can be handled without failing the entire test run.

* For performance tests can be run in multiple browsers in parallel.

The upstream repository has the facility for creating a test manifest in JSON
format. This manifest is used directly to determine which tests exist. Local
metadata files are used to store the expected test results.

"""

logger = None


def setup_logging(args, defaults):
    global logger
    logger = commandline.setup_logging("web-platform-tests", args, defaults)
    setup_stdlib_logger()

    for name in args.keys():
        if name.startswith("log_"):
            args.pop(name)

    return logger


def setup_stdlib_logger():
    logging.root.handlers = []
    logging.root = stdadapter.std_logging_adapter(logging.root)


def do_test_relative_imports(test_root):
    global serve, manifest

    sys.path.insert(0, os.path.join(test_root))
    sys.path.insert(0, os.path.join(test_root, "tools", "scripts"))
    failed = None
    try:
        import serve
    except ImportError:
        failed = "serve"
    try:
        import manifest
    except ImportError:
        failed = "manifest"

    if failed:
        logger.critical(
            "Failed to import %s. Ensure that tests path %s contains web-platform-tests" %
            (failed, test_root))
        sys.exit(1)

class TestEnvironmentError(Exception):
    pass


class TestEnvironment(object):
    def __init__(self, test_path, options):
        """Context manager that owns the test environment i.e. the http and
        websockets servers"""
        self.test_path = test_path
        self.server = None
        self.config = None
        self.external_config = None
        self.test_server_port = options.pop("test_server_port", True)
        self.options = options if options is not None else {}
        self.required_files = options.pop("required_files", [])
        self.files_to_restore = []

    def __enter__(self):
        self.copy_required_files()

        self.config = self.load_config()
        serve.set_computed_defaults(self.config)

        serve.logger = serve.default_logger("info")
        self.external_config, self.servers = serve.start(self.config)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.restore_files()
        for scheme, servers in self.servers.iteritems():
            for port, server in servers:
                server.kill()

    def load_config(self):
        default_config_path = os.path.join(self.test_path, "config.default.json")
        local_config_path = os.path.join(here, "config.json")

        with open(default_config_path) as f:
            default_config = json.load(f)

        with open(local_config_path) as f:
            data = f.read()
            local_config = json.loads(data % self.options)

        local_config["external_host"] = self.options.get("external_host", None)

        return serve.merge_json(default_config, local_config)

    def copy_required_files(self):
        logger.info("Placing required files in server environment.")
        for source, destination, copy_if_exists in self.required_files:
            source_path = os.path.join(here, source)
            dest_path = os.path.join(self.test_path, destination, os.path.split(source)[1])
            dest_exists = os.path.exists(dest_path)
            if not dest_exists or copy_if_exists:
                if dest_exists:
                    backup_path = dest_path + ".orig"
                    logger.info("Backing up %s to %s" % (dest_path, backup_path))
                    self.files_to_restore.append(dest_path)
                    shutil.copy2(dest_path, backup_path)
                logger.info("Copying %s to %s" % (source_path, dest_path))
                shutil.copy2(source_path, dest_path)

    def ensure_started(self):
        
        time.sleep(2)
        for scheme, servers in self.servers.iteritems():
            for port, server in servers:
                if self.test_server_port:
                    s = socket.socket()
                    try:
                        s.connect((self.config["host"], port))
                    except socket.error:
                        raise EnvironmentError(
                            "%s server on port %d failed to start" % (scheme, port))
                    finally:
                        s.close()

                if not server.is_alive():
                    raise EnvironmentError("%s server on port %d failed to start" % (scheme, port))

    def restore_files(self):
        for path in self.files_to_restore:
            os.unlink(path)
            if os.path.exists(path + ".orig"):
                os.rename(path + ".orig", path)


class TestChunker(object):
    def __init__(self, total_chunks, chunk_number):
        self.total_chunks = total_chunks
        self.chunk_number = chunk_number
        assert self.chunk_number <= self.total_chunks

    def __call__(self, manifest):
        raise NotImplementedError


class Unchunked(TestChunker):
    def __init__(self, *args, **kwargs):
        TestChunker.__init__(self, *args, **kwargs)
        assert self.total_chunks == 1

    def __call__(self, manifest):
        for item in manifest:
            yield item


class HashChunker(TestChunker):
    def __call__(self):
        chunk_index = self.chunk_number - 1
        for test_path, tests in manifest:
            if hash(test_path) % self.total_chunks == chunk_index:
                yield test_path, tests


class EqualTimeChunker(TestChunker):
    """Chunker that uses the test timeout as a proxy for the running time of the test"""

    def _get_chunk(self, manifest_items):
        
        
        
        

        total_time = 0
        by_dir = OrderedDict()

        class PathData(object):
            def __init__(self, path):
                self.path = path
                self.time = 0
                self.tests = []

        class Chunk(object):
            def __init__(self):
                self.paths = []
                self.tests = []
                self.time = 0

            def append(self, path_data):
                self.paths.append(path_data.path)
                self.tests.extend(path_data.tests)
                self.time += path_data.time

        class ChunkList(object):
            def __init__(self, total_time, n_chunks):
                self.total_time = total_time
                self.n_chunks = n_chunks

                self.remaining_chunks = n_chunks

                self.chunks = []

                self.update_time_per_chunk()

            def __iter__(self):
                for item in self.chunks:
                    yield item

            def __getitem__(self, i):
                return self.chunks[i]

            def sort_chunks(self):
                self.chunks = sorted(self.chunks, key=lambda x:x.paths[0])

            def get_tests(self, chunk_number):
                return self[chunk_number - 1].tests

            def append(self, chunk):
                if len(self.chunks) == self.n_chunks:
                    raise ValueError("Tried to create more than %n chunks" % self.n_chunks)
                self.chunks.append(chunk)
                self.remaining_chunks -= 1

            @property
            def current_chunk(self):
                if self.chunks:
                    return self.chunks[-1]

            def update_time_per_chunk(self):
                self.time_per_chunk = (self.total_time - sum(item.time for item in self)) / self.remaining_chunks

            def create(self):
                rv = Chunk()
                self.append(rv)
                return rv

            def add_path(self, path_data):
                sum_time = self.current_chunk.time + path_data.time
                if sum_time > self.time_per_chunk and self.remaining_chunks > 0:
                    overshoot = sum_time - self.time_per_chunk
                    undershoot = self.time_per_chunk - self.current_chunk.time
                    if overshoot < undershoot:
                        self.create()
                        self.current_chunk.append(path_data)
                    else:
                        self.current_chunk.append(path_data)
                        self.create()
                else:
                    self.current_chunk.append(path_data)

        for i, (test_path, tests) in enumerate(manifest_items):
            test_dir = tuple(os.path.split(test_path)[0].split(os.path.sep)[:3])

            if not test_dir in by_dir:
                by_dir[test_dir] = PathData(test_dir)

            data = by_dir[test_dir]
            time = sum(wpttest.DEFAULT_TIMEOUT if test.timeout !=
                       "long" else wpttest.LONG_TIMEOUT for test in tests)
            data.time += time
            data.tests.append((test_path, tests))

            total_time += time

        chunk_list = ChunkList(total_time, self.total_chunks)

        if len(by_dir) < self.total_chunks:
            raise ValueError("Tried to split into %i chunks, but only %i subdirectories included" % (
                self.total_chunks, len(by_dir)))

        
        
        while True:
            to_remove = []
            for path_data in by_dir.itervalues():
                if path_data.time > chunk_list.time_per_chunk:
                    to_remove.append(path_data)
            if to_remove:
                for path_data in to_remove:
                    chunk = chunk_list.create()
                    chunk.append(path_data)
                    del by_dir[path_data.path]
                chunk_list.update_time_per_chunk()
            else:
                break

        chunk = chunk_list.create()
        for path_data in by_dir.itervalues():
            chunk_list.add_path(path_data)

        assert len(chunk_list.chunks) == self.total_chunks, len(chunk_list.chunks)
        assert sum(item.time for item in chunk_list) == chunk_list.total_time

        chunk_list.sort_chunks()

        return chunk_list.get_tests(self.chunk_number)

    def __call__(self, manifest_iter):
        manifest = list(manifest_iter)
        tests = self._get_chunk(manifest)
        for item in tests:
            yield item


class TestFilter(object):
    def __init__(self, include=None, exclude=None, manifest_path=None):
        if manifest_path is not None and include is None:
            self.manifest = manifestinclude.get_manifest(manifest_path)
        else:
            self.manifest = manifestinclude.IncludeManifest.create()

        if include is not None:
            self.manifest.set("skip", "true")
            for item in include:
                self.manifest.add_include(item)

        if exclude is not None:
            for item in exclude:
                self.manifest.add_exclude(item)

    def __call__(self, manifest_iter):
        for test_path, tests in manifest_iter:
            include_tests = set()
            for test in tests:
                if self.manifest.include(test):
                    include_tests.add(test)

            if include_tests:
                yield test_path, include_tests


class TestLoader(object):
    def __init__(self, tests_root, metadata_root, test_types, test_filter, run_info,
                 chunk_type="none", total_chunks=1, chunk_number=1):
        self.tests_root = tests_root
        self.metadata_root = metadata_root
        self.test_types = test_types
        self.test_filter = test_filter
        self.run_info = run_info
        self.manifest_path = os.path.join(self.metadata_root, "MANIFEST.json")
        self.manifest = self.load_manifest()
        self.tests = None
        self.disabled_tests = None

        self.chunk_type = chunk_type
        self.total_chunks = total_chunks
        self.chunk_number = chunk_number

        self.chunker = {"none": Unchunked,
                        "hash": HashChunker,
                        "equal_time": EqualTimeChunker}[chunk_type](total_chunks,
                                                                    chunk_number)

        self._test_ids = None
        self._load_tests()

    @property
    def test_ids(self):
        if self._test_ids is None:
            self._test_ids = []
            for test_dict in [self.disabled_tests, self.tests]:
                for test_type in self.test_types:
                    self._test_ids += [item.id for item in test_dict[test_type]]
        return self._test_ids

    def create_manifest(self):
        logger.info("Creating test manifest")
        manifest.setup_git(self.tests_root)
        manifest_file = manifest.Manifest(None)
        manifest.update(manifest_file)
        manifest.write(manifest_file, self.manifest_path)

    def load_manifest(self):
        if not os.path.exists(self.manifest_path):
            self.create_manifest()
        return manifest.load(self.manifest_path)

    def get_test(self, manifest_test, expected_file):
        if expected_file is not None:
            expected = expected_file.get_test(manifest_test.id)
        else:
            expected = None
        return wpttest.from_manifest(manifest_test, expected)

    def load_expected_manifest(self, test_path):
        return manifestexpected.get_manifest(self.metadata_root, test_path, self.run_info)

    def iter_tests(self):
        manifest_items = self.test_filter(self.manifest.itertypes(*self.test_types))

        if self.chunker is not None:
            manifest_items = self.chunker(manifest_items)

        for test_path, tests in manifest_items:
            expected_file = self.load_expected_manifest(test_path)
            for manifest_test in tests:
                test = self.get_test(manifest_test, expected_file)
                test_type = manifest_test.item_type
                yield test_path, test_type, test

    def _load_tests(self):
        """Read in the tests from the manifest file and add them to a queue"""
        tests = {"enabled":defaultdict(list),
                 "disabled":defaultdict(list)}

        for test_path, test_type, test in self.iter_tests():
            key = "enabled" if not test.disabled() else "disabled"
            tests[key][test_type].append(test)

        self.tests = tests["enabled"]
        self.disabled_tests = tests["disabled"]

    def groups(self, test_types, chunk_type="none", total_chunks=1, chunk_number=1):
        groups = set()

        for test_type in test_types:
            for test in self.tests[test_type]:
                group = test.url.split("/")[1]
                groups.add(group)

        return groups


class TestSource(object):
    __metaclass__ = ABCMeta

    @abstractmethod
    def queue_tests(self, test_queue):
        pass

    @abstractmethod
    def requeue_test(self, test):
        pass

    def __enter__(self):
        return self

    def __exit__(self, *args, **kwargs):
        pass


class SingleTestSource(TestSource):
    def __init__(self, test_queue):
        self.test_queue = test_queue

    @classmethod
    def queue_tests(cls, test_queue, test_type, tests):
        for test in tests[test_type]:
            test_queue.put(test)

    def get_queue(self):
        if self.test_queue.empty():
            return None
        return self.test_queue

    def requeue_test(self, test):
        self.test_queue.put(test)

class PathGroupedSource(TestSource):
    def __init__(self, test_queue):
        self.test_queue = test_queue
        self.current_queue = None

    @classmethod
    def queue_tests(cls, test_queue, test_type, tests, depth=None):
        if depth is True:
            depth = None

        prev_path = None
        group = None

        for test in tests[test_type]:
            path = urlparse.urlsplit(test.url).path.split("/")[1:-1][:depth]
            if path != prev_path:
                group = []
                test_queue.put(group)
                prev_path = path

            group.append(test)

    def get_queue(self):
        if not self.current_queue or self.current_queue.empty():
            try:
                data = self.test_queue.get(block=True, timeout=1)
                self.current_queue = Queue()
                for item in data:
                    self.current_queue.put(item)
            except Empty:
                return None

        return self.current_queue

    def requeue_test(self, test):
        self.current_queue.put(test)

    def __exit__(self, *args, **kwargs):
        if self.current_queue:
            self.current_queue.close()

class LogThread(threading.Thread):
    def __init__(self, queue, logger, level):
        self.queue = queue
        self.log_func = getattr(logger, level)
        threading.Thread.__init__(self, name="Thread-Log")
        self.daemon = True

    def run(self):
        while True:
            try:
                msg = self.queue.get()
            except (EOFError, IOError):
                break
            if msg is None:
                break
            else:
                self.log_func(msg)


class LoggingWrapper(StringIO):
    """Wrapper for file like objects to redirect output to logger
    instead"""

    def __init__(self, queue, prefix=None):
        self.queue = queue
        self.prefix = prefix

    def write(self, data):
        if isinstance(data, str):
            data = data.decode("utf8")

        if data.endswith("\n"):
            data = data[:-1]
        if data.endswith("\r"):
            data = data[:-1]
        if not data:
            return
        if self.prefix is not None:
            data = "%s: %s" % (self.prefix, data)
        self.queue.put(data)

    def flush(self):
        pass


def list_test_groups(tests_root, metadata_root, test_types, product, **kwargs):
    do_test_relative_imports(tests_root)

    run_info = wpttest.get_run_info(metadata_root, product, debug=False)
    test_filter = TestFilter(include=kwargs["include"], exclude=kwargs["exclude"],
                             manifest_path=kwargs["include_manifest"])
    test_loader = TestLoader(tests_root, metadata_root, test_types, test_filter, run_info)

    for item in sorted(test_loader.groups()):
        print item


def list_disabled(tests_root, metadata_root, test_types, product, **kwargs):
    do_test_relative_imports(tests_root)

    rv = []
    run_info = wpttest.get_run_info(metadata_root, product, debug=False)
    test_loader = TestLoader(tests_root, metadata_root, test_types, TestFilter(), run_info)

    for test_type, tests in test_loader.disabled_tests.iteritems():
        for test in tests:
            rv.append({"test": test.id, "reason": test.disabled()})
    print json.dumps(rv, indent=2)


def run_tests(config, tests_root, metadata_root, product, **kwargs):
    logging_queue = None
    logging_thread = None
    original_stdio = (sys.stdout, sys.stderr)
    test_queues = None

    try:
        if not kwargs["no_capture_stdio"]:
            logging_queue = Queue()
            logging_thread = LogThread(logging_queue, logger, "info")
            sys.stdout = LoggingWrapper(logging_queue, prefix="STDOUT")
            sys.stderr = LoggingWrapper(logging_queue, prefix="STDERR")
            logging_thread.start()

        do_test_relative_imports(tests_root)

        run_info = wpttest.get_run_info(metadata_root, product, debug=False)

        (check_args,
         browser_cls, get_browser_kwargs,
         executor_classes, get_executor_kwargs,
         env_options) = products.load_product(config, product)

        check_args(**kwargs)

        browser_kwargs = get_browser_kwargs(**kwargs)

        unexpected_total = 0

        if "test_loader" in kwargs:
            test_loader = kwargs["test_loader"]
        else:
            test_filter = TestFilter(include=kwargs["include"],
                                     exclude=kwargs["exclude"],
                                     manifest_path=kwargs["include_manifest"])
            test_loader = TestLoader(tests_root,
                                     metadata_root,
                                     kwargs["test_types"],
                                     test_filter,
                                     run_info,
                                     kwargs["chunk_type"],
                                     kwargs["total_chunks"],
                                     kwargs["this_chunk"])

        if kwargs["run_by_dir"] is False:
            test_source_cls = SingleTestSource
            test_source_kwargs = {}
        else:
            
            test_source_cls = PathGroupedSource
            test_source_kwargs = {"depth": kwargs["run_by_dir"]}

        logger.info("Using %i client processes" % kwargs["processes"])

        with TestEnvironment(tests_root, env_options) as test_environment:
            try:
                test_environment.ensure_started()
            except TestEnvironmentError as e:
                logger.critical("Error starting test environment: %s" % e.message)
                raise

            base_server = "http://%s:%i" % (test_environment.external_config["host"],
                                            test_environment.external_config["ports"]["http"][0])
            repeat = kwargs["repeat"]
            for repeat_count in xrange(repeat):
                if repeat > 1:
                    logger.info("Repetition %i / %i" % (repeat_count + 1, repeat))


                unexpected_count = 0
                logger.suite_start(test_loader.test_ids, run_info)
                for test_type in kwargs["test_types"]:
                    logger.info("Running %s tests" % test_type)

                    for test in test_loader.disabled_tests[test_type]:
                        logger.test_start(test.id)
                        logger.test_end(test.id, status="SKIP")

                    executor_cls = executor_classes.get(test_type)
                    executor_kwargs = get_executor_kwargs(base_server,
                                                          **kwargs)

                    if executor_cls is None:
                        logger.error("Unsupported test type %s for product %s" %
                                     (test_type, product))
                        continue


                    with ManagerGroup("web-platform-tests",
                                      kwargs["processes"],
                                      test_source_cls,
                                      test_source_kwargs,
                                      browser_cls,
                                      browser_kwargs,
                                      executor_cls,
                                      executor_kwargs,
                                      kwargs["pause_on_unexpected"]) as manager_group:
                        try:
                            manager_group.run(test_type, test_loader.tests)
                        except KeyboardInterrupt:
                            logger.critical("Main thread got signal")
                            manager_group.stop()
                            raise
                    unexpected_count += manager_group.unexpected_count()

                unexpected_total += unexpected_count
                logger.info("Got %i unexpected results" % unexpected_count)
                logger.suite_end()
    except KeyboardInterrupt:
        if test_queues is not None:
            for queue in test_queues.itervalues():
                queue.cancel_join_thread()
    finally:
        if test_queues is not None:
            for queue in test_queues.itervalues():
                queue.close()
        sys.stdout, sys.stderr = original_stdio
        if not kwargs["no_capture_stdio"] and logging_queue is not None:
            logger.info("Closing logging queue")
            logging_queue.put(None)
            if logging_thread is not None:
                logging_thread.join(10)
            logging_queue.close()

    return manager_group.unexpected_count() == 0


def main():
    """Main entry point when calling from the command line"""
    kwargs = wptcommandline.parse_args()

    if kwargs["prefs_root"] is None:
        kwargs["prefs_root"] = os.path.abspath(os.path.join(here, "prefs"))

    setup_logging(kwargs, {"raw": sys.stdout})

    if kwargs["list_test_groups"]:
        list_test_groups(**kwargs)
    elif kwargs["list_disabled"]:
        list_disabled(**kwargs)
    else:
        return run_tests(**kwargs)
