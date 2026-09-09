// Test-only transport for an isolated GUI container without /dev/snd/seq.
// Preload explicitly for the test process. The app's MIDI mapping and DSP are real.
#include <portmidi.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

static PmDeviceInfo devices[] = {
    {1, "BiteDJ isolated test", "DDJ-400", 1, 0, 0},
    {1, "BiteDJ isolated test", "DDJ-400", 0, 1, 0}
};
typedef struct { int fd; int device; } Stream;
PmError Pm_Initialize(void) { return pmNoError; }
PmError Pm_Terminate(void) { return pmNoError; }
int Pm_CountDevices(void) { return 2; }
const PmDeviceInfo* Pm_GetDeviceInfo(PmDeviceID id) { return id >= 0 && id < 2 ? &devices[id] : NULL; }
PmDeviceID Pm_GetDefaultInputDeviceID(void) { return 0; }
PmDeviceID Pm_GetDefaultOutputDeviceID(void) { return 1; }
PmError Pm_OpenInput(PortMidiStream** out, PmDeviceID id, void* info,
        int32_t size, PmTimeProcPtr timer, void* timer_info) {
    (void)info; (void)size; (void)timer; (void)timer_info;
    if (id != 0) return pmInvalidDeviceId;
    Stream* s = calloc(1, sizeof(*s));
    if (!s) return pmInsufficientMemory;
    s->fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0); s->device = 0;
    struct sockaddr_in address = {.sin_family=AF_INET, .sin_port=htons(21940),
        .sin_addr.s_addr=htonl(INADDR_LOOPBACK)};
    if (s->fd < 0 || bind(s->fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        if (s->fd >= 0) close(s->fd); free(s); return pmHostError;
    }
    devices[0].opened = 1; *out = s; return pmNoError;
}
PmError Pm_OpenOutput(PortMidiStream** out, PmDeviceID id, void* info,
        int32_t size, PmTimeProcPtr timer, void* timer_info, int32_t latency) {
    (void)info; (void)size; (void)timer; (void)timer_info; (void)latency;
    if (id != 1) return pmInvalidDeviceId;
    Stream* s = calloc(1, sizeof(*s));
    if (!s) return pmInsufficientMemory;
    s->fd = -1; s->device = 1; devices[1].opened = 1; *out = s; return pmNoError;
}
PmError Pm_Close(PortMidiStream* stream) {
    Stream* s = stream; if (!s) return pmBadPtr;
    if (s->fd >= 0) close(s->fd); devices[s->device].opened=0; free(s); return pmNoError;
}
PmError Pm_Poll(PortMidiStream* stream) {
    Stream* s = stream; if (!s || s->fd < 0) return pmNoData;
    struct pollfd p = {.fd=s->fd, .events=POLLIN};
    return poll(&p, 1, 0) > 0 ? pmGotData : pmNoData;
}
int Pm_Read(PortMidiStream* stream, PmEvent* events, int32_t length) {
    Stream* s = stream; int n=0; unsigned char bytes[3];
    while (n < length && recv(s->fd, bytes, 3, MSG_DONTWAIT) == 3) {
        events[n].message=Pm_Message(bytes[0], bytes[1], bytes[2]);
        events[n++].timestamp=0;
        fprintf(stderr, "TEST MIDI RX %02x %02x %02x\n", bytes[0], bytes[1], bytes[2]);
    }
    return n;
}
PmError Pm_WriteShort(PortMidiStream* s, PmTimestamp t, PmMessage message) {
    (void)s; (void)t;
    fprintf(stderr, "TEST MIDI TX %02x %02x %02x\n", Pm_MessageStatus(message), Pm_MessageData1(message), Pm_MessageData2(message));
    return pmNoError;
}
PmError Pm_Write(PortMidiStream* s, PmEvent* events, int32_t length) {
    for (int i=0; i<length; ++i) Pm_WriteShort(s, events[i].timestamp, events[i].message);
    return pmNoError;
}
PmError Pm_WriteSysEx(PortMidiStream* s, PmTimestamp t, unsigned char* message) {
    (void)s; (void)t; (void)message; return pmNoError;
}
PmError Pm_SetFilter(PortMidiStream* s, int32_t filters) { (void)s; (void)filters; return pmNoError; }
PmError Pm_SetChannelMask(PortMidiStream* s, int mask) { (void)s; (void)mask; return pmNoError; }
PmError Pm_Abort(PortMidiStream* s) { (void)s; return pmNoError; }
int Pm_HasHostError(PortMidiStream* s) { (void)s; return 0; }
