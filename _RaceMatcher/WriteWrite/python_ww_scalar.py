"""
WRITE-WRITE | Python | Scalar
Locks: no lock, partial, different locks, Lock, RLock, Condition
Class: via instance (outside) and inside methods
"""
from flask import Flask, jsonify, request
import threading

app = Flask(__name__)

# --- no lock ---
counter = 0

@app.route("/ww/scalar/no_lock/increment", methods=["POST"])
def ww_scalar_no_lock_increment():
    global counter
    counter = counter + 1
    return jsonify({"counter": counter})

@app.route("/ww/scalar/no_lock/reset", methods=["POST"])
def ww_scalar_no_lock_reset():
    global counter
    counter = 0
    return jsonify({"counter": counter})


# --- partial lock (one protected, one not) ---
balance = 1000.0
balance_lock = threading.Lock()

@app.route("/ww/scalar/partial/deposit", methods=["POST"])
def ww_scalar_partial_deposit():
    global balance
    with balance_lock:
        balance += request.get_json().get("amount", 0)
    return jsonify({"balance": balance})

@app.route("/ww/scalar/partial/adjust", methods=["POST"])
def ww_scalar_partial_adjust():
    global balance
    balance = request.get_json().get("amount", 0)   # no lock
    return jsonify({"balance": balance})


# --- different locks ---
score = 0
lock_a = threading.Lock()
lock_b = threading.Lock()

@app.route("/ww/scalar/diff_locks/add", methods=["POST"])
def ww_scalar_diff_locks_add():
    global score
    with lock_a:
        score += 1
    return jsonify({"score": score})

@app.route("/ww/scalar/diff_locks/subtract", methods=["POST"])
def ww_scalar_diff_locks_subtract():
    global score
    with lock_b:
        score -= 1
    return jsonify({"score": score})


# --- RLock ---
version = 0
rlock = threading.RLock()

@app.route("/ww/scalar/rlock/set", methods=["POST"])
def ww_scalar_rlock_set():
    global version
    with rlock:
        version = request.get_json().get("version", 0)
    return jsonify({"version": version})

@app.route("/ww/scalar/rlock/reset", methods=["POST"])
def ww_scalar_rlock_reset():
    global version
    version = 0     # no lock
    return jsonify({"version": version})


# --- Condition ---
data_ready = False
condition = threading.Condition()

@app.route("/ww/scalar/condition/set_ready", methods=["POST"])
def ww_scalar_condition_set_ready():
    global data_ready
    with condition:
        data_ready = True
        condition.notify_all()
    return jsonify({"ready": data_ready})

@app.route("/ww/scalar/condition/reset", methods=["POST"])
def ww_scalar_condition_reset():
    global data_ready
    data_ready = False      # no lock
    return jsonify({"ready": data_ready})


# --- class: via instance (outside) ---
class Config:
    def __init__(self):
        self.timeout = 30
        self.retries = 3

config = Config()

@app.route("/ww/scalar/class_outside/timeout/set", methods=["POST"])
def ww_scalar_class_outside_timeout_set():
    config.timeout = request.get_json().get("timeout", 30)
    return jsonify({"timeout": config.timeout})

@app.route("/ww/scalar/class_outside/timeout/reset", methods=["POST"])
def ww_scalar_class_outside_timeout_reset():
    config.timeout = 30
    return jsonify({"timeout": config.timeout})


# --- class: inside methods ---
class Counter:
    def __init__(self):
        self.value = 0

    def increment(self):
        self.value = self.value + 1

    def reset(self):
        self.value = 0

    def set(self, val: int):
        self.value = val

shared_counter = Counter()

@app.route("/ww/scalar/class_inside/increment", methods=["POST"])
def ww_scalar_class_inside_increment():
    shared_counter.increment()
    return jsonify({"value": shared_counter.value})

@app.route("/ww/scalar/class_inside/reset", methods=["POST"])
def ww_scalar_class_inside_reset():
    shared_counter.reset()
    return jsonify({"value": shared_counter.value})

@app.route("/ww/scalar/class_inside/set", methods=["POST"])
def ww_scalar_class_inside_set():
    shared_counter.set(request.get_json().get("value", 0))
    return jsonify({"value": shared_counter.value})


if __name__ == "__main__":
    app.run(debug=True, threaded=True)
