/* ===============================================================
   Titulo:
   Juego:     Plantilla

   Cliente:

   Fecha:
   Autor:

   Placa:

   Descripcion:

   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
   Propiedad de @logicprops
   ===============================================================
*/

#define DEBUG     1
#if DEBUG == 1
#define LOGN(x) Serial.println(x)
#define LOG(x)  Serial.print(x)
#else
#define LOG(x)
#define LOGN(x)
#endif

// ==== LIBRERIAS EXTERNAS =======================================
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>

// ==== RFID =====================================================
#define RST_PIN         9          // Configurable, see typical pin layout above
#define SS_PIN          10         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

// ===============================================================
//            SETUP
// ===============================================================
void setup()
{
  // -- Serial --
#if DEBUG == 1
  Serial.begin(115200);
  delay(50);
  LOGN("\n\n----------------------------\n\tEMPEZAMOS\n");
#endif

  // -- RFID --
  SPI.begin();                  // Init SPI bus
  mfrc522.PCD_Init();           // Init MFRC522 card
  delay(4);
  mfrc522.PCD_DumpVersionToSerial();

}


void loop ()
{

  WriteRFID(4, 0, 1);
}

// ==== GET RDIF ID ==============================================
bool GetUid(MFRC522::Uid & foundUid)
{
  //    typedef struct {
  //    byte    size;     // Number of bytes in the UID. 4, 7 or 10.
  //    byte    uidByte[10];
  //    byte    sak;      // The SAK (Select acknowledge) byte returned from the PICC after successful selection.
  //  } Uid;

  // Check if RFID present
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return false;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return false;
  }

  foundUid = mfrc522.uid;

  LOG("ID FOUND: ");
  printID(foundUid.uidByte, foundUid.size);

  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  return true;

}

void printID(byte *buffer, byte bufferSize)
{
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
  LOGN("");
}

// ==== READ RFID ================================================
bool ReadData(byte blockAddr, byte column, byte & result)
{
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //some variables we need
  byte len = 18;
  byte buffer1[18];
  MFRC522::StatusCode status;

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
  {

    // 1. AUTHENTICATE
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
      LOG(F("Authentication failed: "));
      LOGN(mfrc522.GetStatusCodeName(status));
      ResetRFID();
      return false;
    }

    // 2. READ DATA
    status = mfrc522.MIFARE_Read(blockAddr, buffer1, &len);
    if (status != MFRC522::STATUS_OK) {
      LOG(F("Reading failed: "));
      LOGN(mfrc522.GetStatusCodeName(status));
      ResetRFID();
      return false;
    }

    // 3. STORE DATA
    result = buffer1[column];

    // mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return true;
  }

  return false;

}

void ResetRFID()
{
  //mfrc522.PCD_Reset();
  LOG("Reset done de RFID");

  LOG("  Estado RFID?: ");
  LOGN(mfrc522.PCD_PerformSelfTest());

  mfrc522.PCD_Init(); // Iniciamos otra vez
}

// ==== WRITE RFID ===============================================
bool WriteRFID(byte blockAddr, byte column, byte val)
{
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return false;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return false;
  }
  byte dataBlock[]    = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
  };
  dataBlock[column] = val;

  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  MFRC522::StatusCode status;

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return false;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return false;
  }

  // 1. AUTHENTICATE USING KEY A
  Serial.println(F("Authenticating using key A..."));
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  else Serial.println(F("PCD_Authenticate() success: "));

  // 2. WRITE BLOCK
  status = mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  else Serial.println(F("MIFARE_Write() success: "));

  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();

  return true;
}
