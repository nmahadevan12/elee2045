# Standard libraries
import os
import secrets
import sqlite3
import uuid
from pathlib import Path

# Flask imports for web app functionality
from flask import Flask, jsonify, redirect, render_template, request, send_file, session, url_for

# Optional Whisper import (for speech-to-text)
try:
    import whisper
except Exception:
    whisper = None  # If not installed, disable transcription

# Initialize Flask app
app = Flask(__name__)

# Secret key for session management (use env var in production)
app.secret_key = os.getenv("FLASK_SECRET_KEY", "dev-secret")

# Database file
DB = "assistant.db"

# Directory for storing uploaded audio files
AUDIO_DIR = Path("audio_uploads").resolve()
AUDIO_DIR.mkdir(exist_ok=True)

# Configurable environment variables
DEVICE_API_KEY = os.getenv("DEVICE_API_KEY", "device-key")  # For device authentication
DASHBOARD_PASSWORD = os.getenv("DASHBOARD_PASSWORD", "dashboard-pass")  # Dashboard login
WHISPER_MODEL_NAME = os.getenv("WHISPER_MODEL", "base")  # Whisper model size

# Cache for Whisper model (avoid reloading)
_whisper_model = None


# Create database connection
def db():
    conn = sqlite3.connect(DB)
    conn.row_factory = sqlite3.Row  # Access rows like dictionaries
    return conn


# Initialize database tables (if they don't exist)
def init_db():
    with db() as conn:
        # Todos table
        conn.execute(
            "CREATE TABLE IF NOT EXISTS todos (id INTEGER PRIMARY KEY, text TEXT, done INTEGER DEFAULT 0, pinned INTEGER DEFAULT 0)"
        )

        # Notes table (stores transcripts + audio)
        conn.execute(
            "CREATE TABLE IF NOT EXISTS notes (id INTEGER PRIMARY KEY, transcript TEXT, summary TEXT, audio_path TEXT)"
        )

        # Ensure 'pinned' column exists (for older DB versions)
        cols = [r["name"] for r in conn.execute("PRAGMA table_info(todos)").fetchall()]
        if "pinned" not in cols:
            conn.execute("ALTER TABLE todos ADD COLUMN pinned INTEGER DEFAULT 0")

        # Ensure 'summary' column exists
        note_cols = [r["name"] for r in conn.execute("PRAGMA table_info(notes)").fetchall()]
        if "summary" not in note_cols:
            conn.execute("ALTER TABLE notes ADD COLUMN summary TEXT")


# Check if user is authenticated (session-based)
def authed():
    return bool(session.get("authed"))


# Validate device API key (for external device access)
def require_device():
    key = request.headers.get("X-API-Key", "")
    return secrets.compare_digest(key, DEVICE_API_KEY)


# Load Whisper model once and reuse it
def get_whisper_model():
    global _whisper_model
    if whisper is None:
        return None
    if _whisper_model is None:
        _whisper_model = whisper.load_model(WHISPER_MODEL_NAME)
    return _whisper_model


# Transcribe audio file → text
def transcribe(path: Path) -> str:
    model = get_whisper_model()
    if model is None:
        return "Whisper not installed. Install requirements and ffmpeg."
    try:
        out = model.transcribe(str(path))
        text = (out.get("text") or "").strip()
        return text or "No speech detected."
    except Exception as e:
        return f"Transcription failed: {e}"


# Simple summarization (first ~14 words)
def summarize(text: str) -> str:
    words = [w for w in text.replace("\n", " ").split(" ") if w]
    short = " ".join(words[:14]).strip()
    if not short:
        return "(no summary)"
    return short + ("..." if len(words) > 14 else "")


# Main dashboard route
@app.route("/", methods=["GET", "POST"])
def index():
    if request.method == "POST":
        action = request.form.get("action", "")

        # Handle login
        if action == "login":
            session["authed"] = request.form.get("password", "") == DASHBOARD_PASSWORD
            return redirect(url_for("index"))

        # Block unauthenticated actions
        if not authed():
            return redirect(url_for("index"))

        # Perform DB actions
        with db() as conn:
            if action == "add_todo":
                text = request.form.get("text", "").strip()
                if text:
                    conn.execute("INSERT INTO todos (text, done, pinned) VALUES (?, 0, 0)", (text,))

            elif action == "edit_todo":
                conn.execute(
                    "UPDATE todos SET text = ? WHERE id = ?",
                    (request.form.get("text", "").strip(), request.form.get("todo_id")),
                )

            elif action == "delete_todo":
                conn.execute("DELETE FROM todos WHERE id = ?", (request.form.get("todo_id"),))

            elif action == "add_note":
                transcript = request.form.get("transcript", "").strip()
                summary = request.form.get("summary", "").strip()
                if transcript:
                    conn.execute(
                        "INSERT INTO notes (transcript, summary, audio_path) VALUES (?, ?, ?)",
                        (transcript, summary or summarize(transcript), ""),
                    )

            elif action == "edit_note":
                transcript = request.form.get("transcript", "").strip()
                summary = request.form.get("summary", "").strip()
                note_id = request.form.get("note_id")
                if transcript and note_id:
                    conn.execute(
                        "UPDATE notes SET transcript = ?, summary = ? WHERE id = ?",
                        (transcript, summary, note_id),
                    )

            elif action == "delete_note":
                note_id = request.form.get("note_id")

                # Remove audio file from disk if it exists
                row = conn.execute("SELECT audio_path FROM notes WHERE id = ?", (note_id,)).fetchone()
                if row:
                    try:
                        if row["audio_path"]:
                            os.remove(row["audio_path"])
                    except OSError:
                        pass
                    conn.execute("DELETE FROM notes WHERE id = ?", (note_id,))

        return redirect(url_for("index"))

    # GET request → load dashboard data
    if authed():
        with db() as conn:
            # Sort: pinned first → undone → then by ID
            todos = conn.execute(
                "SELECT * FROM todos ORDER BY pinned DESC, done ASC, id ASC"
            ).fetchall()

            # Latest notes first
            notes = conn.execute("SELECT * FROM notes ORDER BY id DESC").fetchall()
    else:
        todos, notes = [], []

    return render_template("index.html", authed=authed(), todos=todos, notes=notes)


# Logout route (clears session)
@app.route("/logout")
def logout():
    session.clear()
    return redirect(url_for("index"))


# API: Get todos (device access)
@app.route("/api/todos")
def api_todos():
    if not require_device():
        return jsonify({"error": "unauthorized"}), 401

    with db() as conn:
        rows = conn.execute(
            "SELECT id, text, done, pinned FROM todos ORDER BY pinned DESC, done ASC, id ASC"
        ).fetchall()

    return jsonify([
        {"id": r["id"], "text": r["text"], "done": bool(r["done"]), "pinned": bool(r["pinned"])}
        for r in rows
    ])


# API: Mark todo as done
@app.route("/api/todos/<int:todo_id>/done", methods=["POST"])
def api_todo_done(todo_id):
    if not require_device():
        return jsonify({"error": "unauthorized"}), 401

    with db() as conn:
        conn.execute("UPDATE todos SET done = 1 WHERE id = ?", (todo_id,))

    return jsonify({"ok": True})


# API: Delete todo
@app.route("/api/todos/<int:todo_id>", methods=["DELETE"])
def api_todo_delete(todo_id):
    if not require_device():
        return jsonify({"error": "unauthorized"}), 401

    with db() as conn:
        conn.execute("DELETE FROM todos WHERE id = ?", (todo_id,))

    return jsonify({"ok": True})


# API: Toggle pin/unpin (dashboard only)
@app.route("/api/todos/<int:todo_id>/pin", methods=["POST"])
def api_todo_pin(todo_id):
    if not authed():
        return jsonify({"error": "unauthorized"}), 401

    with db() as conn:
        row = conn.execute("SELECT pinned FROM todos WHERE id = ?", (todo_id,)).fetchone()
        if not row:
            return jsonify({"error": "not found"}), 404

        # Toggle pinned state
        new_pinned = 0 if row["pinned"] else 1
        conn.execute("UPDATE todos SET pinned = ? WHERE id = ?", (new_pinned, todo_id))

    return jsonify({"ok": True, "pinned": bool(new_pinned)})


# API: Upload audio and create note
@app.route("/api/audio", methods=["POST"])
def api_audio():
    if not require_device():
        return jsonify({"error": "unauthorized"}), 401

    f = request.files.get("audio")
    if not f:
        return jsonify({"error": "missing audio"}), 400

    # Save audio with unique filename
    filename = f"{uuid.uuid4().hex}.wav"
    path = AUDIO_DIR / filename
    f.save(path)

    # Reject tiny (likely invalid) files
    file_size = path.stat().st_size
    print(f"Received audio: {file_size} bytes at {path}")
    if file_size < 1000:
        path.unlink(missing_ok=True)
        return jsonify({"error": "audio too small"}), 400

    # Transcribe + summarize
    transcript = transcribe(path)
    summary = summarize(transcript)

    # Store in DB
    with db() as conn:
        cur = conn.execute(
            "INSERT INTO notes (transcript, summary, audio_path) VALUES (?, ?, ?)",
            (transcript, summary, str(path)),
        )

    return jsonify({"id": cur.lastrowid, "transcript": transcript, "summary": summary})


# Serve audio file
@app.route("/audio/<int:note_id>")
def audio(note_id):
    if not authed():
        return ("Unauthorized", 401)

    with db() as conn:
        row = conn.execute("SELECT audio_path FROM notes WHERE id = ?", (note_id,)).fetchone()

    if not row or not row["audio_path"]:
        return ("No audio on file", 404)

    path = Path(row["audio_path"])

    if not path.exists():
        print(f"Audio file missing from disk: {path}")
        return ("Audio file missing from disk", 404)

    return send_file(
        path,
        mimetype="audio/wav",
        conditional=True,
        download_name=f"note_{note_id}.wav"
    )


# Admin: Clean up broken audio paths
@app.route("/admin/cleanup")
def cleanup():
    if not authed():
        return ("Unauthorized", 401)

    with db() as conn:
        notes = conn.execute("SELECT id, audio_path FROM notes").fetchall()
        cleaned = 0

        for note in notes:
            if note["audio_path"] and not Path(note["audio_path"]).exists():
                conn.execute("UPDATE notes SET audio_path = '' WHERE id = ?", (note["id"],))
                cleaned += 1

    return jsonify({"cleaned": cleaned})


# Health check endpoint
@app.route("/health")
def health():
    return {"ok": True}


# Run app
if __name__ == "__main__":
    init_db()  # Ensure DB is ready
    app.run(host="0.0.0.0", port=5001, debug=True)
