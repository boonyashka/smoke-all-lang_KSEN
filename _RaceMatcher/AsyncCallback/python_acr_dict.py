"""
ASYNC CALLBACK RACE | Python | Dict
Criterion: A function passed as target= to Thread (or submitted to Executor)
           accesses the same global dict as the calling endpoint.
           If both accesses without synchronization — Async Callback Race.
"""
from flask import Flask, jsonify, request
import threading
from concurrent.futures import ThreadPoolExecutor

app = Flask(__name__)

# ---------------------------------------------------------------------------
# MATCHES CRITERION
# ---------------------------------------------------------------------------

# --- no lock: both callback and endpoint modify same dict ---
config_dict  = {"timeout": 30, "retries": 3}
metrics_dict = {"requests": 0, "errors": 0}
state_dict   = {}

def cb_dict_update_config():
    global config_dict
    config_dict = {"timeout": 99, "retries": 0, "source": "callback"}  # WRITE (rebind)

@app.route("/acr/dict/no_lock/ww_replace", methods=["POST"])
def acr_dict_no_lock_ww_replace():
    global config_dict
    t = threading.Thread(target=cb_dict_update_config)
    t.start()
    config_dict = request.get_json()    # WRITE (rebind) from endpoint — race
    t.join()
    return jsonify({"config": config_dict})

def cb_dict_inc_requests():
    metrics_dict["requests"] += 1       # WRITE single key from callback

@app.route("/acr/dict/no_lock/ww_key_inc", methods=["POST"])
def acr_dict_no_lock_ww_key_inc():
    t = threading.Thread(target=cb_dict_inc_requests)
    t.start()
    metrics_dict["requests"] += 1       # WRITE same key from endpoint — race
    t.join()
    return jsonify({"metrics": metrics_dict})

def cb_dict_inc_errors():
    metrics_dict["errors"] += 1         # WRITE different key from callback

@app.route("/acr/dict/no_lock/ww_diff_keys", methods=["POST"])
def acr_dict_no_lock_ww_diff_keys():
    t = threading.Thread(target=cb_dict_inc_errors)
    t.start()
    metrics_dict["requests"] += 1       # WRITE different key — concurrent dict modification
    t.join()
    return jsonify({"metrics": metrics_dict})

def cb_dict_set_status():
    state_dict["status"] = "cb_done"    # WRITE key from callback

@app.route("/acr/dict/no_lock/ww_same_key", methods=["POST"])
def acr_dict_no_lock_ww_same_key():
    t = threading.Thread(target=cb_dict_set_status)
    t.start()
    state_dict["status"] = "main_done"  # WRITE same key — race
    t.join()
    return jsonify({"state": state_dict})

def cb_dict_update_multiple():
    config_dict.update({"debug": True, "source": "cb"})    # WRITE multiple keys

@app.route("/acr/dict/no_lock/ww_update", methods=["POST"])
def acr_dict_no_lock_ww_update():
    t = threading.Thread(target=cb_dict_update_multiple)
    t.start()
    config_dict.update(request.get_json())  # WRITE — race
    t.join()
    return jsonify({"config": config_dict})


# --- no lock: RW — callback WRITE, endpoint reads snapshot ---
app_config = {"debug": False, "max_conn": 10}

def cb_dict_reload_config():
    global app_config
    app_config = {"debug": True, "max_conn": 50}    # WRITE from callback

@app.route("/acr/dict/no_lock/rw_config", methods=["GET"])
def acr_dict_no_lock_rw_config():
    t = threading.Thread(target=cb_dict_reload_config)
    t.start()
    snapshot = dict(app_config)         # READ snapshot — may see stale state
    t.join()
    return jsonify({"snapshot": snapshot, "final": app_config})

def cb_dict_clear_metrics():
    global metrics_dict
    metrics_dict = {"requests": 0, "errors": 0}    # WRITE from callback

@app.route("/acr/dict/no_lock/rw_metrics", methods=["GET"])
def acr_dict_no_lock_rw_metrics():
    t = threading.Thread(target=cb_dict_clear_metrics)
    t.start()
    size = len(metrics_dict)            # READ from endpoint — may be stale
    t.join()
    return jsonify({"read_size": size, "final": metrics_dict})


# --- no lock: explicit read-local-write in both ---
registry_dict = {"key1": "val1"}

def cb_registry_rlw():
    global registry_dict
    loc = dict(registry_dict)
    loc["cb_key"] = "cb_val"
    registry_dict = loc                 # WRITE back — lost update

@app.route("/acr/dict/no_lock/rlw_put_a", methods=["POST"])
def acr_dict_no_lock_rlw_put_a():
    global registry_dict
    t = threading.Thread(target=cb_registry_rlw)
    t.start()
    data = request.get_json()
    loc = dict(registry_dict)
    loc.update(data)
    registry_dict = loc                 # WRITE — race with callback
    t.join()
    return jsonify({"size": len(registry_dict)})

@app.route("/acr/dict/no_lock/rlw_put_b", methods=["POST"])
def acr_dict_no_lock_rlw_put_b():
    global registry_dict
    t = threading.Thread(target=cb_registry_rlw)
    t.start()
    data = request.get_json()
    loc = dict(registry_dict)
    loc.update(data)
    registry_dict = loc                 # WRITE — lost update with rlw_put_a
    t.join()
    return jsonify({"size": len(registry_dict)})


# --- Executor.submit() ---
exec_dict = {}

def exec_cb_dict_write():
    exec_dict["source"] = "executor"    # WRITE from executor task

@app.route("/acr/dict/executor/ww_key", methods=["POST"])
def acr_dict_executor_ww_key():
    with ThreadPoolExecutor(max_workers=1) as ex:
        future = ex.submit(exec_cb_dict_write)
        exec_dict["source"] = "main"    # WRITE same key from endpoint — race
        future.result()
    return jsonify({"dict": exec_dict})

def exec_cb_dict_replace():
    global exec_dict
    exec_dict = {"from": "executor", "ts": 999}    # WRITE (rebind) from task

@app.route("/acr/dict/executor/ww_replace", methods=["POST"])
def acr_dict_executor_ww_replace():
    global exec_dict
    with ThreadPoolExecutor(max_workers=1) as ex:
        future = ex.submit(exec_cb_dict_replace)
        exec_dict = request.get_json()  # WRITE (rebind) — race
        future.result()
    return jsonify({"dict": exec_dict})


# --- partial lock ---
partial_dict = {}
dict_lock    = threading.Lock()

def cb_partial_dict_update():
    with dict_lock:
        partial_dict["source"] = "cb_locked"    # WRITE protected

@app.route("/acr/dict/partial/ww_key", methods=["POST"])
def acr_dict_partial_ww_key():
    t = threading.Thread(target=cb_partial_dict_update)
    t.start()
    partial_dict["source"] = "main_unlocked"    # WRITE without lock — race
    t.join()
    return jsonify({"dict": partial_dict})

def cb_partial_dict_replace():
    global partial_dict
    with dict_lock:
        partial_dict = {"source": "cb_locked"}  # WRITE (rebind) protected

@app.route("/acr/dict/partial/ww_replace", methods=["POST"])
def acr_dict_partial_ww_replace():
    global partial_dict
    t = threading.Thread(target=cb_partial_dict_replace)
    t.start()
    partial_dict = request.get_json()   # WRITE without lock — race
    t.join()
    return jsonify({"dict": partial_dict})


# --- different locks ---
diff_dict = {}
lock_a    = threading.Lock()
lock_b    = threading.Lock()

def cb_diff_dict_write():
    with lock_a:
        diff_dict["key"] = "lock_a"     # WRITE under lock_a

@app.route("/acr/dict/diff_locks/ww_key", methods=["POST"])
def acr_dict_diff_locks_ww_key():
    t = threading.Thread(target=cb_diff_dict_write)
    t.start()
    with lock_b:
        diff_dict["key"] = "lock_b"     # WRITE under lock_b — different lock, race
    t.join()
    return jsonify({"dict": diff_dict})

def cb_diff_dict_replace():
    global diff_dict
    with lock_a:
        diff_dict = {"source": "lock_a_replace"}    # WRITE (rebind) under lock_a

@app.route("/acr/dict/diff_locks/ww_replace", methods=["POST"])
def acr_dict_diff_locks_ww_replace():
    global diff_dict
    t = threading.Thread(target=cb_diff_dict_replace)
    t.start()
    with lock_b:
        diff_dict = request.get_json()  # WRITE (rebind) under lock_b — lost update
    t.join()
    return jsonify({"dict": diff_dict})


# --- Lock acquire/release ---
acq_dict = {}
acq_lock = threading.Lock()

def cb_acq_dict_write():
    acq_lock.acquire()
    acq_dict["key"] = "acq_cb"         # WRITE protected
    acq_lock.release()

@app.route("/acr/dict/acquire_release/ww_key", methods=["POST"])
def acr_dict_acquire_release_ww_key():
    t = threading.Thread(target=cb_acq_dict_write)
    t.start()
    acq_dict["key"] = "acq_main"        # WRITE without lock — race
    t.join()
    return jsonify({"dict": acq_dict})

def cb_acq_dict_inc():
    acq_lock.acquire()
    acq_dict["counter"] = acq_dict.get("counter", 0) + 1   # WRITE (RMW) protected
    acq_lock.release()

@app.route("/acr/dict/acquire_release/ww_rmw", methods=["POST"])
def acr_dict_acquire_release_ww_rmw():
    t = threading.Thread(target=cb_acq_dict_inc)
    t.start()
    acq_dict["counter"] = acq_dict.get("counter", 0) + 1   # WRITE (RMW) without lock — race
    t.join()
    return jsonify({"dict": acq_dict})


# --- RLock ---
rlock_dict = {}
rlock      = threading.RLock()

def cb_rlock_dict_write():
    with rlock:
        rlock_dict["key"] = "rlock_cb"  # WRITE protected

@app.route("/acr/dict/rlock/ww_key", methods=["POST"])
def acr_dict_rlock_ww_key():
    t = threading.Thread(target=cb_rlock_dict_write)
    t.start()
    rlock_dict["key"] = "rlock_main"    # WRITE without rlock — race
    t.join()
    return jsonify({"dict": rlock_dict})

def cb_rlock_dict_replace():
    global rlock_dict
    with rlock:
        rlock_dict = {"source": "rlock_cb_replace"}    # WRITE (rebind) protected

@app.route("/acr/dict/rlock/ww_replace", methods=["POST"])
def acr_dict_rlock_ww_replace():
    global rlock_dict
    t = threading.Thread(target=cb_rlock_dict_replace)
    t.start()
    rlock_dict = request.get_json()     # WRITE — lost update
    t.join()
    return jsonify({"dict": rlock_dict})


# --- Condition ---
cond_dict = {}
condition = threading.Condition()

def cb_cond_dict_write():
    with condition:
        cond_dict["key"] = "cond_cb"
        condition.notify_all()          # WRITE inside condition

@app.route("/acr/dict/condition/ww_key", methods=["POST"])
def acr_dict_condition_ww_key():
    t = threading.Thread(target=cb_cond_dict_write)
    t.start()
    cond_dict["key"] = "cond_main"      # WRITE without condition — race
    t.join()
    return jsonify({"dict": cond_dict})

def cb_cond_dict_replace():
    global cond_dict
    with condition:
        cond_dict = {"source": "cond_cb_replace"}
        condition.notify_all()

@app.route("/acr/dict/condition/ww_replace", methods=["POST"])
def acr_dict_condition_ww_replace():
    global cond_dict
    t = threading.Thread(target=cb_cond_dict_replace)
    t.start()
    cond_dict = request.get_json()      # WRITE — lost update
    t.join()
    return jsonify({"dict": cond_dict})


# --- Semaphore ---
sem_dict  = {}
semaphore = threading.Semaphore(1)

def cb_sem_dict_write():
    semaphore.acquire()
    sem_dict["key"] = "sem_cb"          # WRITE protected
    semaphore.release()

@app.route("/acr/dict/semaphore/ww_key", methods=["POST"])
def acr_dict_semaphore_ww_key():
    t = threading.Thread(target=cb_sem_dict_write)
    t.start()
    sem_dict["key"] = "sem_main"        # WRITE without semaphore — race
    t.join()
    return jsonify({"dict": sem_dict})

@app.route("/acr/dict/semaphore/ww_rmw", methods=["POST"])
def acr_dict_semaphore_ww_rmw():
    t = threading.Thread(target=cb_sem_dict_write)
    t.start()
    sem_dict["counter"] = sem_dict.get("counter", 0) + 1   # WRITE (RMW) without semaphore — race
    t.join()
    return jsonify({"dict": sem_dict})


# --- BoundedSemaphore ---
bsem_dict   = {}
bounded_sem = threading.BoundedSemaphore(1)

def cb_bsem_dict_write():
    bounded_sem.acquire()
    bsem_dict["key"] = "bsem_cb"       # WRITE protected
    bounded_sem.release()

@app.route("/acr/dict/bounded_sem/ww_key", methods=["POST"])
def acr_dict_bounded_sem_ww_key():
    t = threading.Thread(target=cb_bsem_dict_write)
    t.start()
    bsem_dict["key"] = "bsem_main"     # WRITE without bounded_sem — race
    t.join()
    return jsonify({"dict": bsem_dict})

def cb_bsem_dict_replace():
    global bsem_dict
    bounded_sem.acquire()
    bsem_dict = {"source": "bsem_cb_replace"}   # WRITE (rebind) protected
    bounded_sem.release()

@app.route("/acr/dict/bounded_sem/ww_replace", methods=["POST"])
def acr_dict_bounded_sem_ww_replace():
    global bsem_dict
    t = threading.Thread(target=cb_bsem_dict_replace)
    t.start()
    bsem_dict = request.get_json()      # WRITE — lost update
    t.join()
    return jsonify({"dict": bsem_dict})


# --- Event ---
event_dict  = {}
ready_event = threading.Event()

def cb_event_dict_write():
    event_dict["key"] = "event_cb"
    ready_event.set()

@app.route("/acr/dict/event/ww_key", methods=["POST"])
def acr_dict_event_ww_key():
    t = threading.Thread(target=cb_event_dict_write)
    t.start()
    event_dict["key"] = "event_main"    # WRITE — race
    t.join()
    return jsonify({"dict": event_dict})

def cb_event_dict_replace():
    global event_dict
    event_dict = {"source": "event_cb_replace"}
    ready_event.set()

@app.route("/acr/dict/event/ww_replace", methods=["POST"])
def acr_dict_event_ww_replace():
    global event_dict
    t = threading.Thread(target=cb_event_dict_replace)
    t.start()
    event_dict = request.get_json()     # WRITE — lost update
    t.join()
    return jsonify({"dict": event_dict})


# --- Aliasing: direct reference alias ---
shared_dict = {"x": 1, "y": 2}
alias_dict  = shared_dict   # alias — same object

def cb_alias_dict_update():
    shared_dict["x"] = 99               # WRITE via original (in-place)

@app.route("/acr/dict/alias/ww_key_original", methods=["POST"])
def acr_dict_alias_ww_key_original():
    t = threading.Thread(target=cb_alias_dict_update)
    t.start()
    alias_dict["x"] = request.get_json().get("x", 42)  # WRITE via alias — race
    t.join()
    return jsonify({"dict": shared_dict})

def cb_alias_dict_rebind():
    global alias_dict
    alias_dict = {"from": "cb_rebind"}  # WRITE (rebind alias) from callback

@app.route("/acr/dict/alias/ww_rebind", methods=["POST"])
def acr_dict_alias_ww_rebind():
    global shared_dict
    t = threading.Thread(target=cb_alias_dict_rebind)
    t.start()
    shared_dict = request.get_json()    # WRITE (rebind original) — race
    t.join()
    return jsonify({"shared": shared_dict, "alias": alias_dict})


# --- Nested dict alias ---
outer_cfg = {"inner": {"timeout": 30, "retries": 3}}
inner_cfg = outer_cfg["inner"]          # alias to inner dict

def cb_inner_cfg_update():
    inner_cfg["timeout"] = 99           # WRITE via inner alias

@app.route("/acr/dict/alias_nested/ww_key", methods=["POST"])
def acr_dict_alias_nested_ww_key():
    t = threading.Thread(target=cb_inner_cfg_update)
    t.start()
    outer_cfg["inner"]["timeout"] = request.get_json().get("timeout", 1)  # WRITE via outer — race
    t.join()
    return jsonify({"inner": outer_cfg["inner"]})


# --- class: via instance (outside) ---
class ConfigStore:
    def __init__(self):
        self.settings  = {"debug": False}
        self.overrides = {}
        self.defaults  = {"retry": 3}

store       = ConfigStore()
store_alias = store     # alias

def cb_store_update_settings():
    store.settings = {"debug": True, "source": "callback"}     # WRITE from callback

@app.route("/acr/dict/class_outside/ww_settings", methods=["POST"])
def acr_dict_class_outside_ww_settings():
    t = threading.Thread(target=cb_store_update_settings)
    t.start()
    store.settings = request.get_json() # WRITE — lost update
    t.join()
    return jsonify({"settings": store.settings})

def cb_store_set_key():
    store.overrides["env"] = "cb"       # WRITE key from callback

@app.route("/acr/dict/class_outside/ww_overrides_key", methods=["POST"])
def acr_dict_class_outside_ww_overrides_key():
    t = threading.Thread(target=cb_store_set_key)
    t.start()
    store.overrides["env"] = "main"     # WRITE same key — race
    t.join()
    return jsonify({"overrides": store.overrides})

def cb_alias_store_update():
    store_alias.overrides = {"env": "cb_alias"}     # WRITE via alias

@app.route("/acr/dict/class_alias/ww_overrides", methods=["POST"])
def acr_dict_class_alias_ww_overrides():
    t = threading.Thread(target=cb_alias_store_update)
    t.start()
    store.overrides = request.get_json()    # WRITE via original — race
    t.join()
    return jsonify({"overrides": store.overrides})


# --- class: inside methods ---
class AsyncDictProcessor:
    def __init__(self):
        self.data    = {}
        self.results = {}
        self.stats   = {"count": 0}

    def _cb_update_key(self, key, val):
        self.data[key] = val + "_cb"    # WRITE from callback

    def _cb_replace_data(self, new_data):
        self.data = dict(new_data)      # WRITE (replace) from callback

    def _cb_inc_stats(self):
        self.stats["count"] = self.stats.get("count", 0) + 1  # WRITE from callback

    def process_key(self, key, val):
        t = threading.Thread(target=self._cb_update_key, args=(key, val))
        t.start()
        self.data[key] = val + "_main"  # WRITE — race
        t.join()
        return self.data

    def process_replace(self, new_data):
        t = threading.Thread(target=self._cb_replace_data, args=(new_data,))
        t.start()
        self.data = {k: v + "_main" for k, v in new_data.items()}  # WRITE — lost update
        t.join()
        return self.data

    def process_stats(self):
        t = threading.Thread(target=self._cb_inc_stats)
        t.start()
        self.stats["count"] = self.stats.get("count", 0) + 1  # WRITE — race
        t.join()
        return self.stats

dict_proc = AsyncDictProcessor()

@app.route("/acr/dict/class_inside/process_key", methods=["POST"])
def acr_dict_class_inside_process_key():
    data = request.get_json()
    return jsonify({"data": dict_proc.process_key(data.get("key", "k"), data.get("val", "v"))})

@app.route("/acr/dict/class_inside/replace", methods=["POST"])
def acr_dict_class_inside_replace():
    return jsonify({"data": dict_proc.process_replace(request.get_json())})

@app.route("/acr/dict/class_inside/stats", methods=["POST"])
def acr_dict_class_inside_stats():
    return jsonify({"stats": dict_proc.process_stats()})


# ---------------------------------------------------------------------------
# DETECTOR BLIND SPOTS
# ---------------------------------------------------------------------------

blind_dict_a = {}
blind_dict_b = {}

# --- blind spot 1: callback and endpoint access different dicts ---
def cb_blind_write_dict_b():
    blind_dict_b["key"] = "cb"          # WRITE to blind_dict_b

@app.route("/acr/dict/blind/different_vars", methods=["POST"])
def acr_dict_blind_different_vars():
    t = threading.Thread(target=cb_blind_write_dict_b)
    t.start()
    blind_dict_a["key"] = "main"        # WRITE to blind_dict_a — different dict — no race
    t.join()
    return jsonify({"a": blind_dict_a, "b": blind_dict_b})


# --- blind spot 2: callback only reads (RR) ---
read_only_dict = {"k": "v"}

def cb_blind_read_only_dict():
    local = dict(read_only_dict)        # READ — no write

@app.route("/acr/dict/blind/rr_no_race", methods=["GET"])
def acr_dict_blind_rr_no_race():
    t = threading.Thread(target=cb_blind_read_only_dict)
    t.start()
    snapshot = dict(read_only_dict)     # READ — RR, not race
    t.join()
    return jsonify({"snapshot": snapshot})


# --- blind spot 3: callback receives dict copy as argument ---
arg_dict = {"x": 1}

def cb_blind_arg_dict(d):
    d["x"] = 99                         # modifies argument copy

@app.route("/acr/dict/blind/arg_copy", methods=["POST"])
def acr_dict_blind_arg_copy():
    global arg_dict
    t = threading.Thread(target=cb_blind_arg_dict, args=(dict(arg_dict),))  # passes copy
    t.start()
    arg_dict = {}                       # WRITE to global — callback works on copy — no race
    t.join()
    return jsonify({"arg_dict": arg_dict})


# --- blind spot 4: Thread not started ---
nostarter_dict = {}

def cb_blind_nostarter_dict():
    nostarter_dict["key"] = "never"    # WRITE — thread never starts

@app.route("/acr/dict/blind/not_started", methods=["POST"])
def acr_dict_blind_not_started():
    t = threading.Thread(target=cb_blind_nostarter_dict)
    # t.start() — intentionally not called
    nostarter_dict["key"] = "main"
    return jsonify({"dict": nostarter_dict})


# --- blind spot 5: join() before further access ---
join_dict = {}

def cb_blind_join_dict_write():
    join_dict["key"] = "cb_item"        # WRITE in callback

@app.route("/acr/dict/blind/join_before_access", methods=["POST"])
def acr_dict_blind_join_before_access():
    t = threading.Thread(target=cb_blind_join_dict_write)
    t.start()
    t.join()                            # join() before access — no race
    snapshot = dict(join_dict)          # READ after join — safe
    return jsonify({"dict": snapshot})


# --- blind spot 6: both sides locked with shared lock ---
safe_dict = {}
safe_lock = threading.Lock()

def cb_blind_safe_dict_write():
    global safe_dict
    with safe_lock:
        safe_dict = {"source": "safe_cb"}   # WRITE under shared lock

@app.route("/acr/dict/blind/both_locked", methods=["POST"])
def acr_dict_blind_both_locked():
    global safe_dict
    t = threading.Thread(target=cb_blind_safe_dict_write)
    t.start()
    with safe_lock:
        safe_dict = request.get_json()  # WRITE under same lock — correctly protected
    t.join()
    return jsonify({"dict": safe_dict})


if __name__ == "__main__":
    app.run(debug=True, threaded=True)
