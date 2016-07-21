#include "audiorecorder.h"
#include "ui_audiorecorder.h"

#include <QDebug>
#include <qendian.h>
#include <QFileDialog>

typedef unsigned char   BYTE;

static QString toString(QAudioFormat::SampleType sampleType)
{
    QString result("Unknown");
    switch (sampleType) {
    case QAudioFormat::SignedInt:
        result = "SignedInt";
        break;
    case QAudioFormat::UnSignedInt:
        result = "UnSignedInt";
        break;
    case QAudioFormat::Float:
        result = "Float";
        break;
    case QAudioFormat::Unknown:
        result = "Unknown";
    }
    return result;
}

static QString toString(QAudioFormat::Endian endian)
{
    QString result("Unknown");
    switch (endian) {
    case QAudioFormat::LittleEndian:
        result = "LittleEndian";
        break;
    case QAudioFormat::BigEndian:
        result = "BigEndian";
        break;
    }
    return result;
}

AudioRecorder::AudioRecorder(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::AudioRecorder)
    , m_audioProcess(0)
    , m_audioInput(0)
    , m_audioOutput(0)
    , m_inDev(0)
    , m_outDev(0)
    , m_tmpFile(0)
{
    ui->setupUi(this);

    setFixedSize(size());

    connect(ui->DeviceComboBox, SIGNAL(activated(int)), this, SLOT(deviceChanged(int)));

    InitWindow();
}

AudioRecorder::~AudioRecorder()
{
    delete ui;
    if(m_tmpFile)
        m_tmpFile->remove();
}

void AudioRecorder::InitWindow()
{
    //setup device
    ui->DeviceComboBox->addItem(tr("None"), QVariant(QString()));
    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(QAudio::AudioInput)) {
        ui->DeviceComboBox->addItem(deviceInfo.deviceName(), qVariantFromValue(deviceInfo));
    }
    m_audioProcess = new AudioProcess(this);
    ui->Layout->addWidget(m_audioProcess);

    ui->CodecComboBox->addItem(tr("None"), QVariant(QString()));
    ui->SampleRateComboBox->addItem(tr("None"), QVariant(QString()));
    ui->ChannelsComboBox->addItem(tr("None"), QVariant(QString()));
    ui->EndiannessComboBox->addItem(tr("None"), QVariant(QString()));
    ui->SampleSizeComboBox->addItem(tr("None"), QVariant(QString()));
    ui->SampleTypeComboBox->addItem(tr("None"), QVariant(QString()));

    //setup AudioFormat
    connect(ui->SampleRateComboBox, SIGNAL(activated(int)), SLOT(sampleRateChanged(int)));
    connect(ui->ChannelsComboBox, SIGNAL(activated(int)), SLOT(channelChanged(int)));
    connect(ui->CodecComboBox, SIGNAL(activated(int)), SLOT(codecChanged(int)));
    connect(ui->SampleSizeComboBox, SIGNAL(activated(int)), SLOT(sampleSizeChanged(int)));
    connect(ui->SampleTypeComboBox , SIGNAL(activated(int)), SLOT(sampleTypeChanged(int)));
    connect(ui->EndiannessComboBox, SIGNAL(activated(int)), SLOT(endianChanged(int)));
}

//setup Amplitude
void AudioRecorder::initProcessValue()
{
    switch (m_format.sampleSize()) {
    case 8:
        switch (m_format.sampleType()) {
        case QAudioFormat::UnSignedInt:
            m_maxAmplitude = 255;
            break;
        case QAudioFormat::SignedInt:
            m_maxAmplitude = 127;
            break;
        default:
            break;
        }
        break;
    case 16:
        switch (m_format.sampleType()) {
        case QAudioFormat::UnSignedInt:
            m_maxAmplitude = 65535;
            break;
        case QAudioFormat::SignedInt:
            m_maxAmplitude = 32767;
            break;
        default:
            break;
        }
        break;

    case 32:
        switch (m_format.sampleType()) {
        case QAudioFormat::UnSignedInt:
            m_maxAmplitude = 0xffffffff;
            break;
        case QAudioFormat::SignedInt:
            m_maxAmplitude = 0x7fffffff;
            break;
        case QAudioFormat::Float:
            m_maxAmplitude = 0x7fffffff; // Kind of
        default:
            break;
        }
        break;

    default:
        break;
    }
}

void AudioRecorder::setProcessValue(char *data, qint64 len)
{
    quint32 maxValue = 0;
    if (m_maxAmplitude) {
        Q_ASSERT(m_format.sampleSize() % 8 == 0);
        const int channelBytes = m_format.sampleSize() / 8;
        const int sampleBytes = m_format.channelCount() * channelBytes;
        Q_ASSERT(len % sampleBytes == 0);
        const int numSamples = len / sampleBytes;

        maxValue = 0;
        const unsigned char *ptr = reinterpret_cast<const unsigned char *>(data);

        for (int i = 0; i < numSamples; ++i) {
            for (int j = 0; j < m_format.channelCount(); ++j) {
                quint32 value = 0;

                if (m_format.sampleSize() == 8 && m_format.sampleType() == QAudioFormat::UnSignedInt) {
                    value = *reinterpret_cast<const quint8*>(ptr);
                } else if (m_format.sampleSize() == 8 && m_format.sampleType() == QAudioFormat::SignedInt) {
                    value = qAbs(*reinterpret_cast<const qint8*>(ptr));
                } else if (m_format.sampleSize() == 16 && m_format.sampleType() == QAudioFormat::UnSignedInt) {
                    if (m_format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qFromLittleEndian<quint16>(ptr);
                    else
                        value = qFromBigEndian<quint16>(ptr);
                } else if (m_format.sampleSize() == 16 && m_format.sampleType() == QAudioFormat::SignedInt) {
                    if (m_format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qAbs(qFromLittleEndian<qint16>(ptr));
                    else
                        value = qAbs(qFromBigEndian<qint16>(ptr));
                } else if (m_format.sampleSize() == 32 && m_format.sampleType() == QAudioFormat::UnSignedInt) {
                    if (m_format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qFromLittleEndian<quint32>(ptr);
                    else
                        value = qFromBigEndian<quint32>(ptr);
                } else if (m_format.sampleSize() == 32 && m_format.sampleType() == QAudioFormat::SignedInt) {
                    if (m_format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qAbs(qFromLittleEndian<qint32>(ptr));
                    else
                        value = qAbs(qFromBigEndian<qint32>(ptr));
                } else if (m_format.sampleSize() == 32 && m_format.sampleType() == QAudioFormat::Float) {
                    value = qAbs(*reinterpret_cast<const float*>(ptr) * 0x7fffffff); // assumes 0-1.0
                }

                maxValue = qMax(value, maxValue);
                ptr += channelBytes;
            }
        }
        maxValue = qMin(maxValue, m_maxAmplitude);
    }
    m_audioProcess->setValue(qreal(maxValue) / m_maxAmplitude);
}

void AudioRecorder::deviceChanged(int index)
{
    if (ui->DeviceComboBox->count() == 0)
        return;

    m_devInfo = ui->DeviceComboBox->itemData(index).value<QAudioDeviceInfo>();

    if (m_devInfo.isNull()) {
        ui->CodecComboBox->clear();
        ui->SampleRateComboBox->clear();
        ui->ChannelsComboBox->clear();
        ui->EndiannessComboBox->clear();
        ui->SampleSizeComboBox->clear();
        ui->SampleTypeComboBox->clear();

        ui->CodecComboBox->addItem(tr("None"), QVariant(QString()));
        ui->SampleRateComboBox->addItem(tr("None"), QVariant(QString()));
        ui->ChannelsComboBox->addItem(tr("None"), QVariant(QString()));
        ui->EndiannessComboBox->addItem(tr("None"), QVariant(QString()));
        ui->SampleSizeComboBox->addItem(tr("None"), QVariant(QString()));
        ui->SampleTypeComboBox->addItem(tr("None"), QVariant(QString()));

        QAudioFormat nullFormat;
        m_format = nullFormat;
        return;
    }

    //setup codec
    ui->CodecComboBox->clear();
    QStringList codecList = m_devInfo.supportedCodecs();
    for (int i = 0; i < codecList.size(); ++i)
        ui->CodecComboBox->addItem(QString("%1").arg(codecList.at(i)));
    m_format.setCodec(codecList.at(0));

    //setup sample rate
    ui->SampleRateComboBox->clear();
    QList<int> rateList = m_devInfo.supportedSampleRates();
    for (int i = 0; i < rateList.size(); ++i)
        ui->SampleRateComboBox->addItem(QString("%1").arg(rateList.at(i)));
    m_format.setSampleRate(rateList.at(0));

    //setup sample size
    ui->SampleSizeComboBox->clear();
    QList<int> sizeList = m_devInfo.supportedSampleSizes();
    for (int i = 0; i < sizeList.size() - 2; ++i)
        ui->SampleSizeComboBox->addItem(QString("%1").arg(sizeList.at(i)));
    if (sizeList.at(0) == 8) {
        m_format.setSampleSize(sizeList.at(1));
    }
    else
        m_format.setSampleSize(sizeList.at(0));

    //setup sample type
    ui->SampleTypeComboBox->clear();
    QList<QAudioFormat::SampleType> typeList = m_devInfo.supportedSampleTypes();
    for (int i = 0; i < typeList.size(); ++i)
        ui->SampleTypeComboBox->addItem(toString(typeList.at(i)));
    m_format.setSampleType(typeList.at(0));

    //setup channel
    ui->ChannelsComboBox->clear();
    QList<int> channelList = m_devInfo.supportedChannelCounts();
    for (int i = 0; i < rateList.size() - 2; ++i)
        ui->ChannelsComboBox->addItem(QString("%1").arg(channelList.at(i)));
    m_format.setChannelCount(channelList.at(0));

    //setup endianness
    ui->EndiannessComboBox->clear();
    QList<QAudioFormat::Endian> endianlList = m_devInfo.supportedByteOrders();
    for (int i = 0; i < endianlList.size(); ++i)
        ui->EndiannessComboBox->addItem(toString(endianlList.at(i)));
    m_format.setByteOrder(endianlList.at(0));

}

void AudioRecorder::sampleRateChanged(int idx)
{
    m_format.setSampleRate(ui->SampleRateComboBox->itemText(idx).toInt());
}

void AudioRecorder::channelChanged(int idx)
{
    m_format.setChannelCount(ui->ChannelsComboBox->itemText(idx).toInt());
}

void AudioRecorder::codecChanged(int idx)
{
    m_format.setCodec(ui->CodecComboBox->itemText(idx));
}

void AudioRecorder::sampleSizeChanged(int idx)
{
    if (ui->SampleSizeComboBox->itemText(idx).toInt() == 8) {
         m_format.setSampleSize(16);
    }
    else
        m_format.setSampleSize(ui->SampleSizeComboBox->itemText(idx).toInt());
}

void AudioRecorder::sampleTypeChanged(int idx)
{
    switch (ui->SampleTypeComboBox->itemData(idx).toInt()) {
        case QAudioFormat::SignedInt:
            m_format.setSampleType(QAudioFormat::SignedInt);
            break;
        case QAudioFormat::UnSignedInt:
            m_format.setSampleType(QAudioFormat::UnSignedInt);
            break;
        case QAudioFormat::Float:
            m_format.setSampleType(QAudioFormat::Float);
    }
}

void AudioRecorder::endianChanged(int idx)
{
    switch (ui->EndiannessComboBox->itemData(idx).toInt()) {
        case QAudioFormat::LittleEndian:
            m_format.setByteOrder(QAudioFormat::LittleEndian);
            break;
        case QAudioFormat::BigEndian:
            m_format.setByteOrder(QAudioFormat::BigEndian);
    }
}

void AudioRecorder::readData()
{
    //read data from input device and write data to output device
    QByteArray byteArray = m_inDev->readAll();
    m_outDev->write(byteArray);

    setProcessValue(byteArray.data(), byteArray.size());

    //write input device's data to PCM file
    writeDataToFile(m_tmpFile, byteArray);
}

void AudioRecorder::writeDataToFile(QFile *file, QByteArray data)
{
    //qDebug() << data;
    file->write(data);
}

//PCM translate to AAC
void AudioRecorder::PCMtoAAC(const char* fileName)
{
    unsigned long   nSampleRate = m_format.sampleRate();
    unsigned int    nChannels = m_format.channelCount();
    unsigned int    nPCMBitSize = m_format.sampleSize();
    if (m_format.sampleSize() == 8)
        nPCMBitSize = 16;
    unsigned long   nInputSamples = 0;
    unsigned long   nMaxOutputBytes = 0;

    FILE* fpIn = fopen("test.pcm", "rb");
    FILE* fpOut = fopen(fileName, "wb");

    if(fpIn == NULL || fpOut == NULL) {
        ui->statusBar->showMessage("Translate failed.");
        return;
    }

    //打开 faac编码器引擎
    faacEncHandle hEncoder = faacEncOpen(nSampleRate, nChannels, &nInputSamples, &nMaxOutputBytes);
    if (hEncoder == NULL) {
        qDebug() << "打开faac编码器引擎失败!";
        return;
    }

    // 分配内存信息
    int     nPCMBufferSize = nInputSamples * nPCMBitSize / 8;
    BYTE*   pbPCMBuffer = new BYTE[nPCMBufferSize];
    BYTE*   pbAACBuffer = new BYTE[nMaxOutputBytes];


    // 获取当前编码器信息
    faacEncConfigurationPtr pConfiguration = {0};
    pConfiguration = faacEncGetCurrentConfiguration(hEncoder);

    // 设置编码配置信息
    /*
        PCM Sample Input Format
        0   FAAC_INPUT_NULL         invalid, signifies a misconfigured config
        1   FAAC_INPUT_16BIT        native endian 16bit
        2   FAAC_INPUT_24BIT        native endian 24bit in 24 bits      (not implemented)
        3   FAAC_INPUT_32BIT        native endian 24bit in 32 bits      (DEFAULT)
        4   FAAC_INPUT_FLOAT        32bit floating point
    */
    switch(nPCMBitSize) {
    case 8:
        pConfiguration->inputFormat = FAAC_INPUT_16BIT;
        break;
    case 16:
        pConfiguration->inputFormat = FAAC_INPUT_16BIT;
        break;
    case 24:
        pConfiguration->inputFormat = FAAC_INPUT_24BIT;
        break;
    case 32:
        pConfiguration->inputFormat = FAAC_INPUT_32BIT;
        break;
    }

    // 0 = Raw; 1 = ADTS
    pConfiguration->outputFormat = 1;

    // AAC object types
    //#define MAIN 1
    //#define LOW  2
    //#define SSR  3
    //#define LTP  4
    pConfiguration->aacObjectType = LOW;
    pConfiguration->allowMidside = 0;
    pConfiguration->bitRate = 48000;
    pConfiguration->bandWidth = 32000;

    // 重置编码器的配置信息
    faacEncSetConfiguration(hEncoder, pConfiguration);

    size_t nRet = 0;

    ui->statusBar->showMessage(tr("Data Translating..."));
    int i = 0;

    while((nRet = fread(pbPCMBuffer, 1, nPCMBufferSize, fpIn)) > 0)
    {
        ++i;
        //printf("\b\b\b\b\b\b\b\b%-8d", ++i);

        nInputSamples = nRet / (nPCMBitSize / 8);

        // 编码
        nRet = faacEncEncode(hEncoder, (int*) pbPCMBuffer, nInputSamples, pbAACBuffer, nMaxOutputBytes);

        // 写入转码后的数据
        int i = fwrite(pbAACBuffer, 1, nRet, fpOut);
        qDebug() << i;
    }

    // 结束
    faacEncClose(hEncoder);
    fclose(fpOut);
    fclose(fpIn);

    delete[] pbAACBuffer;
    delete[] pbPCMBuffer;

    ui->statusBar->showMessage(tr("Done..."));
}

void AudioRecorder::on_recordButton_clicked()
{
    if (!m_format.isValid()) {
        ui->statusBar->showMessage(tr("Please choose options"));
        return;
    }

    //create a temple PCM file
    m_tmpFile = new QFile("test.pcm");
    m_tmpFile->open(QIODevice::ReadWrite | QIODevice::Truncate);

    initProcessValue();

    //audio start inputting and outputting
    m_audioInput = new QAudioInput(m_format);
    m_inDev = m_audioInput->start();

    m_audioOutput = new QAudioOutput(m_format);
    m_outDev = m_audioOutput->start();

    connect(m_inDev, SIGNAL(readyRead()), this, SLOT(readData()));

    ui->statusBar->showMessage(tr("Recording..."));
}

void AudioRecorder::on_pauseButton_clicked()
{
    if (!m_format.isValid())
        return;

    if (m_audioInput->state() == QAudio::SuspendedState) {
         m_audioInput->resume();
         ui->pauseButton->setText(tr("Pause"));
         ui->statusBar->showMessage(tr("Recording..."));
    }
    else if (m_audioInput->state() == QAudio::ActiveState) {
        m_audioInput->suspend();
        ui->pauseButton->setText(tr("Resume"));
        ui->statusBar->showMessage(tr("Paused."));
    }
    else if (m_audioInput->state() == QAudio::StoppedState) {
        m_audioInput->resume();
        ui->pauseButton->setText(tr("Pause"));
        ui->statusBar->showMessage(tr("Recording..."));
    }
}

void AudioRecorder::on_saveButton_clicked()
{
    if (m_audioInput && m_audioInput->state() == QAudio::ActiveState){
        ui->statusBar->showMessage(tr("Press Pause first."));
        return;
    }

    if (!m_tmpFile){
        ui->statusBar->showMessage(tr("Nothing to save."));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save"), "untitled.aac");

    PCMtoAAC(fileName.toStdString().data());

    //remove temple file
    m_tmpFile->remove();

    //disconnect audio device
    m_audioInput->disconnect(this);
    m_audioOutput->disconnect(this);

    m_audioProcess->setValue(0);
}

