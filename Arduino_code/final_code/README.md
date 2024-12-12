This is the final Arduino code for the project.

It receive input from microphone through I2S, package it into a http request, and send it to the server.

After the server side respond, the respond wav file will be stored in SPIFF flash memory and played out with speaker through I2S.