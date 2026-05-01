import paho.mqtt.client as mqtt
from paho.mqtt.enums import CallbackAPIVersion
import time
import numpy as np
import struct
import ssl

BUTTON_TOPIC = "elee2045sp26/button/kjohnsen_arduino"
RANDOM_TOPIC = "elee2045sp26/random/kjohnsen_python"
TEXT_TOPIC = "elee2045sp26/text_topic/kjohnsen_python"
def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Connected")
        client.subscribe(topic=BUTTON_TOPIC)
    else:
        print(f"Failed to connect, reason_code = {reason_code}")
        exit(-1) 

def on_message(client, userdata, msg):
    if msg.topic == BUTTON_TOPIC:
        pressed, t = struct.unpack("<BL", msg.payload)
        print(f"Button {pressed} at: {t}")

# Use Version 2 of the callback API, with WebSocket transport
client = mqtt.Client(CallbackAPIVersion.VERSION2, transport="websockets")
client.username_pw_set(username="giiuser", password="giipassword") # Note, this is not a secure way to handle credentials, a better way would be to use environment variables or a configuration file that is not checked into version control.
client.tls_set(cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLS_CLIENT)
client.ws_set_options(path="/mqtt/")
client.on_connect = on_connect
client.on_message = on_message

print("Connecting to broker...")
client.connect(host="eduoracle.ugavel.com", port=443)
client.loop_start()

while True:
    if client.is_connected:
        to_send = struct.pack("<fL", np.random.rand(), int(time.time()))
        client.publish(RANDOM_TOPIC, to_send)
        to_send_text = f"Time: {time.time()}"
        client.publish(TEXT_TOPIC, to_send_text)
        print(f"Published: {to_send_text}")
    time.sleep(1)


    
