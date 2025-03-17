import os
import subprocess

script_path = os.path.join(os.getcwd(), "gen_version.sh")
subprocess.run(["bash", script_path], check=True)
