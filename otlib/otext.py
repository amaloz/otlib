from __future__ import print_function

import random, time
import numpy as np

import npot
import _otlib as _ot

# TODO: push a lot of this to C for efficiency

def binstr2bytes(s):
    return ''.join([chr(int(s[8*i:8*i+8], 2)) for i in xrange(len(s) / 8)])

def bytes2bintuple(s, length):
    bstr = []
    for b in s:
        b = bin(ord(b))[2:]
        b = ('0' * (8 - len(b))) + b
        bstr.append(b)
    bstr = ''.join(bstr)
    bstr = ('0' * (length - len(bstr))) + bstr
    assert len(bstr) == length
    return [int(b) for b in bstr]

def xor(a, b):
    assert len(a) == len(b)
    return ''.join([chr(ord(a[i]) ^ ord(b[i])) for i in xrange(len(a))])

def transpose(m, length):
    m = np.array([bytes2bintuple(col, length) for col in m]).transpose()
    return [binstr2bytes(''.join([str(e) for e in row])) for row in m]

class OTExtSender(object):
    def __init__(self, state):
        self._state = state

    def send(self, msgs, maxlength, otmodule, secparam=80):
        m = len(msgs)
        assert m % 8 == 0, "length of 'msgs' must be divisible by 8"
        ot = otmodule.OTReceiver(self._state)
        s = [random.randint(0, 1) for _ in xrange(secparam)]
        Q = ot.receive(s, m / 8)
        start = time.time()
        Q = transpose(Q, m)
        end = time.time()
        print("Time spent transposing: %f" % (end - start))
        s = binstr2bytes(''.join([str(e) for e in s]))
        _ot.otext_send(self._state, msgs, maxlength, s, Q)

class OTExtReceiver(object):
    def __init__(self, state):
        self._state = state

    def receive(self, choices, maxlength, otmodule, secparam=80):
        m = len(choices)
        assert m % 8 == 0, "length of 'choices' must be divisible by 8"
        ot = otmodule.OTSender(self._state)
        r = binstr2bytes(''.join([str(c) for c in choices]))
        T = [np.random.bytes(m / 8) for _ in xrange(secparam)]
        inputs = [(t, xor(r, t)) for t in T]
        ot.send(inputs, m / 8)
        start = time.time()
        T = transpose(T, m)
        end = time.time()
        print("Time spent transposing: %f" % (end - start))
        return _ot.otext_receive(self._state, choices, maxlength, T)
