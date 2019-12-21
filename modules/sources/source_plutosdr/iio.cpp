#include "iio.h"
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <QThread>
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
struct iio_channel *rx0_i = NULL;
struct iio_channel *rx0_q = NULL;
struct iio_buffer  *rxbuf = NULL;
extern bool bfinished;

/* cleanup and exit */
void shutdown()
{
	fprintf(stderr,"* Destroying buffers\n");
	if (rxbuf) { iio_buffer_destroy(rxbuf); }

	fprintf(stderr,"* Disabling streaming channels\n");
	if (rx0_i) { iio_channel_disable(rx0_i); }
	if (rx0_q) { iio_channel_disable(rx0_q); }

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
	TASKBUS::push_subject(0xffffffff,0,
						  QString("source=%1.source_plutosdr.taskbus;"
								  "destin=all;"
								  "function=samplerate;"
								  "sample_rate=%2;"
								  )
						  .arg(instance)
						  .arg(sample_rate*1000000).toStdString().c_str());

	// Streaming devices
	struct iio_device *rx;

	// RX and TX sample counters
	size_t nrx = 0;

	// Stream configurations
	struct stream_cfg rxcfg;
	// Listen to ctrl+c and ASSERT
	signal(SIGINT, handle_sig);

	// RX stream config
	rxcfg.bw_hz = MHZ(args.toDouble("bw",2.0));   //2 MHz rf bandwidth
	rxcfg.fs_hz = MHZ(sample_rate);   // 2.5 MS/s rx sample rate
	rxcfg.lo_hz = MHZ(args.toDouble("rf",2.5)); // 2.5 GHz rf frequency
	rxcfg.rfport = "A_BALANCED"; // port A (select for rf freq.)

	fprintf(stderr,"* Acquiring IIO context\n");
	ASSERT((ctx = iio_create_context_from_uri(args.toString("uri","ip:192.168.2.1").c_str())) && "No context");
	ASSERT(iio_context_get_devices_count(ctx) > 0 && "No devices");

	fprintf(stderr,"* Acquiring AD9361 streaming devices\n");
	ASSERT(get_ad9361_stream_dev(ctx, RX, &rx) && "No rx dev found");

	fprintf(stderr,"* Configuring AD9361 for streaming\n");
	ASSERT(cfg_ad9361_streaming_ch(ctx, &rxcfg, RX, 0) && "RX port 0 not found");

	fprintf(stderr,"* Initializing AD9361 IIO streaming channels\n");
	ASSERT(get_ad9361_stream_ch(ctx, RX, rx, 0, &rx0_i) && "RX chan i not found");
	ASSERT(get_ad9361_stream_ch(ctx, RX, rx, 1, &rx0_q) && "RX chan q not found");

	fprintf(stderr,"* Enabling IIO streaming channels\n");
	iio_channel_enable(rx0_i);
	iio_channel_enable(rx0_q);

	fprintf(stderr,"* Creating non-cyclic IIO buffers with 1 MiS\n");
	rxbuf = iio_device_create_buffer(rx, 1024*100, false);
	if (!rxbuf) {
		perror("Could not create RX buffer");
		shutdown();
	}
	fprintf(stderr,"* Starting IO streaming \n");
	while (!bfinished)
	{
		ssize_t nbytes_rx;
		char *p_dat, *p_end;
		ptrdiff_t p_inc;

		// Refill RX buffer
		nbytes_rx = iio_buffer_refill(rxbuf);
		if (nbytes_rx < 0) {
			QThread::msleep(20);
			continue;
		}

		// READ: Get pointers to RX buf and read IQ from RX buf port 0
		p_inc = iio_buffer_step(rxbuf);
		p_end = (char *)iio_buffer_end(rxbuf);
		p_dat = (char *)iio_buffer_first(rxbuf, rx0_i);
		int frame_len = p_end - p_dat;
		if (timestamp)
		{
			push_subject(
			timestamp,				//专题
			instance,				//咱就一路数据，干干净净,用自己的进程ID确保唯一性。
			sizeof(unsigned long long),
			(unsigned char *)&nrx
			);
		}
		if (isource)
		{
			push_subject(
			isource,				//专题
			instance,				//咱就一路数据，干干净净,用自己的进程ID确保唯一性。
			frame_len,
			(unsigned char *)p_dat
			);
		}

		// Sample counter increment and status output
		nrx += nbytes_rx / iio_device_get_sample_size(rx);
		//fprintf(stderr,"\tRX %8.2f MSmp, TX %8.2f MSmp\n", nrx/1e6, ntx/1e6);
	}

	shutdown();

	push_subject(control_subect_id(),/*instance,broadcast_destin_id(),*/0,"Shutting down Pluto SDR Source.");


}
