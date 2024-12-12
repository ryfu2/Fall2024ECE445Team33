
import requests
import json

url = 'https://86ca-153-33-112-212.ngrok-free.app'

data = {
    "character": "kurisu",
    "username": "YourUsername",
    "language": "en"
}

response = requests.post(url + '/set_model', headers={"Content-Type": "application/json"}, data=json.dumps(data))

print(response.text)

files = {'file': open('./record_out.wav', 'rb')}
data = {
    'username': 'YourUsername',
    'language': 'en'
}

response = requests.post(url + '/upload', files=files, data=data)

print(response.status_code)  # This will print the response audio file in WAV format
if response.status_code == 200:
    with open('response.wav', 'wb') as f:
        f.write(response.content)
    print('WAV file saved successfully as response.wav')