"""
ASYNC CALLBACK RACE | Python | Set
Criterion: A function passed as target= to Thread (or submitted to Executor)
           accesses the same global set as the calling endpoint.
           If both accesses without synchronization — Async Callback Race.
"""
from flask import Flask, jsonify, request
import threading
from concurrent.futures import ThreadPoolExecutor

app = Flask(__name__)

# ---------------------------------------------------------------------------
# MATCHES CRITERION
# ---------------------------------------------------------------------------

# --- no lock: both callback and endpoint modify same set ---
allowed_set = set()
feature_set = {"v1", "v2"}
active_set  = {"worker1", "worker2"}

def cb_set_add_user():
    allowed_set.add("cb_user")          # WRITE (in-place add) from callback

@app.route("/acr/set/no_lock/ww_add", methods=["POST"])
def acr_set_no_lock_ww_add():
    item = request.get_json().get("item", "main_user")
    t = threading.Thread(target=cb_set_add_user)
    t.start()
    allowed_set.add(item)               # WRITE (add) from endpoint — race
    t.join()
    return jsonify({"set": list(allowed_set)})

def cb_set_discard():
    allowed_set.discard("cb_user")      # WRITE (discard) from callback

@app.route("/acr/set/no_lock/ww_discard", methods=["POST"])
def acr_set_no_lock_ww_discard():
    item = request.get_json().get("item", "main_user")
    t = threading.Thread(target=cb_set_discard)
    t.start()
    allowed_set.discard(item)           # WRITE (discard) — race
    t.join()
    return jsonify({"set": list(allowed_set)})

def cb_set_replace():
    global feature_set
    feature_set = {"cb_feature"}        # WRITE (rebind) from callback

@app.route("/acr/set/no_lock/ww_replace", methods=["POST"])
def acr_set_no_lock_ww_replace():
    global feature_set
    t = threading.Thread(target=cb_set_replace)
    t.start()
    feature_set = set(request.get_json().get("items", ["main_feature"]))  # WRITE (rebind) — race
    t.join()
    return jsonify({"set": list(feature_set)})

def cb_set_clear():
    global active_set
    active_set = set()                  # WRITE (rebind to empty) from callback

@app.route("/acr/set/no_lock/ww_clear", methods=["POST"])
def acr_set_no_lock_ww_clear():
    global active_set
    t = threading.Thread(target=cb_set_clear)
    t.start()
    active_set = set(request.get_json().get("items", ["main_worker"]))  # WRITE — race
    t.join()
    return jsonify({"set": list(active_set)})


# --- no lock: RW — callback WRITE, endpoint reads snapshot ---
def cb_set_update():
    allowed_set.update(["cb_a", "cb_b"])    # WRITE (update) from callback

@app.route("/acr/set/no_lock/rw_snapshot", methods=["GET"])
def acr_set_no_lock_rw_snapshot():
    t = threading.Thread(target=cb_set_update)
    t.start()
    snapshot = set(allowed_set)         # READ snapshot — may see stale state
    t.join()
    return jsonify({"snapshot": list(snapshot), "final": list(allowed_set)})

def cb_feature_set_clear():
    global feature_set
    feature_set = set()                 # WRITE from callback

@app.route("/acr/set/no_lock/rw_size", methods=["GET"])
def acr_set_no_lock_rw_size():
    t = threading.Thread(target=cb_feature_set_clear)
    t.start()
    size = len(feature_set)             # READ from endpoint — may be stale
    t.join()
    return jsonify({"read_size": size, "final": list(feature_set)})


# --- no lock: explicit read-local-write (both rebind) ---
tags_set = {"tag1", "tag2"}

def cb_tags_rlw():
    global tags_set
    loc = set(tags_set)
    loc.add("cb_tag")
    tags_set = loc                      # WRITE back — lost update

@app.route("/acr/set/no_lock/rlw_add_a", methods=["POST"])
def acr_set_no_lock_rlw_add_a():
    global tags_set
    t = threading.Thread(target=cb_tags_rlw)
    t.start()
    loc = set(tags_set)
    loc.add(request.get_json().get("tag", "main_tag"))
    tags_set = loc                      # WRITE — race with callback
    t.join()
    return jsonify({"size": len(tags_set)})

@app.route("/acr/set/no_lock/rlw_add_b", methods=["POST"])
def acr_set_no_lock_rlw_add_b():
    global tags_set
    t = threading.Thread(target=cb_tags_rlw)
    t.start()
    loc = set(tags_set)
    loc.add(request.get_json().get("tag", "main_tag_b"))
    tags_set = loc                      # WRITE — lost update with rlw_add_a
    t.join()
    return jsonify({"size": len(tags_set)})


# --- Executor.submit() ---
exec_set = set()

def exec_cb_set_add():
    exec_set.add("executor_item")       # WRITE from executor task

@app.route("/acr/set/executor/ww_add", methods=["POST"])
def acr_set_executor_ww_add():
    with ThreadPoolExecutor(max_workers=1) as ex:
        future = ex.submit(exec_cb_set_add)
        exec_set.add(request.get_json().get("item", "main_item"))  # WRITE — race
        future.result()
    return jsonify({"set": list(exec_set)})

def exec_cb_set_replace():
    global exec_set
    exec_set = {"executor_replacement"}    # WRITE (rebind) from task

@app.route("/acr/set/executor/ww_replace", methods=["POST"])
def acr_set_executor_ww_replace():
    global exec_set
    with ThreadPoolExecutor(max_workers=1) as ex:
        future = ex.submit(exec_cb_set_replace)
        exec_set = set(request.get_json().get("items", ["main_replacement"]))  # WRITE — race
        future.result()
    return jsonify({"set": list(exec_set)})


# --- partial lock ---
partial_set = set()
set_lock    = threading.Lock()

def cb_partial_set_add():
    with set_lock:
        partial_set.add("cb_item")      # WRITE protected

@app.route("/acr/set/partial/ww_add", methods=["POST"])
def acr_set_partial_ww_add():
    t = threading.Thread(target=cb_partial_set_add)
    t.start()
    partial_set.add(request.get_json().get("item", "main_item"))    # WRITE without lock — race
    t.join()
    return jsonify({"set": list(partial_set)})

def cb_partial_set_replace():
    global partial_set
    with set_lock:
        partial_set = {"cb_locked"}     # WRITE (rebind) protected

@app.route("/acr/set/partial/ww_replace", methods=["POST"])
def acr_set_partial_ww_replace():
    global partial_set
    t = threading.Thread(target=cb_partial_set_replace)
    t.start()
    partial_set = set(request.get_json().get("items", ["main_unlocked"]))   # WRITE — race
    t.join()
    return jsonify({"set": list(partial_set)})


# --- different locks ---
diff_set = set()
lock_a   = threading.Lock()
lock_b   = threading.Lock()

def cb_diff_set_add():
    with lock_a:
        diff_set.add("lock_a_item")     # WRITE under lock_a

@app.route("/acr/set/diff_locks/ww_add", methods=["POST"])
def acr_set_diff_locks_ww_add():
    t = threading.Thread(target=cb_diff_set_add)
    t.start()
    with lock_b:
        diff_set.add(request.get_json().get("item", "lock_b_item"))  # WRITE under lock_b — race
    t.join()
    return jsonify({"set": list(diff_set)})

def cb_diff_set_replace():
    global diff_set
    with lock_a:
        diff_set = {"lock_a_replace"}   # WRITE (rebind) under lock_a

@app.route("/acr/set/diff_locks/ww_replace", methods=["POST"])
def acr_set_diff_locks_ww_replace():
    global diff_set
    t = threading.Thread(target=cb_diff_set_replace)
    t.start()
    with lock_b:
        diff_set = set(request.get_json().get("items", ["lock_b_replace"]))  # WRITE — lost update
    t.join()
    return jsonify({"set": list(diff_set)})


# --- Lock acquire/release ---
acq_set  = set()
acq_lock = threading.Lock()

def cb_acq_set_add():
    acq_lock.acquire()
    acq_set.add("acq_cb")              # WRITE protected
    acq_lock.release()

@app.route("/acr/set/acquire_release/ww_add", methods=["POST"])
def acr_set_acquire_release_ww_add():
    t = threading.Thread(target=cb_acq_set_add)
    t.start()
    acq_set.add(request.get_json().get("item", "acq_main"))     # WRITE without lock — race
    t.join()
    return jsonify({"set": list(acq_set)})

def cb_acq_set_discard():
    acq_lock.acquire()
    acq_set.discard("acq_cb")          # WRITE protected
    acq_lock.release()

@app.route("/acr/set/acquire_release/ww_discard", methods=["POST"])
def acr_set_acquire_release_ww_discard():
    t = threading.Thread(target=cb_acq_set_discard)
    t.start()
    acq_set.discard(request.get_json().get("item", "acq_main")) # WRITE without lock — race
    t.join()
    return jsonify({"set": list(acq_set)})


# --- RLock ---
rlock_set = set()
rlock     = threading.RLock()

def cb_rlock_set_add():
    with rlock:
        rlock_set.add("rlock_cb")      # WRITE protected

@app.route("/acr/set/rlock/ww_add", methods=["POST"])
def acr_set_rlock_ww_add():
    t = threading.Thread(target=cb_rlock_set_add)
    t.start()
    rlock_set.add(request.get_json().get("item", "rlock_main")) # WRITE without rlock — race
    t.join()
    return jsonify({"set": list(rlock_set)})

def cb_rlock_set_replace():
    global rlock_set
    with rlock:
        rlock_set = {"rlock_cb_replace"}    # WRITE (rebind) protected

@app.route("/acr/set/rlock/ww_replace", methods=["POST"])
def acr_set_rlock_ww_replace():
    global rlock_set
    t = threading.Thread(target=cb_rlock_set_replace)
    t.start()
    rlock_set = set(request.get_json().get("items", ["rlock_main_replace"]))    # WRITE — lost update
    t.join()
    return jsonify({"set": list(rlock_set)})


# --- Condition ---
cond_set  = set()
condition = threading.Condition()

def cb_cond_set_add():
    with condition:
        cond_set.add("cond_cb")
        condition.notify_all()          # WRITE inside condition

@app.route("/acr/set/condition/ww_add", methods=["POST"])
def acr_set_condition_ww_add():
    t = threading.Thread(target=cb_cond_set_add)
    t.start()
    cond_set.add(request.get_json().get("item", "cond_main"))    # WRITE without condition — race
    t.join()
    return jsonify({"set": list(cond_set)})

def cb_cond_set_replace():
    global cond_set
    with condition:
        cond_set = {"cond_cb_replace"}
        condition.notify_all()

@app.route("/acr/set/condition/ww_replace", methods=["POST"])
def acr_set_condition_ww_replace():
    global cond_set
    t = threading.Thread(target=cb_cond_set_replace)
    t.start()
    cond_set = set(request.get_json().get("items", ["cond_main_replace"]))  # WRITE — lost update
    t.join()
    return jsonify({"set": list(cond_set)})


# --- Semaphore ---
sem_set   = set()
semaphore = threading.Semaphore(1)

def cb_sem_set_add():
    semaphore.acquire()
    sem_set.add("sem_cb")               # WRITE protected
    semaphore.release()

@app.route("/acr/set/semaphore/ww_add", methods=["POST"])
def acr_set_semaphore_ww_add():
    t = threading.Thread(target=cb_sem_set_add)
    t.start()
    sem_set.add(request.get_json().get("item", "sem_main"))     # WRITE without semaphore — race
    t.join()
    return jsonify({"set": list(sem_set)})

@app.route("/acr/set/semaphore/ww_discard", methods=["POST"])
def acr_set_semaphore_ww_discard():
    t = threading.Thread(target=cb_sem_set_add)
    t.start()
    sem_set.discard(request.get_json().get("item", "sem_main")) # WRITE — race
    t.join()
    return jsonify({"set": list(sem_set)})


# --- BoundedSemaphore ---
bsem_set    = set()
bounded_sem = threading.BoundedSemaphore(1)

def cb_bsem_set_add():
    bounded_sem.acquire()
    bsem_set.add("bsem_cb")            # WRITE protected
    bounded_sem.release()

@app.route("/acr/set/bounded_sem/ww_add", methods=["POST"])
def acr_set_bounded_sem_ww_add():
    t = threading.Thread(target=cb_bsem_set_add)
    t.start()
    bsem_set.add(request.get_json().get("item", "bsem_main"))   # WRITE without bounded_sem — race
    t.join()
    return jsonify({"set": list(bsem_set)})

def cb_bsem_set_replace():
    global bsem_set
    bounded_sem.acquire()
    bsem_set = {"bsem_cb_replace"}     # WRITE (rebind) protected
    bounded_sem.release()

@app.route("/acr/set/bounded_sem/ww_replace", methods=["POST"])
def acr_set_bounded_sem_ww_replace():
    global bsem_set
    t = threading.Thread(target=cb_bsem_set_replace)
    t.start()
    bsem_set = set(request.get_json().get("items", ["bsem_main_replace"]))  # WRITE — lost update
    t.join()
    return jsonify({"set": list(bsem_set)})


# --- Event ---
event_set   = set()
ready_event = threading.Event()

def cb_event_set_add():
    event_set.add("event_cb")
    ready_event.set()

@app.route("/acr/set/event/ww_add", methods=["POST"])
def acr_set_event_ww_add():
    t = threading.Thread(target=cb_event_set_add)
    t.start()
    event_set.add(request.get_json().get("item", "event_main")) # WRITE — race
    t.join()
    return jsonify({"set": list(event_set)})

def cb_event_set_replace():
    global event_set
    event_set = {"event_cb_replace"}
    ready_event.set()

@app.route("/acr/set/event/ww_replace", methods=["POST"])
def acr_set_event_ww_replace():
    global event_set
    t = threading.Thread(target=cb_event_set_replace)
    t.start()
    event_set = set(request.get_json().get("items", ["event_main_replace"]))    # WRITE — lost update
    t.join()
    return jsonify({"set": list(event_set)})


# --- Aliasing ---
shared_set = {"x", "y"}
alias_set  = shared_set     # alias — same object

def cb_alias_set_add():
    shared_set.add("cb_x")             # WRITE via original (in-place)

@app.route("/acr/set/alias/ww_add_original", methods=["POST"])
def acr_set_alias_ww_add_original():
    t = threading.Thread(target=cb_alias_set_add)
    t.start()
    alias_set.add(request.get_json().get("item", "alias_main")) # WRITE via alias — race
    t.join()
    return jsonify({"set": list(shared_set)})

def cb_alias_set_rebind():
    global alias_set
    alias_set = {"cb_rebind"}           # WRITE (rebind alias) from callback

@app.route("/acr/set/alias/ww_rebind", methods=["POST"])
def acr_set_alias_ww_rebind():
    global shared_set
    t = threading.Thread(target=cb_alias_set_rebind)
    t.start()
    shared_set = set(request.get_json().get("items", ["main_rebind"]))  # WRITE (rebind) — race
    t.join()
    return jsonify({"shared": list(shared_set), "alias": list(alias_set)})


# --- class: via instance (outside) ---
class AccessControl:
    def __init__(self):
        self.allowed = set()
        self.blocked = set()
        self.roles   = {"user", "mod"}

acl       = AccessControl()
acl_alias = acl     # alias

def cb_acl_add_allowed():
    acl.allowed.add("cb_user")          # WRITE from callback

@app.route("/acr/set/class_outside/ww_allowed_add", methods=["POST"])
def acr_set_class_outside_ww_allowed_add():
    t = threading.Thread(target=cb_acl_add_allowed)
    t.start()
    acl.allowed.add(request.get_json().get("user", "main_user"))    # WRITE — race
    t.join()
    return jsonify({"allowed": list(acl.allowed)})

def cb_acl_replace_roles():
    acl.roles = {"cb_role"}             # WRITE (rebind) from callback

@app.route("/acr/set/class_outside/ww_roles_replace", methods=["POST"])
def acr_set_class_outside_ww_roles_replace():
    t = threading.Thread(target=cb_acl_replace_roles)
    t.start()
    acl.roles = set(request.get_json().get("roles", ["main_role"]))  # WRITE — lost update
    t.join()
    return jsonify({"roles": list(acl.roles)})

def cb_acl_alias_add_blocked():
    acl_alias.blocked.add("cb_blocked") # WRITE via alias

@app.route("/acr/set/class_alias/ww_blocked", methods=["POST"])
def acr_set_class_alias_ww_blocked():
    t = threading.Thread(target=cb_acl_alias_add_blocked)
    t.start()
    acl.blocked.add(request.get_json().get("ip", "main_blocked"))   # WRITE via original — race
    t.join()
    return jsonify({"blocked": list(acl.blocked)})


# --- class: inside methods ---
class AsyncSetManager:
    def __init__(self):
        self.tags    = set()
        self.visited = set()
        self.roles   = set()

    def _cb_add_tag(self, tag):
        self.tags.add(tag)              # WRITE from callback

    def _cb_replace_tags(self, new_tags):
        self.tags = set(new_tags)       # WRITE (replace) from callback

    def _cb_add_visited(self, item):
        self.visited.add(item)          # WRITE from callback

    def process_add(self, tag):
        t = threading.Thread(target=self._cb_add_tag, args=("cb_" + str(tag),))
        t.start()
        self.tags.add("main_" + str(tag))   # WRITE — race
        t.join()
        return list(self.tags)

    def process_replace(self, new_tags):
        t = threading.Thread(target=self._cb_replace_tags, args=(new_tags,))
        t.start()
        self.tags = {"main_replace_" + i for i in new_tags}  # WRITE — lost update
        t.join()
        return list(self.tags)

    def process_visited(self, item):
        t = threading.Thread(target=self._cb_add_visited, args=("cb_" + str(item),))
        t.start()
        self.visited.add("main_" + str(item))   # WRITE — race
        t.join()
        return list(self.visited)

set_mgr = AsyncSetManager()

@app.route("/acr/set/class_inside/add_tag", methods=["POST"])
def acr_set_class_inside_add_tag():
    tag = request.get_json().get("tag", "x")
    return jsonify({"tags": set_mgr.process_add(tag)})

@app.route("/acr/set/class_inside/replace_tags", methods=["POST"])
def acr_set_class_inside_replace_tags():
    tags = request.get_json().get("tags", ["a", "b"])
    return jsonify({"tags": set_mgr.process_replace(tags)})

@app.route("/acr/set/class_inside/add_visited", methods=["POST"])
def acr_set_class_inside_add_visited():
    item = request.get_json().get("item", "x")
    return jsonify({"visited": set_mgr.process_visited(item)})


# ---------------------------------------------------------------------------
# DETECTOR BLIND SPOTS
# ---------------------------------------------------------------------------

blind_set_a = set()
blind_set_b = set()

# --- blind spot 1: callback and endpoint access different sets ---
def cb_blind_write_set_b():
    blind_set_b.add("cb")               # WRITE to blind_set_b

@app.route("/acr/set/blind/different_vars", methods=["POST"])
def acr_set_blind_different_vars():
    t = threading.Thread(target=cb_blind_write_set_b)
    t.start()
    blind_set_a.add("main")             # WRITE to blind_set_a — different set — no race
    t.join()
    return jsonify({"a": list(blind_set_a), "b": list(blind_set_b)})


# --- blind spot 2: callback only reads (RR) ---
read_only_set = {"a", "b", "c"}

def cb_blind_read_only_set():
    local = set(read_only_set)          # READ — no write

@app.route("/acr/set/blind/rr_no_race", methods=["GET"])
def acr_set_blind_rr_no_race():
    t = threading.Thread(target=cb_blind_read_only_set)
    t.start()
    snapshot = set(read_only_set)       # READ — RR, not race
    t.join()
    return jsonify({"snapshot": list(snapshot)})


# --- blind spot 3: callback receives set copy as argument ---
arg_set = {"x", "y"}

def cb_blind_arg_set(s):
    s.add("cb_item")                    # modifies argument copy

@app.route("/acr/set/blind/arg_copy", methods=["POST"])
def acr_set_blind_arg_copy():
    global arg_set
    t = threading.Thread(target=cb_blind_arg_set, args=(set(arg_set),))  # passes copy
    t.start()
    arg_set = set()                     # WRITE to global — callback works on copy — no race
    t.join()
    return jsonify({"arg_set": list(arg_set)})


# --- blind spot 4: Thread not started ---
nostarter_set = set()

def cb_blind_nostarter_set():
    nostarter_set.add("never")          # WRITE — thread never starts

@app.route("/acr/set/blind/not_started", methods=["POST"])
def acr_set_blind_not_started():
    t = threading.Thread(target=cb_blind_nostarter_set)
    # t.start() — intentionally not called
    nostarter_set.add("main")
    return jsonify({"set": list(nostarter_set)})


# --- blind spot 5: join() before further access ---
join_set = set()

def cb_blind_join_set_write():
    join_set.add("cb_item")             # WRITE in callback

@app.route("/acr/set/blind/join_before_access", methods=["POST"])
def acr_set_blind_join_before_access():
    t = threading.Thread(target=cb_blind_join_set_write)
    t.start()
    t.join()                            # join() before access — no race
    snapshot = set(join_set)            # READ after join — safe
    return jsonify({"set": list(snapshot)})


# --- blind spot 6: both sides locked with shared lock ---
safe_set  = set()
safe_lock = threading.Lock()

def cb_blind_safe_set_write():
    global safe_set
    with safe_lock:
        safe_set = {"safe_cb"}          # WRITE under shared lock

@app.route("/acr/set/blind/both_locked", methods=["POST"])
def acr_set_blind_both_locked():
    global safe_set
    t = threading.Thread(target=cb_blind_safe_set_write)
    t.start()
    with safe_lock:
        safe_set = {"safe_main"}        # WRITE under same lock — correctly protected
    t.join()
    return jsonify({"set": list(safe_set)})


if __name__ == "__main__":
    app.run(debug=True, threaded=True)
