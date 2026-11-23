"""
ASYNC CALLBACK RACE | Python | List
Criterion: A function passed as target= to Thread (or submitted to Executor)
           accesses the same global list as the calling endpoint.
           If both accesses without synchronization — Async Callback Race.

Patterns:
  WW: callback appends/modifies + endpoint appends/modifies same list
  RW: callback WRITE + endpoint reads snapshot
  element-level: both modify same index
  reference replacement: both rebind the global list variable
"""
from flask import Flask, jsonify, request
import threading
from concurrent.futures import ThreadPoolExecutor

app = Flask(__name__)

# ---------------------------------------------------------------------------
# MATCHES CRITERION
# ---------------------------------------------------------------------------

# --- no lock: both callback and endpoint modify same list ---
task_list    = []
results_list = [0, 0, 0]
pipeline     = ["stage1", "stage2"]

def cb_list_append():
    task_list.append("cb_task")         # WRITE (in-place append) from callback

@app.route("/acr/list/no_lock/ww_append", methods=["POST"])
def acr_list_no_lock_ww_append():
    item = request.get_json().get("item", "main_task")
    t = threading.Thread(target=cb_list_append)
    t.start()
    task_list.append(item)              # WRITE (append) from endpoint — race
    t.join()
    return jsonify({"size": len(task_list), "list": task_list})

def cb_list_replace():
    global task_list
    task_list = ["cb_replacement"]      # WRITE (rebind) from callback

@app.route("/acr/list/no_lock/ww_replace", methods=["POST"])
def acr_list_no_lock_ww_replace():
    global task_list
    t = threading.Thread(target=cb_list_replace)
    t.start()
    task_list = request.get_json().get("items", ["main_replacement"])   # WRITE (rebind) — race
    t.join()
    return jsonify({"list": task_list})

def cb_list_extend():
    task_list.extend(["cb_a", "cb_b"]) # WRITE (extend) from callback

@app.route("/acr/list/no_lock/ww_extend", methods=["POST"])
def acr_list_no_lock_ww_extend():
    t = threading.Thread(target=cb_list_extend)
    t.start()
    task_list.extend(request.get_json().get("items", ["main_a"]))   # WRITE — race
    t.join()
    return jsonify({"size": len(task_list)})


# --- no lock: element-level RMW ---
def cb_list_inc_element(index=0):
    results_list[index] += 1            # WRITE element from callback

@app.route("/acr/list/no_lock/ww_element_inc", methods=["POST"])
def acr_list_no_lock_ww_element_inc():
    idx = request.get_json().get("index", 0)
    t = threading.Thread(target=cb_list_inc_element, args=(idx,))
    t.start()
    results_list[idx] += 1             # WRITE same element from endpoint — race
    t.join()
    return jsonify({"results": results_list})

def cb_list_dec_element(index=0):
    results_list[index] -= 1            # WRITE element from callback

@app.route("/acr/list/no_lock/ww_element_dec", methods=["POST"])
def acr_list_no_lock_ww_element_dec():
    idx = request.get_json().get("index", 0)
    t = threading.Thread(target=cb_list_dec_element, args=(idx,))
    t.start()
    results_list[idx] -= 1             # WRITE — lost update with callback
    t.join()
    return jsonify({"results": results_list})


# --- no lock: RW — callback WRITE, endpoint reads snapshot ---
def cb_list_clear():
    global pipeline
    pipeline = []                       # WRITE from callback

@app.route("/acr/list/no_lock/rw_clear", methods=["GET"])
def acr_list_no_lock_rw_clear():
    t = threading.Thread(target=cb_list_clear)
    t.start()
    snapshot = list(pipeline)           # READ snapshot — may see stale state
    t.join()
    return jsonify({"snapshot": snapshot, "final": pipeline})

def cb_list_reload():
    global pipeline
    pipeline = ["cb_stage1", "cb_stage2", "cb_stage3"]  # WRITE from callback

@app.route("/acr/list/no_lock/rw_reload", methods=["GET"])
def acr_list_no_lock_rw_reload():
    t = threading.Thread(target=cb_list_reload)
    t.start()
    size = len(pipeline)                # READ from endpoint — may be stale
    t.join()
    return jsonify({"read_size": size, "final": pipeline})


# --- no lock: explicit read-local-write in both ---
log_list = []

def cb_log_rlw():
    global log_list
    loc = log_list + ["cb_entry"]
    log_list = loc                      # WRITE back — lost update

@app.route("/acr/list/no_lock/rlw_append_a", methods=["POST"])
def acr_list_no_lock_rlw_append_a():
    global log_list
    t = threading.Thread(target=cb_log_rlw)
    t.start()
    loc = log_list + [request.get_json().get("entry", "main_entry")]
    log_list = loc                      # WRITE — race with callback
    t.join()
    return jsonify({"size": len(log_list)})

@app.route("/acr/list/no_lock/rlw_append_b", methods=["POST"])
def acr_list_no_lock_rlw_append_b():
    global log_list
    t = threading.Thread(target=cb_log_rlw)
    t.start()
    loc = log_list + [request.get_json().get("entry", "main_entry_b")]
    log_list = loc                      # WRITE — lost update with rlw_append_a
    t.join()
    return jsonify({"size": len(log_list)})


# --- Executor.submit() ---
exec_list = []

def exec_cb_list_append():
    exec_list.append("executor_item")   # WRITE from executor task

@app.route("/acr/list/executor/ww_append", methods=["POST"])
def acr_list_executor_ww_append():
    with ThreadPoolExecutor(max_workers=1) as ex:
        future = ex.submit(exec_cb_list_append)
        exec_list.append(request.get_json().get("item", "main_item"))  # WRITE — race
        future.result()
    return jsonify({"size": len(exec_list), "list": exec_list})

def exec_cb_list_replace():
    global exec_list
    exec_list = ["executor_replacement"]    # WRITE (rebind) from task

@app.route("/acr/list/executor/ww_replace", methods=["POST"])
def acr_list_executor_ww_replace():
    global exec_list
    with ThreadPoolExecutor(max_workers=1) as ex:
        future = ex.submit(exec_cb_list_replace)
        exec_list = request.get_json().get("items", ["main_replacement"])  # WRITE — race
        future.result()
    return jsonify({"list": exec_list})


# --- partial lock: callback protected, endpoint not ---
partial_list = []
list_lock    = threading.Lock()

def cb_partial_list_append():
    global partial_list
    with list_lock:
        partial_list.append("cb_item")  # WRITE protected

@app.route("/acr/list/partial/ww_append", methods=["POST"])
def acr_list_partial_ww_append():
    t = threading.Thread(target=cb_partial_list_append)
    t.start()
    partial_list.append(request.get_json().get("item", "main_item"))    # WRITE without lock — race
    t.join()
    return jsonify({"size": len(partial_list)})

def cb_partial_list_replace():
    global partial_list
    with list_lock:
        partial_list = ["cb_locked"]    # WRITE (rebind) protected

@app.route("/acr/list/partial/ww_replace", methods=["POST"])
def acr_list_partial_ww_replace():
    global partial_list
    t = threading.Thread(target=cb_partial_list_replace)
    t.start()
    partial_list = request.get_json().get("items", ["main_unlocked"])   # WRITE — race
    t.join()
    return jsonify({"list": partial_list})


# --- different locks ---
diff_list = []
lock_a    = threading.Lock()
lock_b    = threading.Lock()

def cb_diff_list_append():
    global diff_list
    with lock_a:
        diff_list.append("lock_a_item") # WRITE under lock_a

@app.route("/acr/list/diff_locks/ww_append", methods=["POST"])
def acr_list_diff_locks_ww_append():
    t = threading.Thread(target=cb_diff_list_append)
    t.start()
    with lock_b:
        diff_list.append(request.get_json().get("item", "lock_b_item"))  # WRITE under lock_b — race
    t.join()
    return jsonify({"size": len(diff_list)})

def cb_diff_list_replace():
    global diff_list
    with lock_a:
        diff_list = ["lock_a_replace"]  # WRITE (rebind) under lock_a

@app.route("/acr/list/diff_locks/ww_replace", methods=["POST"])
def acr_list_diff_locks_ww_replace():
    global diff_list
    t = threading.Thread(target=cb_diff_list_replace)
    t.start()
    with lock_b:
        diff_list = request.get_json().get("items", ["lock_b_replace"])  # WRITE — lost update
    t.join()
    return jsonify({"list": diff_list})


# --- Lock acquire/release ---
acq_list = []
acq_lock = threading.Lock()

def cb_acq_list_append():
    acq_lock.acquire()
    acq_list.append("acq_cb")          # WRITE protected
    acq_lock.release()

@app.route("/acr/list/acquire_release/ww_append", methods=["POST"])
def acr_list_acquire_release_ww_append():
    t = threading.Thread(target=cb_acq_list_append)
    t.start()
    acq_list.append(request.get_json().get("item", "acq_main"))     # WRITE without lock — race
    t.join()
    return jsonify({"size": len(acq_list)})

@app.route("/acr/list/acquire_release/ww_element", methods=["POST"])
def acr_list_acquire_release_ww_element():
    if not acq_list:
        acq_list.append(0)
    t = threading.Thread(target=cb_acq_list_append)
    t.start()
    if acq_list:
        acq_list[0] += 1                # WRITE element without lock — race
    t.join()
    return jsonify({"list": acq_list})


# --- RLock ---
rlock_list = []
rlock      = threading.RLock()

def cb_rlock_list_append():
    with rlock:
        rlock_list.append("rlock_cb")  # WRITE protected

@app.route("/acr/list/rlock/ww_append", methods=["POST"])
def acr_list_rlock_ww_append():
    t = threading.Thread(target=cb_rlock_list_append)
    t.start()
    rlock_list.append(request.get_json().get("item", "rlock_main"))  # WRITE without rlock — race
    t.join()
    return jsonify({"size": len(rlock_list)})

def cb_rlock_list_replace():
    global rlock_list
    with rlock:
        rlock_list = ["rlock_cb_replace"]   # WRITE (rebind) protected

@app.route("/acr/list/rlock/ww_replace", methods=["POST"])
def acr_list_rlock_ww_replace():
    global rlock_list
    t = threading.Thread(target=cb_rlock_list_replace)
    t.start()
    rlock_list = request.get_json().get("items", ["rlock_main_replace"])    # WRITE — lost update
    t.join()
    return jsonify({"list": rlock_list})


# --- Condition ---
cond_list = []
condition = threading.Condition()

def cb_cond_list_append():
    with condition:
        cond_list.append("cond_cb")
        condition.notify_all()          # WRITE inside condition

@app.route("/acr/list/condition/ww_append", methods=["POST"])
def acr_list_condition_ww_append():
    t = threading.Thread(target=cb_cond_list_append)
    t.start()
    cond_list.append(request.get_json().get("item", "cond_main"))    # WRITE without condition — race
    t.join()
    return jsonify({"size": len(cond_list)})

def cb_cond_list_replace():
    global cond_list
    with condition:
        cond_list = ["cond_cb_replace"]
        condition.notify_all()          # WRITE (rebind) inside condition

@app.route("/acr/list/condition/ww_replace", methods=["POST"])
def acr_list_condition_ww_replace():
    global cond_list
    t = threading.Thread(target=cb_cond_list_replace)
    t.start()
    cond_list = request.get_json().get("items", ["cond_main_replace"])  # WRITE — lost update
    t.join()
    return jsonify({"list": cond_list})


# --- Semaphore ---
sem_list  = []
semaphore = threading.Semaphore(1)

def cb_sem_list_append():
    semaphore.acquire()
    sem_list.append("sem_cb")           # WRITE protected
    semaphore.release()

@app.route("/acr/list/semaphore/ww_append", methods=["POST"])
def acr_list_semaphore_ww_append():
    t = threading.Thread(target=cb_sem_list_append)
    t.start()
    sem_list.append(request.get_json().get("item", "sem_main"))     # WRITE without semaphore — race
    t.join()
    return jsonify({"size": len(sem_list)})

@app.route("/acr/list/semaphore/ww_element", methods=["POST"])
def acr_list_semaphore_ww_element():
    if not sem_list:
        sem_list.append(0)
    idx = 0
    t = threading.Thread(target=cb_sem_list_append)
    t.start()
    if sem_list:
        sem_list[idx] = request.get_json().get("val", 99)   # WRITE element — race
    t.join()
    return jsonify({"list": sem_list})


# --- BoundedSemaphore ---
bsem_list   = []
bounded_sem = threading.BoundedSemaphore(1)

def cb_bsem_list_append():
    bounded_sem.acquire()
    bsem_list.append("bsem_cb")         # WRITE protected
    bounded_sem.release()

@app.route("/acr/list/bounded_sem/ww_append", methods=["POST"])
def acr_list_bounded_sem_ww_append():
    t = threading.Thread(target=cb_bsem_list_append)
    t.start()
    bsem_list.append(request.get_json().get("item", "bsem_main"))   # WRITE without bounded_sem — race
    t.join()
    return jsonify({"size": len(bsem_list)})

def cb_bsem_list_replace():
    global bsem_list
    bounded_sem.acquire()
    bsem_list = ["bsem_cb_replace"]     # WRITE (rebind) protected
    bounded_sem.release()

@app.route("/acr/list/bounded_sem/ww_replace", methods=["POST"])
def acr_list_bounded_sem_ww_replace():
    global bsem_list
    t = threading.Thread(target=cb_bsem_list_replace)
    t.start()
    bsem_list = request.get_json().get("items", ["bsem_main_replace"])  # WRITE — lost update
    t.join()
    return jsonify({"list": bsem_list})


# --- Event ---
event_list  = []
ready_event = threading.Event()

def cb_event_list_append():
    event_list.append("event_cb")       # WRITE without lock
    ready_event.set()

@app.route("/acr/list/event/ww_append", methods=["POST"])
def acr_list_event_ww_append():
    t = threading.Thread(target=cb_event_list_append)
    t.start()
    event_list.append(request.get_json().get("item", "event_main")) # WRITE — race
    t.join()
    return jsonify({"size": len(event_list)})

def cb_event_list_replace():
    global event_list
    event_list = ["event_cb_replace"]
    ready_event.set()

@app.route("/acr/list/event/ww_replace", methods=["POST"])
def acr_list_event_ww_replace():
    global event_list
    t = threading.Thread(target=cb_event_list_replace)
    t.start()
    event_list = request.get_json().get("items", ["event_main_replace"])    # WRITE — lost update
    t.join()
    return jsonify({"list": event_list})


# --- Aliasing: direct reference alias ---
shared_list = [1, 2, 3]
alias_list  = shared_list   # alias — same object

def cb_alias_list_append():
    shared_list.append(99)              # WRITE via original (in-place)

@app.route("/acr/list/alias/ww_append_original", methods=["POST"])
def acr_list_alias_ww_append_original():
    t = threading.Thread(target=cb_alias_list_append)
    t.start()
    alias_list.append(request.get_json().get("item", 42))   # WRITE via alias — race
    t.join()
    return jsonify({"list": shared_list})

def cb_alias_list_rebind():
    global alias_list
    alias_list = [77, 88]               # WRITE (rebind alias) from callback

@app.route("/acr/list/alias/ww_rebind_alias", methods=["POST"])
def acr_list_alias_ww_rebind_alias():
    global shared_list
    t = threading.Thread(target=cb_alias_list_rebind)
    t.start()
    shared_list = request.get_json().get("items", [11, 22])  # WRITE (rebind original) — race
    t.join()
    return jsonify({"shared": shared_list, "alias": alias_list})


# --- class: via instance (outside) ---
class DataBuffer:
    def __init__(self):
        self.items   = []
        self.history = []
        self.errors  = []

buffer       = DataBuffer()
buffer_alias = buffer   # alias

def cb_buffer_append():
    buffer.items.append("cb_item")      # WRITE from callback

@app.route("/acr/list/class_outside/ww_items_append", methods=["POST"])
def acr_list_class_outside_ww_items_append():
    t = threading.Thread(target=cb_buffer_append)
    t.start()
    buffer.items.append(request.get_json().get("item", "main_item"))    # WRITE — race
    t.join()
    return jsonify({"items": buffer.items})

def cb_buffer_replace_items():
    buffer.items = ["cb_replacement"]   # WRITE (rebind) from callback

@app.route("/acr/list/class_outside/ww_items_replace", methods=["POST"])
def acr_list_class_outside_ww_items_replace():
    t = threading.Thread(target=cb_buffer_replace_items)
    t.start()
    buffer.items = request.get_json().get("items", ["main_replacement"])    # WRITE — lost update
    t.join()
    return jsonify({"items": buffer.items})

def cb_alias_buffer_append():
    buffer_alias.history.append("cb_history")   # WRITE via alias

@app.route("/acr/list/class_alias/ww_history", methods=["POST"])
def acr_list_class_alias_ww_history():
    t = threading.Thread(target=cb_alias_buffer_append)
    t.start()
    buffer.history.append(request.get_json().get("entry", "main_history"))  # WRITE via original — race
    t.join()
    return jsonify({"history": buffer.history})


# --- class: inside methods ---
class AsyncListProcessor:
    def __init__(self):
        self.items   = []
        self.results = []

    def _cb_append(self, item):
        self.items.append(item)         # WRITE from callback method

    def _cb_replace_items(self, new_items):
        self.items = list(new_items)    # WRITE (replace) from callback

    def _cb_append_result(self):
        self.results.append("cb_result")    # WRITE from callback

    def process_append(self, item):
        t = threading.Thread(target=self._cb_append, args=("cb_" + str(item),))
        t.start()
        self.items.append("main_" + str(item))  # WRITE — race
        t.join()
        return self.items

    def process_replace(self, new_items):
        t = threading.Thread(target=self._cb_replace_items, args=(new_items,))
        t.start()
        self.items = ["main_replace_" + i for i in new_items]   # WRITE — lost update
        t.join()
        return self.items

    def process_results(self):
        t = threading.Thread(target=self._cb_append_result)
        t.start()
        self.results.append("main_result")  # WRITE — race
        t.join()
        return self.results

list_proc = AsyncListProcessor()

@app.route("/acr/list/class_inside/append", methods=["POST"])
def acr_list_class_inside_append():
    item = request.get_json().get("item", "x")
    return jsonify({"items": list_proc.process_append(item)})

@app.route("/acr/list/class_inside/replace", methods=["POST"])
def acr_list_class_inside_replace():
    items = request.get_json().get("items", ["a", "b"])
    return jsonify({"items": list_proc.process_replace(items)})

@app.route("/acr/list/class_inside/append_result", methods=["POST"])
def acr_list_class_inside_append_result():
    return jsonify({"results": list_proc.process_results()})


# ---------------------------------------------------------------------------
# DETECTOR BLIND SPOTS
# ---------------------------------------------------------------------------

blind_list_a = []
blind_list_b = []

# --- blind spot 1: callback and endpoint access different lists ---
def cb_blind_write_list_b():
    blind_list_b.append("cb")           # WRITE to blind_list_b

@app.route("/acr/list/blind/different_vars", methods=["POST"])
def acr_list_blind_different_vars():
    t = threading.Thread(target=cb_blind_write_list_b)
    t.start()
    blind_list_a.append("main")         # WRITE to blind_list_a — different list — no race
    t.join()
    return jsonify({"a": blind_list_a, "b": blind_list_b})


# --- blind spot 2: callback only reads (RR) ---
read_only_list = [1, 2, 3]

def cb_blind_read_only_list():
    local = list(read_only_list)        # READ — no write

@app.route("/acr/list/blind/rr_no_race", methods=["GET"])
def acr_list_blind_rr_no_race():
    t = threading.Thread(target=cb_blind_read_only_list)
    t.start()
    snapshot = list(read_only_list)     # READ — RR, not race
    t.join()
    return jsonify({"snapshot": snapshot})


# --- blind spot 3: callback receives list copy as argument ---
arg_list = [1, 2, 3]

def cb_blind_arg_list(lst):
    lst.append(99)                      # modifies argument copy

@app.route("/acr/list/blind/arg_copy", methods=["POST"])
def acr_list_blind_arg_copy():
    global arg_list
    t = threading.Thread(target=cb_blind_arg_list, args=(list(arg_list),))  # passes copy
    t.start()
    arg_list = []                       # WRITE to global — callback works on copy — no race
    t.join()
    return jsonify({"arg_list": arg_list})


# --- blind spot 4: Thread created but not started ---
nostarter_list = []

def cb_blind_nostarter_list():
    nostarter_list.append("never")     # WRITE — thread never starts

@app.route("/acr/list/blind/not_started", methods=["POST"])
def acr_list_blind_not_started():
    t = threading.Thread(target=cb_blind_nostarter_list)
    # t.start() — intentionally not called
    nostarter_list.append("main")
    return jsonify({"list": nostarter_list})


# --- blind spot 5: join() before further access ---
join_list = []

def cb_blind_join_list_write():
    join_list.append("cb_item")         # WRITE in callback

@app.route("/acr/list/blind/join_before_access", methods=["POST"])
def acr_list_blind_join_before_access():
    t = threading.Thread(target=cb_blind_join_list_write)
    t.start()
    t.join()                            # join() before access — no race
    snapshot = list(join_list)          # READ after join — safe
    return jsonify({"list": snapshot})


# --- blind spot 6: both sides locked with shared lock ---
safe_list = []
safe_lock = threading.Lock()

def cb_blind_safe_list_write():
    global safe_list
    with safe_lock:
        safe_list = ["safe_cb"]         # WRITE under shared lock

@app.route("/acr/list/blind/both_locked", methods=["POST"])
def acr_list_blind_both_locked():
    global safe_list
    t = threading.Thread(target=cb_blind_safe_list_write)
    t.start()
    with safe_lock:
        safe_list = ["safe_main"]       # WRITE under same lock — correctly protected
    t.join()
    return jsonify({"list": safe_list})


if __name__ == "__main__":
    app.run(debug=True, threaded=True)
