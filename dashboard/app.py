from collections import deque
from datetime import datetime

from flask import Flask
from flask import jsonify
from flask import render_template


app = Flask(__name__)

SYSMON_DATA_PATH = "/var/lib/sysmon"
TODAY_STR = datetime.today().strftime("%Y%m%d")

def tail(filename, n=60):
    """Return the last n lines of a file
    
    Line schema:
    * timestamp,uptime_seconds,free_memory_bytes,total_memory_bytes,cpu_temperature_millicelsius,cpu_frequency_khz,hostname\n"
    * idx 0    ,idx 1         ,idx 2            ,idx 3             ,idx 4                       ,idx 5            ,idx 6
    """
    return list(deque(open(filename, "r"), n))

@app.route("/")
def index():
    return render_template("index.html") 

@app.route("/api/data")
def get_sysmon_data():
    last_n_entries = tail(f"{SYSMON_DATA_PATH}/{TODAY_STR}.csv")

    entries = [entry.strip().split(",") for entry in last_n_entries]
    
    response = []
    for entry in entries:
        free_memory = int(entry[2])
        total_memory = int(entry[3])
        cpu_temp = int(entry[4])
        cpu_freq = int(entry[5])

        resp_dict = {
            "timestamp": entry[0],
            "uptime_seconds": int(entry[1]),
            "free_memory_mib": free_memory / (1024 ** 2),
            "memory_in_use": (total_memory - free_memory) / total_memory * 100,
            "cpu_temp_celsius": cpu_temp / 1000,
            "cpu_freq_mhz": cpu_freq / 1000,
            "hostname": entry[6]
        }

        response.append(resp_dict)

    return jsonify(response)
        

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
