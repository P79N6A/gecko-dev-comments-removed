""" googleplay.py

    The way to get the API access is to
      1) login in in the Google play admin
      2) Settings
      3) API Access
      4) go in the Google Developers Console
      5) Create "New client ID"
         or download the p12 key (it should remain
         super private)
      6) Move the file in this directory with the name
         'key.p12' or use the --credentials option
"""

import httplib2

from apiclient.discovery import build
from oauth2client import client



class GooglePlayMixin(object):

    def connect_to_play(self):
        """ Connect to the google play interface
        """
        
        
        f = file(self.config["google_play_credentials_file"], 'rb')
        key = f.read()
        f.close()

        
        
        
        
        
        scope = 'https://www.googleapis.com/auth/androidpublisher'
        credentials = client.SignedJwtAssertionCredentials(
            self.config["service_account"],
            key,
            scope)
        http = httplib2.Http()
        http = credentials.authorize(http)

        service = build('androidpublisher', 'v2', http=http)

        return service
