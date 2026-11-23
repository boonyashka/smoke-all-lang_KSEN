from flask import Flask

from jobs import create_job, jobs


app = Flask(__name__)


@app.route("/jobs", methods=["POST"])
def submit_job():
    create_job()
    jobs.append("submitted")
    return "accepted"
