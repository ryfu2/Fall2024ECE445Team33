// Include I2S driver
#include <driver/i2s.h>

// Connections to INMP441 I2S microphone
#define I2S_WS 9
#define I2S_SD 19
#define I2S_SCK 20

// Use I2S Processor 0
#define I2S_PORT I2S_NUM_0
#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 44100
#define bufferLen 1024  //enlarged the buffer
#define wavheaderLen 44
#define buttonPin 18
int16_t audioBuffer[bufferLen];

void i2s_install() {
  // Set up I2S Processor configuration
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
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

void setup() {
  Serial.begin(115200);
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
  uint8_t button_flag = 0;
  pinMode(buttonPin, INPUT_PULLDOWN);
  delay(500);
}

void printWavHeader(uint8_t *header, int size) {
  for (int i = 0; i < size; i++) {
      Serial.print(header[i], HEX);
      Serial.print(" ");
  }
  Serial.println();
}

void loop() {
  // Serial.println(digitalRead(buttonPin));
  if (digitalRead(buttonPin)) {
    size_t bytesRead = 0;
    uint8_t wavHeader[44];
    int totalSamples = 0;

    // Serial.println("Recording audio...");

    // Create a buffer
    int16_t audioBuffer[bufferLen];
    int dataBufferSize = SAMPLE_RATE * 10 * 2; // Tuning
    int16_t *dataBuffer = (int16_t *)ps_malloc(dataBufferSize);

    if (dataBuffer == NULL) {
      Serial.println("Failed to allocate memory for dataBuffer");
      return; 
    }
    // Serial.println("Before");
    // Serial.println("After");
    // Start recording
    while (digitalRead(buttonPin) && totalSamples < SAMPLE_RATE * 10) {  
        if (i2s_read(I2S_PORT, (char *)audioBuffer, bufferLen * sizeof(int16_t), &bytesRead, portMAX_DELAY) == ESP_OK) {
            memcpy(dataBuffer + totalSamples, audioBuffer, bytesRead);
            totalSamples += bytesRead / 2; // the length of each sample is 2 bytes
        }
        // Serial.print("digital: ");
        // Serial.println(digitalRead(buttonPin));
    }

    // Calculate the actual total data szie
    int dataSize = totalSamples * 2;  
    generateWavHeader(wavHeader, dataSize);
    // printWavHeader(wavHeader, 44);
    
    Serial.write(wavHeader, 44);
    Serial.write((uint8_t *)dataBuffer, dataSize);

    free(dataBuffer); 
    Serial.println("Recording complete!");
    delay(5000); // 
  } else {
    // Serial.println("Waiting!");
    delay(100);
  }
}
