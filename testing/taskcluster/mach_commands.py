# -*- coding: utf-8 -*-





import os
import os.path
import json
import copy
import datetime
import subprocess
import sys
import urllib2

from mach.decorators import (
    CommandArgument,
    CommandProvider,
    Command,
)

from taskcluster_graph.commit_parser import parse_commit
from taskcluster_graph.slugid import slugid
from taskcluster_graph.slugidjar import SlugidJar
from taskcluster_graph.from_now import json_time_from_now, current_json_time
from taskcluster_graph.templates import Templates

import taskcluster_graph.build_task

ROOT = os.path.dirname(os.path.realpath(__file__))
GECKO = os.path.realpath(os.path.join(ROOT, '..', '..'))
DOCKER_ROOT = os.path.join(ROOT, '..', 'docker')
MOZHARNESS_CONFIG = os.path.join(GECKO, 'testing', 'mozharness', 'mozharness.json')


ARTIFACT_URL = 'https://queue.taskcluster.net/v1/task/{}/artifacts/{}'
REGISTRY = open(os.path.join(DOCKER_ROOT, 'REGISTRY')).read().strip()

DEFINE_TASK = 'queue:define-task:aws-provisioner/{}'

TREEHERDER_ROUTE_PREFIX = 'tc-treeherder-stage'
TREEHERDER_ROUTES = {
    'staging': 'tc-treeherder-stage',
    'production': 'tc-treeherder'
}

DEFAULT_TRY = 'try: -b do -p all -u all'
DEFAULT_JOB_PATH = os.path.join(
    ROOT, 'tasks', 'branches', 'mozilla-central', 'job_flags.yml'
)

def load_mozharness_info():
    with open(MOZHARNESS_CONFIG) as content:
        return json.loads(content)

def get_hg_url():
    ''' Determine the url for the mercurial repository'''
    try:
        url = subprocess.check_output(
            ['hg', 'path', 'default'],
            stderr=subprocess.PIPE
        )
    except subprocess.CalledProcessError:
        sys.stderr.write(
            "Error: Could not determine the current hg repository url. " \
            "Ensure command is executed within a hg respository"
        )
        sys.exit(1)

    return url

def get_latest_hg_revision(repository):
    ''' Retrieves the revision number of the latest changed head'''
    try:
        revision = subprocess.check_output(
            ['hg', 'id', '-r', 'tip', repository, '-i'],
            stderr=subprocess.PIPE
        ).strip('\n')
    except subprocess.CalledProcessError:
        sys.stderr.write(
            "Error: Could not determine the latest hg revision at {} " \
            "Ensure command is executed within a cloned hg respository and " \
            "remote default remote repository is accessible".format(repository)
        )
        sys.exit(1)

    return revision

def docker_image(name):
    ''' Determine the docker tag/revision from an in tree docker file '''
    repository_path = os.path.join(DOCKER_ROOT, name, 'REPOSITORY')
    repository = REGISTRY

    version = open(os.path.join(DOCKER_ROOT, name, 'VERSION')).read().strip()

    if os.path.isfile(repository_path):
        repository = open(repository_path).read().strip()

    return '{}/{}:{}'.format(repository, name, version)

def get_task(task_id):
    return json.load(urllib2.urlopen("https://queue.taskcluster.net/v1/task/" + task_id))


def gaia_info():
    '''
    Fetch details from in tree gaia.json (which links this version of
    gecko->gaia) and construct the usual base/head/ref/rev pairing...
    '''
    gaia = json.load(open(os.path.join(GECKO, 'b2g', 'config', 'gaia.json')))

    if gaia['git'] is None or \
       gaia['git']['remote'] == '' or \
       gaia['git']['git_revision'] == '' or \
       gaia['git']['branch'] == '':

       
       return {
         'gaia_base_repository': 'https://hg.mozilla.org/{}'.format(gaia['repo_path']),
         'gaia_head_repository': 'https://hg.mozilla.org/{}'.format(gaia['repo_path']),
         'gaia_ref': gaia['revision'],
         'gaia_rev': gaia['revision']
       }

    else:
        
        return {
            'gaia_base_repository': gaia['git']['remote'],
            'gaia_head_repository': gaia['git']['remote'],
            'gaia_rev': gaia['git']['git_revision'],
            'gaia_ref': gaia['git']['branch'],
        }

def decorate_task_treeherder_routes(task, suffix):
    """
    Decorate the given task with treeherder routes.

    Uses task.extra.treeherderEnv if available otherwise defaults to only
    staging.

    :param dict task: task definition.
    :param str suffix: The project/revision_hash portion of the route.
    """

    if 'extra' not in task:
        return

    if 'routes' not in task:
        task['routes'] = []

    treeheder_env = task['extra'].get('treeherderEnv', ['staging'])

    for env in treeheder_env:
        task['routes'].append('{}.{}'.format(TREEHERDER_ROUTES[env], suffix))

@CommandProvider
class DecisionTask(object):
    @Command('taskcluster-decision', category="ci",
        description="Build a decision task")
    @CommandArgument('--project',
        required=True,
        help='Treeherder project name')
    @CommandArgument('--url',
        required=True,
        help='Gecko repository to use as head repository.')
    @CommandArgument('--revision',
        required=True,
        help='Revision for this project')
    @CommandArgument('--revision-hash',
        help='Treeherder revision hash')
    @CommandArgument('--comment',
        required=True,
        help='Commit message for this revision')
    @CommandArgument('--owner',
        required=True,
        help='email address of who owns this graph')
    @CommandArgument('task', help="Path to decision task to run.")
    def run_task(self, **params):
        templates = Templates(ROOT)
        
        parameters = dict(gaia_info().items() + {
            'source': 'http://todo.com/soon',
            'project': params['project'],
            'comment': params['comment'],
            'url': params['url'],
            'revision': params['revision'],
            'revision_hash': params.get('revision_hash', ''),
            'owner': params['owner'],
            'as_slugid': SlugidJar(),
            'from_now': json_time_from_now,
            'now': datetime.datetime.now().isoformat()
        }.items())
        task = templates.load(params['task'], parameters)
        print(json.dumps(task, indent=4))

@CommandProvider
class Graph(object):
    @Command('taskcluster-graph', category="ci",
        description="Create taskcluster task graph")
    @CommandArgument('--base-repository',
        default=os.environ.get('GECKO_BASE_REPOSITORY'),
        help='URL for "base" repository to clone')
    @CommandArgument('--mozharness-repository',
        default='https://hg.mozilla.org/build/mozharness',
        help='URL for custom mozharness repo')
    @CommandArgument('--head-repository',
        default=os.environ.get('GECKO_HEAD_REPOSITORY'),
        help='URL for "head" repository to fetch revision from')
    @CommandArgument('--head-ref',
        default=os.environ.get('GECKO_HEAD_REF'),
        help='Reference (this is same as rev usually for hg)')
    @CommandArgument('--head-rev',
        default=os.environ.get('GECKO_HEAD_REV'),
        help='Commit revision to use from head repository')
    @CommandArgument('--mozharness-rev',
        default='default',
        help='Commit revision to use from mozharness repository')
    @CommandArgument('--mozharness-ref',
        default='master',
        help='Commit ref to use from mozharness repository')
    @CommandArgument('--message',
        help='Commit message to be parsed. Example: "try: -b do -p all -u all"')
    @CommandArgument('--revision-hash',
            required=False,
            help='Treeherder revision hash to attach results to')
    @CommandArgument('--project',
        required=True,
        help='Project to use for creating task graph. Example: --project=try')
    @CommandArgument('--pushlog-id',
        dest='pushlog_id',
        required=False,
        default=0)
    @CommandArgument('--owner',
        required=True,
        help='email address of who owns this graph')
    @CommandArgument('--extend-graph',
        action="store_true", dest="ci", help='Omit create graph arguments')
    def create_graph(self, **params):
        project = params['project']
        message = params.get('message', '') if project == 'try' else DEFAULT_TRY

        
        if project == 'try' and not message:
            sys.stderr.write(
                    "Must supply commit message when creating try graph. " \
                    "Example: --message='try: -b do -p all -u all'"
            )
            sys.exit(1)

        templates = Templates(ROOT)
        job_path = os.path.join(ROOT, 'tasks', 'branches', project, 'job_flags.yml')
        job_path = job_path if os.path.exists(job_path) else DEFAULT_JOB_PATH

        jobs = templates.load(job_path, {})

        job_graph = parse_commit(message, jobs)
        
        parameters = dict(gaia_info().items() + {
            'project': project,
            'pushlog_id': params.get('pushlog_id', 0),
            'docker_image': docker_image,
            'base_repository': params['base_repository'] or \
                params['head_repository'],
            'head_repository': params['head_repository'],
            'head_ref': params['head_ref'] or params['head_rev'],
            'head_rev': params['head_rev'],
            'owner': params['owner'],
            'from_now': json_time_from_now,
            'now': datetime.datetime.now().isoformat(),
            'mozharness_repository': params['mozharness_repository'],
            'mozharness_rev': params['mozharness_rev'],
            'mozharness_ref': params['mozharness_ref'],
            'revision_hash': params['revision_hash']
        }.items())

        treeherder_route = '{}.{}'.format(
            params['project'],
            params.get('revision_hash', '')
        )

        
        graph = {
            'tasks': [],
            'scopes': []
        }

        if params['revision_hash']:
            for env in TREEHERDER_ROUTES:
                graph['scopes'].append('queue:route:{}.{}'.format(TREEHERDER_ROUTES[env], treeherder_route))

        graph['metadata'] = {
            'source': 'http://todo.com/what/goes/here',
            'owner': params['owner'],
            
            'description': 'Task graph generated via ./mach taskcluster-graph',
            'name': 'task graph local'
        }

        for build in job_graph:
            build_parameters = dict(parameters)
            build_parameters['build_slugid'] = slugid()
            build_task = templates.load(build['task'], build_parameters)

            if 'routes' not in build_task['task']:
                build_task['task']['routes'] = []

            if params['revision_hash']:
                decorate_task_treeherder_routes(build_task['task'],
                                                treeherder_route)

            
            taskcluster_graph.build_task.validate(build_task)
            graph['tasks'].append(build_task)

            tests_url = ARTIFACT_URL.format(
                build_parameters['build_slugid'],
                build_task['task']['extra']['locations']['tests']
            )

            build_url = ARTIFACT_URL.format(
                build_parameters['build_slugid'],
                build_task['task']['extra']['locations']['build']
            )

            
            img_url = ARTIFACT_URL.format(
                build_parameters['build_slugid'],
                build_task['task']['extra']['locations'].get('img', '')
            )

            define_task = DEFINE_TASK.format(build_task['task']['workerType'])

            graph['scopes'].append(define_task)
            graph['scopes'].extend(build_task['task'].get('scopes', []))
            route_scopes = map(lambda route: 'queue:route:' + route, build_task['task'].get('routes', []))
            graph['scopes'].extend(route_scopes)

            
            
            build_treeherder_config = build_task['task']['extra']['treeherder']

            if 'machine' not in build_treeherder_config:
                message = '({}), extra.treeherder.machine required for all builds'
                raise ValueError(message.format(build['task']))

            if 'build' not in build_treeherder_config:
                build_treeherder_config['build'] = \
                    build_treeherder_config['machine']

            if 'collection' not in build_treeherder_config:
                build_treeherder_config['collection'] = { 'opt': True }

            if len(build_treeherder_config['collection'].keys()) != 1:
                message = '({}), extra.treeherder.collection must contain one type'
                raise ValueError(message.fomrat(build['task']))

            for test in build['dependents']:
                test = test['allowed_build_tasks'][build['task']]
                test_parameters = copy.copy(build_parameters)
                test_parameters['build_url'] = build_url
                test_parameters['img_url'] = img_url
                test_parameters['tests_url'] = tests_url

                test_definition = templates.load(test['task'], {})['task']
                chunk_config = test_definition['extra']['chunks']

                
                if 'chunks' in test:
                    chunk_config['total'] = test['chunks']

                test_parameters['total_chunks'] = chunk_config['total']

                for chunk in range(1, chunk_config['total'] + 1):
                    if 'only_chunks' in test and \
                        chunk not in test['only_chunks']:
                        continue

                    test_parameters['chunk'] = chunk
                    test_task = templates.load(test['task'], test_parameters)
                    test_task['taskId'] = slugid()

                    if 'requires' not in test_task:
                        test_task['requires'] = []

                    test_task['requires'].append(test_parameters['build_slugid'])

                    if 'treeherder' not in test_task['task']['extra']:
                        test_task['task']['extra']['treeherder'] = {}

                    
                    
                    test_treeherder_config = test_task['task']['extra']['treeherder']

                    test_treeherder_config['collection'] = \
                        build_treeherder_config.get('collection', {})

                    test_treeherder_config['build'] = \
                        build_treeherder_config.get('build', {})

                    test_treeherder_config['machine'] = \
                        build_treeherder_config.get('machine', {})

                    if 'routes' not in test_task['task']:
                        test_task['task']['routes'] = []

                    if 'scopes' not in test_task['task']:
                        test_task['task']['scopes'] = []

                    if params['revision_hash']:
                        decorate_task_treeherder_routes(
                                test_task['task'], treeherder_route)

                    graph['tasks'].append(test_task)

                    define_task = DEFINE_TASK.format(
                        test_task['task']['workerType']
                    )

                    graph['scopes'].append(define_task)
                    graph['scopes'].extend(test_task['task'].get('scopes', []))

        graph['scopes'] = list(set(graph['scopes']))

        
        if params['ci'] is True:
            graph.pop('scopes', None)
            graph.pop('metadata', None)

        print(json.dumps(graph, indent=4))
