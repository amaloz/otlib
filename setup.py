#!/usr/bin/env python2

from setuptools import setup, Extension, find_packages

__name__ = 'otlib'
__version__ = '0.0'
__author__ = 'Alex J. Malozemoff'

naorpinkas = Extension(
    'otlib._naorpinkas',
    libraries = ['gmp', 'ssl', 'crypto'],
    extra_compile_args = ['-g', '-Wall'],
    sources = [
        'src/_naorpinkas.cpp',
        'src/net.cpp',
        'src/utils.cpp',
        'src/log.cpp',
    ],
)

setup(
    name = __name__,
    version = __version__,
    author = __author__,
    description = 'Oblivious transfer library',
    packages = ['otlib'],
    ext_modules=[naorpinkas],
    test_suite = 't',
    classifiers = [
        'Topic :: Security :: Cryptography',
        'Environment :: Console',
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Science/Research',
        'Operating System :: Unix',
        'License :: OSI Approved :: Free For Educational Use',
        'Programming Language :: C'
    ],
)
