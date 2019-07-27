





"""multil10n.py

"""

import os
import sys


sys.path.insert(1, os.path.dirname(sys.path[0]))

from mozharness.mozilla.l10n.multi_locale_build import MultiLocaleBuild

if __name__ == '__main__':
    multi_locale_build = MultiLocaleBuild()
    multi_locale_build.run_and_exit()
