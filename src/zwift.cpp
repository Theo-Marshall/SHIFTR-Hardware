#include <Arduino.h>
#include <vector>
#include <zwift.h>
#include <leb128.h>

std::vector<uint8_t> generateMagicWordAnswer()
{
  std::vector<uint8_t> magicWordAnswer;
  for (uint8_t i = 0; i < strlen(ZWIFT_MAGIC_WORD); i++)
  {
    magicWordAnswer.push_back(ZWIFT_MAGIC_WORD[i]);
  }
  magicWordAnswer.push_back(0x02);
  magicWordAnswer.push_back(0x00);
  return magicWordAnswer;
}

std::vector<uint8_t> generateZwiftAsyncNotificationData(int64_t power, int64_t cadence, int64_t unknown1, int64_t unknown2, int64_t unknown3, int64_t unknown4)
{
  std::vector<uint8_t> notificationData;
  int64_t currentData = 0;
  uint8_t leb128Buffer[16];
  size_t leb128Size = 0;

  notificationData.push_back(0x03);

  for (uint8_t dataBlock = 0x08; dataBlock <= 0x30; dataBlock += 0x08)
  {
    notificationData.push_back(dataBlock);
    if (dataBlock == 0x08)
    {
      currentData = power;
    }
    if (dataBlock == 0x10)
    {
      currentData = cadence;
    }
    if (dataBlock == 0x18)
    {
      currentData = unknown1;
    }
    if (dataBlock == 0x20)
    {
      currentData = unknown2;
    }
    if (dataBlock == 0x28)
    {
      currentData = unknown3;
    }
    if (dataBlock == 0x30)
    {
      currentData = unknown4;
    }
    leb128Size = bfs::EncodeLeb128(currentData, leb128Buffer, sizeof(leb128Buffer));
    for (uint8_t leb128Byte = 0; leb128Byte < leb128Size; leb128Byte++)
    {
      notificationData.push_back(leb128Buffer[leb128Byte]);
    }
  }
  return notificationData;
}