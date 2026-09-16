#ifndef BITEVAULT_CTAPHID_H
#define BITEVAULT_CTAPHID_H

#include <stdint.h>

// CTAPHID: the FIDO transport that runs over 64-byte HID reports.  Messages are
// split into one init packet and as many continuation packets as needed; this
// module reassembles them, dispatches the command, and streams the reply back.

#define CTAPHID_PACKET_SIZE 64

// Largest message we will accept or produce.  The spec allows 7609 bytes, but
// the CH32V203G6U6 only has 10 KB of RAM and nothing implemented here comes
// close, so oversized messages are rejected with ERR_INVALID_LEN.
#define CTAPHID_MAX_MSG 1024

void ctaphid_init(void);

// Called from the USB interrupt with one 64-byte report.  Only queues it; all
// of the work happens in ctaphid_task().
void ctaphid_rx_packet(const uint8_t *packet, int len);

void ctaphid_task(void);

// True while a transaction is being received, processed or sent.
int ctaphid_is_busy(void);

#endif
