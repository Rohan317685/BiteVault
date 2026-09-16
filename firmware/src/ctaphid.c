#include "ctaphid.h"
#include "u2f.h"
#include "led.h"
#include "timebase.h"
#include "fsusb.h"

#define CID_BROADCAST 0xFFFFFFFFu

// Commands (the high bit marks an init packet).
#define CTAPHID_PING      0x01
#define CTAPHID_MSG       0x03
#define CTAPHID_LOCK      0x04
#define CTAPHID_INIT      0x06
#define CTAPHID_WINK      0x08
#define CTAPHID_CBOR      0x10
#define CTAPHID_CANCEL    0x11
#define CTAPHID_ERROR     0x3F

// Error codes carried in a CTAPHID_ERROR payload.
#define ERR_INVALID_CMD     0x01
#define ERR_INVALID_PAR     0x02
#define ERR_INVALID_LEN     0x03
#define ERR_INVALID_SEQ     0x04
#define ERR_MSG_TIMEOUT     0x05
#define ERR_CHANNEL_BUSY    0x06
#define ERR_INVALID_CHANNEL 0x0B
#define ERR_OTHER           0x7F

// Capabilities reported by INIT.  WINK only; CBOR is deliberately absent
// because CTAP2 is not implemented.
#define CAPABILITY_WINK 0x01

#define INIT_PAYLOAD 57 // 64 - 7 bytes of header
#define CONT_PAYLOAD 59 // 64 - 5 bytes of header

// A host has 500 ms between packets of one message before we give up on it.
#define TRANSACTION_TIMEOUT_MS 500

// Packets arriving back to back while the main loop is busy elsewhere queue up
// here.  One slot is always left empty to keep head/tail unambiguous, so this
// holds RX_QUEUE_LEN - 1 packets.
#define RX_QUEUE_LEN 8

struct rx_state {
	uint32_t cid;
	uint8_t  cmd;
	uint16_t expected;
	uint16_t got;
	uint8_t  next_seq;
	uint8_t  active;
	uint32_t last_packet_ms;
	uint8_t  buf[CTAPHID_MAX_MSG];
};

struct tx_state {
	uint32_t cid;
	uint8_t  cmd;
	uint16_t len;
	uint16_t sent;
	uint8_t  seq;
	uint8_t  active;
	uint8_t  buf[CTAPHID_MAX_MSG];
};

static struct rx_state g_rx;
static struct tx_state g_tx;

static volatile uint8_t g_queue[RX_QUEUE_LEN][CTAPHID_PACKET_SIZE];
static volatile uint8_t g_queue_head; // written by the ISR
static volatile uint8_t g_queue_tail; // written by the main loop

static uint32_t g_next_cid = 1;

static uint32_t be32(const uint8_t *p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
	       ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void put_be32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)(v >> 24);
	p[1] = (uint8_t)(v >> 16);
	p[2] = (uint8_t)(v >> 8);
	p[3] = (uint8_t)v;
}

void ctaphid_init(void)
{
	g_rx.active = 0;
	g_tx.active = 0;
	g_queue_head = 0;
	g_queue_tail = 0;
}

void ctaphid_rx_packet(const uint8_t *packet, int len)
{
	uint8_t head = g_queue_head;
	uint8_t next = (uint8_t)((head + 1) % RX_QUEUE_LEN);

	if (next == g_queue_tail) {
		return; // queue full: drop, and let the host's transaction time out
	}
	if (len > CTAPHID_PACKET_SIZE) {
		len = CTAPHID_PACKET_SIZE;
	}

	int i = 0;
	for (; i < len; i++) {
		g_queue[head][i] = packet[i];
	}
	for (; i < CTAPHID_PACKET_SIZE; i++) {
		g_queue[head][i] = 0; // short reports are zero padded
	}

	g_queue_head = next;
}

// Queues a reply.  The payload is copied, so callers may pass the receive
// buffer.  Only ever called with the transmitter idle.
static void send_response(uint32_t cid, uint8_t cmd, const uint8_t *data, uint16_t len)
{
	if (len > CTAPHID_MAX_MSG) {
		len = CTAPHID_MAX_MSG;
	}

	g_tx.cid = cid;
	g_tx.cmd = cmd;
	g_tx.len = len;
	g_tx.sent = 0;
	g_tx.seq = 0;
	for (uint16_t i = 0; i < len; i++) {
		g_tx.buf[i] = data[i];
	}
	g_tx.active = 1;
}

static void send_error(uint32_t cid, uint8_t code)
{
	send_response(cid, CTAPHID_ERROR, &code, 1);
}

static void handle_init(uint32_t cid, const uint8_t *nonce, uint16_t len)
{
	uint8_t reply[17];

	if (len != 8) {
		send_error(cid, ERR_INVALID_LEN);
		return;
	}

	uint32_t assigned;
	if (cid == CID_BROADCAST) {
		// Hand out a fresh channel, skipping the two reserved values.
		assigned = g_next_cid++;
		if (g_next_cid == 0 || g_next_cid == CID_BROADCAST) {
			g_next_cid = 1;
		}
	} else {
		assigned = cid; // resync of an existing channel
	}

	for (int i = 0; i < 8; i++) {
		reply[i] = nonce[i];
	}
	put_be32(&reply[8], assigned);
	reply[12] = 2;    // CTAPHID protocol version
	reply[13] = 0;    // device major
	reply[14] = 1;    // device minor
	reply[15] = 0;    // device build
	reply[16] = CAPABILITY_WINK;

	send_response(cid, CTAPHID_INIT, reply, sizeof(reply));
}

static void dispatch(void)
{
	uint32_t cid = g_rx.cid;

	switch (g_rx.cmd) {
	case CTAPHID_INIT:
		handle_init(cid, g_rx.buf, g_rx.got);
		break;

	case CTAPHID_PING:
		send_response(cid, CTAPHID_PING, g_rx.buf, g_rx.got);
		break;

	case CTAPHID_MSG: {
		uint8_t reply[64];
		uint16_t len = u2f_handle_apdu(g_rx.buf, g_rx.got, reply, sizeof(reply));
		if (len == 0) {
			send_error(cid, ERR_OTHER);
		} else {
			send_response(cid, CTAPHID_MSG, reply, len);
		}
		break;
	}

	case CTAPHID_WINK:
		led_flash(LED_TOUCH, 1200);
		send_response(cid, CTAPHID_WINK, 0, 0);
		break;

	case CTAPHID_CANCEL:
		// Nothing here runs long enough to be cancelled, and the spec wants no
		// response to a cancel of its own.
		break;

	case CTAPHID_CBOR:
	case CTAPHID_LOCK:
	default:
		send_error(cid, ERR_INVALID_CMD);
		break;
	}
}

static void handle_packet(const uint8_t *pkt)
{
	uint32_t cid = be32(pkt);
	uint32_t now = millis();

	if (cid == 0) {
		send_error(cid, ERR_INVALID_CHANNEL);
		return;
	}

	if (pkt[4] & 0x80) {
		uint8_t cmd = pkt[4] & 0x7F;
		uint16_t bcnt = (uint16_t)((pkt[5] << 8) | pkt[6]);

		// INIT on a live channel is how a host resynchronises it, so it is
		// allowed to interrupt that channel's own transaction.
		if (g_rx.active && g_rx.cid != cid && cmd != CTAPHID_INIT) {
			send_error(cid, ERR_CHANNEL_BUSY);
			return;
		}
		if (cid == CID_BROADCAST && cmd != CTAPHID_INIT) {
			send_error(cid, ERR_INVALID_CHANNEL);
			return;
		}
		if (bcnt > CTAPHID_MAX_MSG) {
			send_error(cid, ERR_INVALID_LEN);
			return;
		}

		uint16_t chunk = bcnt < INIT_PAYLOAD ? bcnt : INIT_PAYLOAD;
		for (uint16_t i = 0; i < chunk; i++) {
			g_rx.buf[i] = pkt[7 + i];
		}

		g_rx.cid = cid;
		g_rx.cmd = cmd;
		g_rx.expected = bcnt;
		g_rx.got = chunk;
		g_rx.next_seq = 0;
		g_rx.active = 1;
		g_rx.last_packet_ms = now;
	} else {
		uint8_t seq = pkt[4];

		if (!g_rx.active || g_rx.cid != cid) {
			return; // stray continuation packet, nothing to attach it to
		}
		if (seq != g_rx.next_seq) {
			g_rx.active = 0;
			send_error(cid, ERR_INVALID_SEQ);
			return;
		}

		uint16_t remaining = (uint16_t)(g_rx.expected - g_rx.got);
		uint16_t chunk = remaining < CONT_PAYLOAD ? remaining : CONT_PAYLOAD;
		for (uint16_t i = 0; i < chunk; i++) {
			g_rx.buf[g_rx.got + i] = pkt[5 + i];
		}

		g_rx.got = (uint16_t)(g_rx.got + chunk);
		g_rx.next_seq++;
		g_rx.last_packet_ms = now;
	}

	if (g_rx.got >= g_rx.expected) {
		g_rx.active = 0;
		dispatch();
	}
}

// Pushes as many packets of the pending reply as the endpoint will take.
static void tx_pump(void)
{
	while (g_tx.active) {
		uint8_t *buf = USBFS_GetEPBufferIfAvailable(1);
		if (!buf) {
			return; // endpoint still busy; try again next time round
		}

		for (int i = 0; i < CTAPHID_PACKET_SIZE; i++) {
			buf[i] = 0;
		}
		put_be32(buf, g_tx.cid);

		uint16_t offset;
		uint16_t room;
		if (g_tx.sent == 0) {
			buf[4] = (uint8_t)(g_tx.cmd | 0x80);
			buf[5] = (uint8_t)(g_tx.len >> 8);
			buf[6] = (uint8_t)(g_tx.len & 0xFF);
			offset = 7;
			room = INIT_PAYLOAD;
		} else {
			buf[4] = g_tx.seq++;
			offset = 5;
			room = CONT_PAYLOAD;
		}

		uint16_t remaining = (uint16_t)(g_tx.len - g_tx.sent);
		uint16_t chunk = remaining < room ? remaining : room;
		for (uint16_t i = 0; i < chunk; i++) {
			buf[offset + i] = g_tx.buf[g_tx.sent + i];
		}

		if (USBFS_SendEndpoint(1, CTAPHID_PACKET_SIZE) < 0) {
			if (g_tx.sent != 0) {
				g_tx.seq--; // roll the sequence number back for the retry
			}
			return;
		}

		g_tx.sent = (uint16_t)(g_tx.sent + chunk);
		if (g_tx.sent >= g_tx.len) {
			g_tx.active = 0;
		}
	}
}

void ctaphid_task(void)
{
	// Drain the queue only while the transmitter is free: a command cannot be
	// answered until the previous answer is out, and leaving packets queued is
	// the cheapest backpressure available.
	while (!g_tx.active && g_queue_tail != g_queue_head) {
		uint8_t packet[CTAPHID_PACKET_SIZE];
		uint8_t tail = g_queue_tail;

		for (int i = 0; i < CTAPHID_PACKET_SIZE; i++) {
			packet[i] = g_queue[tail][i];
		}
		g_queue_tail = (uint8_t)((tail + 1) % RX_QUEUE_LEN);

		handle_packet(packet);
	}

	if (g_rx.active &&
	    (uint32_t)(millis() - g_rx.last_packet_ms) > TRANSACTION_TIMEOUT_MS) {
		uint32_t cid = g_rx.cid;
		g_rx.active = 0;
		if (!g_tx.active) {
			send_error(cid, ERR_MSG_TIMEOUT);
		}
	}

	tx_pump();
}

int ctaphid_is_busy(void)
{
	return g_rx.active || g_tx.active || (g_queue_tail != g_queue_head);
}
