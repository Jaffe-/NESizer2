#include <Arduino.h>



void handle_data(byte data);
void init_buffer(byte size);
void load_buffer(byte data);
void print_buffer();
void del_buffer();
void unexpected_byte(byte data);



enum MESSAGE_HEADER { DBG_STOP = 0b10100000, DBG_TEXT, DBG_MIDI_NOTE, DBG_MIDI_CC, DBG_PARAM_EDIT };  // more debug messages types coming soon

byte message_type = DBG_STOP;

byte *buffer;
bool buffer_init = false;
int buffer_size = 0;
int buffer_index = 0;



void handle_data(byte data)
{
    if (message_type == DBG_STOP) {
        switch (data) {
            case DBG_TEXT:
            case DBG_MIDI_NOTE:
            case DBG_MIDI_CC:
            case DBG_PARAM_EDIT:
            case DBG_STOP:
                message_type = data;  // start new message by updating message_type
                break;
            default:  // just mirror any random bytes that don't belong to a message
                unexpected_byte(data);
                break;
        }
    } else {
        if (data == DBG_STOP) {  // print buffer and delete when message stop is received
            print_buffer();  // formatting happens in here
            message_type = data;

        } else if (message_type == DBG_TEXT) {
            Serial.write(data);  // incoming message should just be a text string, print it

        } else {
            if (!buffer_init) {
                init_buffer(data);
            } else {
                load_buffer(data);
            }
        }
    }
}


void init_buffer(byte size)
{
    buffer_size = size;
    buffer = new byte[buffer_size];
    for (byte i = 0; i < buffer_size; i++) { buffer[i] = 0; }
    buffer_index = 0;
    buffer_init = true;
}


void load_buffer(byte data)
{
    if (buffer_index < buffer_size) {
        buffer[buffer_index] = data;
        buffer_index++;
    } else {
        Serial.print("Uh oh, buffer overload: "); Serial.println(data);
    }
}


void print_buffer()
{
    switch (message_type) {
        case DBG_TEXT:
            Serial.println();
            break;

        case DBG_MIDI_NOTE:
            Serial.println("Midi Note On Message:");
            Serial.print("Channel: "); Serial.println(buffer[0]);
            Serial.print("Note: "); Serial.println(buffer[1]);
            Serial.print("Velocity: "); Serial.println(buffer[2]);
            Serial.println();
            del_buffer();
            break;

        case DBG_MIDI_CC:
            Serial.println("Midi CC Message:");
            Serial.print("Channel: "); Serial.println(buffer[0]);
            Serial.print("CC #: "); Serial.println(buffer[1]);
            Serial.print("Value: "); Serial.println(buffer[2]);
            Serial.println();
            del_buffer();
            break;

        case DBG_PARAM_EDIT:
            del_buffer();
            break;

        default:
            Serial.println("\nSomething bad happened...\n");
            break;
    }
}


void del_buffer()
{
    if (buffer_init) delete [] buffer;
    buffer_size = 0;
    buffer_init = false;
}


void unexpected_byte(byte data)
{
    Serial.print("Unexpected byte: "); Serial.println(data);
}



/* mcu functions */

void setup()
{
    Serial.begin(115200);
    Serial2.begin(115200);  // ESP32 UART2 RX: GPIO = 16, (breadboard row = 8)
}


void loop()
{
    if (Serial2.available()) {
        byte data = Serial2.read();
        handle_data(data);
    }
}
