// Include I2S driver
#include <driver/i2s.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// Include OLED Related Libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include "oled.h"
// Connections to INMP441 I2S microphone
#define I2S_WS 9
#define I2S_LR 10
#define I2S_SD 19
#define I2S_SCK 20

// Connections to the OLED Screen
#define LCD_SDA 42
#define LCD_SCL 41
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
OLED display(LCD_SDA,LCD_SCL,-1,OLED::W_128,OLED::H_64,OLED::CTRL_SH1106,0x3C);


// Connections for MAX98357A Speaker Output

#define I2S_OUT_SCK 13
#define I2S_OUT_WS 12
#define I2S_OUT_SD 14
// Use I2S Processor 1 for output
#define I2S_OUTPUT_PORT I2S_NUM_1

// Use I2S Processor 0
#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 8000
#define bufferLen 512  //enlarged the buffer
#define wavheaderLen 44
#define bodyStart_length 106
#define bodyEnd_length 24
#define buttonPin 8
#define MaxAudioTime 10
#define totalSize bodyStart_length + wavheaderLen + SAMPLE_RATE * MaxAudioTime * 2 + bodyEnd_length
String boundary = "----Boundary1234";
String contentType = "multipart/form-data; boundary=" + boundary;
String bodyStart = "--" + boundary + "\r\n"
                    "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n"
                    "Content-Type: audio/wav\r\n\r\n";
String bodyEnd = "\r\n--" + boundary + "--\r\n";

uint8_t payloadBuffer[totalSize];

// Replace with your network credentials
const char* ssid     = "who_am_I";
const char* password = "woshinidie";

void i2s_install() {
  // Set up I2S Processor configuration
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 8000,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = bufferLen,
    .use_apll = false
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin() {
  // Set I2S pin configuration
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}

void i2s_install_output() {
  // Set up I2S Processor configuration for output
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 8000,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = bufferLen,
    .use_apll = false
  };

  esp_err_t err = i2s_driver_install(I2S_OUTPUT_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
      Serial.println("I2S output driver installation failed");
  } else {
      Serial.println("I2S output driver installed successfully");
  }
}

void i2s_setpin_output() {
  // Set I2S pin configuration for output
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_OUT_SCK,
    .ws_io_num = I2S_OUT_WS,
    .data_out_num = I2S_OUT_SD,
    .data_in_num = -1
  };

  i2s_set_pin(I2S_OUTPUT_PORT, &pin_config);
}

void playSilence() {
    const int silenceBufferLen = bufferLen;
    int16_t silenceBuffer[silenceBufferLen] = {0};
    size_t bytesWritten = 0;

    for (int i = 0; i < 3; i++) {
        esp_err_t ret = i2s_write(I2S_OUTPUT_PORT, 
                                  (char *)silenceBuffer, 
                                  sizeof(silenceBuffer), 
                                  &bytesWritten, 
                                  portMAX_DELAY);
        if (ret != ESP_OK) {
            Serial.printf("I2S write error during silence playback: %s\n", esp_err_to_name(ret));
        }
    }

    Serial.println("Silence written to I2S output.");
}

void generateWavHeader(uint8_t *header, int wavDataSize) {
  int byteRate = SAMPLE_RATE * 1 * 16 / 8;
  int blockAlign = 1 * 16 / 8;

  // "RIFF" chunk descriptor
  header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
  int chunkSize = 36 + wavDataSize;
  header[4] = (uint8_t)(chunkSize & 0xff);
  header[5] = (uint8_t)((chunkSize >> 8) & 0xff);
  header[6] = (uint8_t)((chunkSize >> 16) & 0xff);
  header[7] = (uint8_t)((chunkSize >> 24) & 0xff);

  // "WAVE" format
  header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';

  // "fmt" sub-chunk
  header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
  header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0; // Subchunk1Size (16 for PCM)
  header[20] = 1; header[21] = 0;  // Audio format (1 = PCM)
  header[22] = 1; header[23] = 0;  // Num channels (1 = mono)
  header[24] = (uint8_t)(SAMPLE_RATE & 0xff);
  header[25] = (uint8_t)((SAMPLE_RATE >> 8) & 0xff);
  header[26] = (uint8_t)((SAMPLE_RATE >> 16) & 0xff);
  header[27] = (uint8_t)((SAMPLE_RATE >> 24) & 0xff);
  header[28] = (uint8_t)(byteRate & 0xff);
  header[29] = (uint8_t)((byteRate >> 8) & 0xff);
  header[30] = (uint8_t)((byteRate >> 16) & 0xff);
  header[31] = (uint8_t)((byteRate >> 24) & 0xff);
  header[32] = (uint8_t)blockAlign;
  header[33] = 0;
  header[34] = 16; header[35] = 0;  // Bits per sample

  // "data" sub-chunk
  header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
  header[40] = (uint8_t)(wavDataSize & 0xff);
  header[41] = (uint8_t)((wavDataSize >> 8) & 0xff);
  header[42] = (uint8_t)((wavDataSize >> 16) & 0xff);
  header[43] = (uint8_t)((wavDataSize >> 24) & 0xff);
}

void parseWavHeader(File file) {
    uint8_t header[wavheaderLen];
    file.seek(0);
    file.read(header, sizeof(header));

    uint32_t sampleRate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
    uint16_t bitsPerSample = header[34] | (header[35] << 8);
    file.seek(22);
    uint8_t numChannels[2];
    file.read(numChannels, 2);
    uint16_t channels = numChannels[0] | (numChannels[1] << 8);
    Serial.printf("Number of Channels: %d\n", channels);
    Serial.printf("Sample Rate: %d Hz\n", sampleRate);
    Serial.printf("Bits Per Sample: %d\n", bitsPerSample);
}

int uploadWav(size_t dataSize) {
  int wav_time = 0;
  String transcription;
  HTTPClient http;
  http.begin("https://d53c-130-126-255-48.ngrok-free.app/upload");
  http.setTimeout(60000);
  http.addHeader("Content-Type", contentType);
  // uint8_t wavHeader[wavheaderLen];
  // size_t totalSize = bodyStart.length() + wavheaderLen + dataSize + bodyEnd.length();
  generateWavHeader(payloadBuffer + bodyStart.length(), dataSize);
  // uint8_t *payload = (uint8_t *)malloc(totalSize);
  // if (payload == NULL) {
  //     Serial.println("Failed to allocate memory for upload payload");
  //     return;
  // }

  memcpy(payloadBuffer, bodyStart.c_str(), bodyStart.length());                     
  memcpy(payloadBuffer + bodyStart.length() + wavheaderLen + dataSize, bodyEnd.c_str(), bodyEnd.length()); 

  const char* headers[] = {"X-Transcription", "Content-Length"};
  http.collectHeaders(headers, 2);
  // 发送 POST 请求
  int httpResponseCode = http.POST(payloadBuffer, bodyStart.length() + wavheaderLen + dataSize + bodyEnd.length());
  if (httpResponseCode == 200) {
      WiFiClient *stream = http.getStreamPtr();
      File file = SPIFFS.open("/response.wav", FILE_WRITE);

      if (!file) {
          Serial.println("Failed to open file for writing");
          http.end();
          return 0;
      }

      if (http.hasHeader("X-Transcription")) {
          transcription = http.header("X-Transcription");
          Serial.printf("Transcription: %s\n", transcription.c_str());
      } else {
          Serial.println("No X-Transcription header found.");
      }

      Serial.println("Receiving file...");

      uint8_t buffer[1024];
      int totalBytes = 0;
      Serial.printf("Total space: %d bytes\n", SPIFFS.totalBytes());
      Serial.printf("Used space: %d bytes\n", SPIFFS.usedBytes());
      int contentLength = http.getSize();
      wav_time = int(contentLength / 16000) + 1;

      Serial.printf("contentLength: %d\n", contentLength);
      display.printf("\n Retrieving AI response");
      while (totalBytes < contentLength) {
          // Serial.printf("Available bytes before read: %d\n", stream->available());
          int len = stream->readBytes(buffer, sizeof(buffer));
          file.write(buffer, len); // 写入 SPIFFS
          totalBytes += len;
          int percent = int((totalBytes * 100)/contentLength);
          Serial.printf("Chunk received: %d bytes, Total received: %d bytes\n", len, totalBytes);
          if ((percent % 20) == 0) {
            display.printf("\n Fetching: %d %% ", percent);
          }
      }

      file.close();
      Serial.println("File download complete!");
  } else {
      Serial.printf("HTTP POST failed: %s\n", http.errorToString(httpResponseCode).c_str());
      String errormsg = "\n Audio Undetected - please speak closer to the microphone.";
      parsing(errormsg);
  }
  // Serial.printf("size of payload: %d ", totalSize);
  parsing(transcription.c_str());
  http.end(); 
  return wav_time;
}


  void setup() {
  Serial.begin(115200);
  pinMode(I2S_LR, OUTPUT);
  digitalWrite(I2S_LR, LOW);
  // Connect to Wi-Fi network
  WiFi.begin(ssid, password);
  //OLED Set up
  display.begin();
  display.setTTYMode(1);
  display.printf(" Chat with Ruan Mei");
  display.display();
  Serial.print("Connecting to WiFi ..");
  display.printf("\n Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      display.printf("\n .");
      delay(1000);
  }
  Serial.println("Connected to WiFi");
  display.printf("\n Connected to WiFi");
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);

  // Install and configure output I2S
  i2s_install_output();
  i2s_setpin_output();
  i2s_start(I2S_OUTPUT_PORT);

  uint8_t button_flag = 0;
  pinMode(buttonPin, INPUT_PULLDOWN);
  delay(500);

  if (!SPIFFS.begin(true)) {
      Serial.println("Failed to mount SPIFFS");
      return;
  }
}

void printWavHeader(uint8_t *header, int size) {
for (int i = 0; i < size; i++) {
    Serial.print(header[i], HEX);
    Serial.print(" ");
}
Serial.println();
}

void parsing(String transcription) {
  int vertical_counter = 0;
  // Serial.println("Splitting string: \n");
  
  // Convert the Arduino String to a mutable char array
  char buffer[transcription.length() + 1];
  transcription.toCharArray(buffer, sizeof(buffer));
  display.printf("\n"); 
  // Tokenize the string
  char *word = strtok(buffer, " "); // Get the first token
  
  while (word != NULL) {
      int wordLength = strlen(word);
      vertical_counter += (wordLength + 1);
      
      if (vertical_counter >= 18) {
          vertical_counter -= 18;
          display.printf("\n %s", word); // Print on a new line
      } else {
          display.printf(" %s", word);  // Print on the same line
      }
      
      Serial.println(word); // Debugging: Print the word
      word = strtok(NULL, " "); // Get the next token
    }
}

void loop() {
  // Serial.println(digitalRead(buttonPin));
  if (digitalRead(buttonPin)) {
    size_t bytesRead = 0;
    int totalSamples = 0;

    Serial.println("Recording audio...");
    display.printf("\n Recording audio...");

    // Create a buffer
    int16_t audioBuffer[bufferLen];
    // int16_t *dataBuffer = (int16_t *)malloc(dataBufferSize);

    // if (dataBuffer == NULL) {
    //   Serial.println("Failed to allocate memory for dataBuffer"); 
    //   return; 
    // }
    // Start recording
    while (digitalRead(buttonPin) && totalSamples < SAMPLE_RATE * MaxAudioTime * 2) {  
        if (i2s_read(I2S_PORT, (char *)audioBuffer, bufferLen * sizeof(int16_t), &bytesRead, portMAX_DELAY) == ESP_OK) {
            memcpy(payloadBuffer + bodyStart.length() + wavheaderLen + totalSamples, audioBuffer, bytesRead);
            totalSamples += bytesRead; // the length of each sample is 2 bytes
        }
        // Serial.print("digital: ");
        // Serial.println(digitalRead(buttonPin));
    }

    // Calculate the actual total data szie
    int dataSize = totalSamples;  
    
    Serial.println("Recording complete!");
    parsing("Recording complete!");
    Serial.println();
    // Serial.write((uint8_t *)dataBuffer,dataSize);
    int delay_wav_time = uploadWav(dataSize) * 1000;

    File file = SPIFFS.open("/response.wav", FILE_READ);

    if (file) {
        Serial.printf("SPIFFS File size: %d bytes\n", file.size());
    } else {
        Serial.println("Failed to open file for size check.");
    }
    parseWavHeader(file);
    // Skip the 44-byte WAV header
    file.seek(44);
  
    int16_t playBuffer[bufferLen];
    // Play the audio file through the speaker
    while (file.available()) {
      size_t file_bytesRead = file.read((uint8_t *)playBuffer, sizeof(playBuffer));
      size_t file_bytesWritten = 0;

      while (file_bytesWritten < file_bytesRead) {
        size_t bytesToWrite = file_bytesRead - file_bytesWritten;
        if (bytesToWrite > bufferLen * sizeof(int16_t)) {
            bytesToWrite = bufferLen * sizeof(int16_t);  // 限制写入大小
        }
        size_t len = 0;
        esp_err_t ret = i2s_write(I2S_OUTPUT_PORT, 
                                  (char *)(playBuffer + file_bytesWritten / 2), 
                                  bytesToWrite, 
                                  &len, 
                                  portMAX_DELAY);
        if (ret != ESP_OK) {
            Serial.printf("I2S write error: %s\n", esp_err_to_name(ret));
            break;
        }
        file_bytesWritten += len;
      }
    }
    
    file.close();
    delay(1000);
    playSilence();
    Serial.println("Playback complete!");
    if (SPIFFS.exists("/response.wav")) {
      if (SPIFFS.remove("/response.wav")) {
            Serial.println("Old file deleted successfully.");
      } else {
            Serial.println("Failed to delete old file.");
      }
    } else {
        Serial.println("No old file found.");
    }
    delay(1000); // 
  } else {
    // Serial.println("Waiting!");
    delay(100);
  }
}
