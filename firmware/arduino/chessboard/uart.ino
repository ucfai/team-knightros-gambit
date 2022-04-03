char incomingByte;
volatile unsigned long previousISRTime = 0;

int8_t byteNum = -1; // -1 indicates that the start code hasn't been received

enum PiMsgIndices
{
  OPCODE_IDX = 0,
  ITYPE_IDX = 1,
  EXTRA_IDX = 2,
  Y0_IDX = 1,
  X0_IDX = 2,
  Y1_IDX = 3,
  X1_IDX = 4,
  MSG_COUNT_IDX = 5
};

// Send message to Pi when the chess timer is pressed
void chessTimerISR()
{
  unsigned long currentISRTime = millis();
  
  // Check if the difference between button presses is longer than the debounce time
  if ((currentISRTime - previousISRTime > DEBOUNCE_TIME)  ||  
      (currentISRTime < previousISRTime  &&  previousISRTime - currentISRTime > DEBOUNCE_TIME))
  {
    previousISRTime = currentISRTime;
    buttonFlag = true;
  }
}

// Wait for input
void serialEvent2()
{
  // Loop through all available bytes
  while (Serial2.available())
  {
    // Get byte
    incomingByte = (char) Serial2.read();

    // Reset buffer position
    // '~' is the delimiter for our messages
    if (incomingByte == '~')
    {
      // Send message to Pi if the previous instruction was incomplete
      if (byteNum != -1)
      {
        currentState = ERROR;
        extraByte = INCOMPLETE_INSTRUCTION;
        uartMessageIncompleteFlag = true;
      }

      byteNum = 0;
    }
    // Add byte to buffer
    else if (byteNum != -1)
    {
      rxBufferPtr[byteNum++] = incomingByte;
    }

    // Check if the buffer is full, process input
    if (byteNum == INCOMING_MESSAGE_LENGTH)
    {
      // Reset buffer position
      byteNum = -1;

      // Swap rxBufferPtr and receivedMessagePtr pointers
      tempCharPtr = rxBufferPtr;
      rxBufferPtr = receivedMessagePtr;
      receivedMessagePtr = tempCharPtr;

      // Tell game loop to process input
      receivedMessageValidFlag = true;
    }
  }
}

// Check that the instruction is valid
bool validateMessageFromPi(volatile char * message)
{
  uint8_t i;

  // Print the most recent message received
  if (DEBUG)
  {
    Serial.println("Incoming message:  ");
    for (i = 0; i < INCOMING_MESSAGE_LENGTH; i++)
      Serial.print(receivedMessagePtr[i]);
    Serial.println("\n");
  }

  // If no error occurs to change the extraByte, it should store the opcode
  extraByte = message[OPCODE_IDX];

  if (message[OPCODE_IDX] == DIRECT  ||  message[OPCODE_IDX] == EDGES)
  {
    if (isInvalidCoord(message[X0_IDX])  ||  isInvalidCoord(message[Y0_IDX]) ||
        isInvalidCoord(message[X1_IDX])  ||  isInvalidCoord(message[Y1_IDX]))
    {
      extraByte = INVALID_LOCATION;
      currentState = ERROR;
      return false;
    }
  }
  else if (message[OPCODE_IDX] == ALIGN)
  {
    if (isInvalidCoord(message[X0_IDX])  ||  isInvalidCoord(message[Y0_IDX]))
    {
      extraByte = INVALID_LOCATION;
      currentState = ERROR;
      return false;
    }
  }
  else if (message[OPCODE_IDX] == INSTRUCTION)
  {
    // Check if message[1] holds an invalid instruction type or if message[2] is an invalid code
    if ((message[ITYPE_IDX] != ALIGN_AXIS         ||  message[EXTRA_IDX] < '0'  ||  message[EXTRA_IDX] > '3')  &&  
        (message[ITYPE_IDX] != SET_ELECTROMAGNET  ||  message[EXTRA_IDX] < '0'  ||  message[EXTRA_IDX] > '1')  &&
         message[ITYPE_IDX] != RETRANSMIT)
    {
      extraByte = INVALID_LOCATION;
      currentState = ERROR;
      return false;
    }
  }
  else
  {
    // Invalid opcode
    extraByte = INVALID_OP;
    currentState = ERROR;
    return false;
  }

  // Update last valid move count
  moveCount = message[MSG_COUNT_IDX];

  return true;
}

bool makeMove(volatile char * message)
{
  // If no error occurs to change the extraByte, it should store the opcode
  extraByte = message[OPCODE_IDX];

  // Move type 0
  if (message[OPCODE_IDX] == DIRECT)
  {
    // Since we're moving a piece, we want the magnet on, so pass in true.
    if (!moveDirect(message[X0_IDX] - 'A', message[Y0_IDX] - 'A', message[X1_IDX] - 'A', message[Y1_IDX] - 'A', true))
    {
      currentState = ERROR;
      extraByte = MOVEMENT_ERROR;
      return false;
    }
  }
  // Move type 1
  else if (message[OPCODE_IDX] == EDGES)
  {
    if(!moveAlongEdges(message[X0_IDX] - 'A', message[Y0_IDX] - 'A', message[X1_IDX] - 'A', message[Y1_IDX] - 'A'))
    {
      currentState = ERROR;
      extraByte = MOVEMENT_ERROR;
      return false;
    }
  }
  // Move type 2
  else if (message[OPCODE_IDX] == ALIGN)
  {
    if(!alignPiece(message[X0_IDX] - 'A', message[Y0_IDX] - 'A'))
    {
      currentState = ERROR;
      extraByte = MOVEMENT_ERROR;
      return false;
    }
  }
  // Move type 3 - special instructions
  else if (message[OPCODE_IDX] == INSTRUCTION)
  {
    // Align Axis
    if (message[ITYPE_IDX] == ALIGN)
    {
      if (message[EXTRA_IDX] == '0')
        alignAxis(xMotor, ZERO_POSITION);
      else if (message[EXTRA_IDX] == '1')
        alignAxis(yMotor, ZERO_POSITION);
      else if (message[EXTRA_IDX] == '2')
        alignAxis(xMotor, MAX_POSITION);
      else if (message[EXTRA_IDX] == '3')
        alignAxis(yMotor, MAX_POSITION);
    }
    // Enable/Disable Electromagnet
    else if (message[ITYPE_IDX] == SET_ELECTROMAGNET)
    {
      if (message[EXTRA_IDX] == '0')
        digitalWrite(ELECTROMAGNET, LOW);
      else if (message[EXTRA_IDX] == '1')
        ledcWrite(EM_PWM_CHANNEL, PWM_HALF);
    }
    // Retransmit last message
    else if (message[ITYPE_IDX] == RETRANSMIT)
    {
      sendMessageToPi(sentMessage);
    }
  }
  else
  {
    // Invalid opcode
    extraByte = INVALID_OP;
    currentState = ERROR;
    return false;
  }

  // Move was made
  currentState = IDLE;
  return true;
}

void sendParamsToPi(volatile char currentState, volatile char extraByte, volatile char moveCount)
{
  sentMessage[0] = currentState;
  sentMessage[1] = extraByte;
  sentMessage[2] = moveCount;
  sendMessageToPi(sentMessage);
}

void sendMessageToPi(volatile char * message)
{
  // Print outgoing message
  if (DEBUG)
  {
    Serial.println("Outgoing Message:  ");
    Serial.print(message[0]);
    Serial.print(" ");
    Serial.print(message[1]);
    Serial.print(" ");
    Serial.print(message[2]);
    Serial.println("\n");

    // If we have an error, print the error code
    if (message[0] == ERROR)
    {
      Serial.print("Encountered an error: ");
      Serial.print(message[1]);
      Serial.println("\n");
    }
  }

  // '~' is the delimiter for our messages
  Serial2.write('~');
  Serial2.write(message[0]);
  Serial2.write(message[1]);
  Serial2.write(message[2]);
}

bool isInvalidCoord (volatile char c)
{
  return (c < 'A'  ||  c > 'W');
}
