import struct, time, requests
from flask import Flask, render_template_string, redirect, url_for
import paho.mqtt.client as mqtt
from paho.mqtt.enums import CallbackAPIVersion

# --- CONFIG ---
WEBHOOK = "https://discord.com/api/webhooks/1482193637134700586/WIsO81agY0Yd-GvSOsVOkvkDFeV4uwsNZWyYkZ53SjDNWdNa5DQ6Uqh_6LS61W7c6X3S"
TOPIC = "elee2045sp26/nm25652/"
MODES = {0: "OFF", 1: "HOME", 2: "AWAY"}

state = {"mode": "OFF", "door": "CLOSED", "last_updated": "Never", "last_ts": 0}

def notify(msg):
    try: requests.post(WEBHOOK, json={"content": f"🔔 {msg}"})
    except: pass

def on_message(client, userdata, msg):
    d = struct.unpack('<BB', msg.payload[:2])
    new_mode, new_door = MODES.get(d[0], "??"), "OPEN" if d[1] else "CLOSED"
    
    # Notify on Mode Change
    if new_mode != state["mode"]:
        notify(f"System mode changed to **{new_mode}**")

    # Notify on Door Change
    if new_door != state["door"]:
        notify(f"Door is now **{new_door}** (System: {new_mode})")
    
    state.update({"mode": new_mode, "door": new_door, "last_updated": time.strftime('%H:%M:%S'), "last_ts": time.time()})

# --- MQTT SETUP ---
client = mqtt.Client(CallbackAPIVersion.VERSION2)
client.username_pw_set("giiuser", "giipassword")
client.on_connect = lambda c, u, f, r, p: c.subscribe(TOPIC + "+")
client.on_message = on_message
client.connect("eduoracle.ugavel.com", 1883)
client.loop_start()

# --- WEB SERVER ---
app = Flask(__name__)

@app.route('/')
def index():
    online = state["last_ts"] > 0 and (time.time() - state["last_ts"] < 1800)
    status, color = ("ONLINE", "online") if online else ("OFFLINE", "offline")
    return render_template_string(HTML_TEMPLATE, data=state, conn_status=status, conn_class=color)

@app.route('/set_mode/<int:mid>')
def set_mode(mid):
    # This triggers the notification via the on_message callback once the broker confirms
    client.publish(TOPIC + "mode", struct.pack('<BBL', mid, 0, 0))
    return redirect('/')

HTML_TEMPLATE = """
<!DOCTYPE html><html><head><title>Door Monitor</title><meta http-equiv="refresh" content="5">
<style>
    body { font-family: sans-serif; text-align: center; background: #1a1a1a; color: white; padding-top: 50px; }
    .card { background: #333; padding: 30px; border-radius: 15px; display: inline-block; min-width: 300px; }
    .status { font-size: 3em; font-weight: bold; color: #00ff00; }
    .open { color: #ff4444; }
    .online { color: #28a745; } .offline { color: #dc3545; }
    .btn { padding: 10px; margin: 5px; cursor: pointer; border: none; border-radius: 5px; font-weight: bold; color: white; background: #555; }
</style></head>
<body><div class="card">
    <div class="{{ conn_class }}">● {{ conn_status }}</div>
    <p>Current Mode: <b>{{ data.mode }}</b></p>
    <div class="status {{ 'open' if data.door == 'OPEN' else '' }}">{{ data.door }}</div>
    <p><small>Last Update: {{ data.last_updated }}</small></p>
    <a href="/set_mode/0"><button class="btn">OFF</button></a>
    <a href="/set_mode/1"><button class="btn" style="background:#28a745">HOME</button></a>
    <a href="/set_mode/2"><button class="btn" style="background:#007bff">AWAY</button></a>
</div></body></html>
"""

if __name__ == '__main__':
    app.run(port=5000)