import asyncio
import websockets
import base64
import hashlib
import json
import uuid
import time
import inspect

class obswsProxyServer:
    def __init__(self, hostname='0.0.0.0', port=6969, password=None, loop: asyncio.BaseEventLoop=None):
        self.hostname = hostname
        self.port = port
        self.password = password
        self.loop = loop or asyncio.get_event_loop()

        self.clientSocket = None
        self.wsServer = None

        self.eventFunctions = []
        self.answers = {}

    async def load(self):
        if self.wsServer != None:
            raise Exception("The proxy server is already loaded!")
        self.wsServer = await websockets.serve(self._websocketConnectionHandler, self.hostname, self.port)

    async def unload(self):
        if self.clientSocket != None:
            await self.clientSocket.close(1001, "rproxy-server is unloading.")
            self.clientSocket = None
        if self.wsServer != None:
            self.wsServer.close()
            self.wsServer = None
        self.answers = {}

    def isConnected(self):
        if self.clientSocket == None:
            return False
        return True

    async def call(self, request_type, data=None, timeout=15):
        if type(data) != dict and data != None:
            raise Exception('Input data must be valid dict object')
        if self.clientSocket == None:
            raise Exception('There is no rproxy-client connected.')
        request_id = str(uuid.uuid1())
        requestpayload = {'message-id':request_id, 'request-type':request_type}
        if data != None:
            for key in data.keys():
                if key == 'message-id':
                    continue
                requestpayload[key] = data[key]
        await self.clientSocket.send(json.dumps(requestpayload))
        waittimeout = time.time() + timeout
        await asyncio.sleep(0.05)
        while time.time() < waittimeout:
            if request_id in self.answers:
                returndata = self.answers.pop(request_id)
                returndata.pop('message-id')
                return returndata
            await asyncio.sleep(0.1)
        raise Exception('The request with type {} timed out after {} seconds.'.format(request_type, timeout))

    async def emit(self, request_type, data=None):
        if type(data) != dict and data != None:
            raise Exception('Input data must be valid dict object')
        if self.clientSocket == None:
            raise Exception('There is no rproxy-client connected.')
        request_id = str(uuid.uuid1())
        requestpayload = {'message-id':'emit_{}'.format(request_id), 'request-type':request_type}
        if data != None:
            for key in data.keys():
                if key == 'message-id':
                    continue
                requestpayload[key] = data[key]
        await self.clientSocket.send(json.dumps(requestpayload))

    def register(self, function, event=None):
        if inspect.iscoroutinefunction(function) == False:
            raise EventRegistrationError('Registered functions must be async')
        else:
            self.eventFunctions.append((function, event))

    def unregister(self, function, event=None):
        for c, t in self.eventFunctions:
            if (c == function) and (event == None or t == event):
                self.eventFunctions.remove((c, t))

    async def _websocketConnectionHandler(self, websocket, path):
        if self.clientSocket != None:
            await websocket.close(code=1000, reason='There is already an active rproxy-client connected.')
        self.clientSocket = websocket
        recvTask = self.loop.create_task(self._websocketRecvTask())
        authResponse = await self.call('GetAuthRequired')
        if authResponse['authRequired']:
            if self.password == None:
                await self.unload()
                raise Exception('A password is required by obs-websocket but was not provided')
            secret = base64.b64encode(hashlib.sha256((self.password + authResponse['salt']).encode('utf-8')).digest())
            auth = base64.b64encode(hashlib.sha256(secret + authResponse['challenge'].encode('utf-8')).digest()).decode('utf-8')
            authResult = await self.call('Authenticate', {'auth':auth})
            if authResult['status'] != 'ok':
                await self.unload()
                raise Exception('obs-websocket returned error to Authenticate request: {}'.format(authResult['error']))
        try:
            await recvTask
        except Exception as e:
            traceback.print_exc()
        self.clientSocket = None

    async def _websocketRecvTask(self):
        while self.clientSocket != None and self.clientSocket.open:
            message = ''
            try:
                message = await self.clientSocket.recv()
                if not message:
                    continue
                result = json.loads(message)
                if 'update-type' in result:
                    for callback, trigger in self.eventFunctions:
                        if trigger == None or trigger == result['update-type']:
                            self.loop.create_task(callback(result))
                elif 'message-id' in result:
                    if result['message-id'].startswith('emit_'): # We drop any responses to emit requests to not leak memory
                        continue
                    self.answers[result['message-id']] = result
                else:
                    print('Unknown message: {}'.format(result))
            except (websockets.exceptions.ConnectionClosed, websockets.exceptions.ConnectionClosedError, websockets.exceptions.ConnectionClosedOK, asyncio.exceptions.CancelledError):
                break