enum PatternCommands {DRUM_KICK, DRUM_SNARE, SILENCE, EMPTY, END};

typedef struct {
    uint8_t* dmc_data;
    uint8_t* noise_data;
    uint8_t current;
} DrumPattern;

DrumPattern dp;

void drum_pattern_update(DrumPattern* pattern);

