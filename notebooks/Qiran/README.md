# Here is Qiran (Wesley) Pang and this is my notebook.
I am mainly responsible for the embedded system development, especially the audio I/O and processing

# Week of 9/30
Discussion with the team during the meeting to settled down more detail on the project.

Including components, detailed dataflow, and work distribution.

I will mainly take charge of the embedded system part.

Asked Dongming and Jason for some advices on the project regarding the embedded system.

Learn more about audio I/O and interaction with AI.

# Week of 10/7
The audio output is easier to test so I started to test the audio output from the SP-1504 speaker with dev board.

Took some time to connect the amplifier and the speaker together with the dev board.

Try to use arduino-audio-tools from pschatzmann to test but not making much progress.

https://github.com/pschatzmann/arduino-audio-tools

I2S is similar with I2C but has three signal lines: SCK, WS, and SD.

# Week of 10/14
Still cannot play sound output from the speaker so started testing I2S input on the development board with INMP441 microphone.

Hard to see the effect so need to use oscilliscope to monitor the signal

I test audio based on this dronebotworkship website. It has a lot of details.

dronebotworkshop.com/esp32-i2s

We also ordered PCB this week.

# Week of 10/21
Wrote sine_wave_speaker_test code to test the output.

Turns out that I did not connect the pins to the correct gpio on the dev board correctly because some of the gpios on the dev board should not be used.

After fixing it with my teammates, we successfully hear the audio output from the speaker. 

However, the output sound quality is very bad. Even the sine wave output sounds noisy. Decide to buy a new speaker.

PCB have not arrived yet. TA told us that it may take another week.

# Week of 10/28
Since Chengyuan finished his development of server, I started to test the server with python code.

The code (server.py) sends http requests first with config info and second with an wav file audio.

The response is a wav file and will be saved locally to see the result.

Testing went well and the AI web client respond in time.

PCB finally arrived later this week.

# Week of 11/4 
Since the PCB arrived. We started testing on it. 

However, after soldering, we found that the ESP32 cannot enter the programming state, therefore we cannot flash code onto the microcontroller.

After discussion with Dongming, we realized that the PCB layout wiring is faulty such that we cannot use button to perform the required ESP32 control sequence to enter the programming state.

Ryan redesigned the board and send another order. This is likely to be our last time to get PCB order since the last PCB arrived late.

# Week of 11/11
While waiting for the new PCB, I continue my testing on the audio I/O system.

Because I still cannot get input audio from the microphone, I suspect that the microphone to be faulty.

We ordered some new INMP441 online for testing.

# Week of 11/18
With new microphone, the input system is finally working.

By combining the input and output code, I can try outputting the input audio onto the new speaker.

After fixing some small bugs, it is successfuly at last. Incorporating the button, I can press the button to speak and release the button to let the speaker play out my voice.

# Week of 11/25 Thanksgiving break
We need to very hard during the break because the week after will be the demo.

After soldering the PCB, I started to test the audio I/O on the PCB.

This time we can successfully flash program into the processor and the audio system works well.



Start to incorporate Chengyuan's code on wav packaging.

The connections between components were often unstable. Ryan soldered the wires together for more stability.

After verifying the wav file was packed correctly, we started testing http request packaging.

On the server, I added the text as a header so it can respond the wav file with text for text display.



Everything works fine now but the latency of uploading and downloading is still high. 

We decide to optimize it by compressing the input and output audio data.

# Week of 12/2
Before the demo, we successfully compress the input and output data such that the speed of our system reached the requirement.

It is time to wrap up for demo, presentation, and final report.