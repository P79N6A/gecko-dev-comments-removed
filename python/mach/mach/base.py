



from __future__ import unicode_literals


class ArgumentProvider(object):
    """Base class for classes wishing to provide CLI arguments to mach."""

    @staticmethod
    def populate_argparse(parser):
        raise Exception("populate_argparse not implemented.")
