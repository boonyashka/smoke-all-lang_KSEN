"""
WRITE-WRITE | Python | Set
Locks: no lock, partial, different locks, Lock, RLock, Condition
Class: via instance (outside) and inside methods
"""
from flask import Flask, jsonify, request
import threading

app = Flask(__name__)

# --- no lock ---
active_users = set()

@app.route("/ww/set/no_lock/add", methods=["POST"])
def ww_set_no_lock_add():
    active_users.add(request.get_json().get("user"))
    return jsonify({"size": len(active_users)})

@app.route("/ww/set/no_lock/remove", methods=["POST"])
def ww_set_no_lock_remove():
    active_users.discard(request.get_json().get("user"))
    return jsonify({"size": len(active_users)})


# --- partial lock ---
banned_users = set()
ban_lock = threading.Lock()

@app.route("/ww/set/partial/ban", methods=["POST"])
def ww_set_partial_ban():
    with ban_lock:
        banned_users.add(request.get_json().get("user"))
    return jsonify({"size": len(banned_users)})

@app.route("/ww/set/partial/unban", methods=["POST"])
def ww_set_partial_unban():
    banned_users.discard(request.get_json().get("user"))    # no lock
    return jsonify({"size": len(banned_users)})


# --- different locks ---
subscribers = set()
lock_a = threading.Lock()
lock_b = threading.Lock()

@app.route("/ww/set/diff_locks/subscribe", methods=["POST"])
def ww_set_diff_locks_subscribe():
    with lock_a:
        subscribers.add(request.get_json().get("user"))
    return jsonify({"size": len(subscribers)})

@app.route("/ww/set/diff_locks/unsubscribe", methods=["POST"])
def ww_set_diff_locks_unsubscribe():
    with lock_b:
        subscribers.discard(request.get_json().get("user"))
    return jsonify({"size": len(subscribers)})


# --- RLock ---
online_users = set()
rlock = threading.RLock()

@app.route("/ww/set/rlock/connect", methods=["POST"])
def ww_set_rlock_connect():
    with rlock:
        online_users.add(request.get_json().get("user"))
    return jsonify({"size": len(online_users)})

@app.route("/ww/set/rlock/disconnect", methods=["POST"])
def ww_set_rlock_disconnect():
    online_users.discard(request.get_json().get("user"))    # no lock
    return jsonify({"size": len(online_users)})


# --- Condition ---
ready_workers = set()
condition = threading.Condition()

@app.route("/ww/set/condition/register", methods=["POST"])
def ww_set_condition_register():
    with condition:
        ready_workers.add(request.get_json().get("worker"))
        condition.notify_all()
    return jsonify({"size": len(ready_workers)})

@app.route("/ww/set/condition/deregister", methods=["POST"])
def ww_set_condition_deregister():
    ready_workers.discard(request.get_json().get("worker"))     # no lock
    return jsonify({"size": len(ready_workers)})


# --- class: via instance (outside) ---
class AccessControl:
    def __init__(self):
        self.allowed = set()
        self.denied = set()

acl = AccessControl()

@app.route("/ww/set/class_outside/allow", methods=["POST"])
def ww_set_class_outside_allow():
    acl.allowed.add(request.get_json().get("user"))
    return jsonify({"size": len(acl.allowed)})

@app.route("/ww/set/class_outside/deny", methods=["POST"])
def ww_set_class_outside_deny():
    acl.allowed.discard(request.get_json().get("user"))
    acl.denied.add(request.get_json().get("user"))
    return jsonify({"size": len(acl.denied)})


# --- class: inside methods ---
class TagRegistry:
    def __init__(self):
        self.tags = set()
        self.deprecated = set()

    def add_tag(self, tag: str):
        self.tags.add(tag)

    def remove_tag(self, tag: str):
        self.tags.discard(tag)
        self.deprecated.add(tag)

    def restore_tag(self, tag: str):
        self.deprecated.discard(tag)
        self.tags.add(tag)

    def clear_deprecated(self):
        self.deprecated.clear()

registry = TagRegistry()

@app.route("/ww/set/class_inside/add", methods=["POST"])
def ww_set_class_inside_add():
    registry.add_tag(request.get_json().get("tag"))
    return jsonify({"size": len(registry.tags)})

@app.route("/ww/set/class_inside/remove", methods=["POST"])
def ww_set_class_inside_remove():
    registry.remove_tag(request.get_json().get("tag"))
    return jsonify({"size": len(registry.tags)})

@app.route("/ww/set/class_inside/restore", methods=["POST"])
def ww_set_class_inside_restore():
    registry.restore_tag(request.get_json().get("tag"))
    return jsonify({"size": len(registry.tags)})


if __name__ == "__main__":
    app.run(debug=True, threaded=True)
