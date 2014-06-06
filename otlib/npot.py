import _naorpinkas as _np

class NPOTSender(object):
    def __init__(self, host, port, length):
        self._state = _np.init(host, repr(port), length, True)

    def send(self, msgs):
        _np.send(self._state, msgs)

class NPOTReceiver(object):
    def __init__(self, host, port, length):
        self._state = _np.init(host, repr(port), length, False)

    def receive(self, choices, N=2):
        return _np.receive(self._state, N, choices)
