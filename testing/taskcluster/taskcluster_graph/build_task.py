





class BuildTaskValidationException(Exception):
    pass


def validate(task):
    '''
    The build tasks have some required fields in extra this function ensures
    they are there.
    '''
    if 'task' not in task:
        raise BuildTaskValidationException('must have task field')

    task_def = task['task']

    if 'extra' not in task_def:
        raise BuildTaskValidationException('build task must have task.extra props')

    if 'locations' not in task_def['extra']:
        raise BuildTaskValidationException('task.extra.locations missing')

    locations = task_def['extra']['locations']

    if 'build' not in locations:
        raise BuildTaskValidationException('task.extra.locations.build missing')

    if 'tests' not in locations:
        raise BuildTaskValidationException('task.extra.locations.tests missing')
