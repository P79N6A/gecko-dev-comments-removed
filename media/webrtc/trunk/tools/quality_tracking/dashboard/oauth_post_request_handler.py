
#-*- coding: utf-8 -*-








"""Provides a OAuth request handler base class."""

__author__ = 'phoglund@webrtc.org (Patrik HÃ¶glund)'

from google.appengine.api import oauth
import logging
import webapp2


class UserNotAuthenticatedException(Exception):
  """Gets thrown if a user is not permitted to store data."""
  pass


class OAuthPostRequestHandler(webapp2.RequestHandler):
  """Works like a normal request handler but adds OAuth authentication.

     This handler will expect a proper OAuth request over POST. This abstract
     class deals with the authentication but leaves user-defined data handling
     to its subclasses. Subclasses should not implement the post() method but
     the _parse_and_store_data() method. Otherwise they may act like regular
     request handlers. Subclasses should NOT override the get() method.

     The handler will accept an OAuth request if it is correctly formed and
     the consumer is acting on behalf of an administrator for the dashboard.
  """

  def post(self):
    try:
      self._authenticate_user()
    except UserNotAuthenticatedException as exception:
      logging.warn('Failed to authenticate: %s.' % exception)
      self.response.set_status(403)
      return

    
    self._parse_and_store_data()

  def _parse_and_store_data(self):
    """Reads data from POST request and responds accordingly."""
    raise NotImplementedError('You must override this method!')

  def _authenticate_user(self):
    try:
      if oauth.is_current_user_admin():
        
        
        logging.info('Authenticated on behalf of user %s.' %
                     oauth.get_current_user())
        return
      else:
        raise UserNotAuthenticatedException('We are acting on behalf of '
                                            'user %s, but that user is not '
                                            'an administrator.' %
                                            oauth.get_current_user())
    except oauth.OAuthRequestError as exception:
      raise UserNotAuthenticatedException('Invalid OAuth request: %s' %
                                          exception.__class__.__name__)
