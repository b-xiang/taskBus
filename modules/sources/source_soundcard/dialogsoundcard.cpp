#include "dialogsoundcard.h"
#include "ui_dialogsoundcard.h"
#include <QLayout>
#include <QDebug>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>

DialogSoundCard::DialogSoundCard(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DialogSoundCard),
	m_devInputModule(new QStandardItemModel(this))
{
	ui->setupUi(this);
	ui->comboBox_audioDevices->setModel(m_devInputModule);
	m_devInputlist = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
	foreach(QAudioDeviceInfo info, m_devInputlist)
		m_devInputModule->appendRow(new QStandardItem(info.deviceName()));

	//---------- recorders ----------
	miMaxValue = 0;
	miVolume = ui->horizontalSlider->value();

	mpAudioInputSound = nullptr;
	mpInputDevSound = nullptr;

	ui->btn_stop->setDisabled(true);

	connect(ui->btn_start, SIGNAL(clicked()), this,SLOT(OnRecordStart()));
	connect(ui->btn_stop, SIGNAL(clicked()), this,SLOT(OnRecordStop()));

}

DialogSoundCard::DialogSoundCard(const TASKBUS::cmdlineParser * pline,QWidget *parent ):
	DialogSoundCard(parent)
{
	m_cmdline = pline;
	if (m_cmdline)
	{
		int sample_rate = m_cmdline->toInt("sample_rate",48000);
		QString device = QString::fromStdString( m_cmdline->toString("device","default"));
		int channel = m_cmdline->toInt("channel",2);

		ui->spinbox_channels->setValue(channel);
		ui->spinbox_sprate->setValue(sample_rate);
		QList<QStandardItem*> items = m_devInputModule->findItems(device);
		if (items.size())
		{
			QModelIndex nitem = m_devInputModule->indexFromItem(items.first());
			if (nitem.row()>=0 && nitem.row()<m_devInputModule->rowCount())
				ui->comboBox_audioDevices->setCurrentIndex(nitem.row());
		}

		//Listen thread to recieve messages from platform
		m_pListenThread = new listen_thread(m_cmdline,this);
		connect(m_pListenThread,&listen_thread::quit_app,this,&DialogSoundCard::close);
		m_pListenThread->start();


		int hiden = m_cmdline->toInt("hide",0);
		int autostart = m_cmdline->toInt("autostart",0);
		m_batch_size = m_cmdline->toInt("batch_size",0);

		if (hiden || autostart)
			OnRecordStart();

	}
}

DialogSoundCard::~DialogSoundCard()
{
	if (m_pListenThread)
		m_pListenThread->terminate();
	delete ui;
}

void DialogSoundCard::OnRecordStart()
{
	ui->btn_start->setDisabled(true);
	ui->btn_stop->setDisabled(false);
	m_nTotalSps = 0;
	InitMonitor();

}


void DialogSoundCard::OnRecordStop()
{
	ui->btn_start->setDisabled(false);
	ui->btn_stop->setDisabled(true);
	if (mpInputDevSound != 0) {
		disconnect(mpInputDevSound, 0, this, 0);
		mpInputDevSound = 0;
	}
}

void DialogSoundCard::OnStateChange(QAudio::State state)
{
	if(state == QAudio::IdleState)
		OnRecordStop();
}

void DialogSoundCard::InitMonitor()
{
	mFormatSound.setSampleSize(16); //set sample sze to 16 bit
	mFormatSound.setSampleType(QAudioFormat::UnSignedInt ); //Sample type as usigned integer sample
	mFormatSound.setByteOrder(QAudioFormat::LittleEndian); //Byte order
	mFormatSound.setCodec("audio/pcm"); //set codec as simple audio/pcm
	mFormatSound.setSampleRate(ui->spinbox_sprate->value());
	mFormatSound.setChannelCount(ui->spinbox_channels->value());
	QAudioDeviceInfo infoIn;
	if (ui->comboBox_audioDevices->currentIndex()>=0 &&
			ui->comboBox_audioDevices->currentIndex()<m_devInputModule->rowCount())
		infoIn = m_devInputlist[ui->comboBox_audioDevices->currentIndex()];
	else
		infoIn = QAudioDeviceInfo::defaultInputDevice();
	if (!infoIn.isFormatSupported(mFormatSound))
	{
		//Default format not supported - trying to use nearest
		mFormatSound = infoIn.nearestFormat(mFormatSound);
	}

	QAudioDeviceInfo infoOut(QAudioDeviceInfo::defaultOutputDevice());
	if (!infoOut.isFormatSupported(mFormatSound))
	{
		//Default format not supported - trying to use nearest
		mFormatSound = infoOut.nearestFormat(mFormatSound);
	}

	CreateAudioInput();

	mpInputDevSound = mpAudioInputSound->start();
	connect(mpInputDevSound, SIGNAL(readyRead()), SLOT(OnReadMore()));

	connect(ui->horizontalSlider, SIGNAL(valueChanged(int)),
		this, SLOT(OnSliderValueChanged(int)));
}

void DialogSoundCard::CreateAudioInput()
{
	if (mpInputDevSound != 0) {
		disconnect(mpInputDevSound, 0, this, 0);
		mpInputDevSound = 0;
	}

	QAudioDeviceInfo inputDevice(QAudioDeviceInfo::defaultInputDevice());
	mpAudioInputSound = new QAudioInput(inputDevice, mFormatSound, this);
}


int DialogSoundCard::ApplyVolumeToSample(short iSample)
{
	//Calculate volume, Volume limited to  max 30000 and min -30000
	return std::max(std::min(((iSample * miVolume) / 50) ,30000), -30000);
}

void DialogSoundCard::OnSliderValueChanged(int value)
{
	miVolume = value;
}

void DialogSoundCard::OnReadMore()
{
	//Return if audio input is null
	if(!mpAudioInputSound || !m_cmdline)
		return;

	const int nid_wav = m_cmdline->toInt("wav",0);
	const int nid_timestamp = m_cmdline->toInt("timestamp",0);
	const int nins = m_cmdline->toInt("instance",0);
	const int channel = m_cmdline->toInt("channel",2);
	//Read sound samples from input device to buffer
	QByteArray mBuffer = mpInputDevSound->readAll();
	qint64 l = mBuffer.length();
	if(l > 0) {
		//Assign sound samples to short array
		short* resultingData = (short*)mBuffer.data();
		const int len = l/2;

		short *outdata=resultingData;
		outdata[ 0 ] = resultingData [ 0 ];
		int iIndex;
		miMaxValue = 0;
		for ( iIndex=0; iIndex < len; iIndex++ ) {
			//Change volume to each integer data in a sample
			int value = ApplyVolumeToSample( outdata[ iIndex ]);
			outdata[ iIndex ] = value;

			miMaxValue = miMaxValue>=value ? miMaxValue : value;
		}


		if (m_batch_size<=0)
		{
			//write modified sound sample to outputdevice for playback audio
			if (nid_timestamp)
				TASKBUS::push_subject(nid_timestamp,nins,sizeof(quint64),(unsigned char *)&m_nTotalSps);
			if (nid_wav>0)
				TASKBUS::push_subject(nid_wav,nins,len*2,(unsigned char *)outdata);
			m_nTotalSps += len*2;
			QTimer::singleShot(1000, this, SLOT(OnTimeOut()));
		}
		else
		{
			m_buffer.append((char *)outdata,len*2);
			const int groupBytes = channel*m_batch_size*2;
			const int groups = m_buffer.size()/groupBytes;
			const char * pdata = m_buffer.constData();
			for (int i=0;i<groups;++i)
			{
				//take blocking check
				if ((m_nTotalSps - listen_thread::m_refTms )<=19200*16 ||
						listen_thread::m_refTms==0)
				{
					//write modified sound sample to outputdevice for playback audio
					if (nid_timestamp)
						TASKBUS::push_subject(nid_timestamp,nins,sizeof(quint64),(unsigned char *)&m_nTotalSps);
					if (nid_wav>0)
						TASKBUS::push_subject(nid_wav,nins,
											  groupBytes,
											  (unsigned char *)(pdata+i*groupBytes));
					QTimer::singleShot(1000, this, SLOT(OnTimeOut()));
				}
				m_nTotalSps += m_batch_size*2*channel;
			}
			//tail
			m_buffer.remove(0,groups*groupBytes);
		}
	}
}

void DialogSoundCard::OnTimeOut()
{
	ui->progress->setValue(miMaxValue);
}
