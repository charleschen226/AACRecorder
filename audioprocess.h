#ifndef AUDIOPROCESS_H
#define AUDIOPROCESS_H

#include <QWidget>

class AudioProcess: public QWidget
{
    Q_OBJECT
public:
    AudioProcess(QWidget *parent = 0);

public:
    void setValue(qreal value);
    void paintEvent(QPaintEvent *event);

private:
    qreal m_value;
};

#endif // AUDIOPROCESS_H
