




import shlex
import argparse
import functools
import copy
from try_test_parser import parse_test_opts

TRY_DELIMITER='try:'



BUILD_TYPE_ALIASES = {
    'o': 'opt',
    'd': 'debug'
}

class InvalidCommitException(Exception):
    pass

def normalize_platform_list(all_builds, build_list):
    if build_list == 'all':
        return all_builds

    return [ build.strip() for build in build_list.split(',') ]

def normalize_test_list(all_tests, job_list):
    '''
    Normalize a set of jobs (builds or tests) there are three common cases:

        - job_list is == 'none' (meaning an empty list)
        - job_list is == 'all' (meaning use the list of jobs for that job type)
        - job_list is comma delimited string which needs to be split

    :param list all_tests: test flags from job_flags.yml structure.
    :param str job_list: see above examples.
    :returns: List of jobs
    '''

    
    if job_list is None or job_list == 'none':
        return []

    tests = parse_test_opts(job_list)

    if not tests:
        return []

    
    if tests[0]['test'] == 'all':
        results = []
        all_entry = tests[0]
        for test in all_tests:
            entry = { 'test': test }
            
            if 'platforms' in all_entry:
                entry['platforms'] = list(all_entry['platforms'])
            results.append(entry)
        return results
    else:
        return tests

def extract_tests_from_platform(test_jobs, build_platform, build_task, tests):
    '''
    Build the list of tests from the current build.

    :param dict test_jobs: Entire list of tests (from job_flags.yml).
    :param dict build_platform: Current build platform.
    :param str build_task: Build task path.
    :param list tests: Test flags.
    :return: List of tasks (ex: [{ task: 'test_task.yml' }]
    '''
    if tests is None:
        return []

    results = []

    for test_entry in tests:
        if test_entry['test'] not in test_jobs:
            continue

        test_job = test_jobs[test_entry['test']]

        
        if 'allowed_build_tasks' in test_job and build_task not in test_job['allowed_build_tasks']:
            continue

        if 'platforms' in test_entry:
            
            
            
            if 'platforms' not in build_platform:
                continue

            
            
            common_platforms = set(test_entry['platforms']) & set(build_platform['platforms'])
            if not common_platforms:
                
                continue

        
        
        results.append(copy.deepcopy(test_job))

    return results

'''
This module exists to deal with parsing the options flags that try uses. We do
not try to build a graph or anything here but match up build flags to tasks via
the "jobs" datastructure (see job_flags.yml)
'''

def parse_commit(message, jobs):
    '''
    :param message: Commit message that is typical to a try push.
    :param jobs: Dict (see job_flags.yml)
    '''

    
    parts = shlex.split(message)

    if parts[0] != TRY_DELIMITER:
        raise InvalidCommitException('Invalid commit format must start with' +
                TRY_DELIMITER)

    
    parser = argparse.ArgumentParser()
    parser.add_argument('-b', dest='build_types')
    parser.add_argument('-p', dest='platforms')
    parser.add_argument('-u', dest='tests')
    args, unknown = parser.parse_known_args(parts[1:])

    
    if args.platforms is None:
        return []

    
    if args.build_types is None:
        return []

    build_types = [ BUILD_TYPE_ALIASES.get(build_type, build_type) for
            build_type in args.build_types ]

    platforms = normalize_platform_list(jobs['flags']['builds'], args.platforms)
    tests = normalize_test_list(jobs['flags']['tests'], args.tests)

    result = []

    
    for platform in platforms:
        
        if platform not in jobs['builds']:
            continue

        platform_builds = jobs['builds'][platform]

        for build_type in build_types:
            
            if build_type not in platform_builds['types']:
                continue

            platform_build = platform_builds['types'][build_type]
            build_task = platform_build['task']

            if 'additional-parameters' in platform_build:
                additional_parameters = platform_build['additional-parameters']
            else:
                additional_parameters = {}

            
            result.append({
                'task': build_task,
                'dependents': extract_tests_from_platform(
                    jobs['tests'], platform_builds, build_task, tests
                ),
                'additional-parameters': additional_parameters
            })

    return result
