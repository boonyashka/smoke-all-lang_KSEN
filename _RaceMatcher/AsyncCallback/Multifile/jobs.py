import threading


jobs = []


def update_jobs():
    jobs.append("completed")


def create_job():
    thread = threading.Thread(target=update_jobs)
    thread.start()
