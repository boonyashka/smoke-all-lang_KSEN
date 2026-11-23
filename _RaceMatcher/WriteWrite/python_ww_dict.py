"""
WRITE-WRITE | Python | Dict
Locks: no lock, partial, different locks, Lock, RLock, Condition
Class: via instance (outside) and inside methods
"""
from flask import Flask, jsonify, request
import threading

app = Flask(__name__)

# --- no lock ---
sessions = {}

@app.route("/ww/dict/no_lock/create", methods=["POST"])
def ww_dict_no_lock_create():
    data = request.get_json()
    sessions[data["user"]] = data["token"]
    return jsonify({"ok": True})

@app.route("/ww/dict/no_lock/delete", methods=["POST"])
def ww_dict_no_lock_delete():
    sessions.pop(request.get_json().get("user"), None)
    return jsonify({"ok": True})


# --- partial lock ---
cache = {}
cache_lock = threading.Lock()

@app.route("/ww/dict/partial/set", methods=["POST"])
def ww_dict_partial_set():
    data = request.get_json()
    with cache_lock:
        cache[data["key"]] = data["value"]
    return jsonify({"ok": True})

@app.route("/ww/dict/partial/clear_key", methods=["POST"])
def ww_dict_partial_clear_key():
    cache.pop(request.get_json().get("key"), None)   # no lock
    return jsonify({"ok": True})


# --- different locks ---
rate_limits = {}
lock_a = threading.Lock()
lock_b = threading.Lock()

@app.route("/ww/dict/diff_locks/set", methods=["POST"])
def ww_dict_diff_locks_set():
    data = request.get_json()
    with lock_a:
        rate_limits[data["user"]] = data["limit"]
    return jsonify({"ok": True})

@app.route("/ww/dict/diff_locks/remove", methods=["POST"])
def ww_dict_diff_locks_remove():
    with lock_b:
        rate_limits.pop(request.get_json().get("user"), None)
    return jsonify({"ok": True})


# --- RLock ---
config_map = {}
rlock = threading.RLock()

@app.route("/ww/dict/rlock/set", methods=["POST"])
def ww_dict_rlock_set():
    data = request.get_json()
    with rlock:
        config_map[data["key"]] = data["value"]
    return jsonify({"ok": True})

@app.route("/ww/dict/rlock/update", methods=["POST"])
def ww_dict_rlock_update():
    data = request.get_json()
    config_map.update(data)     # no lock
    return jsonify({"ok": True})


# --- Condition ---
shared_state = {}
condition = threading.Condition()

@app.route("/ww/dict/condition/publish", methods=["POST"])
def ww_dict_condition_publish():
    data = request.get_json()
    with condition:
        shared_state[data["key"]] = data["value"]
        condition.notify_all()
    return jsonify({"ok": True})

@app.route("/ww/dict/condition/clear", methods=["POST"])
def ww_dict_condition_clear():
    shared_state.clear()    # no lock
    return jsonify({"ok": True})


# --- class: via instance (outside) ---
class UserStore:
    def __init__(self):
        self.users = {}
        self.metadata = {}

store = UserStore()

@app.route("/ww/dict/class_outside/users/add", methods=["POST"])
def ww_dict_class_outside_users_add():
    data = request.get_json()
    store.users[data["id"]] = data["name"]
    return jsonify({"size": len(store.users)})

@app.route("/ww/dict/class_outside/users/remove", methods=["POST"])
def ww_dict_class_outside_users_remove():
    store.users.pop(request.get_json().get("id"), None)
    return jsonify({"size": len(store.users)})


# --- class: inside methods ---
class SessionStore:
    def __init__(self):
        self.sessions = {}
        self.tokens = {}

    def create(self, user: str, token: str):
        self.sessions[user] = token
        self.tokens[token] = user

    def revoke(self, user: str):
        token = self.sessions.pop(user, None)
        if token:
            self.tokens.pop(token, None)

    def refresh(self, user: str, new_token: str):
        old_token = self.sessions.get(user)
        if old_token:
            self.tokens.pop(old_token, None)
        self.sessions[user] = new_token
        self.tokens[new_token] = user

session_store = SessionStore()

@app.route("/ww/dict/class_inside/create", methods=["POST"])
def ww_dict_class_inside_create():
    data = request.get_json()
    session_store.create(data["user"], data["token"])
    return jsonify({"ok": True})

@app.route("/ww/dict/class_inside/revoke", methods=["POST"])
def ww_dict_class_inside_revoke():
    session_store.revoke(request.get_json().get("user"))
    return jsonify({"ok": True})

@app.route("/ww/dict/class_inside/refresh", methods=["POST"])
def ww_dict_class_inside_refresh():
    data = request.get_json()
    session_store.refresh(data["user"], data["token"])
    return jsonify({"ok": True})


if __name__ == "__main__":
    app.run(debug=True, threaded=True)
