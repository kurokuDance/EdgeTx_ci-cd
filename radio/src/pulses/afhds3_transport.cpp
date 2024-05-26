/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "afhds3_transport.h"
#include "opentx_helpers.h"
#include "debug.h"

#include "board.h"
#include "dataconstants.h"
#include "mixer_scheduler.h"

// timer is 2 MHz
#if defined(AFHDS3_SLOW)
  // 1000000/57600 = 17,36 us
  #define BITLEN_AFHDS (35)
#else
  // 1000000/115200 = 8,68 us
  #define BITLEN_AFHDS (17)
#endif

#define MAX_RETRIES_AFHDS3 5

namespace afhds3
{

enum AfhdsSpecialChars {
  END = 0xC0,  // Frame end
  START = END,
  ESC_END = 0xDC,  // Escaped frame end - in case END occurs in fame then ESC
                   // ESC_END must be used
  ESC = 0xDB,      // Escaping character
  ESC_ESC = 0xDD,  // Escaping character in case ESC occurs in fame then ESC
                   // ESC_ESC  must be used
};

void FrameTransport::init(void* buffer, uint8_t fAddr)
{
  frameAddress = fAddr;
  trsp_buffer = (uint8_t*)buffer;
  clear();
}

void FrameTransport::clear()
{
  // reset send buffer
  data_ptr = trsp_buffer;

  // reset parser
  esc_state = 0;
}

void FrameTransport::putByte(uint8_t b)
{
  *(data_ptr++) = b;
}

void FrameTransport::putBytes(uint8_t* data, int length)
{
  for (int i = 0; i < length; i++) {
    uint8_t byte = data[i];
    crc += byte;
    if (END == byte) {
      putByte(ESC);
      putByte(ESC_END);
    }
    else if (ESC == byte) {
      putByte(ESC);
      putByte(ESC_ESC);
    }
    else {
      putByte(byte);
    }
  }  
}

void FrameTransport::putFrame(COMMAND command, FRAME_TYPE frameType,
                              uint8_t* data, uint8_t dataLength,
                              uint8_t frameIndex)
{
  // header
  data_ptr = trsp_buffer;

  crc = 0;
  putByte(START);

  uint8_t buffer[] = {frameAddress, frameIndex, frameType, command};
  putBytes(buffer, 4);

  // payload
  if (dataLength > 0) {
    putBytes(data, dataLength);
  }

  // footer
  uint8_t crcValue = crc ^ 0xff;
  putBytes(&crcValue, 1);
  putByte(END);
}

uint32_t FrameTransport::getFrameSize()
{
  return data_ptr - trsp_buffer;
}

static bool _checkCRC(const uint8_t* data, uint8_t size)
{
  uint8_t crc = 0;
  //skip start byte
  for (uint8_t i = 1; i < size; i++) {
    crc += data[i];
  }
  return (crc ^ 0xff) == data[size];
}

bool FrameTransport::processTelemetryData(uint8_t byte, uint8_t* rxBuffer,
                                          uint8_t& rxBufferCount,
                                          uint8_t maxSize)
{
  if (rxBufferCount == 0 && byte != START) {
//     TRACE("AFHDS3 [SKIP] %02X", byte);
    this->esc_state = 0;
    return false;
  }

  if (byte == ESC) {
    this->esc_state = rxBufferCount;
    return false;
  }

  if (rxBufferCount > 1 && byte == END) {
    rxBuffer[rxBufferCount++] = byte;

    if (!_checkCRC(rxBuffer, rxBufferCount - 2)) {
      TRACE("AFHDS3 [INVALID CRC]");
      rxBufferCount = 0;
      return false;
    }

    return true;
  }

  if (this->esc_state && byte == ESC_END) {
    byte = END;
  }
  else if (esc_state && byte == ESC_ESC) {
    byte = ESC;
  }
  //reset esc index
  this->esc_state = 0;

  if (rxBufferCount >= maxSize) {
    TRACE("AFHDS3 [BUFFER OVERFLOW]");
    rxBufferCount = 0;
  }
  rxBuffer[rxBufferCount++] = byte;
  return false;
}

void CommandFifo::clearCommandFifo()
{
  memclear(commandFifo, sizeof(commandFifo));
  setIndex = getIndex = 0;
}

Frame* CommandFifo::getCommand()
{
  if (isEmpty()) return nullptr;
  return &commandFifo[getIndex];
}

void CommandFifo::enqueueACK(COMMAND command, uint8_t frameNumber)
{
  uint32_t next = nextIndex(setIndex);
  if (next != getIndex) {
    commandFifo[setIndex].command = command;
    commandFifo[setIndex].frameType = FRAME_TYPE::RESPONSE_ACK;
    commandFifo[setIndex].payload = 0;
    commandFifo[setIndex].payloadSize = 0;
    commandFifo[setIndex].frameNumber = frameNumber;
    commandFifo[setIndex].useFrameNumber = true;
    setIndex = next;
  }
}

void CommandFifo::enqueue(COMMAND command, FRAME_TYPE frameType, bool useData,
                          uint8_t byteContent)
{
  uint32_t next = nextIndex(setIndex);
  if (next != getIndex) {
    commandFifo[setIndex].command = command;
    commandFifo[setIndex].frameType = frameType;
    commandFifo[setIndex].payload = byteContent;
    commandFifo[setIndex].payloadSize = useData ? 1 : 0;
    commandFifo[setIndex].frameNumber = 0;
    commandFifo[setIndex].useFrameNumber = false;
    setIndex = next;
  }
}

void Transport::init(void* buffer, etx_module_state_t* mod_st, uint8_t fAddr)
{
  trsp.init(buffer, fAddr);
  this->mod_st = mod_st;
}

void Transport::clear()
{
  // reset frame
  trsp.clear();
  
  // reset command layer
  fifo.clearCommandFifo();

  frameIndex = 1;
  repeatCount = 0;

  // reset internal state
  operationState = State::UNKNOWN;
}

void Transport::putFrame(COMMAND command, FRAME_TYPE frameType, uint8_t* data,
                          uint8_t dataLength)
{
  operationState = State::SENDING_COMMAND;
  repeatCount = 0;

  trsp.putFrame(command, frameType, data, dataLength, frameIndex);
  frameIndex++;

  switch (frameType) {
    case FRAME_TYPE::REQUEST_GET_DATA:
    case FRAME_TYPE::REQUEST_SET_EXPECT_ACK:
    case FRAME_TYPE::REQUEST_SET_EXPECT_DATA:
      operationState = State::AWAITING_RESPONSE;
      break;
    default:
      operationState = State::IDLE;
  }
}

void Transport::enqueue(COMMAND command, FRAME_TYPE frameType, bool useData,
                        uint8_t byteContent)
{
  fifo.enqueue(command, frameType, useData, byteContent);
}

void Transport::sendBuffer()
{
#if !defined(SIMU)
  auto drv = modulePortGetSerialDrv(mod_st->tx);
  auto ctx = modulePortGetCtx(mod_st->tx);
  drv->sendBuffer(ctx, (uint8_t*)trsp.trsp_buffer, trsp.getFrameSize());
#endif
}

bool Transport::processQueue()
{
  // check waiting commands
  auto f = fifo.getCommand();
  if (!f) return false;

  trsp.putFrame(f->command, f->frameType, &f->payload, f->payloadSize,
                f->useFrameNumber ? f->frameNumber : frameIndex);

//   TRACE(
//       "AFHDS3 [CMD QUEUE] cmd: 0x%02x frameType 0x%02x, useFrameNumber %d "
//       "frame Number %d size %d",
//       f->command, f->frameType, f->useFrameNumber, f->frameNumber,
//       f->payloadSize);

  if (!f->useFrameNumber) frameIndex++;
  fifo.skip();

  return true;
}

bool Transport::handleRetransmissions(bool& error)
{
  if (operationState == State::AWAITING_RESPONSE) {
    if (repeatCount++ < MAX_RETRIES_AFHDS3) {
      error = false;
      return true; // re-send
    }

//     TRACE("AFHDS3 [NO RESP]");
    error = true;
    return false;
  }

  if (operationState == State::UNKNOWN) {
    error = true;
    return false;
  }

  error = false;
  repeatCount = 0;

  return false;
}

bool Transport::handleReply(uint8_t* buffer, uint8_t len)
{
  // TODO: check len...

  AfhdsFrame* responseFrame = reinterpret_cast<AfhdsFrame*>(buffer);
  if (responseFrame->frameType == FRAME_TYPE::REQUEST_SET_EXPECT_ACK) {

    // check if such request is not queued
    auto f = fifo.getCommand();
    if (f && f->frameType == FRAME_TYPE::RESPONSE_ACK &&
        f->frameNumber == responseFrame->frameNumber) {

      // absorb retransmission
      TRACE("ACK for frame %02X already queued", responseFrame->frameNumber);
      return true;
    }

//     TRACE("AFHDS3 [SEND ACK] cmd %02X type %02X number %02X",
//           responseFrame->command, responseFrame->frameType,
//           responseFrame->frameNumber);

    auto command = (enum COMMAND)responseFrame->command;
    trsp.putFrame(command, FRAME_TYPE::RESPONSE_ACK, nullptr, 0,
                  responseFrame->frameNumber);
    sendBuffer();

  } else if (responseFrame->frameType == FRAME_TYPE::RESPONSE_DATA ||
             responseFrame->frameType == FRAME_TYPE::RESPONSE_ACK) {

    if (operationState == State::AWAITING_RESPONSE) {
      operationState = State::IDLE;
    }
  }

  return false;
}

bool Transport::processTelemetryData(uint8_t byte, uint8_t* rxBuffer,
                                     uint8_t& rxBufferCount, uint8_t maxSize)
{
  bool has_frame =
      trsp.processTelemetryData(byte, rxBuffer, rxBufferCount, maxSize);
  if (has_frame && handleReply(rxBuffer, rxBufferCount)) {
    rxBufferCount = 0;
    return false;
  }

  return has_frame;
}
};  // namespace afhds3
