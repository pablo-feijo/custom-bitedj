// Test-only MIDI injection into an explicitly selected application ALSA port.
#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char** argv) {
    if (argc != 6) { fprintf(stderr, "usage: %s client port status note value\n", argv[0]); return 2; }
    snd_seq_t* seq = NULL;
    snd_midi_event_t* parser = NULL;
    if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_OUTPUT, 0) < 0) return 1;
    snd_seq_set_client_name(seq, "BiteDJ isolated regression");
    int port = snd_seq_create_simple_port(seq, "test", SND_SEQ_PORT_CAP_READ, SND_SEQ_PORT_TYPE_APPLICATION);
    if (port < 0 || snd_midi_event_new(16, &parser) < 0) return 1;
    unsigned char bytes[3];
    for (int i = 0; i < 3; ++i) bytes[i] = strtoul(argv[i + 3], NULL, 0);
    snd_seq_event_t event;
    snd_seq_ev_clear(&event);
    int result = snd_midi_event_encode(parser, bytes, 3, &event) == 3 ? 0 : 1;
    snd_seq_ev_set_source(&event, port);
    snd_seq_ev_set_dest(&event, atoi(argv[1]), atoi(argv[2]));
    snd_seq_ev_set_direct(&event);
    if (!result && snd_seq_event_output_direct(seq, &event) < 0) result = 1;
    snd_midi_event_free(parser);
    snd_seq_close(seq);
    return result;
}
