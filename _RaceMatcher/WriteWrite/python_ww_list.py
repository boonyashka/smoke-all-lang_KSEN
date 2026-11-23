"""
WRITE-WRITE | Python | List
Locks: no lock, partial, different locks, Lock, RLock, Condition
Class: via instance (outside) and inside methods
"""
from flask import Flask, jsonify, request
import threading

app = Flask(__name__)

# --- no lock ---
audit_log = []

@app.route("/ww/list/no_lock/add", methods=["POST"])
def ww_list_no_lock_add():
    audit_log.append(request.get_json().get("entry"))
    return jsonify({"size": len(audit_log)})

@app.route("/ww/list/no_lock/clear", methods=["POST"])
def ww_list_no_lock_clear():
    audit_log.clear()
    return jsonify({"size": 0})


# --- partial lock ---
task_queue = []
queue_lock = threading.Lock()

@app.route("/ww/list/partial/push", methods=["POST"])
def ww_list_partial_push():
    with queue_lock:
        task_queue.append(request.get_json().get("task"))
    return jsonify({"size": len(task_queue)})

@app.route("/ww/list/partial/pop", methods=["POST"])
def ww_list_partial_pop():
    if task_queue:
        task_queue.pop(0)   # no lock
    return jsonify({"size": len(task_queue)})


# --- different locks ---
events = []
lock_a = threading.Lock()
lock_b = threading.Lock()

@app.route("/ww/list/diff_locks/add", methods=["POST"])
def ww_list_diff_locks_add():
    with lock_a:
        events.append(request.get_json().get("event"))
    return jsonify({"size": len(events)})

@app.route("/ww/list/diff_locks/remove", methods=["POST"])
def ww_list_diff_locks_remove():
    with lock_b:
        idx = request.get_json().get("index", 0)
        if idx < len(events):
            events.pop(idx)
    return jsonify({"size": len(events)})


# --- RLock ---
notifications = []
rlock = threading.RLock()

@app.route("/ww/list/rlock/add", methods=["POST"])
def ww_list_rlock_add():
    with rlock:
        notifications.append(request.get_json().get("msg"))
    return jsonify({"size": len(notifications)})

@app.route("/ww/list/rlock/clear", methods=["POST"])
def ww_list_rlock_clear():
    notifications.clear()   # no lock
    return jsonify({"size": 0})


# --- Condition ---
job_results = []
condition = threading.Condition()

@app.route("/ww/list/condition/publish", methods=["POST"])
def ww_list_condition_publish():
    with condition:
        job_results.append(request.get_json().get("result"))
        condition.notify_all()
    return jsonify({"size": len(job_results)})

@app.route("/ww/list/condition/reset", methods=["POST"])
def ww_list_condition_reset():
    job_results.clear()     # no lock
    return jsonify({"size": 0})


# --- class: via instance (outside) ---
class Pipeline:
    def __init__(self):
        self.stages = []
        self.errors = []

pipeline = Pipeline()

@app.route("/ww/list/class_outside/stages/add", methods=["POST"])
def ww_list_class_outside_stages_add():
    pipeline.stages.append(request.get_json().get("stage"))
    return jsonify({"size": len(pipeline.stages)})

@app.route("/ww/list/class_outside/stages/clear", methods=["POST"])
def ww_list_class_outside_stages_clear():
    pipeline.stages.clear()
    return jsonify({"size": 0})


# --- class: inside methods ---
class MessageQueue:
    def __init__(self):
        self.messages = []
        self.dead_letter = []

    def enqueue(self, msg: str):
        self.messages.append(msg)

    def dequeue(self):
        if self.messages:
            return self.messages.pop(0)
        return None

    def reject(self, msg: str):
        self.messages.remove(msg)
        self.dead_letter.append(msg)

    def purge(self):
        self.dead_letter.clear()
        self.messages.clear()

mq = MessageQueue()

@app.route("/ww/list/class_inside/enqueue", methods=["POST"])
def ww_list_class_inside_enqueue():
    mq.enqueue(request.get_json().get("msg"))
    return jsonify({"size": len(mq.messages)})

@app.route("/ww/list/class_inside/dequeue", methods=["POST"])
def ww_list_class_inside_dequeue():
    msg = mq.dequeue()
    return jsonify({"msg": msg})

@app.route("/ww/list/class_inside/reject", methods=["POST"])
def ww_list_class_inside_reject():
    mq.reject(request.get_json().get("msg"))
    return jsonify({"size": len(mq.messages)})

@app.route("/ww/list/class_inside/purge", methods=["POST"])
def ww_list_class_inside_purge():
    mq.purge()
    return jsonify({"size": 0})


if __name__ == "__main__":
    app.run(debug=True, threaded=True)
