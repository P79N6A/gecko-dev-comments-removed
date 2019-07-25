
#-*- coding: utf-8 -*-








"""Contains tweakable constants for quality dashboard utility scripts."""

__author__ = 'phoglund@webrtc.org (Patrik HÃ¶glund)'



DASHBOARD_SERVER = 'webrtc-dashboard.appspot.com'
DASHBOARD_SERVER_HTTP = 'http://' + DASHBOARD_SERVER
CONSUMER_KEY = DASHBOARD_SERVER
CONSUMER_SECRET_FILE = 'consumer.secret'
ACCESS_TOKEN_FILE = 'access.token'


REQUEST_TOKEN_URL = DASHBOARD_SERVER_HTTP + '/_ah/OAuthGetRequestToken'
AUTHORIZE_TOKEN_URL = DASHBOARD_SERVER_HTTP + '/_ah/OAuthAuthorizeToken'
ACCESS_TOKEN_URL = DASHBOARD_SERVER_HTTP + '/_ah/OAuthGetAccessToken'


BUILD_MASTER_SERVER = 'webrtc-cb-linux-master.cbf.corp.google.com:8010'
BUILD_MASTER_TRANSPOSED_GRID_URL = '/tgrid'


BUILD_BOT_COVERAGE_WWW_DIRECTORY = '/var/www/coverage'


ADD_COVERAGE_DATA_URL = DASHBOARD_SERVER_HTTP + '/add_coverage_data'
ADD_BUILD_STATUS_DATA_URL = DASHBOARD_SERVER_HTTP + '/add_build_status_data'
