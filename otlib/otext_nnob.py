from __future__ import division, print_function

import math, random, time

import _otlib as _ot

class OTExtSender(object):
    def __init__(self, state):
        self._state = state

    def send(self, bits, otmodule, secparam=80):
        m = len(msgs)
        assert m > secparam, "number of messages must be greater than security parameter"
        num = math.ceil(8 / 3 * secparam)
        rbits = [random.randint(0, 1) for _ in xrange(num)]
        ot = otmodule.OTReceiver(self._state)
        ells = ot.receive(rbits, secparam / 8)
        _ot.otext_nnob_send(self._state, bits, ells, secparam)

class OTExtReceiver(object):
    def __init__(self, state):
        self._state = state

    def receive(self, choices, msglength, otmodule, secparam=80):
        m = len(choices)
        assert m > secparam, "number of choices must be greater than security parameter"
        def randseed():
            return random.randint(0, 2 ** secparam - 1)
        num = math.ceil(8 / 3 * secparam)
        seeds = [(randseed(), randseed()) for _ in xrange(num)]
        ot = otmodule.OTSender(self._state)
        ot.send(seeds, secparam / 8)
        _ot.otext_nnob_receive(self._state, choices, seeds, secparam)
            
