#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "iomanip"
#include <cstdint>
#include <vector>

#include <SDL2/SDL.h>

enum NALUnitType {
    TRAIL_N = 1, TRAIL_R = 0, IDR_W_RADL = 19, IDR_N_LP = 20, CRA_NUT = 21
};

enum SliceType { P_SLICE = 0, B_SLICE = 1, I_SLICE = 2 };

// 模拟从比特流中读取 Exp-Golomb 编码
uint32_t readExpGolomb(uint8_t* data, size_t& bitOffset) {
    uint32_t leadingZeros = 0;
    while ((data[bitOffset / 8] & (1 << (7 - (bitOffset % 8)))) == 0) {
        leadingZeros++;
        bitOffset++;
    }
    bitOffset++; // 跳过终止位 '1'
    uint32_t value = (1 << leadingZeros) - 1;
    for (uint32_t i = 0; i < leadingZeros; i++) {
        value |= ((data[bitOffset / 8] >> (7 - (bitOffset % 8))) & 1) << (leadingZeros - 1 - i);
        bitOffset++;
    }
    return value;
}

// 解析 Slice Header 中的 slice_type
uint8_t parseSliceType(uint8_t* nalUnit) {
    size_t bitOffset = 16; // 跳过 2 字节 NAL 头
    // 1. 跳过 first_slice_segment_in_pic_flag（1 bit）
    bitOffset++;
    // 2. 跳过 slice_pic_parameter_set_id（Exp-Golomb）
    readExpGolomb(nalUnit, bitOffset);
    // 3. 读取 slice_type（Exp-Golomb）
    return readExpGolomb(nalUnit, bitOffset); // 返回值 0=P, 1=B, 2=I
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file.h265>\n";
        return -1;
    }

    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
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
    AVCodecContext* pCodecCtx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(pCodecCtx, codec_params) < 0) {
        std::cerr << "Could not copy codec parameters to context\n";
        return -1;
    }

    // 打开解码器
    if (avcodec_open2(pCodecCtx, codec, nullptr) < 0) {
        std::cerr << "Could not open codec\n";
        return -1;
    }

    // 创建SDL窗口
    SDL_Window *screen = SDL_CreateWindow("H.265 Player",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          pCodecCtx->width,
                                          pCodecCtx->height,
                                          SDL_WINDOW_RESIZABLE);

    SDL_Renderer *renderer = SDL_CreateRenderer(screen, -1, 0);
    SDL_Texture *texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_YV12,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             pCodecCtx->width,
                                             pCodecCtx->height);

    // 分配帧结构
    AVFrame* pFrameYUV = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();

    // 创建SWS上下文用于颜色空间转换
    SwsContext* sws_ctx = sws_getContext(
            pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
            pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);


    SDL_Event event;
    // 读取并解码帧
    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == video_stream_index) {
            std::cout<<"===>"
                         <<static_cast<int>(packet->data[0])
                    <<"_"<<static_cast<int>(packet->data[1])
                    <<"_"<<static_cast<int>(packet->data[2])
                    <<"_"<<static_cast<int>(packet->data[3])
                    <<"_"<<static_cast<int>(packet->data[4])
                    <<"_"<<static_cast<int>(packet->data[5])
                    <<"_"<<static_cast<int>(packet->data[6])
                    <<"_"<<static_cast<int>(packet->data[7])
                    <<"_"<<static_cast<int>(packet->data[8])
                    <<"_"<<static_cast<int>(packet->data[9])
                    <<std::endl;
            uint8_t nal_type = (packet->data[4] >> 1) & 0x3F;
            // 如果是切片数据，解析slice_type
            if (nal_type == TRAIL_N || nal_type == TRAIL_R ||
                nal_type == IDR_W_RADL || nal_type == IDR_N_LP) {
                uint8_t sliceType=parseSliceType((packet->data+4));
                std::cout<<"===>帧类型:(0=P, 1=B, 2=I) -> "<<static_cast<int>(sliceType)<<std::endl;

            }

            // 发送数据包给解码器
            int ret = avcodec_send_packet(pCodecCtx, packet);
            if (ret < 0) {
                std::cerr << "Error sending packet for decoding\n";
                continue;
            }

            // 接收解码后的帧
            while (ret >= 0) {
                ret = avcodec_receive_frame(pCodecCtx, pFrameYUV);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    std::cerr << "Error during decoding\n";
                    break;
                }

                switch (static_cast<int>(pFrameYUV->pict_type)) {
                    case 1:
                        std::cout<<"===>AV_PICTURE_TYPE_I : "<<AV_PICTURE_TYPE_I;
                        break;
                    case 2:
                        std::cout<<"===>AV_PICTURE_TYPE_P : "<<AV_PICTURE_TYPE_P;
                        break;
                    case 3:
                        std::cout<<"===>AV_PICTURE_TYPE_B : "<<AV_PICTURE_TYPE_B;
                        break;
                }
                // 这里可以处理解码后的帧数据
                std::cout << "Decoded frame " << pFrameYUV->pts << " ("
                          << pFrameYUV->width << "x" << pFrameYUV->height << ")\n";

                // 转换为RGB格式示例
                uint8_t* rgb_data[1] = { new uint8_t[pFrameYUV->width * pFrameYUV->height * 3] };
                int rgb_linesize[1] = { pFrameYUV->width * 3 };

                sws_scale(sws_ctx, pFrameYUV->data, pFrameYUV->linesize, 0,
                          pFrameYUV->height, rgb_data, rgb_linesize);

                // 使用rgb_data...
                // 更新SDL纹理
                SDL_UpdateYUVTexture(texture, NULL,
                                     pFrameYUV->data[0], pFrameYUV->linesize[0],
                                     pFrameYUV->data[1], pFrameYUV->linesize[1],
                                     pFrameYUV->data[2], pFrameYUV->linesize[2]);

                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);

                // 控制帧率 for local 265 file open it
                //SDL_Delay(40); // 约25fps

                delete[] rgb_data[0];
            }
        }
        av_packet_unref(packet);

        // 处理事件
        SDL_PollEvent(&event);
        switch (event.type) {
            case SDL_QUIT:
                goto end;
            default:
                break;
        }
    }

end:
    // 清理资源
    av_frame_free(&pFrameYUV);
    av_packet_free(&packet);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&format_ctx);
    sws_freeContext(sws_ctx);
    SDL_Quit();

    return 0;
}