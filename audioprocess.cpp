#include "audioprocess.h"
#include <QPainter>


AudioProcess::AudioProcess(QWidget *parent)
    : QWidget(parent)
    , m_value(0.0)
{
    setMinimumHeight(15);
    setMaximumHeight(15);
}

void AudioProcess::setValue(qreal value)
{
    if (m_value != value) {
        m_value = value;
        update();
    }
}

void AudioProcess::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setPen(Qt::black);
    painter.drawRect(QRect(painter.viewport().left()+10,
                           painter.viewport().top()+10,
                           painter.viewport().right()-20,
                           painter.viewport().bottom()-20));
    if (m_value == 0.0)
        return;

    int pos = ((painter.viewport().right() - 20) - (painter.viewport().left() + 11)) * m_value;

    painter.fillRect(painter.viewport().left() + 11,
                     painter.viewport().top() + 10,
                     pos,
                     painter.viewport().height() - 21,
                     Qt::red);
}
