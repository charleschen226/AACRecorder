#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QMainWindow>
#include <QAudioDeviceInfo>
#include <QAudioInput>
#include <QAudioOutput>
#include <QIODevice>
#include <QByteArray>
#include <QFile>
#include <stdio.h>

#include "audioprocess.h"
#include "./libfaac/faac.h"

namespace Ui {
class AudioRecorder;
}

class AudioRecorder : public QMainWindow
{
    Q_OBJECT

public:
    explicit AudioRecorder(QWidget *parent = 0);
    ~AudioRecorder();

public:
    void InitWindow();
    void setProcessValue(char *data, qint64 len);
    void refleshDisplay();
    void initProcessValue();
    void writeDataToFile(QFile *file, QByteArray data);
    void PCMtoAAC(const char *fileName);

private slots:
    void sampleRateChanged(int idx);
    void channelChanged(int idx);
    void codecChanged(int idx);
    void sampleSizeChanged(int idx);
    void sampleTypeChanged(int idx);
    void endianChanged(int idx);
    void deviceChanged(int index);
    void readData();

    void on_recordButton_clicked();
    void on_pauseButton_clicked();
    void on_saveButton_clicked();

private:
    Ui::AudioRecorder   *ui;

    quint32             m_maxAmplitude;     //最大振幅
    AudioProcess        *m_audioProcess;

    QAudioDeviceInfo    m_devInfo;
    QAudioFormat        m_format;

    QAudioInput         *m_audioInput;
    QAudioOutput        *m_audioOutput;

    QIODevice           *m_inDev;
    QIODevice           *m_outDev;

    QFile               *m_tmpFile;
};

#endif // AUDIORECORDER_H
