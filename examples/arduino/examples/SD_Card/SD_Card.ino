/* microSD 4-bit SDMMC read/write demo for Waveshare ESP32-P4-Platform. */
#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <Waveshare_Arduino_Logging.h>

constexpr int PLATFORM_SD_CLK = 43;
constexpr int PLATFORM_SD_CMD = 44;
constexpr int PLATFORM_SD_D0 = 39;
constexpr int PLATFORM_SD_D1 = 40;
constexpr int PLATFORM_SD_D2 = 41;
constexpr int PLATFORM_SD_D3 = 42;

void setup() {
  waveshare::logging::beginSerialLog();
  if (!SD_MMC.setPins(PLATFORM_SD_CLK, PLATFORM_SD_CMD, PLATFORM_SD_D0,
                      PLATFORM_SD_D1, PLATFORM_SD_D2, PLATFORM_SD_D3)) {
    Serial.println("Platform SDMMC pin configuration failed");
    return;
  }
  if (!SD_MMC.begin("/sdcard", false /* four-bit mode */)) {
    Serial.println("Card mount failed; insert a FAT32-formatted microSD card");
    return;
  }

  Serial.printf("Card size: %llu MB\n", SD_MMC.cardSize() / (1024ULL * 1024ULL));
  const char *path = "/platform_sd_card.txt";
  File file = SD_MMC.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open the Platform SD test file");
    return;
  }
  file.println("Waveshare ESP32-P4-Platform SDMMC test");
  file.close();

  file = SD_MMC.open(path, FILE_READ);
  if (!file) {
    Serial.println("Failed to read the Platform SD test file");
    return;
  }
  Serial.println("Read back:");
  while (file.available()) Serial.write(file.read());
  file.close();
}

void loop() {
  delay(1000);
}
