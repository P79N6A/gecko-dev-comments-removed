


import json

def read(log_f, raise_on_error=False):
    """Return a generator that will return the entries in a structured log file

    :param log_f: file-like object containing the log enteries, one per line
    :param raise_on_error: boolean indicating whether ValueError should be raised
                           for lines that cannot be decoded."""
    for line in log_f:
        try:
            yield json.loads(line)
        except ValueError:
            if raise_on_error:
                raise


def map_action(log_iter, action_map):
    """Call a callback per action for each item in a iterable containing structured
    log entries

    :param log_iter: Iterator returning structured log entries
    :param action_map: Dictionary mapping action name to callback function. Log items
                       with actions not in this dictionary will be skipped.
    """
    for item in log_iter:
        if item["action"] in action_map:
            yield action_map[item["action"]](item)
