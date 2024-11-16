import requests
import sys
import os
from dotenv import load_dotenv

load_dotenv()

opponent = sys.argv[1]
time_seconds = sys.argv[2]

header = {
    "Authorization": "Bearer " + str(os.environ.get("TOKEN"))
}

body = {
    "clock.limit": f"{time_seconds}",
    "clock.increment": "3",
    "variant": "standard"
}

resp = requests.post(f"https://lichess.org/api/challenge/{opponent}", json=body, headers=header)
print(resp.json)

