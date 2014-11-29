import os

import pystache
import yaml

# Key used in template inheritance...
INHERITS_KEY = '$inherits'

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

    def _inherits(self, path, obj, seen):
        blueprint = obj.pop(INHERITS_KEY)
        seen.add(path)

        # Resolve the path here so we can detect circular references.
        template = self.resolve_path(blueprint.get('from'))
        variables = blueprint.get('variables', {})

        if not template:
            msg = '"{}" inheritance template missing'.format(path)
            raise TemplatesException(msg)

        if template in seen:
            msg = 'Error in "{}" circular template inheritance'.format(path)
            raise TemplatesException(msg)

        return self.load(template, variables, seen)

    def render(self, path, content, parameters, seen):
        '''
        Renders a given yaml string.

        :param str path:  used to prevent infinite recursion in inheritance.
        :param str content: Of yaml file.
        :param dict parameters: For mustache templates.
        :param set seen: Seen files (used for inheritance)
        '''
        content = pystache.render(content, parameters)
        result = yaml.load(content)

        # In addition to the usual template logic done by mustache we also
        # handle special '$inherit' dict keys.
        if isinstance(result, dict) and INHERITS_KEY in result:
            return self._inherits(path, result, seen)

        return result

    def resolve_path(self, path):
        return os.path.join(self.root, path)

    def load(self, path, parameters=None, seen=set()):
        '''
        Load an render the given yaml path.

        :param str path: Location of yaml file to load (relative to root).
        :param dict parameters: To template yaml file with.
        '''
        if not path:
            raise TemplateException('path is required')

        path = self.resolve_path(path)

        if not os.path.isfile(path):
            raise TemplateException('"{}" is not a file'.format(path))

        content = open(path).read()
        return self.render(path, content, parameters, seen)
