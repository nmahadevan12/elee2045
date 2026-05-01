"""
WebSocket server — Wit.ai STT + Gemini 3.1 Flash Lite LLM (Vertex AI Express).
"""
import json
import os
import urllib.request

from flask import Flask
from flask_sock import Sock
from google import genai

SAMPLE_RATE = 16000
WIT_TOKEN = os.environ["WIT_TOKEN"]

llm = genai.Client(vertexai=True, api_key=os.environ["VERTEX_API_KEY"])
LLM_MODEL = "gemini-3.1-flash-lite-preview"
SYSTEM_PROMPT = (
    "You are a helpful voice assistant on a tiny screen. "
    "Keep responses brief and concise, under 80 words."
)

app = Flask(__name__)
sock = Sock(app)


def parse_last_json(text):
    """Wit.ai may return multiple JSON objects; grab the last one."""
    decoder = json.JSONDecoder()
    last = None
    pos = 0
    text = text.strip()
    while pos < len(text):
        try:
            obj, end = decoder.raw_decode(text, pos)
            last = obj
            pos = end
            while pos < len(text) and text[pos] in " \t\n\r":
                pos += 1
        except json.JSONDecodeError:
            break
    return last or {}


def transcribe(audio_bytes):
    """Send raw PCM audio to Wit.ai and return transcription text."""
    req = urllib.request.Request(
        "https://api.wit.ai/speech?v=20240101",
        data=audio_bytes,
        headers={
            "Authorization": f"Bearer {WIT_TOKEN}",
            "Content-Type": "audio/raw;encoding=signed-integer;bits=16;rate=16000;endian=little",
        },
    )
    with urllib.request.urlopen(req, timeout=30) as resp:
        body = resp.read().decode()
    result = parse_last_json(body)
    return result.get("text", "")


@sock.route("/example_llm")
def ws_handler(ws):
    print("Client connected")
    audio_buffer = bytearray()
    recording = False

    while True:
        message = ws.receive()
        if message is None:
            break

        if isinstance(message, str):
            if message == "start":
                audio_buffer.clear()
                recording = True
                print("Recording started")

            elif message == "stop":
                recording = False
                duration = len(audio_buffer) / (SAMPLE_RATE * 2)
                print(f"Recording stopped: {duration:.1f}s ({len(audio_buffer)} bytes)")

                if len(audio_buffer) < SAMPLE_RATE:
                    ws.send("T:(too short)")
                    ws.send("D")
                    continue

                # Transcribe with Wit.ai
                print("Transcribing...")
                try:
                    text = transcribe(bytes(audio_buffer))
                except Exception as e:
                    print(f"Wit.ai error: {e}")
                    text = ""

                print(f"Transcription: '{text}'")
                ws.send(f"T:{text}" if text else "T:(no speech detected)")

                if not text:
                    ws.send("D")
                    continue

                # Stream LLM response
                print(f"Querying LLM: '{text}'")
                try:
                    for chunk in llm.models.generate_content_stream(
                        model=LLM_MODEL,
                        contents=text,
                        config=genai.types.GenerateContentConfig(
                            system_instruction=SYSTEM_PROMPT,
                        ),
                    ):
                        if chunk.text:
                            ws.send(f"R:{chunk.text}")
                except Exception as e:
                    print(f"LLM error: {e}")
                    ws.send(f"R:Error: {e}")

                ws.send("D")
                print("Done.\n")

        elif isinstance(message, bytes) and recording:
            audio_buffer.extend(message)

    print("Client disconnected")


if __name__ == "__main__":
    app.run(host="127.0.0.1", port=8765)
