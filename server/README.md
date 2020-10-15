# rproxy-server

This is the server side of the obsws-rproxy system. It is in the form of a Python library. This is based on the `simpleobsws` python library, with the main change being that it acts as a websocket server instead of a websocket client.

## Using

- Install the dependencies with `pip install -r requirements.txt`
- A basic example is available as [`main.py`](main.py). It will create a websocket server running on all interfaces and port `6969`. This is where you can connect your client to.

## Docs

- Since this is based on `simpleobsws`, the `simpleobsws` [usage docs](https://github.com/IRLToolkit/simpleobsws/blob/master/usage.md) are pretty useful, though not fully accurate.
- obs-websocket docs: https://github.com/Palakis/obs-websocket/blob/4.x-current/docs/generated/protocol.md