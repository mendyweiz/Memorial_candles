import socket
import threading
import json
import time
import os
from datetime import datetime, date
from flask import Flask, render_template, request, jsonify

# ================= CONFIG =================
UDP_PORT = 4210
BUF_SIZE = 1024
DB_FILE = "devices.json"

app = Flask(__name__)
lock = threading.Lock()

# ================= DB =================
def load_db():
    if not os.path.exists(DB_FILE):
        return {}
    with open(DB_FILE, "r", encoding="utf-8") as f:
        return json.load(f)

def save_db(data):
    tmp = DB_FILE + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    os.replace(tmp, DB_FILE)

devices = load_db()

# ================= DATE LOGIC =================
def is_active_today(device):
    today = date.today()

    for key in ("birth_date", "passing_date"):
        d = device.get(key)
        if not d:
            continue
        try:
            if datetime.strptime(d, "%Y-%m-%d").date() == today:
                return True
        except ValueError:
            continue

    return False

# ================= UDP =================
def send_udp(device_id, cmd):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    msg = f"{device_id},{cmd}"
    sock.sendto(msg.encode(), ("255.255.255.255", UDP_PORT))
    sock.close()

def udp_listener():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("", UDP_PORT))

    while True:
        data, _ = sock.recvfrom(BUF_SIZE)
        msg = data.decode(errors="ignore").strip()
        if "," not in msg:
            continue

        device_id, state = msg.split(",", 1)
        now = int(time.time())

        with lock:
            if device_id not in devices:
                devices[device_id] = {
                    "birth_date": "",
                    "passing_date": "",
                    "last_state": state,
                    "last_seen": now
                }
            else:
                devices[device_id]["last_state"] = state
                devices[device_id]["last_seen"] = now

            save_db(devices)

# ================= SCHEDULER =================
def memorial_scheduler():
    while True:
        with lock:
            for device_id, d in devices.items():
                active = is_active_today(d)

                if active and d["last_state"] != "ON":
                    send_udp(device_id, "ON")
                    d["last_state"] = "ON"
                    save_db(devices)

                if not active and d["last_state"] == "ON":
                    send_udp(device_id, "OFF")
                    d["last_state"] = "OFF"
                    save_db(devices)

        time.sleep(60)

# ================= ROUTES =================
@app.route("/")
def index():
    return render_template("index.html")

@app.route("/devices")
def get_devices():
    with lock:
        return jsonify(devices)

@app.route("/update", methods=["POST"])
def update_device():
    data = request.json
    device_id = data.get("device_id")

    if not device_id:
        return {"error": "missing device_id"}, 400

    with lock:
        if device_id not in devices:
            return {"error": "unknown device"}, 400

        if "birth_date" in data:
            devices[device_id]["birth_date"] = data["birth_date"]
        if "passing_date" in data:
            devices[device_id]["passing_date"] = data["passing_date"]

        save_db(devices)

    return {"ok": True}

@app.route("/control", methods=["POST"])
def control():
    data = request.json
    device_id = data.get("device_id")
    cmd = data.get("cmd")

    if not device_id or cmd not in ("ON", "OFF"):
        return {"error": "bad request"}, 400

    send_udp(device_id, cmd)

    with lock:
        if device_id in devices:
            devices[device_id]["last_state"] = cmd
            save_db(devices)

    return {"ok": True}

# ================= MAIN =================
if __name__ == "__main__":
    threading.Thread(target=udp_listener, daemon=True).start()
    threading.Thread(target=memorial_scheduler, daemon=True).start()
    app.run(host="0.0.0.0", port=5000)
