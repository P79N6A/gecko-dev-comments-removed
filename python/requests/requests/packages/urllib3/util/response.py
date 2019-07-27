def is_fp_closed(obj):
    """
    Checks whether a given file-like object is closed.

    :param obj:
        The file-like object to check.
    """

    try:
        
        return obj.closed
    except AttributeError:
        pass

    try:
        
        
        return obj.fp is None
    except AttributeError:
        pass

    raise ValueError("Unable to determine whether fp is closed.")
