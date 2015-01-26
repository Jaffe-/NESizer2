#pragma once

#define MIDI_CMD_NOTE_OFF 0
#define MIDI_CMD_NOTE_ON 1
#define MIDI_CMD_AFTERTOUCH 2
#define MIDI_CMD_CONTROL_CHANGE 3
#define MIDI_CMD_PATCH_CHANGE 4
#define MIDI_CMD_CHANNEL_PRESSURE 5
#define MIDI_CMD_PITCH_BEND 6

#define MIDI_CMD_SYSEX 0x08
#define MIDI_CMD_TIMECODE 0x09
#define MIDI_CMD_SONGPOS 0x0A
#define MIDI_CMD_SONGSEL 0x0B
#define MIDI_CMD_TUNEREQUEST 0x0E
#define MIDI_CMD_SYSEX_END 0x0F

#define MIDI_CMD_CLOCK 0x10
#define MIDI_CMD_START 0x12
#define MIDI_CMD_CONTINUE 0x13
#define MIDI_CMD_STOP 0x14
#define MIDI_CMD_ACTIVESENSE 0x16
#define MIDI_CMD_RESET 0x17

typedef struct {
    uint8_t command;
    uint8_t channel;
    uint8_t data1;
    uint8_t data2;
} MIDIMessage;

void midi_io_setup();
void midi_io_handler();
uint8_t midi_io_buffer_nonempty();
MIDIMessage midi_io_read();
uint8_t midi_io_buffer_read();
uint8_t midi_io_buffer_bytes_remaining();
uint8_t midi_is_channel_message(uint8_t command);
