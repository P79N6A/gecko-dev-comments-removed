



















import os

path = os.path.abspath(os.path.split(__file__)[0])
execfile(os.path.join(path, 'gyp_webrtc'))
