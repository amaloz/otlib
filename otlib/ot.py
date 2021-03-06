from __future__ import print_function

class OTSender(object):
    def __init__(self, state, send):
        self._state = state
        self._send = send
    def send(self, msgs, msglength):
        self._send(self._state, msgs, msglength)

class OTReceiver(object):
    def __init__(self, state, recv):
        self._state = state
        self._recv = recv
    def receive(self, choices, msglength, N=2):
        return self._recv(self._state, choices, N, msglength)
