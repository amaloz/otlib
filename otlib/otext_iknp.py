from __future__ import print_function

import random, time
import numpy as np

import _otlib as _ot

def binstr2bytes(s):
    return ''.join([chr(int(s[8*i:8*i+8], 2)) for i in xrange(len(s) / 8)])

class OTExtSender(object):
    def __init__(self, state):
        self._state = state

    def send(self, msgs, maxlength, otmodule, secparam=80):
        m = len(msgs)
        assert m % 8 == 0, "length of 'msgs' must be divisible by 8"

        print('---OT---')

        start = time.time()
        ot = otmodule.OTReceiver(self._state)
        s = [random.randint(0, 1) for _ in xrange(secparam)]
        end = time.time()
        print('Initialize: %f' % (end - start))

        start = time.time()
        Q = ot.receive(s, m / 8)
        end = time.time()
        print('OT receive: %f' % (end - start))

        print('---OT Extension---')

        start = time.time()
        s = binstr2bytes(''.join([str(e) for e in s]))
        end = time.time()
        print('binstr2bytes: %f' % (end - start))
        
        _ot.otext_iknp_send(self._state, msgs, Q, s, maxlength, secparam)

class OTExtReceiver(object):
    def __init__(self, state):
        self._state = state

    def receive(self, choices, maxlength, otmodule, secparam=80):
        nchoices = len(choices)
        assert nchoices % 8 == 0, "length of 'choices' must be divisible by 8"

        print('---OT---')

        start = time.time()
        ot = otmodule.OTSender(self._state)
        end = time.time()
        print('Initialize OTSender: %f' % (end - start))

        start = time.time()
        r = binstr2bytes(''.join([str(c) for c in choices]))
        end = time.time()
        print('binstr2bytes: %f' % (end - start))

        start = time.time()
        T = [np.random.bytes(nchoices / 8) for _ in xrange(secparam)]
        end = time.time()
        print('build T: %f' % (end - start))

        start = time.time()
        inputs = _ot.otext_iknp_matrix_xor(self._state, T, r, nchoices, secparam)
        end = time.time()
        print('xor: %f' % (end - start))

        start = time.time()
        ot.send(inputs, nchoices / 8)
        end = time.time()
        print('OT send: %f' % (end - start))

        print('---OT Extension---')

        r = _ot.otext_iknp_receive(self._state, choices, T, maxlength, secparam)

        return r
