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
	m_devOutputModule(new QStandardItemModel(this))
{
	ui->setupUi(this);
	ui->comboBox_audioDevices->setModel(m_devOutputModule);
	m_devOutputlist = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
	foreach(QAudioDeviceInfo info, m_devOutputlist)
		m_devOutputModule->appendRow(new QStandardItem(info.deviceName()));

	//---------- players ----------
	miMaxValue = 0;
	miVolume = ui->horizontalSlider->value();

	mpAudioOutputSound = nullptr;
	mpOutDevSound = nullptr;

	ui->btn_stop->setDisabled(true);

	connect(ui->btn_start, SIGNAL(clicked()), this,SLOT(OnPlayStart()));
	connect(ui->btn_stop, SIGNAL(clicked()), this,SLOT(OnPlayStop()));

}

DialogSoundCard::DialogSoundCard(const TASKBUS::cmdlineParser * pline,QWidget *parent ):
	DialogSoundCard(parent)
{
	m_cmdline = pline;
	if (m_cmdline)
	{
		int sample_rate = m_cmdline->toInt("sample_rate",48000);
		setInstance(m_cmdline->toInt("instance",0));
		QString device = QString::fromStdString( m_cmdline->toString("device","default"));
		int channel = m_cmdline->toInt("channel",2);

		ui->spinbox_channels->setValue(channel);
		ui->spinbox_sprate->setValue(sample_rate);
		QList<QStandardItem*> items = m_devOutputModule->findItems(device);
		if (items.size())
		{
			QModelIndex nitem = m_devOutputModule->indexFromItem(items.first());
			if (nitem.row()>=0 && nitem.row()<m_devOutputModule->rowCount())
				ui->comboBox_audioDevices->setCurrentIndex(nitem.row());
		}

		//Listen thread to recieve messages from platform
		m_pListenThread = new listen_thread(m_cmdline,this);
		connect(m_pListenThread,&listen_thread::quit_app,this,&DialogSoundCard::close);
		connect(m_pListenThread,&listen_thread::sig_play,this,&DialogSoundCard::sltPlay);
		m_pListenThread->start();


		int hiden = m_cmdline->toInt("hide",0);
		int autostart = m_cmdline->toInt("autostart",0);
		m_batch_size = m_cmdline->toInt("batch_size",0);

		if (hiden || autostart)
			OnPlayStart();

	}
}

DialogSoundCard::~DialogSoundCard()
{
	if (m_pListenThread)
		m_pListenThread->terminate();
	delete ui;
}

void DialogSoundCard::OnPlayStart()
{
	ui->btn_start->setDisabled(true);
	ui->btn_stop->setDisabled(false);
	m_nTotalSps = 0;
	InitMonitor();

}


void DialogSoundCard::OnPlayStop()
{
	ui->btn_start->setDisabled(false);
	ui->btn_stop->setDisabled(true);
	if (mpOutDevSound != 0) {
		disconnect(mpOutDevSound, 0, this, 0);
		mpOutDevSound = 0;
	}
}

void DialogSoundCard::OnStateChange(QAudio::State state)
{
	if(state == QAudio::IdleState)
		OnPlayStop();
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
			ui->comboBox_audioDevices->currentIndex()<m_devOutputModule->rowCount())
		infoIn = m_devOutputlist[ui->comboBox_audioDevices->currentIndex()];
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

	CreateAudioOutput();

	mpOutDevSound = mpAudioOutputSound->start();
	connect(ui->horizontalSlider, SIGNAL(valueChanged(int)),
			this, SLOT(OnSliderValueChanged(int)));
}

void DialogSoundCard::CreateAudioOutput()
{
	if (mpOutDevSound != 0) {
		disconnect(mpOutDevSound, 0, this, 0);
		mpOutDevSound = 0;
	}

	QAudioDeviceInfo outputDevice(QAudioDeviceInfo::defaultOutputDevice());
	mpAudioOutputSound = new QAudioOutput(outputDevice, mFormatSound, this);
}
void DialogSoundCard::sltPlay(QByteArray v)
{
	miMaxValue = 0;
	if (mpOutDevSound)
	{
		qint16 * outdata = (qint16 *)v.data();
		int len = v.size()/2;
		miMaxValue = 0;
		for ( int iIndex=0; iIndex < len; iIndex++ ) {
			//Change volume to each integer data in a sample
			int value = ApplyVolumeToSample( outdata[ iIndex ]);
			outdata[ iIndex ] = value;

			miMaxValue = miMaxValue>=value ? miMaxValue : value;
		}
		mpOutDevSound->write(v);
	}
	ui->progress->setValue(miMaxValue);
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

void DialogSoundCard::OnTimeOut()
{
	ui->progress->setValue(miMaxValue);
}
