import asyncio
import json
import rproxy

async def on_event(data):
    print('New event! Type: {} | Raw Data:\n{}'.format(data['update-type'], json.dumps(data, indent=4)))

loop = asyncio.get_event_loop()
ws = rproxy.obswsProxyServer(loop=loop)
loop.run_until_complete(ws.load())
ws.register(on_event)
loop.run_forever()