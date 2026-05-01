from flask import Flask, request, jsonify, render_template, g
import sqlite3
import os

app = Flask(__name__)
# Allow overriding db path for testing
app.config['DATABASE'] = os.environ.get('DATABASE_URL', 'sensors.db')

def get_db():
    db = getattr(g, '_database', None)
    if db is None:
        db = g._database = sqlite3.connect(app.config['DATABASE'])
        db.row_factory = sqlite3.Row
    return db

@app.teardown_appcontext
def close_connection(exception):
    db = getattr(g, '_database', None)
    if db is not None:
        db.close()

def init_db():
    with app.app_context():
        db = get_db()
        db.execute('''
            CREATE TABLE IF NOT EXISTS sensor_data (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                mac_address TEXT NOT NULL,
                noise_level REAL NOT NULL,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        db.commit()

# Initialize db on startup
init_db()

@app.route("/")
def dashboard():
    db = get_db()
    # Get the latest report for each sensor
    cursor = db.execute('''
        SELECT mac_address, noise_level, MAX(timestamp) as last_seen
        FROM sensor_data
        GROUP BY mac_address
        ORDER BY last_seen DESC
    ''')
    sensors = cursor.fetchall()
    return render_template('dashboard.html', sensors=sensors)

@app.route("/api/sensors")
def api_sensors():
    db = get_db()
    cursor = db.execute('''
        SELECT mac_address, noise_level, MAX(timestamp) as last_seen
        FROM sensor_data
        GROUP BY mac_address
        ORDER BY last_seen DESC
    ''')
    sensors = [dict(row) for row in cursor.fetchall()]
    return jsonify(sensors)

@app.route("/report", methods=['POST'])
def report_data():
    data = request.get_json()
    if not data or 'mac_address' not in data or 'noise_level' not in data:
        return jsonify({'error': 'Missing mac_address or noise_level'}), 400

    mac_address = data['mac_address']
    try:
        noise_level = float(data['noise_level'])
    except ValueError:
        return jsonify({'error': 'noise_level must be a number'}), 400

    db = get_db()
    db.execute('INSERT INTO sensor_data (mac_address, noise_level) VALUES (?, ?)',
               (mac_address, noise_level))
    db.commit()

    return jsonify({'status': 'success'}), 201

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
