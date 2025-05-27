#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QThread>
#include <QImage>
#include <QDebug>

extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

class VideoPlayer : public QThread
{
    Q_OBJECT
public:
    VideoPlayer();
    void startVideoPlay(QString videoPath);
signals:
    void sig_GetOneFrame(QImage image);
private:
    qreal rationalToDouble(AVRational* rational);
    void initHWDecoder(const AVCodec* codec);
protected:
    void run();
private:
    QString m_Path;
    AVPacket* m_Packet;
    AVFrame* m_Frame;
    uint8_t* m_Buffer;
    AVCodecContext* videoContext = nullptr;
    QList<int> m_HWDeviceTypes;                   // 保存当前环境支持的硬件解码器
};

#endif // VIDEOPLAYER_H
