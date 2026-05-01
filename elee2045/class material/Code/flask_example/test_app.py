import os
import tempfile
import pytest
from app import app, init_db, get_db

@pytest.fixture
def client():
    db_fd, db_path = tempfile.mkstemp()
    app.config['DATABASE'] = db_path
    app.config['TESTING'] = True

    with app.test_client() as client:
        with app.app_context():
            init_db()
        yield client

    os.close(db_fd)
    os.unlink(db_path)

def test_empty_dashboard(client):
    rv = client.get('/')
    assert b'No sensor data available' in rv.data

def test_report_and_dashboard(client):
    rv = client.post('/report', json={'mac_address': 'AA:BB:CC', 'noise_level': 55.5})
    assert rv.status_code == 201

    rv = client.get('/')
    assert b'AA:BB:CC' in rv.data
    assert b'55.5' in rv.data
