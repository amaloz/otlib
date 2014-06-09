from __future__ import print_function

import random
import numpy as np

import npot
import _otlib as _ot

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

SECPARAM = 80

class OTExtSender(object):
    def __init__(self, state):
        self._state = state

    def send(self, msgs):
        assert len(msgs) % 8 == 0, "length of 'msgs' must be divisible by 8"
        ot = npot.NPOTReceiver(self._state)
        s = [random.randint(0, 1) for _ in xrange(SECPARAM)]
        Q = ot.receive(s, len(msgs) / 8)
        print(Q)
        for col in Q:
            print(''.join('%08d' % int(bin(ord(c))[2:]) for c in col))
        Q = transpose(Q, len(msgs))
        # for col in Q:
        #     print(''.join('%08d' % int(bin(ord(c))[2:]) for c in col))
        s = binstr2bytes(''.join([str(e) for e in s]))
        _ot.otext_send(self._state, msgs, s, Q)

class OTExtReceiver(object):
    def __init__(self, state):
        self._state = state

    def receive(self, choices):
        ot = npot.NPOTSender(self._state)
        m = len(choices)
        assert m % 8 == 0, "length of 'choices' must be divisible by 8"
        r = binstr2bytes(''.join([str(c) for c in choices]))
        T = [np.random.bytes(m / 8) for _ in xrange(SECPARAM)]
        inputs = [(t, xor(r, t)) for t in T]
        ot.send(inputs, m / 8)
        T = transpose(T, m)
        # for col in T:
        #     print(''.join('%08d' % int(bin(ord(c))[2:]) for c in col))
        print(_ot.otext_receive(self._state, choices, T))
