# 2025-12-12T16:53:20.363053871
import vitis

client = vitis.create_client()
client.set_workspace(path="RPU")

platform = client.get_component(name="platform")
status = platform.build()

status = platform.build()

comp = client.get_component(name="gpio_app")
comp.build()

status = comp.clean()

status = platform.build()

comp.build()

status = comp.clean()

status = platform.build()

status = platform.build()

comp.build()

status = comp.clean()

vitis.dispose()

