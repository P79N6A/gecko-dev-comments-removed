# -*- coding: utf-8 -*-





import os
import os.path
import json
import copy
import datetime

import pystache
import yaml

from mach.decorators import (
    CommandArgument,
    CommandProvider,
    Command,
)


from taskcluster_graph.commit_parser import parse_commit
from taskcluster_graph.slugid import slugid
from taskcluster_graph.from_now import json_time_from_now

import taskcluster_graph.build_task

ROOT = os.path.dirname(os.path.realpath(__file__))
DOCKER_ROOT = os.path.join(ROOT, '..', 'docker')
LOCAL_WORKER_TYPES = ['b2gtest', 'b2gbuild']


ARTIFACT_URL = 'https://queue.taskcluster.net/v1/task/{}/artifacts/{}'
REGISTRY = open(os.path.join(DOCKER_ROOT, 'REGISTRY')).read().strip()


def import_yaml(path, variables=None):
    ''' Load a yml file relative to the root of this file'''
    content = open(os.path.join(ROOT, path)).read()
    if variables is not None:
        content = pystache.render(content, variables)
    task = yaml.load(content)
    return task

def docker_image(name):
    ''' Determine the docker tag/revision from an in tree docker file '''
    repository_path = os.path.join(DOCKER_ROOT, name, 'REPOSITORY')
    repository = REGISTRY

    version = open(os.path.join(DOCKER_ROOT, name, 'VERSION')).read().strip()

    if os.path.isfile(repository_path):
        repository = open(repository_path).read().strip()

    return '{}/{}:{}'.format(repository, name, version)

@CommandProvider
class TryGraph(object):
    @Command('trygraph', category="ci",
        description="Create taskcluster try server graph")
    @CommandArgument('--revision',
        help='revision in gecko to use in sub tasks')
    @CommandArgument('--message',
        required=True,
        help='Commit message to be parsed')
    @CommandArgument('--repository',
        help='full path to hg repository to use in sub tasks')
    @CommandArgument('--owner',
        help='email address of who owns this graph')
    @CommandArgument('--extend-graph',
        action="store_true", dest="ci", help='Omit create graph arguments')
    def create_graph(self, revision="", message="", repository="", owner="",
            ci=False):
        """ Create the taskcluster graph from the try commit message.

        :param args: commit message (ex: "â€“ try: -b o -p linux64_gecko -u gaia-unit -t none")
        """
        jobs = import_yaml('job_flags.yml')
        job_graph = parse_commit(message, jobs)

        
        parameters = {
            'docker_image': docker_image,
            'repository': repository,
            'revision': revision,
            'owner': owner,
            'from_now': json_time_from_now,
            'now': datetime.datetime.now().isoformat()
        }

        
        graph = {
            'tasks': []
        }

        if ci is False:
            
            
            graph['scopes'] = [
                "docker-worker:cache:sources-mozilla-central",
                "docker-worker:cache:sources-gaia",
                "docker-worker:cache:build-b2g-desktop-objects"
            ]

            
            
            
            for worker_type in LOCAL_WORKER_TYPES:
                graph['scopes'].append(
                    'queue:define-task:{}/{}'.format('aws-provisioner',
                        worker_type)
                )

                graph['scopes'].append(
                    'queue:create-task:{}/{}'.format('aws-provisioner',
                        worker_type)
                )

            graph['metadata'] = {
                'source': 'http://todo.com/what/goes/here',
                'owner': owner,
                
                'description': 'Try task graph generated via ./mach trygraph',
                'name': 'trygraph local'
            }

        for build in job_graph:
            build_parameters = copy.copy(parameters)
            build_parameters['build_slugid'] = slugid()
            build_task = import_yaml(build['task'], build_parameters)

            
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

            for test in build['dependents']:
                test_parameters = copy.copy(build_parameters)
                test_parameters['build_url'] = build_url
                test_parameters['tests_url'] = tests_url
                test_parameters['total_chunks'] = 1

                if 'chunks' in test:
                    test_parameters['total_chunks'] = test['chunks']

                for chunk in range(1, test_parameters['total_chunks'] + 1):
                    test_parameters['chunk'] = chunk
                    test_task = import_yaml(test['task'], test_parameters)
                    test_task['taskId'] = slugid()

                    if 'requires' not in test_task:
                        test_task['requires'] = []

                    test_task['requires'].append(test_parameters['build_slugid'])
                    graph['tasks'].append(test_task)

        print(json.dumps(graph, indent=4))
