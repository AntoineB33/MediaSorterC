#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <direct.h>
#include <cjson/cJSON.h>

#include <SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <libavutil/samplefmt.h>

#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#include <libswresample/swresample.h>

#include <libavutil/time.h>


#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_VOLUME 128  // Max SDL volume

#define MAX_LISTS 100
#define MAX_URLS_PER_LIST 100
#define MAX_URL_LEN 1024
#define MAX_FOUND_FILES 1000


// Global variable to store volume level
int volume_level = MAX_VOLUME / 2;  // Start at 50% volume
int audio_stream_index = -1; // Declare it globally

int64_t video_clock = 0;  // For video PTS
int64_t audio_clock = 0;  // For audio PTS


void audio_callback(void* userdata, Uint8* stream, int len) {
    // Instead of mixing the audio data, fill the stream with silence (zeros).
    SDL_memset(stream, 0, len);
    return;
    AVCodecContext* audio_ctx = (AVCodecContext*)userdata;
    static AVFrame* frame = NULL;
    static int audio_buffer_pos = 0;
    static int audio_buffer_size = 0;
    static uint8_t audio_buffer[(192000 * 3) / 2]; // Adjust buffer size if needed

    int len_to_copy;

    // Initialize the frame if needed
    if (!frame) {
        frame = av_frame_alloc();
    }

    while (len > 0) {
        if (audio_buffer_pos >= audio_buffer_size) {
            // Decode the next audio frame and fill the buffer
            int ret = avcodec_receive_frame(audio_ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                // No more frames or error, fill the remaining stream with silence
                SDL_memset(stream, 0, len);
                return;
            }
            else if (ret < 0) {
                fprintf(stderr, "Error while decoding audio.\n");
                return;
            }

            // Convert the frame data into the audio buffer format
            audio_buffer_size = frame->nb_samples * av_get_bytes_per_sample(frame->format);
            memcpy(audio_buffer, frame->data[0], audio_buffer_size);
            audio_buffer_pos = 0;
        }

        // Copy audio data from the buffer to the SDL stream
        len_to_copy = audio_buffer_size - audio_buffer_pos;
        if (len_to_copy > len) {
            len_to_copy = len;
        }

        SDL_MixAudioFormat(stream, audio_buffer + audio_buffer_pos, AUDIO_S16SYS, len_to_copy, SDL_MIX_MAXVOLUME);
        audio_buffer_pos += len_to_copy;
        stream += len_to_copy;
        len -= len_to_copy;
    }
}


// Function to play a video
int play_video(const char* filename) {
    // FFmpeg components
    AVFormatContext* pFormatCtx = NULL;
    AVCodecContext* pCodecCtx = NULL, * audioCodecCtx = NULL;
    AVCodec* pCodec = NULL, * audioCodec = NULL;
    AVFrame* pFrame = NULL;
    AVPacket* packet = NULL;
    struct SwsContext* sws_ctx = NULL;
    struct SwrContext* swr_ctx = NULL;

    int video_stream_index = -1, audio_stream_index = -1;

    // SDL components
    SDL_Window* screen = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Texture* texture = NULL;
    SDL_Event event;
    int is_paused = 0; // Pause state flag

    // Initialize FFmpeg and SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    // Open video file
    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open video file.\n");
        return -1;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information.\n");
        return -1;
    }

    // Find the first video and audio stream
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index == -1) {
            video_stream_index = i;
        }
        else if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index == -1) {
            audio_stream_index = i;
        }
    }
    if (video_stream_index == -1 || audio_stream_index == -1) {
        fprintf(stderr, "Could not find a video or audio stream.\n");
        return -1;
    }

    // Get codec context for the video stream
    AVCodecParameters* pCodecParams = pFormatCtx->streams[video_stream_index]->codecpar;
    pCodec = avcodec_find_decoder(pCodecParams->codec_id);
    if (pCodec == NULL) {
        fprintf(stderr, "Unsupported video codec.\n");
        return -1;
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pCodecParams);
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        fprintf(stderr, "Could not open video codec.\n");
        return -1;
    }

    // Get codec context for the audio stream
    AVCodecParameters* audioCodecParams = pFormatCtx->streams[audio_stream_index]->codecpar;
    audioCodec = avcodec_find_decoder(audioCodecParams->codec_id);
    if (audioCodec == NULL) {
        fprintf(stderr, "Unsupported audio codec.\n");
        return -1;
    }

    audioCodecCtx = avcodec_alloc_context3(audioCodec);
    avcodec_parameters_to_context(audioCodecCtx, audioCodecParams);
    if (avcodec_open2(audioCodecCtx, audioCodec, NULL) < 0) {
        fprintf(stderr, "Could not open audio codec.\n");
        return -1;
    }

    // Allocate frames
    pFrame = av_frame_alloc();
    packet = av_packet_alloc();











    // Set up SDL window with smaller initial dimensions
    screen = SDL_CreateWindow(
        "Video Player",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        640, 480,  // Initial window size
        SDL_WINDOW_SHOWN // No fullscreen flag initially
    );
    if (!screen) {
        fprintf(stderr, "SDL: could not create window - exiting\n");
        return -1;
    }

    renderer = SDL_CreateRenderer(screen, -1, 0);
    if (!renderer) {
        fprintf(stderr, "SDL: could not create renderer - %s\n", SDL_GetError());
        return -1;
    }
    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_YV12,
        SDL_TEXTUREACCESS_STREAMING,
        pCodecCtx->width, pCodecCtx->height
    );
    if (!texture) {
        fprintf(stderr, "SDL: could not create texture - %s\n", SDL_GetError());
        return -1;
    }



    // Setup for audio device with SDL_OpenAudioDevice
    SDL_AudioSpec audioSpec, obtainedSpec;
    SDL_AudioDeviceID dev;

    SDL_memset(&audioSpec, 0, sizeof(audioSpec));
    audioSpec.freq = audioCodecParams->sample_rate;
    audioSpec.format = AUDIO_S16SYS;
    audioSpec.channels = audioCodecParams->ch_layout.nb_channels;
    audioSpec.samples = SDL_AUDIO_BUFFER_SIZE;
    audioSpec.callback = audio_callback;
    audioSpec.userdata = audioCodecCtx;

    dev = SDL_OpenAudioDevice(NULL, 0, &audioSpec, &obtainedSpec, 0);
    if (dev == 0) {
        fprintf(stderr, "SDL: could not open audio device - %s\n", SDL_GetError());
        return -1;
    }

    // Start audio playback
    SDL_PauseAudioDevice(dev, 0);




    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == video_stream_index) {
            // Send the video packet to the decoder
            if (avcodec_send_packet(pCodecCtx, packet) >= 0) {
                // Receive the frame
                while (avcodec_receive_frame(pCodecCtx, pFrame) >= 0) {
                    // Get the presentation timestamp (PTS) for the video
                    int64_t pts = av_frame_get_best_effort_timestamp(pFrame);
                    video_clock = av_rescale_q(pts, pFormatCtx->streams[video_stream_index]->time_base, AV_TIME_BASE_Q);

                    // Wait until it's time to display the next frame
                    int64_t delay = video_clock - av_gettime();
                    if (delay > 0) {
                        SDL_Delay(delay / 1000);  // Convert delay to milliseconds
                    }

                    // Convert the image from its native format to YUV for SDL
                    sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                        pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
                        SWS_BILINEAR, NULL, NULL, NULL);

                    AVFrame* pFrameYUV = av_frame_alloc();
                    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
                    uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
                    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, buffer, AV_PIX_FMT_YUV420P,
                        pCodecCtx->width, pCodecCtx->height, 1);

                    // Convert the image
                    sws_scale(sws_ctx, (uint8_t const* const*)pFrame->data, pFrame->linesize, 0,
                        pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

                    // Update the SDL texture with the new YUV data
                    SDL_UpdateYUVTexture(texture, NULL,
                        pFrameYUV->data[0], pFrameYUV->linesize[0],
                        pFrameYUV->data[1], pFrameYUV->linesize[1],
                        pFrameYUV->data[2], pFrameYUV->linesize[2]);

                    // Render the texture
                    SDL_RenderClear(renderer);
                    SDL_RenderCopy(renderer, texture, NULL, NULL);
                    SDL_RenderPresent(renderer);

                    av_free(buffer);
                    av_frame_free(&pFrameYUV);
                }
            }
        }
        else if (packet->stream_index == audio_stream_index) {
            // Send the audio packet to the decoder
            if (avcodec_send_packet(audioCodecCtx, packet) >= 0) {
                // Handle audio clock (optional)
                audio_clock = av_gettime();  // Update audio clock based on the current time
            }
        }

        av_packet_unref(packet);

        // Handle SDL events for quitting, pausing, fullscreen toggle, etc.
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return 0; // Quit
            }
            else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    return 0;  // Exit on Escape
                case SDLK_SPACE:
                    is_paused = !is_paused;  // Toggle pause
                    break;
                case SDLK_f:  // Press 'f' to go full-screen
                    if (SDL_GetWindowFlags(screen) & SDL_WINDOW_FULLSCREEN) {
                        SDL_SetWindowFullscreen(screen, 0);  // Go back to windowed mode
                        SDL_SetWindowSize(screen, 640, 480);  // Optionally reset the window size
                    }
                    else {
                        SDL_SetWindowFullscreen(screen, SDL_WINDOW_FULLSCREEN);  // Enter fullscreen
                    }
                    break;

                case SDLK_w:  // Press 'w' to go back to windowed mode
                    SDL_SetWindowFullscreen(screen, 0);

                    // You can optionally resize the window back to its smaller size here
                    SDL_SetWindowSize(screen, 640, 480);
                    break;
                default:
                    break;
                }
            }
        }
    }


    // Clean up
    if (sws_ctx) {
        sws_freeContext(sws_ctx);
    }
    if (swr_ctx) {
        swr_free(&swr_ctx);
    }
    av_frame_free(&pFrame);
    av_packet_free(&packet);
    avcodec_free_context(&pCodecCtx);
    avcodec_free_context(&audioCodecCtx);
    avformat_close_input(&pFormatCtx);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    SDL_Quit();
    return 0;
}



int main(int argc, char* argv[]) {
    play_video("qj0sopzku6kc1.mp4");
    return 0;
}