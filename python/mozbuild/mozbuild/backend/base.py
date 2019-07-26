



from __future__ import unicode_literals

from abc import (
    ABCMeta,
    abstractmethod,
)

import os
import sys

from mach.mixin.logging import LoggingMixin

from ..frontend.data import SandboxDerived
from .configenvironment import ConfigEnvironment


class BuildBackend(LoggingMixin):
    """Abstract base class for build backends.

    A build backend is merely a consumer of the build configuration (the output
    of the frontend processing). It does something with said data. What exactly
    is the discretion of the specific implementation.
    """

    __metaclass__ = ABCMeta

    def __init__(self, environment):
        assert isinstance(environment, ConfigEnvironment)

        self.populate_logger()

        self.environment = environment

        self._environments = {}
        self._environments[environment.topobjdir] = environment

        self._init()

    def _init():
        """Hook point for child classes to perform actions during __init__.

        This exists so child classes don't need to implement __init__.
        """

    def get_environment(self, obj):
        """Obtain the ConfigEnvironment for a specific object.

        This is used to support external source directories which operate in
        their own topobjdir and have their own ConfigEnvironment.

        This is somewhat hacky and should be considered for rewrite if external
        project integration is rewritten.
        """
        environment = self._environments.get(obj.topobjdir, None)
        if not environment:
            config_status = os.path.join(obj.topobjdir, 'config.status')

            environment = ConfigEnvironment.from_config_status(config_status)
            self._environments[obj.topobjdir] = environment

        return environment

    def consume(self, objs):
        """Consume a stream of TreeMetadata instances.

        This is the main method of the interface. This is what takes the
        frontend output and does something with it.

        Child classes are not expected to implement this method. Instead, the
        base class consumes objects and calls methods (possibly) implemented by
        child classes.
        """

        for obj in objs:
            self.consume_object(obj)

        
        age_file = os.path.join(self.environment.topobjdir,
            'backend.%s.built' % self.__class__.__name__)
        with open(age_file, 'a'):
            os.utime(age_file, None)

        self.consume_finished()

    @abstractmethod
    def consume_object(self, obj):
        """Consumes an individual TreeMetadata instance.

        This is the main method used by child classes to react to build
        metadata.
        """

    def consume_finished(self):
        """Called when consume() has completed handling all objects."""

