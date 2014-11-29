import os

import unittest
import mozunit
from taskcluster_graph.templates import (
    Templates,
    TemplatesException
)

class TemplatesTest(unittest.TestCase):

    def setUp(self):
        abs_path = os.path.abspath(os.path.dirname(__file__))
        self.subject = Templates(os.path.join(abs_path, 'fixtures'))


    def test_invalid_path(self):
        with self.assertRaisesRegexp(TemplatesException, 'must be a directory'):
            Templates('/zomg/not/a/dir')

    def test_no_templates(self):
        content = self.subject.load('simple.yml', {})
        self.assertEquals(content, {
            'is_simple': True
        })

    def test_with_templates(self):
        content = self.subject.load('templates.yml', {
            'woot': 'bar'
        })

        self.assertEquals(content, {
            'content': 'content',
            'variable': 'bar'
        })

    def test_inheritance(self):
        '''
        The simple single pass inheritance case.
        '''
        content = self.subject.load('inherit.yml')
        self.assertEqual(content, {
            'content': 'content',
            'variable': 'inherit'
        })

    def test_inheritance_circular(self):
        '''
        Circular reference handling.
        '''
        with self.assertRaisesRegexp(TemplatesException, 'circular'):
            self.subject.load('circular.yml', {})

if __name__ == '__main__':
    mozunit.main()

