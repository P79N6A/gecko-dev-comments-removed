



from .errors import *
from .local import *
from .local import LocalRunner as Runner
from .remote import *

runners = local_runners
runners.update(remote_runners)
