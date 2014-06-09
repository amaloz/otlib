import _otlib as _ot

import itertools

class NPOTSender(object):
    def __init__(self, state):
        self._state = state

    def send(self, msgs, maxlength):
        _ot.np_send(self._state, msgs, maxlength)

class NPOTReceiver(object):
    def __init__(self, state):
        self._state = state

    def receive(self, choices, maxlength, N=2):
        return _ot.np_receive(self._state, choices, N, maxlength)
