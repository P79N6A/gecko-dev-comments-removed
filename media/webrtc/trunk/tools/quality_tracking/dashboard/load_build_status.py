
#-*- coding: utf-8 -*-








"""Loads build status data for the dashboard."""

__author__ = 'phoglund@webrtc.org (Patrik HÃ¶glund)'

from google.appengine.ext import db


def _status_not_ok(status):
  return status not in ('OK', 'warnings')


def _all_ok(statuses):
  return filter(_status_not_ok, statuses) == []


def _get_first_entry(iterable):
  if not iterable:
    return None
  for item in iterable:
    return item


class BuildStatusLoader:
  """ Loads various build status data from the database."""

  def load_build_status_data(self):
    """Returns the latest conclusive build status for each bot.

       The statuses OK, failed and warnings are considered to be conclusive.

       The algorithm looks at the 100 most recent status entries, which should
       give data on roughly the last five revisions if the number of bots stay
       around 20 (The number 100 should be increased if the number of bots
       increases significantly). This should give us enough data to get a
       conclusive build status for all active bots.

       With this limit, the algorithm will adapt automatically if a bot is
       decommissioned - it will eventually disappear. The limit should not be
       too high either since we will perhaps remember offline bots too long,
       which could be confusing. The algorithm also adapts automatically to new
       bots - these show up immediately if they get a build status for a recent
       revision.

       Returns:
           A list of BuildStatusData entities with one entity per bot.
    """

    build_status_entries = db.GqlQuery('SELECT * '
                                       'FROM BuildStatusData '
                                       'ORDER BY revision DESC '
                                       'LIMIT 100')

    bots_to_latest_conclusive_entry = dict()
    for entry in build_status_entries:
      if entry.status == 'building':
        
        
        
        
        continue
      if bots_to_latest_conclusive_entry.has_key(entry.bot_name):
        
        continue

      bots_to_latest_conclusive_entry[entry.bot_name] = entry

    return bots_to_latest_conclusive_entry.values()

  def load_last_modified_at(self):
    build_status_root = db.GqlQuery('SELECT * '
                                    'FROM BuildStatusRoot').get()
    if not build_status_root:
      
      return None

    return build_status_root.last_updated_at

  def compute_lkgr(self):
    """ Finds the most recent revision for which all bots are green.

        Returns:
            The last known good revision (as an integer) or None if there
            is no green revision in the database.

        Implementation note: The data store fetches stuff as we go, so we won't
        read in the whole status table unless the LKGR is right at the end or
        we don't have a LKGR. Bots that are offline do not affect the LKGR
        computation (e.g. they are not considered to be failed).
    """
    build_status_entries = db.GqlQuery('SELECT * '
                                       'FROM BuildStatusData '
                                       'ORDER BY revision DESC ')

    first_entry = _get_first_entry(build_status_entries)
    if first_entry is None:
      
      return None

    current_lkgr = first_entry.revision
    statuses_for_current_lkgr = [first_entry.status]

    for entry in build_status_entries:
      if current_lkgr == entry.revision:
        statuses_for_current_lkgr.append(entry.status)
      else:
        
        if _all_ok(statuses_for_current_lkgr):
          
          return current_lkgr
        else:
          
          current_lkgr = entry.revision
          statuses_for_current_lkgr = [entry.status]

    if _all_ok(statuses_for_current_lkgr):
      
      return current_lkgr

    
    return None
