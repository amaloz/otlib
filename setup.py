#!/usr/bin/env python2

from setuptools import setup, Extension, find_packages

__name__ = 'otlib'
__version__ = '0.0'
__author__ = 'Alex J. Malozemoff'

extra_sources = [
    'src/log.cpp',
    'src/net.cpp',
    'src/state.cpp',
    'src/utils.cpp',
]

naorpinkas = Extension(
    'otlib._naorpinkas',
    libraries = ['gmp', 'ssl', 'crypto'],
    extra_compile_args = ['-g', '-Wall'],
    sources = [
        'src/_naorpinkas.cpp',
    ] + extra_sources,
)

otextension = Extension(
    'otlib._otextension',
    libraries = ['gmp', 'ssl', 'crypto'],
    extra_compile_args = ['-g', '-Wall'],
    sources = [
        'src/_otextension.cpp',
    ] + extra_sources,
)


setup(
    name = __name__,
    version = __version__,
    author = __author__,
    description = 'Oblivious transfer library',
    packages = ['otlib'],
    ext_modules=[naorpinkas, otextension],
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
