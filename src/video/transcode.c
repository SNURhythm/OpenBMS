/*
 * Copyright (c) 2010 Nicolas George
 * Copyright (c) 2011 Stefano Sabatini
 * Copyright (c) 2014 Andrey Utkin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file demuxing, decoding, filtering, encoding and muxing API usage example
 * @example transcode.c
 *
 * Convert input to output file, applying some hard-coded filter-graph on both
 * audio and video streams.
 */
// ReSharper disable CppZeroConstantCanBeReplacedWithNullptr
#include "transcode.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <stdbool.h>

typedef struct FilteringContext {
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;

    AVPacket *enc_pkt;
    AVFrame *filtered_frame;
} FilteringContext;

typedef struct StreamContext {
    AVCodecContext *dec_ctx;
    AVCodecContext *enc_ctx;

    AVFrame *dec_frame;
} StreamContext;

typedef struct TranscodeContext{
    StreamContext *stream_ctx;
    FilteringContext *filter_ctx;
    AVFormatContext *ifmt_ctx;
    AVFormatContext *ofmt_ctx;
    
} TranscodeContext;
int open_input_file(const char *filename, TranscodeContext* ctx)
{
    int ret;
    unsigned int i;

    ctx->ifmt_ctx = NULL;
    if ((ret = avformat_open_input(&ctx->ifmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(ctx->ifmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    ctx->stream_ctx = (StreamContext*)av_calloc(ctx->ifmt_ctx->nb_streams, sizeof(*(ctx->stream_ctx)));
    if (!ctx->stream_ctx)
        return AVERROR(ENOMEM);

    for (i = 0; i < ctx->ifmt_ctx->nb_streams; i++) {
        AVStream *stream = ctx->ifmt_ctx->streams[i];
        const AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
        AVCodecContext *codec_ctx;
        if (!dec) {
            av_log(NULL, AV_LOG_ERROR, "Failed to find decoder for stream #%u\n", i);
            return AVERROR_DECODER_NOT_FOUND;
        }
        av_log(NULL, AV_LOG_INFO, "Found decoder for stream #%u: %s\n", i, dec->name);
        codec_ctx = avcodec_alloc_context3(dec);
        if (!codec_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate the decoder context for stream #%u\n", i);
            return AVERROR(ENOMEM);
        }
        ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy decoder parameters to input decoder context "
                   "for stream #%u\n", i);
            return ret;
        }

        /* Inform the decoder about the timebase for the packet timestamps.
         * This is highly recommended, but not mandatory. */
        codec_ctx->pkt_timebase = stream->time_base;

        /* Reencode video & audio and remux subtitles etc. */
        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
                || codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
                codec_ctx->framerate = av_guess_frame_rate(ctx->ifmt_ctx, stream, NULL);
            /* Open decoder */
            ret = avcodec_open2(codec_ctx, dec, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
                return ret;
            }
        }
        ctx->stream_ctx[i].dec_ctx = codec_ctx;

        ctx->stream_ctx[i].dec_frame = av_frame_alloc();
        if (!ctx->stream_ctx[i].dec_frame)
            return AVERROR(ENOMEM);
    }

    av_dump_format(ctx->ifmt_ctx, 0, filename, 0);
    return 0;
}

int open_output_file(const char *filename, TranscodeContext* ctx)
{
    AVStream *out_stream;
    AVStream *in_stream;
    AVCodecContext *dec_ctx, *enc_ctx;
    const AVCodec *encoder;
    int ret;
    unsigned int i;

    ctx->ofmt_ctx = NULL;
    // h264
    const AVOutputFormat *ofmt = av_guess_format(NULL, filename, NULL);
    avformat_alloc_output_context2(&(ctx->ofmt_ctx), ofmt, NULL, filename);
    if (!ctx->ofmt_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        return AVERROR_UNKNOWN;
    }


    for (i = 0; i < ctx->ifmt_ctx->nb_streams; i++) {
        out_stream = avformat_new_stream(ctx->ofmt_ctx, NULL);
        if (!out_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
            return AVERROR_UNKNOWN;
        }

        in_stream = ctx->ifmt_ctx->streams[i];
        dec_ctx = ctx->stream_ctx[i].dec_ctx;

        if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
                || dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            /* use h264 encoder */
            if(dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                // libx264
                encoder = avcodec_find_encoder_by_name("libx264");
                // if not found, fallback
                if(!encoder) encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
            } else
            {
                encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
            }
            if (!encoder) {
                av_log(NULL, AV_LOG_FATAL, "Necessary encoder not found\n");
                return AVERROR_INVALIDDATA;
            }
            av_log(NULL, AV_LOG_INFO, "Found encoder for stream #%u: %s\n", i, encoder->name);
            enc_ctx = avcodec_alloc_context3(encoder);
            if (!enc_ctx) {
                av_log(NULL, AV_LOG_FATAL, "Failed to allocate the encoder context\n");
                return AVERROR(ENOMEM);
            }

            /* In this example, we transcode to same properties (picture size,
             * sample rate etc.). These properties can be changed for output
             * streams easily using filters */
            if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
                enc_ctx->height = dec_ctx->height;
                enc_ctx->width = dec_ctx->width;
                enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
                // print all supported pixel formats in encoder
                for (int ipf = 0; encoder->pix_fmts[ipf] != AV_PIX_FMT_NONE; ipf++) {
                    av_log(NULL, AV_LOG_INFO, "Encoder pix fmt: %d\n", encoder->pix_fmts[ipf]);
                }
                // /* take first format from list of supported formats */
                // if (encoder->pix_fmts)
                //     enc_ctx->pix_fmt = encoder->pix_fmts[0];
                // else
                    enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
                av_log(NULL, AV_LOG_INFO, "Dec pix fmt: %d, Enc pix fmt: %d\n", dec_ctx->pix_fmt, enc_ctx->pix_fmt);
                /* video time_base can be set to whatever is handy and supported by encoder */
                enc_ctx->time_base = av_inv_q(dec_ctx->framerate);
            } else {
                // log audio codec
                av_log(NULL, AV_LOG_INFO, "Audio Codec: %d\n", dec_ctx->codec_id);
                enc_ctx->sample_rate = dec_ctx->sample_rate;
                // stereo
                av_channel_layout_copy(&(enc_ctx->ch_layout), &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO);
                /* take first format from list of supported formats */
                if(encoder->sample_fmts) enc_ctx->sample_fmt = encoder->sample_fmts[0];
                enc_ctx->time_base = (AVRational){1, enc_ctx->sample_rate};
            }

            if (ctx->ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            /* Third parameter can be used to pass settings to encoder */
            AVDictionary *opts = NULL;
            av_dict_set(&opts, "preset", "ultrafast", 0);
            

            ret = avcodec_open2(enc_ctx, encoder, &opts);
            av_dict_free(&opts);

            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder for stream #%u\n", i);
                return ret;
            }
          
            ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream #%u\n", i);
                return ret;
            }

            out_stream->time_base = enc_ctx->time_base;
            ctx->stream_ctx[i].enc_ctx = enc_ctx;
        } else if (dec_ctx->codec_type == AVMEDIA_TYPE_UNKNOWN) {
            av_log(NULL, AV_LOG_FATAL, "Elementary stream #%d is of unknown type, cannot proceed\n", i);
            return AVERROR_INVALIDDATA;
        } else {
            /* if this stream must be remuxed */
            ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Copying parameters for stream #%u failed\n", i);
                return ret;
            }
            out_stream->time_base = in_stream->time_base;
        }

    }
    av_dump_format(ctx->ofmt_ctx, 0, filename, 1);

    if (!(ctx->ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&(ctx->ofmt_ctx->pb), filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename);
            return ret;
        }
    }

    /* init muxer, write output file header */
    // fast start
    AVDictionary *opts2 = NULL;
    ret = av_dict_set(&opts2, "movflags", "+faststart", 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set movflags\n");
        return ret;
    }
    
    ret = avformat_write_header(ctx->ofmt_ctx, &opts2);
    av_dict_free(&opts2);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
        return ret;
    }

    return 0;
}

int init_filter(FilteringContext* fctx, AVCodecContext *dec_ctx,
        AVCodecContext *enc_ctx, const char *filter_spec)
{
    char args[512];
    int ret = 0;
    const AVFilter *buffersrc = NULL;
    const AVFilter *buffersink = NULL;
    AVFilterContext *buffersrc_ctx = NULL;
    AVFilterContext *buffersink_ctx = NULL;
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    AVFilterGraph *filter_graph = avfilter_graph_alloc();

    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        buffersrc = avfilter_get_by_name("buffer");
        buffersink = avfilter_get_by_name("buffersink");
        if (!buffersrc || !buffersink) {
            av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        av_log(NULL, AV_LOG_INFO, "filtering source or sink element found\n");

        snprintf(args, sizeof(args),
                "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
                dec_ctx->pkt_timebase.num, dec_ctx->pkt_timebase.den,
                dec_ctx->sample_aspect_ratio.num,
                dec_ctx->sample_aspect_ratio.den);

        ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                args, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
            goto end;
        }
        
        ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                NULL, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
            goto end;
        }

        ret = av_opt_set_bin(buffersink_ctx, "pix_fmts",
                (uint8_t*)&enc_ctx->pix_fmt, sizeof(enc_ctx->pix_fmt),
                AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
            goto end;
        }
        av_log(NULL, AV_LOG_INFO, "Set output pixel format success\n");
    } else if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
        char buf[64];
        buffersrc = avfilter_get_by_name("abuffer");
        buffersink = avfilter_get_by_name("abuffersink");
        if (!buffersrc || !buffersink) {
            av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        av_log(NULL, AV_LOG_INFO, "filtering source or sink element found\n");

        if (dec_ctx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC)
            av_channel_layout_default(&dec_ctx->ch_layout, dec_ctx->ch_layout.nb_channels);
        av_channel_layout_describe(&dec_ctx->ch_layout, buf, sizeof(buf));
        snprintf(args, sizeof(args),
                "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s",
                dec_ctx->pkt_timebase.num, dec_ctx->pkt_timebase.den, dec_ctx->sample_rate,
                av_get_sample_fmt_name(dec_ctx->sample_fmt),
                buf);
        ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                args, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
            goto end;
        }
        av_log(NULL, AV_LOG_INFO, "Create audio buffer source success\n");

        ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                NULL, NULL, filter_graph);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
            goto end;
        }
        av_log(NULL, AV_LOG_INFO, "Create audio buffer sink success\n");

        ret = av_opt_set_bin(buffersink_ctx, "sample_fmts",
                (uint8_t*)&enc_ctx->sample_fmt, sizeof(enc_ctx->sample_fmt),
                AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
            goto end;
        }
        av_log(NULL, AV_LOG_INFO, "Set output sample format success\n");

        av_channel_layout_describe(&enc_ctx->ch_layout, buf, sizeof(buf));
        ret = av_opt_set(buffersink_ctx, "ch_layouts",
                         buf, AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
            goto end;
        }
        av_log(NULL, AV_LOG_INFO, "Set output channel layout success\n");

        ret = av_opt_set_bin(buffersink_ctx, "sample_rates",
                (uint8_t*)&enc_ctx->sample_rate, sizeof(enc_ctx->sample_rate),
                AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
            goto end;
        }
        av_log(NULL, AV_LOG_INFO, "Set output sample rate success\n");
    } else {
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    /* Endpoints for the filter graph. */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    if (!outputs->name || !inputs->name) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    av_log(NULL, AV_LOG_INFO, "Set filter graph endpoints success\n");

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_spec,
                    &inputs, &outputs, NULL)) < 0)
        goto end;
    av_log(NULL, AV_LOG_INFO, "Parse filter graph success\n");

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        goto end;
    av_log(NULL, AV_LOG_INFO, "Config filter graph success\n");

    /* Fill FilteringContext */
    fctx->buffersrc_ctx = buffersrc_ctx;
    fctx->buffersink_ctx = buffersink_ctx;
    fctx->filter_graph = filter_graph;

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

int init_filters(TranscodeContext* ctx)
{
    const char *filter_spec;
    unsigned int i;
    int ret;
    ctx->filter_ctx = (FilteringContext*)av_malloc_array(ctx->ifmt_ctx->nb_streams, sizeof(*(ctx->filter_ctx)));
    if (!ctx->filter_ctx)
        return AVERROR(ENOMEM);

    for (i = 0; i < ctx->ifmt_ctx->nb_streams; i++) {
        ctx->filter_ctx[i].buffersrc_ctx  = NULL;
        ctx->filter_ctx[i].buffersink_ctx = NULL;
        ctx->filter_ctx[i].filter_graph   = NULL;
        if (!(ctx->ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO
                || ctx->ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO))
            continue;


        if (ctx->ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            filter_spec = "format=pix_fmts=yuv420p";
        else
            filter_spec = "anull"; /* passthrough (dummy) filter for audio */
        ret = init_filter(&(ctx->filter_ctx[i]), ctx->stream_ctx[i].dec_ctx,
                ctx->stream_ctx[i].enc_ctx, filter_spec);
        if (ret)
            return ret;
        av_log(NULL, AV_LOG_INFO, "Init filter for stream #%u success\n", i);

        ctx->filter_ctx[i].enc_pkt = av_packet_alloc();
        if (!ctx->filter_ctx[i].enc_pkt)
            return AVERROR(ENOMEM);
        av_log(NULL, AV_LOG_INFO, "Alloc packet for stream #%u success\n", i);

        ctx->filter_ctx[i].filtered_frame = av_frame_alloc();
        if (!ctx->filter_ctx[i].filtered_frame)
            return AVERROR(ENOMEM);
        av_log(NULL, AV_LOG_INFO, "Alloc frame for stream #%u success\n", i);
    }
    return 0;
}

int encode_write_frame(unsigned int stream_index, int flush, TranscodeContext* ctx)
{
    StreamContext *stream = &(ctx->stream_ctx[stream_index]);
    FilteringContext *filter = &(ctx->filter_ctx[stream_index]);
    AVFrame *filt_frame = flush ? NULL : filter->filtered_frame;
    AVPacket *enc_pkt = filter->enc_pkt;
    int ret;

    // av_log(NULL, AV_LOG_INFO, "Encoding frame\n");
    /* encode filtered frame */
    av_packet_unref(enc_pkt);

    if (filt_frame && filt_frame->pts != AV_NOPTS_VALUE)
        filt_frame->pts = av_rescale_q(filt_frame->pts, filt_frame->time_base,
                                       stream->enc_ctx->time_base);

    ret = avcodec_send_frame(stream->enc_ctx, filt_frame);

    if (ret < 0)
        return ret;

    while (ret >= 0) {
        ret = avcodec_receive_packet(stream->enc_ctx, enc_pkt);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return 0;

        /* prepare packet for muxing */
        enc_pkt->stream_index = stream_index;
        av_packet_rescale_ts(enc_pkt,
                             stream->enc_ctx->time_base,
                             ctx->ofmt_ctx->streams[stream_index]->time_base);

        av_log(NULL, AV_LOG_DEBUG, "Muxing frame\n");
        /* mux encoded frame */
        ret = av_interleaved_write_frame(ctx->ofmt_ctx, enc_pkt);
    }

    return ret;
}

int filter_encode_write_frame(AVFrame *frame, unsigned int stream_index, TranscodeContext* ctx)
{
    FilteringContext *filter = &ctx->filter_ctx[stream_index];
    int ret;

    // av_log(NULL, AV_LOG_INFO, "Pushing decoded frame to filters\n");
    /* push the decoded frame into the filtergraph */
    ret = av_buffersrc_add_frame_flags(filter->buffersrc_ctx,
            frame, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
        return ret;
    }

    /* pull filtered frames from the filtergraph */
    while (1) {
        // av_log(NULL, AV_LOG_INFO, "Pulling filtered frame from filters\n");
        ret = av_buffersink_get_frame(filter->buffersink_ctx,
                                      filter->filtered_frame);
        if (ret < 0) {
            /* if no more frames for output - returns AVERROR(EAGAIN)
             * if flushed and no more frames for output - returns AVERROR_EOF
             * rewrite retcode to 0 to show it as normal procedure completion
             */
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                ret = 0;
            break;
        }

        filter->filtered_frame->time_base = av_buffersink_get_time_base(filter->buffersink_ctx);;
        filter->filtered_frame->pict_type = AV_PICTURE_TYPE_NONE;
        ret = encode_write_frame(stream_index, 0, ctx);
        av_frame_unref(filter->filtered_frame);
        if (ret < 0)
            break;
    }

    return ret;
}

int flush_encoder(unsigned int stream_index, TranscodeContext* ctx)
{
    if (!(ctx->stream_ctx[stream_index].enc_ctx->codec->capabilities &
                AV_CODEC_CAP_DELAY))
        return 0;

    av_log(NULL, AV_LOG_INFO, "Flushing stream #%u encoder\n", stream_index);
    return encode_write_frame(stream_index, 1, ctx);
}


int transcode(const char* inPath, const char* outPath, void* isCancelled)
{
    bool* isCancelledBool = (bool*)isCancelled;
    int ret;
    AVPacket *packet = NULL;
    unsigned int stream_index;
    unsigned int i;
    TranscodeContext* ctx = (TranscodeContext*)av_malloc(sizeof(TranscodeContext));
    ctx->stream_ctx = NULL;
    ctx->filter_ctx = NULL;
    ctx->ifmt_ctx = NULL;
    ctx->ofmt_ctx = NULL;

    if ((ret = open_input_file(inPath, ctx)) < 0)
        goto end;
    av_log(NULL, AV_LOG_INFO, "Open input file success\n");
    if (*isCancelledBool) {
        av_log(NULL, AV_LOG_INFO, "Transcode cancelled, %d\n", *isCancelledBool);
        goto end;
    }
    if ((ret = open_output_file(outPath, ctx)) < 0)
        goto end;
    av_log(NULL, AV_LOG_INFO, "Open output file success\n");
    if (*isCancelledBool) {
        av_log(NULL, AV_LOG_INFO, "Transcode cancelled\n");
        goto end;
    }
    if ((ret = init_filters(ctx)) < 0)
        goto end;
    av_log(NULL, AV_LOG_INFO, "Init filters success\n");
    if (*isCancelledBool) {
        av_log(NULL, AV_LOG_INFO, "Transcode cancelled\n");
        goto end;
    }
    if (!(packet = av_packet_alloc()))
        goto end;
    av_log(NULL, AV_LOG_INFO, "Alloc packet success\n");
    if (*isCancelledBool) {
        av_log(NULL, AV_LOG_INFO, "Transcode cancelled\n");
        goto end;
    }

    /* read all packets */
    while (1) { 
        if (*isCancelledBool) {
            av_log(NULL, AV_LOG_INFO, "Transcode cancelled\n");
            goto end;
        }
        if ((ret = av_read_frame(ctx->ifmt_ctx, packet)) < 0)
            break;
        stream_index = packet->stream_index;
        if (ctx->ifmt_ctx->streams[stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            av_packet_unref(packet);
            continue;
        }
        av_log(NULL, AV_LOG_DEBUG, "Demuxer gave frame of stream_index %u\n",
                stream_index);

        if (ctx->filter_ctx[stream_index].filter_graph) {
            StreamContext *stream = &ctx->stream_ctx[stream_index];

            av_log(NULL, AV_LOG_DEBUG, "Going to reencode&filter the frame\n");

            ret = avcodec_send_packet(stream->dec_ctx, packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(stream->dec_ctx, stream->dec_frame);
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                    break;
                else if (ret < 0)
                    goto end;

                stream->dec_frame->pts = stream->dec_frame->best_effort_timestamp;
                ret = filter_encode_write_frame(stream->dec_frame, stream_index, ctx);
                if (ret < 0)
                    goto end;
            }
        } else {
            /* remux this frame without reencoding */
            av_packet_rescale_ts(packet,
                                 ctx->ifmt_ctx->streams[stream_index]->time_base,
                                 ctx->ofmt_ctx->streams[stream_index]->time_base);

            ret = av_interleaved_write_frame(ctx->ofmt_ctx, packet);
            if (ret < 0)
                goto end;
        }
        av_packet_unref(packet);
    }

    /* flush decoders, filters and encoders */
    for (i = 0; i < ctx->ifmt_ctx->nb_streams; i++) {
        if (*isCancelledBool) {
            av_log(NULL, AV_LOG_INFO, "Transcode cancelled\n");
            goto end;
        }
        // Skip processing if the stream is audio
        if (ctx->ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            continue;
        StreamContext *stream;

        if (!ctx->filter_ctx[i].filter_graph)
            continue;

        stream = &ctx->stream_ctx[i];

        av_log(NULL, AV_LOG_INFO, "Flushing stream %u decoder\n", i);

        /* flush decoder */
        ret = avcodec_send_packet(stream->dec_ctx, NULL);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Flushing decoding failed\n");
            goto end;
        }

        while (ret >= 0) {
            if (*isCancelledBool) {
                av_log(NULL, AV_LOG_INFO, "Transcode cancelled\n");
                goto end;
            }
            ret = avcodec_receive_frame(stream->dec_ctx, stream->dec_frame);
            if (ret == AVERROR_EOF)
                break;
            else if (ret < 0)
                goto end;

            stream->dec_frame->pts = stream->dec_frame->best_effort_timestamp;
            ret = filter_encode_write_frame(stream->dec_frame, i, ctx);
            if (ret < 0)
                goto end;
        }

        /* flush filter */
        ret = filter_encode_write_frame(NULL, i, ctx);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Flushing filter failed\n");
            goto end;
        }

        /* flush encoder */
        ret = flush_encoder(i, ctx);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Flushing encoder failed\n");
            goto end;
        }
    }

    av_write_trailer(ctx->ofmt_ctx);
end:
    av_packet_free(&packet);
    if(ctx->ifmt_ctx)
    {
        for (i = 0; i < ctx->ifmt_ctx->nb_streams; i++) {
            avcodec_free_context(&ctx->stream_ctx[i].dec_ctx);
            if (ctx->ofmt_ctx && ctx->ofmt_ctx->nb_streams > i && ctx->ofmt_ctx->streams[i] && ctx->stream_ctx[i].enc_ctx)
                avcodec_free_context(&ctx->stream_ctx[i].enc_ctx);
            if (ctx->filter_ctx && ctx->filter_ctx[i].filter_graph) {
                avfilter_graph_free(&ctx->filter_ctx[i].filter_graph);
                av_packet_free(&ctx->filter_ctx[i].enc_pkt);
                av_frame_free(&ctx->filter_ctx[i].filtered_frame);
            }

            av_frame_free(&ctx->stream_ctx[i].dec_frame);
        }
    }
    av_free(ctx->filter_ctx);
    av_free(ctx->stream_ctx);
    avformat_close_input(&ctx->ifmt_ctx);
    if (ctx->ofmt_ctx && !(ctx->ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&ctx->ofmt_ctx->pb);
    avformat_free_context(ctx->ofmt_ctx);
    av_free(ctx);

    if (ret < 0)
        av_log(NULL, AV_LOG_ERROR, "Error occurred: %s\n", av_err2str(ret));
    return ret;
}