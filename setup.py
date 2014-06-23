#!/usr/bin/env python2

from setuptools import setup, Extension, find_packages

__name__ = 'otlib'
__version__ = '0.0'
__author__ = 'Alex J. Malozemoff'

extra_sources = [
    'naorpinkas.cpp',
    'otextension.cpp',
    'pvw.cpp',
    # utils
    'crypto.cpp',
    'gmputils.cpp',
    'log.cpp',
    'net.cpp',
    'state.cpp',
    'utils.cpp',
]
extra_sources = map(lambda f: 'src/' + f, extra_sources)

otlib = Extension(
    'otlib._otlib',
    libraries = ['gmp', 'ssl', 'crypto'],
    extra_compile_args = ['-g', '-Wall'],
    sources = [
        'src/_otlib.cpp',
    ] + extra_sources,
)

setup(
    name = __name__,
    version = __version__,
    author = __author__,
    description = 'Oblivious transfer library',
    packages = ['otlib'],
    ext_modules=[otlib],
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
