#!/usr/bin/env python2

from setuptools import setup, Extension, find_packages

__name__ = 'otlib'
__version__ = '0.0'
__author__ = 'Alex J. Malozemoff'

extra_sources = [
    'ot_np.cpp',
    #'ot_pvw.cpp',
    'otext_iknp.cpp',
    #'otext_nnob.cpp',
    # python wrappers
    'python/py_state.cpp',
    'python/py_ot.cpp',
    'python/py_ot_np.cpp',
    'python/py_otext_iknp.cpp',
    # utils
    'aes.cpp',
    'ghash.cpp',
    'crypto.cpp',
    'gmputils.cpp',
    'log.cpp',
    'net.cpp',
    'utils.cpp',
    # cmp
    'cmp/cmp.c',
]
extra_sources = map(lambda f: 'src/' + f, extra_sources)

otlib = Extension(
    'otlib._otlib',
    libraries = ['gmp', 'ssl', 'crypto'],
    extra_compile_args = ['-g', '-Wall', '-maes', '-msse4', '-mpclmul'],
    extra_objects = ['src/gfmul.a'],
    sources = [
        'src/python/_otlib.cpp',
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
