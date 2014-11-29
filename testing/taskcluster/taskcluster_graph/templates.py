import os

import pystache
import yaml

class TemplatesException(Exception):
    pass

class Templates():
    '''
    The taskcluster integration makes heavy use of yaml to describe tasks this
    class handles the loading/rendering.
    '''

    def __init__(self, root):
        '''
        Initialize the template render.

        :param str root: Root path where to load yaml files.
        '''
        if not root:
            raise TemplatesException('Root is required')

        if not os.path.isdir(root):
            raise TemplatesException('Root must be a directory')

        self.root = root;

    def render(self, content, parameters):
        '''
        Renders a given yaml string.

        :param str content: Of yaml file.
        :param dict parameters: For mustache templates.
        '''
        content = pystache.render(content, parameters)
        return yaml.load(content)

    def load(self, path, parameters=None):
        '''
        Load an render the given yaml path.

        :param str path: Location of yaml file to load (relative to root).
        :param dict parameters: To template yaml file with.
        '''
        if not path:
            raise TemplateException('path is required')

        path = os.path.join(self.root, path)

        if not os.path.isfile(path):
            raise TemplateException('"%s" is not a file'.format(path))

        content = open(path).read()
        return self.render(content, parameters)
