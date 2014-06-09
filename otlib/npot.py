import _otlib as _ot

class NPOTSender(object):
    def __init__(self, host, port, length):
        self._state = _ot.init(host, repr(port), length, True)

    def send(self, msgs):
        _ot.np_send(self._state, msgs)

class NPOTReceiver(object):
    def __init__(self, host, port, length):
        self._state = _ot.init(host, repr(port), length, False)

    def receive(self, choices, N=2):
        return _ot.np_receive(self._state, N, choices)
