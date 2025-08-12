#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file.h265>\n";
        return -1;
    }

    // 注册所有编解码器
    //av_register_all(); //ffmpeg 5.0已移除

    // 打开输入文件
    AVFormatContext* format_ctx = nullptr;
    if (avformat_open_input(&format_ctx, argv[1], nullptr, nullptr) != 0) {
        std::cerr << "Could not open input file\n";
        return -1;
    }

    // 查找流信息
    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        std::cerr << "Could not find stream information\n";
        return -1;
    }

    // 查找视频流
    int video_stream_index = -1;
    AVCodecParameters* codec_params = nullptr;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            codec_params = format_ctx->streams[i]->codecpar;
            break;
        }
    }

    if (video_stream_index == -1) {
        std::cerr << "Could not find video stream\n";
        return -1;
    }

    // 查找解码器
    auto codec = avcodec_find_decoder(codec_params->codec_id);
    if (!codec) {
        std::cerr << "Unsupported codec\n";
        return -1;
    }

    // 创建解码器上下文
    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codec_ctx, codec_params) < 0) {
        std::cerr << "Could not copy codec parameters to context\n";
        return -1;
    }

    // 打开解码器
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        std::cerr << "Could not open codec\n";
        return -1;
    }

    // 分配帧结构
    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();

    // 创建SWS上下文用于颜色空间转换
    SwsContext* sws_ctx = sws_getContext(
            codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
            codec_ctx->width, codec_ctx->height, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);

    // 读取并解码帧
    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == video_stream_index) {
            // 发送数据包给解码器
            int ret = avcodec_send_packet(codec_ctx, packet);
            if (ret < 0) {
                std::cerr << "Error sending packet for decoding\n";
                continue;
            }

            // 接收解码后的帧
            while (ret >= 0) {
                ret = avcodec_receive_frame(codec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    std::cerr << "Error during decoding\n";
                    break;
                }

                // 这里可以处理解码后的帧数据
                std::cout << "Decoded frame " << frame->pts << " ("
                          << frame->width << "x" << frame->height << ")\n";

                // 转换为RGB格式示例
                uint8_t* rgb_data[1] = { new uint8_t[frame->width * frame->height * 3] };
                int rgb_linesize[1] = { frame->width * 3 };

                sws_scale(sws_ctx, frame->data, frame->linesize, 0,
                          frame->height, rgb_data, rgb_linesize);

                // 使用rgb_data...

                delete[] rgb_data[0];
            }
        }
        av_packet_unref(packet);
    }

    // 清理资源
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    sws_freeContext(sws_ctx);

    return 0;
}