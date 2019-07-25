











import socket

class Master(object):
  
  server_url = 'http://webrtc.googlecode.com'
  git_server_url =  'http://webrtc.googlecode.com/git'
  repo_root = '/svn'

  
  googlecode_url = 'http://%s.googlecode.com/svn'
  sourceforge_url = 'https://%(repo)s.svn.sourceforge.net/svnroot/%(repo)s'

  
  webkit_root_url = 'http://svn.webkit.org/repository/webkit'
  nacl_trunk_url = 'http://src.chromium.org/native_client/trunk'

  llvm_url = 'http://llvm.org/svn/llvm-project'

  
  repo_root_internal = None
  trunk_internal_url = None
  trunk_internal_url_src = None
  gears_url_internal = None
  o3d_url_internal = None
  nacl_trunk_url_internal = None
  nacl_url_internal = None

  syzygy_internal_url = None

  
  repo_root_internal = None
  trunk_internal_url = None
  trunk_internal_url_src = None

  
  master_domain = 'webrtc.org'
  permitted_domains = ('webrtc.org',)

  
  smtp = 'smtp'

  
  
  bot_password = None

  class _Base(object):
    
    
    master_host = 'webrtc-cb-linux-master.cbf.corp.google.com'
    
    
    
    is_production_host = socket.getfqdn() == master_host
    
    
    tree_closing_notification_recipients = []
    
    from_address = 'webrtc-cb-watchlist@google.com'
    
    
    
    
    
    
    code_review_site = 'https://webrtc-codereview.appspot.com/status_listener'

    
    

    
    master_port = 0
    
    slave_port = 0
    
    master_port_alt = 0
    
    try_job_port = 0

  

  class _ChromiumBase(_Base):
    
    
    
    
    
    
    
    
    base_app_url = 'http://localhost:8080'
    
    
    tree_status_url = base_app_url + '/status'
    
    store_revisions_url = base_app_url + '/revisions'
    
    last_good_url = 'http://webrtc-dashboard.appspot.com/lkgr'

  class WebRTC(_ChromiumBase):
    
    project_name = 'WebRTC'
    master_port = 9010
    slave_port = 9112
    master_port_alt = 9014

  class WebRTCMemory(_ChromiumBase):
    project_name = 'WebRTC Memory'
    master_port = 9014
    slave_port = 9119
    master_port_alt = 9047

  class WebRTCPerf(_ChromiumBase):
    project_name = 'WebRTC Perf'
    master_port = 9050
    slave_port = 9151
    master_port_alt = 9052

  class TryServer(_ChromiumBase):
    project_name = 'WebRTC Try Server'
    master_port = 9011
    slave_port = 9113
    master_port_alt = 9015
    try_job_port = 9018
    
    
    svn_url = None

class Archive(object):
  archive_host = 'localhost'
  
  
  exes_to_skip_entirely = []
  
  www_dir_base = "\\\\" + archive_host + "\\www\\"

  @staticmethod
  def Internal():
    pass


class Distributed(object):
  """Not much to describe."""
