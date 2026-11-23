"""
ASYNC CALLBACK RACE | Python | Scalar
Criterion: A function passed as target= to Thread (or submitted to Executor)
           accesses the same global variable as the calling endpoint.
           If both accesses without synchronization — Async Callback Race.
           By nature: WRITE-WRITE or READ-WRITE across thread boundary.

Pattern:
  result = None

  def callback():
      global result
      result = "from_callback"   # WRITE from callback thread

  @app.route("/process")
  def endpoint():
      global result
      t = Thread(target=callback)
      t.start()
      result = "from_main"       # WRITE from endpoint — race
      t.join()
      return jsonify({"result": result})

Locks: no lock, partial (one side), different locks, Lock (acquire/release),
       RLock, Condition, Semaphore, BoundedSemaphore, Event
Executor: ThreadPoolExecutor.submit()
Aliasing: mutable wrapper, class instance alias
Class: via instance (outside) and inside methods

DETECTOR BLIND SPOTS:
  - callback and endpoint access different variables
  - callback only reads (RR, not race)
  - callback receives variable as argument (no global sharing)
  - Thread created but not started
  - thread.join() before any further access (no race window)
  - both sides locked with same shared lock
  - callback via lambda (detector may not trace)
"""
from flask import Flask, jsonify, request
import threading
from concurrent.futures import ThreadPoolExecutor

app = Flask(__name__)

# ---------------------------------------------------------------------------
# MATCHES CRITERION
# ---------------------------------------------------------------------------

# --- no lock: Thread callback and endpoint both WRITE same variable ---
counter        = 0
balance        = 1000.0
score          = 0
config_timeout = 30

def cb_inc_counter():
     global counter
     counter += 1                        # WRITE from callback thread

@app.route("/acr/scalar/no_lock/ww_counter_inc", methods=["POST"])
def acr_scalar_no_lock_ww_counter_inc():
     global counter
     t = threading.Thread(target=cb_inc_counter)
     t.start()
     counter += 1                        # WRITE from endpoint — race with callback
     t.join()
     return jsonify({"counter": counter})

def cb_dec_counter():
     global counter
     counter -= 1                        # WRITE from callback

@app.route("/acr/scalar/no_lock/ww_counter_dec", methods=["POST"])
def acr_scalar_no_lock_ww_counter_dec():
     global counter
     t = threading.Thread(target=cb_dec_counter)
     t.start()
     counter -= 1                        # WRITE from endpoint — lost update
     t.join()
     return jsonify({"counter": counter})

def cb_deposit():
     global balance
     balance += 100.0                    # WRITE from callback

@app.route("/acr/scalar/no_lock/ww_deposit", methods=["POST"])
def acr_scalar_no_lock_ww_deposit():
     global balance
     t = threading.Thread(target=cb_deposit)
     t.start()
     balance += request.get_json().get("amount", 50.0)   # WRITE — race
     t.join()
     return jsonify({"balance": balance})

def cb_withdraw():
     global balance
     balance -= 100.0                    # WRITE from callback

@app.route("/acr/scalar/no_lock/ww_withdraw", methods=["POST"])
def acr_scalar_no_lock_ww_withdraw():
     global balance
     t = threading.Thread(target=cb_withdraw)
     t.start()
     balance -= request.get_json().get("amount", 50.0)   # WRITE — lost update
     t.join()
     return jsonify({"balance": balance})

def cb_add_score():
     global score
     score += 10                         # WRITE from callback

@app.route("/acr/scalar/no_lock/ww_score_add", methods=["POST"])
def acr_scalar_no_lock_ww_score_add():
     global score
     t = threading.Thread(target=cb_add_score)
     t.start()
     score += request.get_json().get("pts", 5)           # WRITE — race
     t.join()
     return jsonify({"score": score})

def cb_multiply_score():
     global score
     score *= 2                          # WRITE from callback

@app.route("/acr/scalar/no_lock/ww_score_multiply", methods=["POST"])
def acr_scalar_no_lock_ww_score_multiply():
     global score
     t = threading.Thread(target=cb_multiply_score)
     t.start()
     score /= request.get_json().get("factor", 1.5)      # WRITE — lost update
     t.join()
     return jsonify({"score": score})


# # --- no lock: RW — callback WRITE, endpoint READ ---
error_count = 0

def cb_update_timeout():
     global config_timeout
     config_timeout = 99                 # WRITE from callback

@app.route("/acr/scalar/no_lock/rw_timeout", methods=["GET"])
def acr_scalar_no_lock_rw_timeout():
     t = threading.Thread(target=cb_update_timeout)
     t.start()
     local = config_timeout              # READ from endpoint — may see stale value
     t.join()
     return jsonify({"read": local, "final": config_timeout})

def cb_inc_errors():
     global error_count
     error_count += 1                    # WRITE from callback

@app.route("/acr/scalar/no_lock/rw_errors", methods=["GET"])
def acr_scalar_no_lock_rw_errors():
     t = threading.Thread(target=cb_inc_errors)
     t.start()
     snapshot = error_count              # READ — may miss callback write
     t.join()
     return jsonify({"snapshot": snapshot, "final": error_count})


# # --- no lock: explicit read-local-write in both callback and endpoint ---
visits = 0

def cb_rlw_inc():
     global visits
     loc = visits + 1
     visits = loc                        # WRITE back — lost update

@app.route("/acr/scalar/no_lock/rlw_inc", methods=["POST"])
def acr_scalar_no_lock_rlw_inc():
     global visits
     t = threading.Thread(target=cb_rlw_inc)
     t.start()
     loc = visits + 1
     visits = loc                        # WRITE — race with callback
     t.join()
     return jsonify({"visits": visits})

def cb_rlw_dec():
     global visits
     loc = visits - 1
     visits = loc                        # WRITE back — lost update

@app.route("/acr/scalar/no_lock/rlw_dec", methods=["POST"])
def acr_scalar_no_lock_rlw_dec():
     global visits
     t = threading.Thread(target=cb_rlw_dec)
     t.start()
     loc = visits - 1
     visits = loc                        # WRITE — lost update with rlw_inc
     t.join()
     return jsonify({"visits": visits})


# # --- Executor.submit() ---
exec_counter = 0
exec_result  = None

def exec_cb_inc():
     global exec_counter
     exec_counter += 1                   # WRITE from submitted task

@app.route("/acr/scalar/executor/ww_counter", methods=["POST"])
def acr_scalar_executor_ww_counter():
     global exec_counter
     with ThreadPoolExecutor(max_workers=1) as ex:
         future = ex.submit(exec_cb_inc)
         exec_counter += 1               # WRITE from endpoint — race
         future.result()
     return jsonify({"exec_counter": exec_counter})

def exec_cb_set_result():
     global exec_result
     exec_result = "from_executor"       # WRITE from task

@app.route("/acr/scalar/executor/ww_result", methods=["POST"])
def acr_scalar_executor_ww_result():
     global exec_result
     with ThreadPoolExecutor(max_workers=1) as ex:
         future = ex.submit(exec_cb_set_result)
         exec_result = "from_main"       # WRITE — race
         future.result()
     return jsonify({"exec_result": exec_result})

def exec_cb_update_config():
     global config_timeout
     config_timeout = 77                 # WRITE from task

@app.route("/acr/scalar/executor/rw_config", methods=["GET"])
def acr_scalar_executor_rw_config():
     with ThreadPoolExecutor(max_workers=1) as ex:
         future = ex.submit(exec_cb_update_config)
         local = config_timeout          # READ — race with submitted WRITE
         future.result()
     return jsonify({"read": local, "final": config_timeout})


# # --- partial lock: callback protected, endpoint not ---
partial_val  = 0
partial_lock = threading.Lock()

def cb_partial_write():
     global partial_val
     with partial_lock:
         partial_val += 1                # WRITE protected

@app.route("/acr/scalar/partial/ww_unprotected_inc", methods=["POST"])
def acr_scalar_partial_ww_unprotected_inc():
     global partial_val
     t = threading.Thread(target=cb_partial_write)
     t.start()
     partial_val += 1                    # WRITE without lock — race
     t.join()
     return jsonify({"partial_val": partial_val})

@app.route("/acr/scalar/partial/ww_unprotected_dec", methods=["POST"])
def acr_scalar_partial_ww_unprotected_dec():
     global partial_val
     t = threading.Thread(target=cb_partial_write)
     t.start()
     partial_val -= 1                    # WRITE without lock — lost update
     t.join()
     return jsonify({"partial_val": partial_val})


# # --- different locks ---
diff_val = 0
lock_a   = threading.Lock()
lock_b   = threading.Lock()

def cb_diff_lock_inc():
     global diff_val
     with lock_a:
         diff_val += 1                   # WRITE under lock_a

@app.route("/acr/scalar/diff_locks/ww_inc", methods=["POST"])
def acr_scalar_diff_locks_ww_inc():
     global diff_val
     t = threading.Thread(target=cb_diff_lock_inc)
     t.start()
     with lock_b:
         diff_val += 1                   # WRITE under lock_b — different lock, race
     t.join()
     return jsonify({"diff_val": diff_val})

def cb_diff_lock_dec():
     global diff_val
     with lock_a:
         diff_val -= 1                   # WRITE under lock_a

@app.route("/acr/scalar/diff_locks/ww_dec", methods=["POST"])
def acr_scalar_diff_locks_ww_dec():
     global diff_val
     t = threading.Thread(target=cb_diff_lock_dec)
     t.start()
     with lock_b:
         diff_val -= 1                   # WRITE under lock_b — lost update
     t.join()
     return jsonify({"diff_val": diff_val})


# # --- Lock acquire/release ---
acq_val  = 0
acq_lock = threading.Lock()

def cb_acq_inc():
     global acq_val
     acq_lock.acquire()
     acq_val += 1                        # WRITE protected
     acq_lock.release()

@app.route("/acr/scalar/acquire_release/ww_inc", methods=["POST"])
def acr_scalar_acquire_release_ww_inc():
     global acq_val
     t = threading.Thread(target=cb_acq_inc)
     t.start()
     acq_val += 1                        # WRITE without lock — race
     t.join()
     return jsonify({"acq_val": acq_val})

@app.route("/acr/scalar/acquire_release/ww_dec", methods=["POST"])
def acr_scalar_acquire_release_ww_dec():
     global acq_val
     t = threading.Thread(target=cb_acq_inc)
     t.start()
     acq_val -= 1                        # WRITE without lock — lost update
     t.join()
     return jsonify({"acq_val": acq_val})


# # --- RLock ---
rlock_val = 0
rlock     = threading.RLock()

def cb_rlock_inc():
     global rlock_val
     with rlock:
         rlock_val += 1                  # WRITE protected

@app.route("/acr/scalar/rlock/ww_inc", methods=["POST"])
def acr_scalar_rlock_ww_inc():
     global rlock_val
     t = threading.Thread(target=cb_rlock_inc)
     t.start()
     rlock_val += 1                      # WRITE without rlock — race
     t.join()
     return jsonify({"rlock_val": rlock_val})

@app.route("/acr/scalar/rlock/ww_multiply", methods=["POST"])
def acr_scalar_rlock_ww_multiply():
     global rlock_val
     t = threading.Thread(target=cb_rlock_inc)
     t.start()
     rlock_val *= request.get_json().get("factor", 2)    # WRITE — lost update
     t.join()
     return jsonify({"rlock_val": rlock_val})


# # --- Condition ---
cond_val  = 0
condition = threading.Condition()

def cb_cond_inc():
     global cond_val
     with condition:
         cond_val += 1
         condition.notify_all()          # WRITE inside condition

@app.route("/acr/scalar/condition/ww_inc", methods=["POST"])
def acr_scalar_condition_ww_inc():
     global cond_val
     t = threading.Thread(target=cb_cond_inc)
     t.start()
     cond_val += 1                       # WRITE without condition — race
     t.join()
     return jsonify({"cond_val": cond_val})

@app.route("/acr/scalar/condition/ww_dec", methods=["POST"])
def acr_scalar_condition_ww_dec():
     global cond_val
     t = threading.Thread(target=cb_cond_inc)
     t.start()
     cond_val -= 1                       # WRITE — lost update
     t.join()
     return jsonify({"cond_val": cond_val})


# # --- Semaphore ---
sem_val   = 0
semaphore = threading.Semaphore(1)

def cb_sem_inc():
     global sem_val
     semaphore.acquire()
     sem_val += 1                        # WRITE protected
     semaphore.release()

@app.route("/acr/scalar/semaphore/ww_inc", methods=["POST"])
def acr_scalar_semaphore_ww_inc():
     global sem_val
     t = threading.Thread(target=cb_sem_inc)
     t.start()
     sem_val += 1                        # WRITE without semaphore — race
     t.join()
     return jsonify({"sem_val": sem_val})

@app.route("/acr/scalar/semaphore/ww_dec", methods=["POST"])
def acr_scalar_semaphore_ww_dec():
     global sem_val
     t = threading.Thread(target=cb_sem_inc)
     t.start()
     sem_val -= 1                        # WRITE — lost update
     t.join()
     return jsonify({"sem_val": sem_val})


# # --- BoundedSemaphore ---
bsem_val    = 0
bounded_sem = threading.BoundedSemaphore(1)

def cb_bsem_inc():
     global bsem_val
     bounded_sem.acquire()
     bsem_val += 1                       # WRITE protected
     bounded_sem.release()

@app.route("/acr/scalar/bounded_sem/ww_inc", methods=["POST"])
def acr_scalar_bounded_sem_ww_inc():
     global bsem_val
     t = threading.Thread(target=cb_bsem_inc)
     t.start()
     bsem_val += 1                       # WRITE without bounded_sem — race
     t.join()
     return jsonify({"bsem_val": bsem_val})

@app.route("/acr/scalar/bounded_sem/ww_multiply", methods=["POST"])
def acr_scalar_bounded_sem_ww_multiply():
     global bsem_val
     t = threading.Thread(target=cb_bsem_inc)
     t.start()
     bsem_val *= 2                       # WRITE — lost update
     t.join()
     return jsonify({"bsem_val": bsem_val})


# # --- Event ---
event_val   = 0
ready_event = threading.Event()

def cb_event_inc():
     global event_val
     event_val += 1                      # WRITE without lock
     ready_event.set()

@app.route("/acr/scalar/event/ww_inc", methods=["POST"])
def acr_scalar_event_ww_inc():
     global event_val
     t = threading.Thread(target=cb_event_inc)
     t.start()
     event_val += 1                      # WRITE — race with callback
     t.join()
     return jsonify({"event_val": event_val})

@app.route("/acr/scalar/event/ww_dec", methods=["POST"])
def acr_scalar_event_ww_dec():
     global event_val
     t = threading.Thread(target=cb_event_inc)
     t.start()
     event_val -= 1                      # WRITE — lost update
     t.join()
     return jsonify({"event_val": event_val})


# # --- Aliasing: mutable wrapper ---
box_val   = [0]
alias_box = box_val     # alias — same list object

def cb_box_inc():
     box_val[0] += 1                     # WRITE via original

@app.route("/acr/scalar/alias/ww_inc_original", methods=["POST"])
def acr_scalar_alias_ww_inc_original():
     t = threading.Thread(target=cb_box_inc)
     t.start()
     alias_box[0] += 1                   # WRITE via alias — race with callback via original
     t.join()
     return jsonify({"value": box_val[0]})

def cb_alias_dec():
     alias_box[0] -= 1                   # WRITE via alias

@app.route("/acr/scalar/alias/ww_dec_alias", methods=["POST"])
def acr_scalar_alias_ww_dec_alias():
     t = threading.Thread(target=cb_alias_dec)
     t.start()
     box_val[0] -= 1                     # WRITE via original — lost update with alias callback
     t.join()
     return jsonify({"value": box_val[0]})


# --- class: via instance (outside) ---
class AppState:
    def __init__(self):
        self.counter = 0
        self.rate    = 1.0
        self.status  = "idle"

state       = AppState()
state_alias = state     # alias

def cb_state_inc():
    state.counter += 1                  # WRITE via instance

@app.route("/acr/scalar/class_outside/ww_counter_inc", methods=["POST"])
def acr_scalar_class_outside_ww_counter_inc():
    t = threading.Thread(target=cb_state_inc)
    t.start()
    state.counter += 10                 # WRITE — race with callback
    t.join()
    return jsonify({"counter": state.counter})

def cb_state_dec():
    state.counter -= 1                  # WRITE via instance

@app.route("/acr/scalar/class_outside/ww_counter_dec", methods=["POST"])
def acr_scalar_class_outside_ww_counter_dec():
    t = threading.Thread(target=cb_state_dec)
    t.start()
    state.counter -= 1                  # WRITE — lost update
    t.join()
    return jsonify({"counter": state.counter})

def cb_alias_set_status():
    state_alias.status = "cb_done"      # WRITE via alias

@app.route("/acr/scalar/class_alias/ww_status", methods=["POST"])
def acr_scalar_class_alias_ww_status():
    t = threading.Thread(target=cb_alias_set_status)
    t.start()
    state.status = "main_done"          # WRITE via original — race with alias callback
    t.join()
    return jsonify({"status": state.status})

def cb_alias_multiply_rate():
    state_alias.rate *= 2.0             # WRITE via alias

@app.route("/acr/scalar/class_alias/ww_rate", methods=["POST"])
def acr_scalar_class_alias_ww_rate():
    t = threading.Thread(target=cb_alias_multiply_rate)
    t.start()
    state.rate *= request.get_json().get("factor", 1.5)  # WRITE — lost update
    t.join()
    return jsonify({"rate": state.rate})


# --- class: inside methods ---
class SharedCounter:
     def __init__(self):
         self.value = 0
         self.total = 0

     def _cb_increment(self):
         self.value += 1                 # WRITE from callback method

     def _cb_decrement(self):
         self.value -= 1                 # WRITE from callback method

     def _cb_multiply(self, factor):
         self.value *= factor            # WRITE from callback method

     def _cb_add_total(self, delta):
         self.total += delta             # WRITE from callback method

     def process_inc(self):
         t = threading.Thread(target=self._cb_increment)
         t.start()
         self.value += 100               # WRITE — race with _cb_increment
         t.join()
         return self.value

     def process_dec(self):
         t = threading.Thread(target=self._cb_decrement)
         t.start()
         self.value -= 1                 # WRITE — lost update with _cb_decrement
         t.join()
         return self.value

     def process_multiply(self, factor):
         t = threading.Thread(target=self._cb_multiply, args=(factor,))
         t.start()
         self.value *= factor            # WRITE — race
         t.join()
         return self.value

     def process_total(self, delta):
         t = threading.Thread(target=self._cb_add_total, args=(delta,))
         t.start()
         self.total += delta * 2         # WRITE — lost update
         t.join()
         return self.total

shared_ctr = SharedCounter()

@app.route("/acr/scalar/class_inside/inc", methods=["POST"])
def acr_scalar_class_inside_inc():
     return jsonify({"value": shared_ctr.process_inc()})

@app.route("/acr/scalar/class_inside/dec", methods=["POST"])
def acr_scalar_class_inside_dec():
     return jsonify({"value": shared_ctr.process_dec()})

@app.route("/acr/scalar/class_inside/multiply", methods=["POST"])
def acr_scalar_class_inside_multiply():
     factor = request.get_json().get("factor", 2.0)
     return jsonify({"value": shared_ctr.process_multiply(factor)})

@app.route("/acr/scalar/class_inside/add_total", methods=["POST"])
def acr_scalar_class_inside_add_total():
     delta = request.get_json().get("delta", 1)
     return jsonify({"total": shared_ctr.process_total(delta)})


# # ---------------------------------------------------------------------------
# # DETECTOR BLIND SPOTS
# # ---------------------------------------------------------------------------

blind_a = 0
blind_b = 0

# # --- blind spot 1: callback and endpoint access different variables ---
def cb_blind_write_b():
     global blind_b
     blind_b += 1                        # WRITE to blind_b

@app.route("/acr/scalar/blind/different_vars", methods=["POST"])
def acr_scalar_blind_different_vars():
     global blind_a
     t = threading.Thread(target=cb_blind_write_b)
     t.start()
     blind_a += 1                        # WRITE to blind_a — different variable — no race
     t.join()
     return jsonify({"a": blind_a, "b": blind_b})


# # --- blind spot 2: callback only reads (RR, not race) ---
read_val = 42

def cb_blind_read_only():
     local = read_val                    # READ only — no write

@app.route("/acr/scalar/blind/rr_no_race", methods=["GET"])
def acr_scalar_blind_rr_no_race():
     t = threading.Thread(target=cb_blind_read_only)
     t.start()
     local = read_val                    # READ only — RR, not race
     t.join()
     return jsonify({"value": local})


# # --- blind spot 3: callback receives variable as argument (no global sharing) ---
arg_val = 0

def cb_blind_arg(val):
     local = val + 1                     # works on argument, not global

@app.route("/acr/scalar/blind/arg_no_global", methods=["POST"])
def acr_scalar_blind_arg_no_global():
     global arg_val
     t = threading.Thread(target=cb_blind_arg, args=(arg_val,))  # passes copy
     t.start()
     arg_val = 99                        # WRITE to global — callback doesn't touch it
     t.join()
     return jsonify({"arg_val": arg_val})


# # --- blind spot 4: Thread created but not started ---
nostarter_val = 0

def cb_blind_nostarter():
     global nostarter_val
     nostarter_val += 1                  # WRITE — but thread never starts

@app.route("/acr/scalar/blind/not_started", methods=["POST"])
def acr_scalar_blind_not_started():
     global nostarter_val
     t = threading.Thread(target=cb_blind_nostarter)
#     # t.start() — intentionally not called — no race
     nostarter_val += 1
     return jsonify({"value": nostarter_val})


# # --- blind spot 5: join() before any further access ---
join_val = 0

def cb_blind_join_write():
     global join_val
     join_val += 1                       # WRITE in callback

@app.route("/acr/scalar/blind/join_before_access", methods=["POST"])
def acr_scalar_blind_join_before_access():
     global join_val
     t = threading.Thread(target=cb_blind_join_write)
     t.start()
     t.join()                            # join() before any access — no race window
     local = join_val                    # READ after join — safe
     return jsonify({"value": local})


# # --- blind spot 6: both sides locked with same shared lock ---
safe_val  = 0
safe_lock = threading.Lock()

def cb_blind_safe_write():
     global safe_val
     with safe_lock:
         safe_val += 1                   # WRITE under shared lock

@app.route("/acr/scalar/blind/both_locked", methods=["POST"])
def acr_scalar_blind_both_locked():
     global safe_val
     t = threading.Thread(target=cb_blind_safe_write)
     t.start()
     with safe_lock:
         safe_val += 1                   # WRITE under same lock — correctly protected
     t.join()
     return jsonify({"safe_val": safe_val})


if __name__ == "__main__":
     app.run(debug=True, threaded=True)
