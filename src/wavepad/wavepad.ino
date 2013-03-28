
#include <SPI.h>

#define NUM_ROWS 8
#define NUM_COLS 8

// Digital pins for selecting the MCP3008 ADC used
// in each row. Note that the CS (chip-select) line
// is inverted (active LOW).
// Each CS_PINS entry is the pin number of the
// CS (chip-select) line of the corresponding sensor row.
// For example, CS_PINS[0] holds the pin number of the
// CS line for row 0.

const int CS_PINS[] = { 2, 3, 4, 5, 6, 7, 8, 9 };

// Uncomment to see Arduino debugging information in the USB serial console:
// #define DEBUG 

uint16_t SENSOR_DATA[NUM_ROWS][NUM_COLS];

// MISO, MOSI, and SCK are defined by Arduino for this board.
// For ATmega328, these are: MISO 12, MOSI 11, and SCK 13.
// In addition, pins 0 and 1 are used for USB serial (USART).

void setup()
{
	// Setup chip-select pins
	for (int row = 0; row < NUM_ROWS; row++)
		pinMode(CS_PINS[row], OUTPUT);

	// Initialize sensor data matrix to 0
	for (int row = 0; row < NUM_ROWS; row++)
		for (int col = 0; col < NUM_COLS; col++)
			SENSOR_DATA[row][col] = 0;

	// Configure SPI for MCP3008s
    SPI.begin();
	SPI.setBitOrder(MSBFIRST); // MCP3008 sends MSB first.
	SPI.setClockDivider(SPI_CLOCK_DIV16); // 16MHz / 16 = 1MHz
    SPI.setDataMode(SPI_MODE0); // CLock idle LOW, falling edge transfer

	// Open serial port to USB
	Serial.begin(115200);
}

void loop()
{
	for (int row = 0; row < NUM_ROWS; row++)
		for (int col = 0; col < NUM_COLS; col ++)
			read_row(row, col);

	report_values();

    //Serial.println(SENSOR_DATA[0][0]);
    delay(10000);
}

int read_sensor(uint8_t row, uint8_t col)
{
	if (row > NUM_ROWS - 1) return -1;
	if (col > NUM_COLS - 1) return -1;

	select_row(row);
	
	uint8_t initial = SPI.transfer(0b00000001); // zero-pad then START; receive {8x hi-z}

	#ifdef DEBUG
		Serial.println("Sending START: 0x01");
		Serial.print("Received ");
		Serial.println(initial, BIN);
		delay(10);
	#endif
	
	uint8_t col_sel = (col & 0x7) << 4; // 3 LSBs left-shifted to bits 6:4

	#ifdef DEBUG
		Serial.print("First sent byte is ");
		Serial.print(0b10000000 | col_sel, BIN);
		Serial.print(" corresponding to col ");
		Serial.println(col);
		delay(10);
	#endif

	uint8_t top_byte = SPI.transfer(0b10000000 | col_sel); // {single-ended input, d2, d1, d0, 4x don't care}; receive {5x hi-z, null, B9, B8}

	#ifdef DEBUG
		Serial.print("received top_byte = ");
		Serial.println(top_byte, BIN);
		delay(10);
	#endif

	uint8_t bottom_byte = SPI.transfer(0x00); // 8x don't care; receive B7:B0

	#ifdef DEBUG
		Serial.print("received bottom_byte = ");
		Serial.println(bottom_byte, BIN);
		delay(10);
	#endif

	uint16_t assembled_value = (((uint16_t)top_byte & 0x3) << 8) | (bottom_byte);

	#ifdef DEBUG
		Serial.print("assembled_value = ");
		Serial.println(assembled_value, DEC);
		delay(10);
	#endif

	SENSOR_DATA[row][col] = assembled_value;

	deselect_row(row);

	return 0;
}

/* Function: deselect_row
 * ----------------------
 * Set the corresponding row's CS pin HIGH,
 * disabling its SPI interface.
 */
void deselect_row(uint8_t row)
{
	if (row > NUM_ROWS - 1) return;
	digitalWrite(CS_PINS[row], HIGH);
}

/* Function: select_row
 * --------------------
 * Set the corresponding row's CS pin LOW,
 * enabling its SPI interface. Note that
 * each call to select_row should be followed
 * by a corresponding call to deselect_row,
 * or multiple rows will be selected! This will
 * cause SPI problems.
 */
void select_row(uint8_t row)
{
	if (row > NUM_ROWS - 1) return;
	digitalWrite(CS_PINS[row], LOW); 
}

void report_values()
{
	Serial.println(); // Blank line signals data start
	for (int row = 0; row < NUM_ROWS; row++){
		for (int col = 0; col < NUM_COLS; col++){
            // TODO: eventually, use Serial.write to make transfer far more compact
			Serial.print(SENSOR_DATA[row][col]);
			Serial.print(" ");
		}
		Serial.println();
	}
}
