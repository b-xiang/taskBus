#ifndef DIALOGSOUNDCARD_H
#define DIALOGSOUNDCARD_H

#include <QDialog>
#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QList>
#include <QStandardItemModel>
#include "cmdlineparser.h"
#include "tb_interface.h"
#include "listen_thread.h"
namespace Ui {
	class DialogSoundCard;
}

class DialogSoundCard : public QDialog
{
	Q_OBJECT

public:
	explicit DialogSoundCard(QWidget *parent = nullptr);
	explicit DialogSoundCard(const TASKBUS::cmdlineParser * pline,QWidget *parent = nullptr);
	~DialogSoundCard();
	void setInstance(const int i){m_n_instance = i;}
private:
	Ui::DialogSoundCard *ui;
	QList<QAudioDeviceInfo> m_devInputlist;
	QStandardItemModel * m_devInputModule = nullptr;

private:
	int AddWavHeader(char *);
	int ApplyVolumeToSample(short iSample);
	void InitMonitor();
	void CreateAudioInput();
private slots:
	void OnRecordStart();
	void OnRecordStop();
	void OnStateChange(QAudio::State s);
	void OnReadMore();
	void OnSliderValueChanged(int);
	void OnTimeOut();

private:
	int m_n_instance = 0;
	int m_batch_size = 0;
	int miVolume;
	int miMaxValue;
	QByteArray m_buffer;
	quint64 m_nTotalSps = 0;
	const TASKBUS::cmdlineParser * m_cmdline = nullptr;
	listen_thread * m_pListenThread = nullptr;
private:
	QAudioFormat mFormatSound;
	QAudioInput *mpAudioInputSound;		// 负责监听声音
	QIODevice *mpInputDevSound;

};

#endif // DIALOGSOUNDCARD_H
