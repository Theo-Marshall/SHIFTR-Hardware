#include <Arduino.h>
#include <DirConMessage.h>
#include <utils.h>

DirConMessage::DirConMessage() {}

std::vector<uint8_t>* DirConMessage::encode(uint8_t sequenceNumber) {
  // log_d("Encoding DirCon message");
  if (this->Identifier != DIRCON_MSGID_ERROR) {
    this->encodedMessage.clear();
    this->MessageVersion = 1;
    if (this->Request) {
      // log_d("Increasing sequence number");
      if (this->SequenceNumber < 255) {
        this->SequenceNumber++;
      } else {
        this->SequenceNumber = 0;
      }
      //this->SequenceNumber = (sequenceNumber & 0xFF);
    } else if (this->Identifier == DIRCON_MSGID_UNSOLICITED_CHARACTERISTIC_NOTIFICATION) {
      // log_d("Setting sequence number to 0");
      this->SequenceNumber = 0;
    } else {
      // log_d("Setting sequence number to last sequence number %d", sequenceNumber);
      this->SequenceNumber = sequenceNumber;
    }
    this->encodedMessage.push_back(this->MessageVersion);
    this->encodedMessage.push_back(this->Identifier);
    this->encodedMessage.push_back(this->SequenceNumber);
    this->encodedMessage.push_back(this->ResponseCode);

    if (!this->Request && this->ResponseCode != DIRCON_RESPCODE_SUCCESS_REQUEST) {
      // log_d("Message isn't a request and responsecode isn't DIRCON_RESPCODE_SUCCESS_REQUEST, so length = 0");
      this->Length = 0;
      this->encodedMessage.push_back((uint8_t)(this->Length >> 8));
      this->encodedMessage.push_back((uint8_t)(this->Length));
    } else if (this->Identifier == DIRCON_MSGID_DISCOVER_SERVICES) {
      // log_d("Message type is DIRCON_MSGID_DISCOVER_SERVICES");
      if (this->Request) {
        // log_d("Message is a request, so length = 0");
        this->Length = 0;
        this->encodedMessage.push_back((uint8_t)(this->Length >> 8));
        this->encodedMessage.push_back((uint8_t)(this->Length));
      } else {
        this->Length = this->AdditionalUUIDs.size() * 16;
        // log_d("Message isn't a request, so length is count(AdditionalUUIDs) * 16 = %d", this->Length);
        this->encodedMessage.push_back((uint8_t)(this->Length >> 8));
        this->encodedMessage.push_back((uint8_t)(this->Length));
        // log_d("Appending %d UUIDs", this->AdditionalUUIDs.size());
        for (size_t counter = 0; counter < this->AdditionalUUIDs.size(); counter++) {
          uint8_t *uuidBytes = (uint8_t *)this->AdditionalUUIDs[counter].to128().getNative();
          for (uint8_t index = 16; index > 0; index--) {
            this->encodedMessage.push_back(uuidBytes[index]);
          }
        }
      }
    } else if (this->Identifier == DIRCON_MSGID_DISCOVER_CHARACTERISTICS && !this->Request) {
      this->Length = 16 + this->AdditionalUUIDs.size() * 17;
      // log_d("Message isn't a request and type is DIRCON_MSGID_DISCOVER_CHARACTERISTICS, so length is 16 + count(AdditionalUUIDs) * 17 = %d", this->Length);
      this->encodedMessage.push_back((uint8_t)(this->Length >> 8));
      this->encodedMessage.push_back((uint8_t)(this->Length));
      // log_d("Appending UUID");
      uint8_t *uuidBytes = (uint8_t *)this->UUID.to128().getNative();
      for (uint8_t index = 16; index > 0; index--) {
        this->encodedMessage.push_back(uuidBytes[index]);
      }
      // log_d("Appending %d UUIDs and additional data of size %d", this->AdditionalUUIDs.size(), this->AdditionalData.size());
      size_t dataIndex = 0;
      for (size_t counter = 0; counter < this->AdditionalUUIDs.size(); counter++) {
        uint8_t *uuidBytes = (uint8_t *)this->AdditionalUUIDs[counter].to128().getNative();
        for (uint8_t index = 16; index > 0; index--) {
          this->encodedMessage.push_back(uuidBytes[index]);
        }
        this->encodedMessage.push_back(this->AdditionalData[dataIndex]);
        dataIndex++;
      }
    } else if (((this->Identifier == DIRCON_MSGID_READ_CHARACTERISTIC ||
                 this->Identifier == DIRCON_MSGID_DISCOVER_CHARACTERISTICS) &&
                this->Request) ||
               (this->Identifier == DIRCON_MSGID_ENABLE_CHARACTERISTIC_NOTIFICATIONS && !this->Request)) {
      this->Length = 16;
      // log_d("Message is a request and type is DIRCON_MSGID_[READ|DISCOVER]_CHARACTERISTICS or isn't a request and type is DIRCON_MSGID_ENABLE_CHARACTERISTIC_NOTIFICATIONS, so length = %d", this->Length);
      this->encodedMessage.push_back((uint8_t)(this->Length >> 8));
      this->encodedMessage.push_back((uint8_t)(this->Length));
      // log_d("Appending UUID");
      uint8_t *uuidBytes = (uint8_t *)this->UUID.to128().getNative();
      for (uint8_t index = 16; index > 0; index--) {
        this->encodedMessage.push_back(uuidBytes[index]);
      }

    } else if (this->Identifier == DIRCON_MSGID_WRITE_CHARACTERISTIC ||
               this->Identifier == DIRCON_MSGID_UNSOLICITED_CHARACTERISTIC_NOTIFICATION ||
               (this->Identifier == DIRCON_MSGID_READ_CHARACTERISTIC && !this->Request) ||
               (this->Identifier == DIRCON_MSGID_ENABLE_CHARACTERISTIC_NOTIFICATIONS && this->Request)) {
      this->Length = 16 + this->AdditionalData.size();
      // log_d("Message type is DIRCON_MSGID_WRITE_CHARACTERISTICS or DIRCON_MSGID_UNSOLICITED_CHARACTERISTIC_NOTIFICATION or isn't a request and DIRCON_MSGID_READ_CHARACTERISTIC or is a request and DIRCON_MSGID_ENABLE_CHARACTERISTIC_NOTIFICATIONS, so length = %d", this->Length);
      this->encodedMessage.push_back((uint8_t)(this->Length >> 8));
      this->encodedMessage.push_back((uint8_t)(this->Length));
      // log_d("Appending UUID");
      uint8_t *uuidBytes = (uint8_t *)this->UUID.to128().getNative();
      for (uint8_t index = 16; index > 0; index--) {
        this->encodedMessage.push_back(uuidBytes[index]);
      }
      // log_d("Appending additional data of size %d", this->AdditionalData.size());
      for (size_t counter = 0; counter < this->AdditionalData.size(); counter++) {
        this->encodedMessage.push_back(this->AdditionalData[counter]);
      }
    }
  } else {
    // log_d("Identifier is DIRCON_MSGID_ERROR, returning no data");
  }

  //log_d("Encoded DirCon message with length %d and hex value: %s", encodedMessage.size(), Utils::getHexString(encodedMessage.data(), encodedMessage.size()).c_str());
  return &(this->encodedMessage);
}

size_t DirConMessage::parse(uint8_t *data, size_t len, uint8_t sequenceNumber) {
  // //log_d("Parsing DirCon message with hex value: %s", getHexString(data, len).c_str());
  if (len < DIRCON_MESSAGE_HEADER_LENGTH) {
    log_e("Error parsing DirCon message: Header length %d < %d", len, DIRCON_MESSAGE_HEADER_LENGTH);
    this->Identifier = DIRCON_MSGID_ERROR;
    return 0;
  }
  this->MessageVersion = data[0];
  this->Identifier = data[1];
  this->SequenceNumber = data[2];
  this->ResponseCode = data[3];
  this->Length = (data[4] << 8) | data[5];
  this->Request = false;
  this->UUID = NimBLEUUID();
  this->AdditionalData.clear();
  this->AdditionalUUIDs.clear();

  if ((len - DIRCON_MESSAGE_HEADER_LENGTH) < this->Length) {
    log_e("Error parsing DirCon message: Content length %d < %d", (len - DIRCON_MESSAGE_HEADER_LENGTH), this->Length);
    this->Identifier = DIRCON_MSGID_ERROR;
    return 0;
  }

  size_t parsedBytes = 6;
  switch (this->Identifier) {
    case DIRCON_MSGID_DISCOVER_SERVICES:
      // log_d("Parsing DirCon request type DIRCON_MSGID_DISCOVER_SERVICES");
      if (!this->Length) {
        // log_d("DirCon request has no length, just checking if isRequest() of sequence %d", sequenceNumber);
        this->Request = this->isRequest(sequenceNumber);
      } else if ((this->Length % 16) == 0) {
        // log_d("Parsing UUIDs in request");
        this->AdditionalUUIDs.clear();
        size_t index = 0;
        while (this->Length >= index + 16) {
          NimBLEUUID uuid(data + DIRCON_MESSAGE_HEADER_LENGTH + index, 16, true);
          this->AdditionalUUIDs.push_back(uuid);
          index += 16;
          parsedBytes += 16;
        }
      } else {
        log_e("Error parsing DirCon message: Length %d isn't a multiple of 16", this->Length);
        this->Identifier = DIRCON_MSGID_ERROR;
        return 0;
      }
      break;

    case DIRCON_MSGID_DISCOVER_CHARACTERISTICS:
      // log_d("Parsing DirCon request type DIRCON_MSGID_DISCOVER_CHARACTERISTICS");
      if (this->Length >= 16) {
        // log_d("Parsing UUID of request");
        NimBLEUUID uuid(data + DIRCON_MESSAGE_HEADER_LENGTH, 16, true);
        this->UUID = uuid;
        parsedBytes += 16;
        if (this->Length == 16) {
          // log_d("DirCon request only has one UUID, just checking if isRequest() of sequence %d", sequenceNumber);
          this->Request = this->isRequest(sequenceNumber);
        } else if ((this->Length - 16) % 17 == 0) {
          // log_d("Parsing additional UUIDs and additional data of request");
          this->AdditionalUUIDs.clear();
          this->AdditionalData.clear();
          size_t index = 16;
          while (this->Length >= index + 17) {
            NimBLEUUID uuid(data + DIRCON_MESSAGE_HEADER_LENGTH + index, 16, true);
            this->AdditionalUUIDs.push_back(uuid);
            this->AdditionalData.push_back((uint8_t)data[DIRCON_MESSAGE_HEADER_LENGTH + index + 16]);
            index += 17;
            parsedBytes += 17;
          }
        } else {
          log_e("Error parsing additional UUIDs and data: Length %d isn't a multiple of 17", (this->Length - 16));
          this->Identifier = DIRCON_MSGID_ERROR;
          return 0;
        }
      } else {
        log_e("Error parsing DirCon message: Length %d < 16", this->Length);
        this->Identifier = DIRCON_MSGID_ERROR;
        return 0;
      }
      break;

    case DIRCON_MSGID_READ_CHARACTERISTIC:
      // log_d("Parsing DirCon request type DIRCON_MSGID_READ_CHARACTERISTIC");
      if (this->Length >= 16) {
        // log_d("Parsing UUID of request");
        NimBLEUUID uuid(data + DIRCON_MESSAGE_HEADER_LENGTH, 16, true);
        this->UUID = uuid;
        parsedBytes += 16;
        if (this->Length == 16) {
          // log_d("DirCon request only has one UUID, just checking if isRequest() of sequence %d", sequenceNumber);
          this->Request = this->isRequest(sequenceNumber);
        } else {
          // log_d("Parsing additional data of request");
          this->AdditionalData.clear();
          for (size_t dataIndex = 0; dataIndex < (this->Length - 16); dataIndex++) {
            this->AdditionalData.push_back((uint8_t)data[DIRCON_MESSAGE_HEADER_LENGTH + dataIndex + 16]);
            parsedBytes += 1;
          }
        }
      } else {
        log_e("Error parsing DirCon message: Length %d < 16", this->Length);
        this->Identifier = DIRCON_MSGID_ERROR;
        return 0;
      }
      break;

    case DIRCON_MSGID_WRITE_CHARACTERISTIC:
      // log_d("Parsing DirCon request type DIRCON_MSGID_WRITE_CHARACTERISTIC");
      if (this->Length > 16) {
        // log_d("Parsing UUID and additional data of request and checking if isRequest() of sequence %d", sequenceNumber);
        NimBLEUUID uuid(data + DIRCON_MESSAGE_HEADER_LENGTH, 16, true);
        this->UUID = uuid;
        parsedBytes += 16;
        this->Request = this->isRequest(sequenceNumber);
        this->AdditionalData.clear();
        for (size_t dataIndex = 0; dataIndex < (this->Length - 16); dataIndex++) {
          this->AdditionalData.push_back((uint8_t)data[DIRCON_MESSAGE_HEADER_LENGTH + dataIndex + 16]);
          parsedBytes += 1;
        }
      } else {
        log_e("Error parsing DirCon message: Length %d < 16", this->Length);
        this->Identifier = DIRCON_MSGID_ERROR;
        return 0;
      }
      break;

    case DIRCON_MSGID_ENABLE_CHARACTERISTIC_NOTIFICATIONS:
      // log_d("Parsing DirCon request type DIRCON_MSGID_ENABLE_CHARACTERISTIC_NOTIFICATIONS");
      if ((this->Length == 16) || (this->Length == 17)) {
        // log_d("Parsing UUID of request");
        NimBLEUUID uuid(data + DIRCON_MESSAGE_HEADER_LENGTH, 16, true);
        this->UUID = uuid;
        parsedBytes += 16;
        if (this->Length == 17) {
          // log_d("Parsing additional data of request, setting Request to true");
          this->Request = true;
          this->AdditionalData.clear();
          this->AdditionalData.push_back((uint8_t)data[DIRCON_MESSAGE_HEADER_LENGTH + 16]);
          parsedBytes += 1;
        }
      } else {
        log_e("Error parsing DirCon message: Length %d isn't 16 or 17", this->Length);
        this->Identifier = DIRCON_MSGID_ERROR;
        return 0;
      }
      break;

    case DIRCON_MSGID_UNSOLICITED_CHARACTERISTIC_NOTIFICATION:
      // log_d("Parsing DirCon request type DIRCON_MSGID_UNSOLICITED_CHARACTERISTIC_NOTIFICATION");
      if (this->Length > 16) {
        // log_d("Parsing UUID and additional data of request and checking if isRequest() of sequence %d", sequenceNumber);
        NimBLEUUID uuid(data + DIRCON_MESSAGE_HEADER_LENGTH, 16, true);
        this->UUID = uuid;
        parsedBytes += 16;
        this->AdditionalData.clear();
        for (size_t dataIndex = 0; dataIndex < (this->Length - 16); dataIndex++) {
          this->AdditionalData.push_back((uint8_t)data[DIRCON_MESSAGE_HEADER_LENGTH + dataIndex + 16]);
          parsedBytes += 1;
        }
      } else {
        log_e("Error parsing DirCon message: Length %d < 16", this->Length);
        this->Identifier = DIRCON_MSGID_ERROR;
        return 0;
      }
      break;

    default:
      log_e("Error parsing DirCon message: Unknown identifier %d", this->Identifier);
      this->Identifier = DIRCON_MSGID_ERROR;
      return 0;
      break;
  }

  return parsedBytes;
}

bool DirConMessage::isRequest(int sequenceNumber) {
  return this->ResponseCode == DIRCON_RESPCODE_SUCCESS_REQUEST && (sequenceNumber <= 0 || sequenceNumber != this->SequenceNumber);
}
