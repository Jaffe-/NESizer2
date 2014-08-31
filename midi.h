#pragma once

#define MIDI_CMD_NOTE_OFF 0
#define MIDI_CMD_NOTE_ON 1
#define MIDI_CMD_AFTERTOUCH 2
#define MIDI_CMD_CONTROL_CHANGE 3
#define MIDI_CMD_PATCH_CHANGE 4
#define MIDI_CMD_CHANNEL_PRESSURE 5
#define MIDI_CMD_PITCH_BEND 6
#define MIDI_CMD_SYSEX 7

typedef struct {
    uint8_t command : 4;
    uint8_t channel : 4;
    uint8_t data1;
    uint8_t data2;
} MIDIMessage;

void midi_setup();
void midi_handler();
uint8_t midi_buffer_state();
MIDIMessage midi_buffer_read();
