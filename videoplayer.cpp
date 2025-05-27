#include "videoplayer.h"

VideoPlayer::VideoPlayer() {}

void VideoPlayer::startVideoPlay(QString videoPath)
{
    m_Path = videoPath;
    AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;                                    // ffmpeg支持的硬件解码器
    QStringList strTypes;
    while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)       // 遍历支持的设备类型。
    {
        m_HWDeviceTypes.append(type);
        const char* ctype = av_hwdevice_get_type_name(type);                        // 获取AVHWDeviceType的字符串名称。
        if(ctype)
        {
            strTypes.append(QString(ctype));
        }
    }
    this->start();
}

void VideoPlayer::run()
{
    std::string stdVideoPath = m_Path.toStdString();
    char *videoPathChar = const_cast<char *>(stdVideoPath.c_str());
    AVFormatContext *avFormatContext = nullptr;
    if (avformat_open_input(&avFormatContext, videoPathChar, nullptr, nullptr) < 0) {
        qDebug() << "OPEN VIDEO FAIL" << m_Path;
        return;
    }
    if(avformat_find_stream_info(avFormatContext,nullptr) < 0){
        qDebug() << "VIDEO NOT FIND STREAM" << m_Path;
        return;
    }
    int videoStream = -1;
    for (int i = 0; i < (int)avFormatContext->nb_streams; ++i) {
        if(avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            videoStream = i;
            break;
        }
    }
    if(videoStream < 0){
        qDebug() << "NOT FIND VIDEO STREAM";
        return;
    }
    AVCodecParameters* coderParameters = avFormatContext->streams[videoStream]->codecpar;
    const AVCodec* videoAVCodec = avcodec_find_decoder(coderParameters->codec_id);
    if(videoAVCodec == nullptr){
        qDebug() << "NOT FIND AVCODEC";
        return;
    }
    videoContext = avcodec_alloc_context3(videoAVCodec);

    // 把参数填充到解码器上下文中
    if (avcodec_parameters_to_context(videoContext,coderParameters) < 0) {
        return;
    }
    // 解码线程
    videoContext->thread_count = 8;
    // 初始化硬件编码器
    // initHWDecoder(videoAVCodec);
    // 解码器填充到解码器上下文中
    if(avcodec_open2(videoContext, videoAVCodec, nullptr) < 0){
        return;
    }
    // 图像转换
    static struct SwsContext *img_convert_ctx;
    img_convert_ctx = sws_getContext(videoContext->width,
                                     videoContext->height,
                                     videoContext->pix_fmt ,
                                     videoContext->width,
                                     videoContext->height,
                                     AV_PIX_FMT_RGB32,
                                     SWS_BICUBIC,
                                     NULL,
                                     NULL,
                                     NULL);

    // 申请空间
    m_Packet = av_packet_alloc();
    m_Frame = av_frame_alloc();
    AVFrame* frameRGB = av_frame_alloc();
    int size = av_image_get_buffer_size(AV_PIX_FMT_RGB32,videoContext->width,videoContext->height,4);
    m_Buffer = (uint8_t *) av_malloc(size * sizeof(uint8_t));
    av_image_fill_arrays(frameRGB->data,
                         frameRGB->linesize,
                         m_Buffer,
                         AV_PIX_FMT_RGB32,
                         videoContext->width,
                         videoContext->height,
                         1);

    // 获取视频流的时间基
    AVRational time_base = avFormatContext->streams[videoStream]->time_base;

    // 获取帧率 (FPS)
    AVRational frame_rate = av_guess_frame_rate(avFormatContext, avFormatContext->streams[videoStream], NULL);
    double fps = av_q2d(frame_rate);
    int frame_delay = (fps > 0) ? (1000 / fps) : 40; // 默认40ms(25fps)
    AVRational display_time_base = {1, AV_TIME_BASE};
    int64_t start_time = av_gettime(); // 获取开始时间
    int64_t pts_time = 0;
    int64_t last_pts = 0;

    // while(1){
    //     if(av_read_frame(avFormatContext,m_Packet) < 0){
    //         avcodec_send_packet(videoContext,m_Packet);
    //         break;
    //     }

    //     if(m_Packet->stream_index == videoStream){
    //         m_Packet->pts = qRound64(m_Packet->pts * (100000 * rationalToDouble(&avFormatContext->streams[videoStream]->time_base)));
    //         m_Packet->dts = qRound64(m_Packet->dts * (100000 * rationalToDouble(&avFormatContext->streams[videoStream]->time_base)));
    //         if(avcodec_send_packet(videoContext,m_Packet) < 0){
    //             qDebug() << "SEND PACKET ERROR";
    //             break;
    //         }
    //         while(avcodec_receive_frame(videoContext,m_Frame) == 0){
    //             sws_scale(img_convert_ctx,
    //                       (const uint8_t * const *)m_Frame->data,
    //                       m_Frame->linesize,
    //                       0,
    //                       videoContext->height,
    //                       frameRGB->data,
    //                       frameRGB->linesize);
    //             QImage tmpImg((uint8_t *)m_Buffer,
    //                           videoContext->width,
    //                           videoContext->height,
    //                           QImage::Format_RGB32);
    //             QImage image = tmpImg.copy();
    //             emit sig_GetOneFrame(image);
    //         }

    //         av_packet_unref(m_Packet);
    //     }
    // }

    while(1) {
        if(av_read_frame(avFormatContext, m_Packet) < 0) {
            avcodec_send_packet(videoContext, m_Packet);
            break;
        }
        if(m_Packet->stream_index == videoStream) {
            if(avcodec_send_packet(videoContext, m_Packet) < 0) {
                qDebug() << "SEND PACKET ERROR";
                break;
            }

            while(avcodec_receive_frame(videoContext, m_Frame) == 0) {
                // 转换帧数据...
                sws_scale(img_convert_ctx,
                          (const uint8_t * const *)m_Frame->data,
                          m_Frame->linesize,
                          0,
                          videoContext->height,
                          frameRGB->data,
                          frameRGB->linesize);

                QImage tmpImg((uint8_t *)m_Buffer,
                              videoContext->width,
                              videoContext->height,
                              QImage::Format_RGB32);
                QImage image = tmpImg.copy();

                // 计算显示时间
                if(m_Frame->pts != AV_NOPTS_VALUE) {
                    pts_time = av_rescale_q(m_Frame->pts, time_base, display_time_base);
                } else {
                    pts_time = 0;
                }

                // 计算应该显示的时间
                int64_t now = av_gettime() - start_time;
                int64_t delay = pts_time - last_pts;

                // 根据帧持续时间和当前时间计算是否需要等待
                if(delay > 0 && delay < 1000000) { // 限制最大延迟为1秒
                    // 如果显示的太快，等待一段时间
                    if((pts_time - now) > 0) {
                        av_usleep(pts_time - now);
                    }
                } else {
                    // 如果时间戳无效或异常，使用默认帧率
                    av_usleep(frame_delay * 1000);
                }

                emit sig_GetOneFrame(image);
                last_pts = pts_time;
            }
            av_packet_unref(m_Packet);
        }
    }

    av_free(m_Buffer);
    av_free(frameRGB);
    avcodec_free_context(&videoContext);
    avformat_close_input(&avFormatContext);
}

qreal VideoPlayer::rationalToDouble(AVRational* rational)
{
    qreal frameRate = (rational->den == 0) ? 0 : (qreal(rational->num) / rational->den);
    return frameRate;
}

void VideoPlayer::initHWDecoder(const AVCodec *codec)
{
    for (int i = 0;; ++i) {
        const AVCodecHWConfig* config = avcodec_get_hw_config(codec,i);
        if(!config){
            qDebug() << "CANT OPEN HW CONDEC";
            return;
        }
        AVBufferRef* hw_device_ctx = nullptr;
        if(config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX){
            foreach (auto i, m_HWDeviceTypes) {
                if(config->device_type == AVHWDeviceType(i)){
                    if(av_hwdevice_ctx_create(&hw_device_ctx, config->device_type, nullptr, nullptr, 0) < 0){
                        return;
                    }
                    videoContext->hw_device_ctx = av_buffer_ref(hw_device_ctx);
                    return;
                }
            }
        }
    }
}
