from __future__ import division, print_function

import math, random, time

import _otlib as _ot

class OTExtSender(object):
    def __init__(self, state):
        self._state = state

    def send(self, bits, otmodule, secparam=80):
        m = len(bits)
        assert m > secparam, "number of messages must be greater than security parameter"
        num = int(math.ceil(8 / 3 * secparam))
        print('ceil(8/3 secparam) = %d' % num)
        rbits = [random.randint(0, 1) for _ in xrange(num)]
        ot = otmodule.OTReceiver(self._state)
        ells = ot.receive(rbits, int(secparam / 8))
        _ot.otext_nnob_send(self._state, bits, ells, secparam)

class OTExtReceiver(object):
    def __init__(self, state):
        self._state = state

    def receive(self, choices, otmodule, secparam=80):
        m = len(choices)
        assert m > secparam, "number of choices must be greater than security parameter"
        def randseed():
            return random.randint(0, 2 ** secparam - 1)
        num = int(math.ceil(8 / 3 * secparam))
        print('ceil(8/3 secparam) = %d' % num)
        seeds = [(randseed(), randseed()) for _ in xrange(num)]
        ot = otmodule.OTSender(self._state)
        ot.send(seeds, int(secparam / 8))
        _ot.otext_nnob_receive(self._state, choices, seeds, secparam)
            
