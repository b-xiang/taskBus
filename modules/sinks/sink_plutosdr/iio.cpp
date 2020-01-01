#include "iio.h"
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <QThread>
#include <cmath>
#include "cmdlineparser.h"
#include "tb_interface.h"
using namespace TASKBUS;
/* helper macros */
#define MHZ(x) ((long long)((x)*1000000.0 + .5))
#define GHZ(x) ((long long)((x)*1000000000.0 + .5))

#define ASSERT(expr) { \
	if (!(expr)) { \
	(void) fprintf(stderr, "assertion failed (%s:%d)\n", __FILE__, __LINE__); \
	(void) abort(); \
	} \
	}

/* RX is input, TX is output */
enum iodev { RX, TX };

/* common RX and TX streaming params */
struct stream_cfg {
	long long bw_hz; // Analog banwidth in Hz
	long long fs_hz; // Baseband sample rate in Hz
	long long lo_hz; // Local oscillator frequency in Hz
	const char* rfport; // Port name
};

/* scratch mem for strings */
char tmpstr[64];

/* IIO structs required for streaming */
struct iio_context *ctx   = NULL;
struct iio_channel *tx0_i = NULL;
struct iio_channel *tx0_q = NULL;
struct iio_buffer  *txbuf = NULL;
extern bool bfinished;

/* cleanup and exit */
void shutdown()
{
	fprintf(stderr,"* Destroying buffers\n");
	if (txbuf) { iio_buffer_destroy(txbuf); }

	fprintf(stderr,"* Disabling streaming channels\n");
	if (tx0_i) { iio_channel_disable(tx0_i); }
	if (tx0_q) { iio_channel_disable(tx0_q); }

	fprintf(stderr,"* Destroying context\n");
	if (ctx) { iio_context_destroy(ctx); }
	exit(0);
}

void handle_sig(int sig)
{
	fprintf(stderr,"Waiting for process to finish...\n");
	bfinished = true;
}

/* check return value of attr_write function */
void errchk(int v, const char* what) {
	if (v < 0) { fprintf(stderr, "Error %d writing to channel \"%s\"\nvalue may not be supported.\n", v, what); shutdown(); }
}

/* write attribute: long long int */
void wr_ch_lli(struct iio_channel *chn, const char* what, long long val)
{
	errchk(iio_channel_attr_write_longlong(chn, what, val), what);
}

/* write attribute: string */
void wr_ch_str(struct iio_channel *chn, const char* what, const char* str)
{
	errchk(iio_channel_attr_write(chn, what, str), what);
}

/* helper function generating channel names */
char* get_ch_name(const char* type, int id)
{
	snprintf(tmpstr, sizeof(tmpstr), "%s%d", type, id);
	return tmpstr;
}

/* returns ad9361 phy device */
struct iio_device* get_ad9361_phy(struct iio_context *ctx)
{
	struct iio_device *dev =  iio_context_find_device(ctx, "ad9361-phy");
	ASSERT(dev && "No ad9361-phy found");
	return dev;
}

/* finds AD9361 streaming IIO devices */
bool get_ad9361_stream_dev(struct iio_context *ctx, enum iodev d, struct iio_device **dev)
{
	switch (d) {
	case TX: *dev = iio_context_find_device(ctx, "cf-ad9361-dds-core-lpc"); return *dev != NULL;
	case RX: *dev = iio_context_find_device(ctx, "cf-ad9361-lpc");  return *dev != NULL;
	default: ASSERT(0); return false;
	}
}

/* finds AD9361 streaming IIO channels */
bool get_ad9361_stream_ch(struct iio_context *ctx, enum iodev d, struct iio_device *dev, int chid, struct iio_channel **chn)
{
	*chn = iio_device_find_channel(dev, get_ch_name("voltage", chid), d == TX);
	if (!*chn)
		*chn = iio_device_find_channel(dev, get_ch_name("altvoltage", chid), d == TX);
	return *chn != NULL;
}

/* finds AD9361 phy IIO configuration channel with id chid */
bool get_phy_chan(struct iio_context *ctx, enum iodev d, int chid, struct iio_channel **chn)
{
	switch (d) {
	case RX: *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("voltage", chid), false); return *chn != NULL;
	case TX: *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("voltage", chid), true);  return *chn != NULL;
	default: ASSERT(0); return false;
	}
}

/* finds AD9361 local oscillator IIO configuration channels */
bool get_lo_chan(struct iio_context *ctx, enum iodev d, struct iio_channel **chn)
{
	switch (d) {
	// LO chan is always output, i.e. true
	case RX: *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("altvoltage", 0), true); return *chn != NULL;
	case TX: *chn = iio_device_find_channel(get_ad9361_phy(ctx), get_ch_name("altvoltage", 1), true); return *chn != NULL;
	default: ASSERT(0); return false;
	}
}

/* applies streaming configuration through IIO */
bool cfg_ad9361_streaming_ch(struct iio_context *ctx, struct stream_cfg *cfg, enum iodev type, int chid)
{
	struct iio_channel *chn = NULL;

	// Configure phy and lo channels
	fprintf(stderr,"* Acquiring AD9361 phy channel %d\n", chid);
	if (!get_phy_chan(ctx, type, chid, &chn)) {	return false; }
	wr_ch_str(chn, "rf_port_select",     cfg->rfport);
	wr_ch_lli(chn, "rf_bandwidth",       cfg->bw_hz);
	wr_ch_lli(chn, "sampling_frequency", cfg->fs_hz);

	// Configure LO channel
	fprintf(stderr,"* Acquiring AD9361 %s lo channel\n", type == TX ? "TX" : "RX");
	if (!get_lo_chan(ctx, type, &chn)) { return false; }
	wr_ch_lli(chn, "frequency", cfg->lo_hz);
	return true;
}


/* simple configuration and streaming */
/* usage:
 * Default context, assuming local IIO devices, i.e., this script is run on ADALM-Pluto for example
 $./a.out
 * URI context, find out the uri by typing `iio_info -s` at the command line of the host PC
 $./a.out usb:x.x.x
 */
int do_iio(const cmdlineParser & args)
{
	//获得平台告诉自己的实例名
	const int instance	  = args.toInt("instance",0);
	const int isource		  = args.toInt("source",0);
	const int timestamp	  = args.toInt("timestamp",0);
	const double sample_rate =  args.toDouble("sprate",2.5);
	const double bw =  args.toDouble("bw",2.5);
	const double rf =  args.toDouble("rf",1800);
	fprintf(stderr,"* Init SDR Pluto...\n");
	fflush(stderr);
	// Streaming devices
	struct iio_device *tx;

	// RX and TX sample counters
	size_t ntx = 0;

	// Stream configurations
	struct stream_cfg txcfg;
	// Listen to ctrl+c and ASSERT
	signal(SIGINT, handle_sig);

	// RX stream config
	txcfg.bw_hz = MHZ(bw);   //2 MHz rf bandwidth
	txcfg.fs_hz = MHZ(sample_rate);   // 2.5 MS/s rx sample rate
	txcfg.lo_hz = MHZ(rf); // 2.5 GHz rf frequency
	txcfg.rfport = "A"; // port A (select for rf freq.)

	fprintf(stderr,"* RF=%lldHz, BW=%lldHz, SPR=%lldsps...\n",txcfg.lo_hz,txcfg.bw_hz,txcfg.fs_hz);
	fflush(stderr);
	fprintf(stderr,"* Acquiring IIO context\n");
	ASSERT((ctx = iio_create_context_from_uri(args.toString("uri","ip:192.168.2.1").c_str())) && "No context");
	ASSERT(iio_context_get_devices_count(ctx) > 0 && "No devices");

	fprintf(stderr,"* Acquiring AD9361 streaming devices\n");
	ASSERT(get_ad9361_stream_dev(ctx, TX, &tx) && "No tx dev found");

	ASSERT(cfg_ad9361_streaming_ch(ctx, &txcfg, TX, 0) && "TX port 0 not found");

	fprintf(stderr,"* Initializing AD9361 IIO streaming channels\n");
	ASSERT(get_ad9361_stream_ch(ctx, TX, tx, 0, &tx0_i) && "TX chan i not found");
	ASSERT(get_ad9361_stream_ch(ctx, TX, tx, 1, &tx0_q) && "TX chan q not found");

	fprintf(stderr,"* Enabling IIO streaming channels\n");
	iio_channel_enable(tx0_i);
	iio_channel_enable(tx0_q);

	fprintf(stderr,"* Creating non-cyclic IIO buffers with 1 MiS\n");
	txbuf = iio_device_create_buffer(tx, 1024*1024, false);
	if (!txbuf) {
		perror("Could not create TX buffer");
		shutdown();
	}
	fprintf(stderr,"* Starting IO streaming \n");
	int clockc = 0;
	while (!bfinished)
	{
		ssize_t nbytes_tx;
		char *p_dat, *p_end;
		ptrdiff_t p_inc;

		nbytes_tx = iio_buffer_push(txbuf);
		if (nbytes_tx < 0) {
			QThread::msleep(20);
			continue;
		}

		p_inc = iio_buffer_step(txbuf);
		p_end = (char *) iio_buffer_end(txbuf);
		for (p_dat = (char *)iio_buffer_first(txbuf, tx0_i); p_dat < p_end; p_dat += p_inc) {
			// Example: fill with zeros
			// 12-bit sample needs to be MSB alligned so shift by 4
			// https://wiki.analog.com/resources/eval/user-guides/ad-fmcomms2-ebz/software/basic_iq_datafiles#binary_format
			((int16_t*)p_dat)[0] = short(cos(2*3.14*clockc/64)*512) << 4; // Real (I)
			((int16_t*)p_dat)[1] = short(sin(2*3.14*clockc/64)*512) << 4; // Imag (Q)
			++clockc;
		}

	}

	shutdown();

	push_subject(control_subect_id(),/*instance,broadcast_destin_id(),*/0,"Shutting down Pluto SDR Source.");


}
