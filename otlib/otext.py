from __future__ import print_function

import random
import numpy as np

import npot

def binstr2bytes(s):
    return ''.join([chr(int(s[8*i:8*i+8], 2)) for i in xrange(len(s) / 8)])

def xor(a, b):
    assert len(a) == len(b)
    return ''.join([chr(ord(a[i] ^ ord(b[i]))) for i in xrange(len(a))])

class OTExtSender(object):
    def __init__(self, host, port, secparam=80):
        self._host = host
        self._port = repr(port)
        self._secparam = secparam

    def send(self, msgs):
        ot = npot.NPOTReceiver(self._host, self._port, len(msgs))
        s = [random.randint(0, 1) for _ in xrange(self._secparam)]
        r = ot.receive(s)
        # _otext.send(ot._state, r, 80, 160)

class OTExtReceiver(object):
    def __init__(self, host, port, secparam=80):
        self._host = host
        self._port = repr(port)
        self._secparam = secparam

    def receive(self, choices):
        ot = npot.NPOTSender(self._host, self._port, len(choices))
        m = len(choices)
        assert m % 8 == 0, "'choices' must be divisible by 8"
        s = ''.join([str(c) for c in choices])
        print(s)
        r = binstr2bytes(''.join(s))
        print(r)
        T = [np.random.bytes(m / 8) for _ in xrange(self.secparam)]
        assert xor(xor(r, T[0]), r) == T[0]
        inputs = [(t, xor(r, t)) for t in T]
        r = ot.send(inputs)
        print(r)
        # _otext.receive(ot._state, T, 80, 160)
