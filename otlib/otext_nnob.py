from __future__ import print_function

class OTExtSender(object):
    def __init__(self, state):
        self._state = state

    def send(self, msgs, msglength, otmodule, secparam=80):
        m = len(msgs)
        pass
