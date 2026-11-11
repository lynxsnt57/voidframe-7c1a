#include "Generated/Scripts.hpp"

#include "Asset/Sound.hpp"
#include "Asset/Sprite.hpp"
#include "Asset/Surface.hpp"
#include "AppHandler.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QtPlugin>
#include <cerrno>
#include <mutex>

#ifdef OS_WINDOWS
	#include <mfapi.h>
	#include <mfidl.h>
	#include <mfreadwrite.h>
	#include <windows.h>
#endif

#ifdef OS_WINDOWS
	Q_IMPORT_PLUGIN(QGifPlugin)
#endif

extern "C"
{
	#include <libavutil/opt.h>
	#include <libavutil/mathematics.h>
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libswresample/swresample.h>
}

#define INPUT_PIXEL_FORMAT					AV_PIX_FMT_RGBA
#define STREAM_VIDEO_PIXEL_FORMAT			AV_PIX_FMT_YUV420P

#define STREAM_AUDIO_BIT_RATE				320000
#define STREAM_AUDIO_SAMPLE_RATE			44100
#define STREAM_AUDIO_FRAME_SIZE				1152
#define STREAM_AUDIO_SAMPLE_FORMAT			AV_SAMPLE_FMT_S16
#define STREAM_AUDIO_SAMPLE_FORMAT_MOVIE	AV_SAMPLE_FMT_FLTP
#define STREAM_AUDIO_CHANNEL_LAYOUT			AV_CH_LAYOUT_STEREO
#define STREAM_AUDIO_CHANNELS				2

namespace CppProject
{
#ifdef OS_WINDOWS
	struct ComLock
	{
		ComLock() : result(CoInitializeEx(nullptr, COINIT_MULTITHREADED)) {}
		~ComLock()
		{
			if (SUCCEEDED(result))
				CoUninitialize();
		}

		HRESULT result;
	};
#endif

	// Base class for the different video decoders used by video tracks.
	struct VideoSource
	{
		virtual ~VideoSource() {}
		virtual double Duration() const = 0;
		virtual double Aspect() const = 0;
		virtual IntType FrameAt(double seconds) = 0;

		int references = 0;

	protected:
		static IntType Publish(Sprite*& sprite, const QImage& image)
		{
			if (image.isNull())
				return sprite ? sprite->id : -1;
			if (!sprite)
				sprite = new Sprite(image, QPoint(0, 0));
			else
				sprite->SetImage(image);
			return sprite->id;
		}
	};

	// Animated GIF: small enough to decode once into a frame list.
	struct GifClip : VideoSource
	{
		~GifClip()
		{
			deleteAndReset(sprite);
		}

		bool Open(const QString& filename)
		{
			QImageReader reader(filename);
			reader.setDecideFormatFromContent(true);
			if (!reader.canRead())
				return false;

			qint64 pixels = 0;
			while (frames.size() < 5000)
			{
				QImage image = reader.read();
				if (image.isNull())
					break;
				if (image.format() != QImage::Format_ARGB32)
					image = image.convertToFormat(QImage::Format_ARGB32);
				pixels += (qint64)image.width() * image.height();
				if (pixels > 120LL * 1024 * 1024)
					break;

				int delay = reader.nextImageDelay();
				if (delay < 20)
					delay = 100;
				start.append(total);
				frames.append(image);
				total += delay / 1000.0;
			}
			return !frames.isEmpty() && total > 0;
		}

		double Duration() const override { return total; }
		double Aspect() const override
		{
			return frames.isEmpty() ? 1.0 : (double)frames.first().width() / frames.first().height();
		}

		IntType FrameAt(double seconds) override
		{
			if (frames.isEmpty())
				return -1;
			int index = 0;
			while (index + 1 < frames.size() && start[index + 1] <= seconds)
				index++;
			if (index == lastIndex && sprite)
				return sprite->id;
			lastIndex = index;
			return Publish(sprite, frames[index]);
		}

		QVector<QImage> frames;
		QVector<double> start;
		double total = 0;
		int lastIndex = -1;
		Sprite* sprite = nullptr;
	};

#ifdef OS_WINDOWS
	// Windows Media Foundation decoder. The FFmpeg build that ships with the
	// project only contains audio decoders and MPEG-4 part 2, so H.264/HEVC
	// files are decoded by the operating system instead.
	struct MediaFoundationClip : VideoSource
	{
		~MediaFoundationClip()
		{
			deleteAndReset(sprite);
			if (reader)
				reader->Release();
		}

		bool Open(const QString& filename)
		{
			static std::once_flag startup;
			std::call_once(startup, []()
			{
				MFStartup(MF_VERSION);
			});

			static thread_local ComLock comLock;
			if (FAILED(comLock.result) && comLock.result != RPC_E_CHANGED_MODE)
				return false;

			IMFAttributes* attributes = nullptr;
			if (FAILED(MFCreateAttributes(&attributes, 2)))
				return false;
			attributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
			attributes->SetUINT32(MF_SOURCE_READER_DISABLE_DXVA, TRUE);
			HRESULT result = MFCreateSourceReaderFromURL(reinterpret_cast<LPCWSTR>(filename.utf16()), attributes, &reader);
			attributes->Release();
			if (FAILED(result))
				return false;

			BOOL selected = FALSE;
			hasAudio = SUCCEEDED(reader->GetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &selected));

			if (FAILED(reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE)) ||
				FAILED(reader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE)))
				return false;

			IMFMediaType* outputType = nullptr;
			if (FAILED(MFCreateMediaType(&outputType)))
				return false;
			outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
			outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
			result = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputType);
			outputType->Release();
			if (FAILED(result))
				return false;

			IMFMediaType* currentType = nullptr;
			if (FAILED(reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &currentType)))
				return false;
			UINT32 videoWidth = 0, videoHeight = 0;
			MFGetAttributeSize(currentType, MF_MT_FRAME_SIZE, &videoWidth, &videoHeight);
			UINT32 frameStride = 0;
			currentType->GetUINT32(MF_MT_DEFAULT_STRIDE, &frameStride);
			currentType->Release();
			if (videoWidth == 0 || videoHeight == 0)
				return false;
			width = (int)videoWidth;
			height = (int)videoHeight;
			stride = frameStride != 0 ? (int)frameStride : width * 4;

			IMFMediaSource* source = nullptr;
			if (SUCCEEDED(reader->GetServiceForStream(MF_SOURCE_READER_MEDIASOURCE, GUID_NULL, IID_PPV_ARGS(&source))))
			{
				IMFPresentationDescriptor* descriptor = nullptr;
				if (SUCCEEDED(source->CreatePresentationDescriptor(&descriptor)))
				{
					UINT64 length = 0;
					if (SUCCEEDED(descriptor->GetUINT64(MF_PD_DURATION, &length)))
						duration = (double)length / 10000000.0;
					descriptor->Release();
				}
				source->Release();
			}
			return duration > 0;
		}

		double Duration() const override { return duration; }
		double Aspect() const override { return height > 0 ? (double)width / height : 1.0; }

		IntType FrameAt(double seconds) override
		{
			if (!reader)
				return -1;

			double target = seconds;
			if (duration > 0)
				target = qBound(0.0, target, duration);
			// Repeated scrubbing over already requested times can reuse a bounded
			// cache rather than repeatedly seeking the media decoder.
			if (sprite && qAbs(target - lastRequested) < 0.0001)
				return sprite->id;
			for (int i = recentFrames.size() - 1; i >= 0; --i)
			{
				if (qAbs(recentFrames[i].requested - target) < 0.0001)
				{
					lastRequested = target;
					// The source reader is still at its previous decode position.
					// Force a seek if the next request misses the cache.
					lastPosition = -1;
					return Publish(sprite, recentFrames[i].image);
				}
			}

			// The last decoded sample can lie ahead of the requested time. Reuse
			// it until the playhead passes that sample, instead of consuming a
			// new sample for every render pass.
			if (sprite && target >= lastRequested && target <= lastPosition)
			{
				lastRequested = target;
				return sprite->id;
			}

			// Re-seek whenever the playhead moves backwards or jumps far ahead.
			if (lastPosition < 0 || target < lastRequested || target - lastPosition > 0.75)
			{
				PROPVARIANT position = {};
				position.vt = VT_I8;
				position.hVal.QuadPart = (LONGLONG)(target * 10000000.0);
				if (FAILED(reader->SetCurrentPosition(GUID_NULL, position)))
					return sprite ? sprite->id : -1;
				lastPosition = -1;
			}

			for (int attempt = 0; attempt < 900; attempt++)
			{
				DWORD flags = 0;
				LONGLONG timestamp = 0;
				IMFSample* sample = nullptr;
				HRESULT result = reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, &flags, &timestamp, &sample);
				if (FAILED(result))
					break;
				if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
					break;
				if (!sample)
					continue;

				double frameTime = (double)timestamp / 10000000.0;
				lastPosition = frameTime;
				if (frameTime + 0.0001 >= target)
				{
					QImage image = FrameImage(sample);
					sample->Release();
					if (image.isNull())
						return sprite ? sprite->id : -1;
					lastRequested = target;
					RememberFrame(target, image);
					return Publish(sprite, image);
				}
				sample->Release();
			}
			return sprite ? sprite->id : -1;
		}

		void RememberFrame(double requested, const QImage& image)
		{
			const qint64 limit = 32LL * 1024 * 1024;
			const qint64 bytes = (qint64)image.bytesPerLine() * image.height();
			if (image.isNull() || bytes <= 0 || bytes > limit)
				return;
			while (!recentFrames.isEmpty() && (cacheBytes + bytes > limit || recentFrames.size() >= 32))
			{
				const QImage& oldest = recentFrames.first().image;
				cacheBytes -= (qint64)oldest.bytesPerLine() * oldest.height();
				recentFrames.removeFirst();
			}
			CachedFrame entry;
			entry.requested = requested;
			entry.image = image;
			recentFrames.append(entry);
			cacheBytes += bytes;
		}

		QImage FrameImage(IMFSample* sample)
		{
			IMFMediaBuffer* buffer = nullptr;
			if (FAILED(sample->ConvertToContiguousBuffer(&buffer)))
				return QImage();

			QImage image;
			BYTE* data = nullptr;
			DWORD maxLength = 0, length = 0;
			if (SUCCEEDED(buffer->Lock(&data, &maxLength, &length)) && data)
			{
				image = QImage(width, height, QImage::Format_RGB32);
				if (!image.isNull())
				{
					int sourceStride = stride;
					int rowBytes = qMin(width * 4, qAbs(sourceStride));
					for (int y = 0; y < height; y++)
					{
						int sourceY = sourceStride >= 0 ? y : (height - 1 - y);
						memcpy(image.scanLine(y), data + (size_t)sourceY * qAbs(sourceStride), rowBytes);
					}
				}
				buffer->Unlock();
			}
			buffer->Release();
			return image;
		}

		struct CachedFrame
		{
			double requested;
			QImage image;
		};
		QVector<CachedFrame> recentFrames;
		qint64 cacheBytes = 0;
		IMFSourceReader* reader = nullptr;
		int width = 0, height = 0, stride = 0;
		bool hasAudio = false;
		double duration = 0;
		double lastPosition = -1;
		double lastRequested = -1;
		Sprite* sprite = nullptr;
	};
#endif

	// Fallback decoder built on the FFmpeg libraries that ship with the
	// project. Only the current decoded frame is kept in memory.
	namespace
	{
		struct VideoClip : VideoSource
		{
			~VideoClip()
			{
				deleteAndReset(sprite);
				if (scale)
					sws_freeContext(scale);
				if (frame)
					av_frame_free(&frame);
				if (packet)
					av_packet_free(&packet);
				if (codec)
					avcodec_free_context(&codec);
				if (format)
					avformat_close_input(&format);
			}

			bool Open(const QString& filename)
			{
				if (avformat_open_input(&format, filename.toUtf8().constData(), nullptr, nullptr) < 0 ||
					avformat_find_stream_info(format, nullptr) < 0)
					return false;

				const AVCodec* decoder = nullptr;
				streamIndex = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
				if (streamIndex < 0 || !decoder)
					return false;

				AVStream* stream = format->streams[streamIndex];
				streamStart = stream->start_time == AV_NOPTS_VALUE ? 0.0 : stream->start_time * av_q2d(stream->time_base);
				codec = avcodec_alloc_context3(decoder);
				if (!codec || avcodec_parameters_to_context(codec, stream->codecpar) < 0 || avcodec_open2(codec, decoder, nullptr) < 0)
					return false;

				frame = av_frame_alloc();
				packet = av_packet_alloc();
				if (!frame || !packet)
					return false;

				if (stream->duration != AV_NOPTS_VALUE)
					duration = stream->duration * av_q2d(stream->time_base);
				else if (format->duration != AV_NOPTS_VALUE)
					duration = (double)format->duration / AV_TIME_BASE;
				return codec->width > 0 && codec->height > 0;
			}

		double Duration() const { return duration; }
		double Aspect() const
		{
			return codec && codec->height > 0 ? (double)codec->width / codec->height : 1.0;
		}

		bool HasAudio() const
		{
			if (!format)
				return false;
			for (unsigned int i = 0; i < format->nb_streams; i++)
				if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
					return true;
			return false;
		}

			IntType FrameAt(double seconds)
			{
				if (!format || !codec || !frame || !packet || seconds < 0)
					return -1;

				AVStream* stream = format->streams[streamIndex];
				double target = seconds;
				if (duration > 0)
					target = qBound(0.0, target, duration);

				if (sprite && qAbs(target - lastRequested) < 0.0001)
					return sprite->id;

				// Seeking is needed when the user scrubs backwards or jumps far
				// ahead; normal playback decodes sequentially.
				if (lastPts < 0 || target + 0.0001 < lastPts || target - lastPts > 1.0)
				{
					int64_t ts = av_rescale_q((int64_t)(target * AV_TIME_BASE), AVRational{ 1, AV_TIME_BASE }, stream->time_base);
					if (stream->start_time != AV_NOPTS_VALUE)
						ts += stream->start_time;
					av_seek_frame(format, streamIndex, ts, AVSEEK_FLAG_BACKWARD);
					avcodec_flush_buffers(codec);
					lastPts = -1;
					lastImage = QImage();
				}

				bool flushed = false;
				while (true)
				{
					int readResult = av_read_frame(format, packet);
					bool atEnd = (readResult < 0);
					int sendResult = 0;
					if (atEnd)
					{
						if (flushed)
							break;
						flushed = true;
						sendResult = avcodec_send_packet(codec, nullptr);
					}
					else if (packet->stream_index == streamIndex)
						sendResult = avcodec_send_packet(codec, packet);
					if (!atEnd)
						av_packet_unref(packet);
					if (sendResult < 0 && sendResult != AVERROR(EAGAIN) && sendResult != AVERROR_EOF)
						continue;

					while (avcodec_receive_frame(codec, frame) >= 0)
					{
						int64_t pts = frame->best_effort_timestamp;
						double frameTime = pts == AV_NOPTS_VALUE ? lastPts : pts * av_q2d(stream->time_base) - streamStart;
						QImage image(frame->width, frame->height, QImage::Format_RGBA8888);
						if (!image.isNull())
						{
							scale = sws_getCachedContext(scale, frame->width, frame->height, (AVPixelFormat)frame->format,
								frame->width, frame->height, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
							if (scale)
							{
								uint8_t* dst[] = { image.bits(), nullptr, nullptr, nullptr };
								int stride[] = { image.bytesPerLine(), 0, 0, 0 };
								sws_scale(scale, frame->data, frame->linesize, 0, frame->height, dst, stride);
								lastPts = frameTime;
								lastImage = image;
								if (frameTime + 0.0001 >= target)
								{
									if (!sprite)
										sprite = new Sprite(image, QPoint(0, 0));
									else
										sprite->SetImage(image);
									lastRequested = target;
									av_frame_unref(frame);
									return sprite->id;
								}
							}
						}
						av_frame_unref(frame);
					}
				}
			if (!lastImage.isNull())
			{
				if (!sprite)
					sprite = new Sprite(lastImage, QPoint(0, 0));
				else
					sprite->SetImage(lastImage);
				lastRequested = target;
			}
			return sprite ? sprite->id : -1;
			}

			AVFormatContext* format = nullptr;
			AVCodecContext* codec = nullptr;
			AVFrame* frame = nullptr;
			AVPacket* packet = nullptr;
			SwsContext* scale = nullptr;
			Sprite* sprite = nullptr;
			int streamIndex = -1;
			double duration = 0;
			double streamStart = 0;
			double lastPts = -1;
			double lastRequested = -1;
			QImage lastImage;
		};

		QHash<QString, VideoSource*> videoClips;

		VideoSource* OpenVideoSource(const QString& path)
		{
			QString suffix = QFileInfo(path).suffix().toLower();
			VideoSource* source = nullptr;

			if (suffix == "gif")
			{
				GifClip* gif = new GifClip();
				if (gif->Open(path))
					source = gif;
				else
					delete gif;
			}
#ifdef OS_WINDOWS
			if (!source)
			{
				MediaFoundationClip* media = new MediaFoundationClip();
				if (media->Open(path))
					source = media;
				else
					delete media;
			}
#endif
			if (!source)
			{
				VideoClip* clip = new VideoClip();
				if (clip->Open(path))
					source = clip;
				else
					delete clip;
			}
			if (source)
				DEBUG(QString("Video: opened ") + path);
			else
				DEBUG(QString("Video: could not decode ") + path);
			return source;
		}

		VideoSource* GetVideoClip(const StringType& filename)
		{
			QString path = QDir::cleanPath(QString::fromStdString(filename.ToStdString()));
			if (videoClips.contains(path))
				return videoClips.value(path);

			VideoSource* clip = OpenVideoSource(path);
			if (!clip)
				return nullptr;
			if (videoClips.size() >= 8)
			{
				for (auto it = videoClips.begin(); it != videoClips.end() && videoClips.size() >= 8; )
				{
					if (it.value()->references == 0)
					{
						delete it.value();
						it = videoClips.erase(it);
					}
					else
						++it;
				}
			}
			videoClips.insert(path, clip);
			return clip;
		}
	}

	void video_clip_retain(StringType filename)
	{
		if (VideoSource* clip = GetVideoClip(filename))
			clip->references++;
	}

	void video_clip_release(StringType filename)
	{
		QString path = QDir::cleanPath(QString::fromStdString(filename.ToStdString()));
		auto it = videoClips.find(path);
		if (it == videoClips.end() || it.value()->references <= 0)
			return;
		it.value()->references--;
		if (it.value()->references == 0)
		{
			delete it.value();
			videoClips.erase(it);
		}
	}

	void video_clip_discard(StringType filename)
	{
		QString path = QDir::cleanPath(QString::fromStdString(filename.ToStdString()));
		auto it = videoClips.find(path);
		if (it != videoClips.end() && it.value()->references == 0)
		{
			delete it.value();
			videoClips.erase(it);
		}
	}

	IntType video_frame_sprite(StringType filename, RealType seconds)
	{
		if (VideoSource* clip = GetVideoClip(filename))
			return clip->FrameAt(seconds);
		return -1;
	}

	RealType video_file_duration(StringType filename)
	{
		if (VideoSource* clip = GetVideoClip(filename))
			return clip->Duration();
		return 0;
	}

	RealType video_file_aspect(StringType filename)
	{
		if (VideoSource* clip = GetVideoClip(filename))
			return clip->Aspect();
		return 1.0;
	}

	BoolType video_file_has_audio(StringType filename)
	{
		QString path = QDir::cleanPath(QString::fromStdString(filename.ToStdString()));
		AVFormatContext* format = nullptr;
		if (avformat_open_input(&format, path.toUtf8().constData(), nullptr, nullptr) < 0)
			return false;

		BoolType hasAudio = false;
		for (unsigned int i = 0; i < format->nb_streams; i++)
		{
			if (format->streams[i]->codecpar->codec_type != AVMEDIA_TYPE_AUDIO)
				continue;
			if (avcodec_find_decoder(format->streams[i]->codecpar->codec_id))
			{
				hasAudio = true;
				break;
			}
		}
		avformat_close_input(&format);
		return hasAudio;
	}

	// Media file output
	AVFormatContext* outContext;

	// Video
	IntType videoWidth, videoHeight, videoBitRate, videoFrameRate, videoColorType, videoLegacy;
	AVStream* videoStream;
	AVCodecContext* videoCodecContext;
	IntType videoFrameNum;
	SwsContext* videoSwsContext;
	uchar* videoFrameData = nullptr;

	// Audio
	BoolType audioEnabled;
	AVStream* audioStream;
	AVCodec* audioCodec;
	AVCodecContext* audioCodecContext;
	AVRational audioTimeBase;
	IntType audioFrameNum;
	
	AVPixelFormat GetStreamPixelFormat()
	{
		switch (videoColorType)
		{
			case 0: return AV_PIX_FMT_YUV420P; // YUV 4:2:0
			case 1: return AV_PIX_FMT_YUV444P; // YUV 4:4:4
			default: return AV_PIX_FMT_YUV420P;
		}
	}

	// Movie sound
	struct MovieSound
	{
		Sound* sound;
		IntType frame, start, end;
		RealType volume, pitch;
	};
	QVector<MovieSound*> movieSounds;

	RealType lib_movie_init()
	{
		// Do nothing
		return 0;
	}

	RealType lib_movie_audio_file_decode(StringType source, StringType destination)
	{
		// Audio tracks use this helper to convert any FFmpeg-readable source
		// into the same 44.1 kHz stereo PCM format used by the editor.
		Sound decoded(source);
		QByteArray pcm = decoded.buffer.buffer();
		if (decoded.samples <= 0 || pcm.isEmpty())
			return -1;

		QFile output(QString::fromStdString(destination.ToStdString()));
		if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate))
			return -1;
		qint64 written = output.write(pcm);
		output.close();
		return written == pcm.size() ? 0 : -1;
	}

	Sound::Sound(StringType filename) : Asset(ID_Sound)
	{
		AVFormatContext* formatContext = nullptr;
		const AVCodec* codec = nullptr;
		int audioStreamIndex = -1;
		AVCodecContext* codecContext = nullptr;
		SwrContext* swrContext = nullptr;
		AVFrame* decodedFrame = nullptr;
		uint8_t* convertedData = nullptr;
		AVPacket* inPacket = nullptr;

		// Decode file into buffer
		buffer.open(QIODevice::WriteOnly);
		QDataStream outStream(&buffer);

		try
		{
			Timer tmr;
			std::string filenameStd = filename.ToStdString();
			log({ "VIDEO_NATIVE 10 Sound decoder started" });

			// Get format from audio file
			formatContext = avformat_alloc_context();
			if (avformat_open_input(&formatContext, filenameStd.c_str(), nullptr, nullptr) != 0)
				throw "avformat_open_input failed";

			if (avformat_find_stream_info(formatContext, nullptr) < 0)
				throw "avformat_find_stream_info failed";

			codecContext = avcodec_alloc_context3(nullptr);
			for (IntType i = 0; i < formatContext->nb_streams; i++)
			{
				AVCodecParameters* par = formatContext->streams[i]->codecpar;
				if (par->codec_type == AVMEDIA_TYPE_AUDIO)
				{
					audioStreamIndex = static_cast<int>(i);
					codec = avcodec_find_decoder(par->codec_id);
					if (!codec)
						throw "avcodec_find_decoder failed";
					if (avcodec_parameters_to_context(codecContext, par) < 0)
						throw "avcodec_parameters_to_context failed";
					break;
				}
			}

			if (!codec)
				throw "No codec found";

			// Open codec of the audio file
			if (avcodec_open2(codecContext, codec, nullptr) < 0)
				throw "avcodec_open2 failed";

			// Prepare resampling
			swrContext = swr_alloc();
			if (!swrContext)
				throw "swr_alloc failed";

			av_opt_set_int(swrContext, "in_channel_count", codecContext->channels, 0);
			av_opt_set_int(swrContext, "in_channel_layout", codecContext->channel_layout, 0);
			av_opt_set_int(swrContext, "in_sample_rate", codecContext->sample_rate, 0);
			av_opt_set_sample_fmt(swrContext, "in_sample_fmt", codecContext->sample_fmt, 0);

			av_opt_set_int(swrContext, "out_channel_count", STREAM_AUDIO_CHANNELS, 0);
			av_opt_set_int(swrContext, "out_channel_layout", STREAM_AUDIO_CHANNEL_LAYOUT, 0);
			av_opt_set_int(swrContext, "out_sample_rate", STREAM_AUDIO_SAMPLE_RATE, 0);
			av_opt_set_sample_fmt(swrContext, "out_sample_fmt", STREAM_AUDIO_SAMPLE_FORMAT, 0);

			if (swr_init(swrContext))
				throw "swr_init failed";

			// Create packet
			inPacket = av_packet_alloc();
			if (!inPacket)
				throw "av_packet_alloc failed";

			// Prepare to read data
			decodedFrame = av_frame_alloc();
			if (!decodedFrame)
				throw "av_packet_alloc failed";

			// Read frames and store the decoded buffer in the resample context
			log({ "VIDEO_NATIVE 11 before reading audio packets" });
			IntType inSamples = 0;
			while (av_read_frame(formatContext, inPacket) >= 0)
			{
				// MP4 containers also contain video packets. Never feed those to
				// the audio decoder, and release every packet as soon as it is used.
				if (inPacket->stream_index != audioStreamIndex)
				{
					av_packet_unref(inPacket);
					continue;
				}

				if (avcodec_send_packet(codecContext, inPacket) < 0 ||
					avcodec_receive_frame(codecContext, decodedFrame) < 0)
				{
					av_frame_unref(decodedFrame);
					av_packet_unref(inPacket);
					continue;
				}

				// extended_data is required for planar formats with many channels.
				int convertResult = swr_convert(swrContext, nullptr, 0,
					(const uint8_t**)decodedFrame->extended_data, decodedFrame->nb_samples);
				inSamples += decodedFrame->nb_samples;
				av_frame_unref(decodedFrame);
				av_packet_unref(inPacket);
				if (convertResult < 0)
					throw "swr_convert input failed";
			}

			log({ "VIDEO_NATIVE 12 finished reading audio packets" });
			// Allocate data
			if (av_samples_alloc(&convertedData, nullptr, STREAM_AUDIO_CHANNELS, STREAM_AUDIO_FRAME_SIZE, STREAM_AUDIO_SAMPLE_FORMAT, 0) < 0)
				throw "av_samples_alloc failed";

			// Read from the resample context buffer and convert
			RealType rateDiv = codecContext->sample_rate / (RealType)STREAM_AUDIO_SAMPLE_RATE;
			IntType totalSamples = (IntType)(inSamples / rateDiv);
			while (samples < totalSamples)
			{
				// Convert
				IntType outSamples = swr_convert(swrContext, &convertedData, STREAM_AUDIO_FRAME_SIZE, nullptr, 0);
				if (outSamples == 0)
					break;
				else if (outSamples < 0)
					throw "swr_convert failed";
				samples += outSamples;

				// Calculate buffer size
				IntType bufferSize = av_samples_get_buffer_size(nullptr, STREAM_AUDIO_CHANNELS, outSamples, STREAM_AUDIO_SAMPLE_FORMAT, 0);
				if (bufferSize < 0)
					throw "av_samples_get_buffer_size failed";

				outStream.writeRawData((const char*)convertedData, bufferSize);
			}

			log({ "VIDEO_NATIVE 13 audio conversion finished" });
			tmr.Print("Decoded " + (QString)filename + " into " + NumStr(samples) + " samples");

			// Create OpenAL buffer
			if (App->audioSupported)
			{
				alGenBuffers(1, &alBuffer);
				alBufferData(alBuffer, AL_FORMAT_STEREO16, buffer.data().constData(), buffer.size(), sample_rate_);
			}

			buffer.close();
		}
		catch (const char* err)
		{
			log({ err });
			WARNING(err);
			samples = 0;
			buffer.close();
			buffer.buffer().clear();
		}

		// Cleanup
		if (decodedFrame)
			av_frame_free(&decodedFrame);
		if (swrContext)
			swr_free(&swrContext);
		if (codecContext)
			avcodec_close(codecContext);
		if (formatContext)
		{
			avformat_close_input(&formatContext);
			avformat_free_context(formatContext);
		}
		if (convertedData)
			av_freep(&convertedData);
		if (inPacket)
			av_packet_free(&inPacket);
	}

	Sound::~Sound()
	{
		if (alBuffer)
			alDeleteBuffers(1, &alBuffer);
	}

	RealType lib_movie_set(RealType width, RealType height, RealType bitRate, RealType frameRate, RealType audio, RealType colortype, RealType legacy)
	{
		videoWidth = width;
		videoHeight = height;
		videoBitRate = bitRate;
		videoFrameRate = frameRate;
		videoColorType = colortype;
		videoLegacy = legacy;
		audioEnabled = audio > 0.0;
		return 0;
	}

	RealType lib_movie_start(StringType outFile, StringType outFormat)
	{
		std::string outFileStd = outFile.ToStdString();
		std::string outFormatStd = outFormat.ToStdString();
		
		try
		{
			// Find format
			const AVOutputFormat* outFmt = av_guess_format(outFormatStd.c_str(), 0, 0);
			if (!outFmt)
				throw "av_guess_format failed";

			// Allocate the output media context
			avformat_alloc_output_context2(&outContext, outFmt, 0, 0);
			if (!outContext)
				throw "avformat_alloc_output_context2 failed";

			// Find video encoder
			const AVCodec* videoCodec = avcodec_find_encoder(outContext->oformat->video_codec);
			if (!videoCodec)
				throw "avcodec_find_encoder failed";

			DEBUG("Encoder: " + QString(videoCodec->long_name));
			
			if (!videoLegacy)
			{
				// ---------------------- NEWER ------------------------
				// Start video stream
				videoStream = avformat_new_stream(outContext, videoCodec);
				if (!videoStream)
					throw "avformat_new_stream failed";

				videoStream->id = 0;
				videoStream->codecpar->width = videoWidth;
				videoStream->codecpar->height = videoHeight;
				videoStream->codecpar->codec_id = videoCodec->id;
				videoStream->codecpar->codec_type = videoCodec->type;
				videoStream->avg_frame_rate = { (int)videoFrameRate, 1 };

				// Setup context
				videoCodecContext = avcodec_alloc_context3(videoCodec);
				videoCodecContext->bit_rate = videoBitRate * 5;
				videoCodecContext->width = videoWidth;
				videoCodecContext->height = videoHeight;
				videoCodecContext->time_base = { 1, (int)videoFrameRate };
				videoCodecContext->framerate = { (int)videoFrameRate, 1 };
				videoCodecContext->pix_fmt = GetStreamPixelFormat();
				videoCodecContext->gop_size = 12;
				videoCodecContext->color_range = AVCOL_RANGE_JPEG;
				videoCodecContext->thread_type = 'AUTO';
				videoCodecContext->thread_count = 0;  // 0 = auto-detect and use all CPU threads
				if (outContext->oformat->flags & AVFMT_GLOBALHEADER)
					videoCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

				// Open the codec
				AVDictionary* opt = NULL;
				av_dict_set(&opt, "tune", "animation", 0);
				av_dict_set(&opt, "preset", videoColorType ? "veryslow" : "slow", 0);
				if (avcodec_open2(videoCodecContext, videoCodec, &opt) < 0)
					throw "avcodec_open2 failed";
				videoFrameNum = 0;

				avcodec_parameters_from_context(videoStream->codecpar, videoCodecContext);

				AVPixelFormat targetPixFmt = GetStreamPixelFormat();

				videoSwsContext = sws_getContext(
					videoWidth, videoHeight, INPUT_PIXEL_FORMAT,
					videoWidth, videoHeight, targetPixFmt,
					SWS_LANCZOS, nullptr, nullptr, nullptr);

				sws_setColorspaceDetails(
					videoSwsContext,
					sws_getCoefficients(SWS_CS_DEFAULT),
					AVCOL_RANGE_JPEG,
					sws_getCoefficients(SWS_CS_DEFAULT),
					(targetPixFmt == AV_PIX_FMT_YUV444P ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG),
					0, 1, 1);
			} 
			else
			{
				// ---------------------- LEGACY ------------------------
				// Start video stream
				videoStream = avformat_new_stream(outContext, videoCodec);
				if (!videoStream)
					throw "avformat_new_stream failed";

				videoStream->id = 0;
				videoStream->codecpar->width = videoWidth;
				videoStream->codecpar->height = videoHeight;
				videoStream->codecpar->codec_id = videoCodec->id;
				videoStream->codecpar->codec_type = videoCodec->type;
				videoStream->avg_frame_rate = { (int)videoFrameRate, 1 };

				// Setup context
				videoCodecContext = avcodec_alloc_context3(videoCodec);
				videoCodecContext->bit_rate = videoBitRate * 5;
				videoCodecContext->width = videoWidth;
				videoCodecContext->height = videoHeight;
				videoCodecContext->time_base = { 1, (int)videoFrameRate };
				videoCodecContext->framerate = { (int)videoFrameRate, 1 };
				videoCodecContext->pix_fmt = STREAM_VIDEO_PIXEL_FORMAT;
				videoCodecContext->gop_size = 12;
				videoCodecContext->thread_type = 'AUTO';
				videoCodecContext->thread_count = 0;  // 0 = auto-detect and use all CPU threads
				if (outContext->oformat->flags & AVFMT_GLOBALHEADER)
					videoCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

				// Open the codec
				AVDictionary* opt = NULL;
				av_dict_set(&opt, "tune", "zerolatency", 0);
				if (avcodec_open2(videoCodecContext, videoCodec, &opt) < 0)
					throw "avcodec_open2 failed";

				videoFrameNum = 0;
				avcodec_parameters_from_context(videoStream->codecpar, videoCodecContext);

				// Scaling context
				videoSwsContext = sws_getContext(videoWidth, videoHeight, INPUT_PIXEL_FORMAT,
					videoWidth, videoHeight, STREAM_VIDEO_PIXEL_FORMAT,
					SWS_BILINEAR, nullptr, nullptr, nullptr);
			}

			if (!videoSwsContext)
				throw "sws_getContext failed";

			deleteArrayAndReset(videoFrameData);
			videoFrameData = new uchar[videoWidth * videoHeight * 4];

			if (audioEnabled)
			{
				// Find audio encoder
				const AVCodec* audioCodec = avcodec_find_encoder(outContext->oformat->audio_codec);
				if (!audioCodec)
					throw "avcodec_find_encoder failed";

				// Start audio stream
				audioStream = avformat_new_stream(outContext, audioCodec);
				if (!audioStream)
					throw "avformat_new_stream failed";
				audioStream->id = 1;
				audioStream->codecpar->sample_rate = STREAM_AUDIO_SAMPLE_RATE;
				audioStream->codecpar->frame_size = STREAM_AUDIO_FRAME_SIZE;
				audioStream->codecpar->codec_id = audioCodec->id;
				audioStream->codecpar->codec_type = audioCodec->type;

				// Setup context
				audioCodecContext = avcodec_alloc_context3(audioCodec);
				audioCodecContext->sample_fmt = STREAM_AUDIO_SAMPLE_FORMAT_MOVIE;
				audioCodecContext->sample_rate = STREAM_AUDIO_SAMPLE_RATE;
				audioCodecContext->bit_rate = STREAM_AUDIO_BIT_RATE;
				audioCodecContext->channels = STREAM_AUDIO_CHANNELS;
				audioCodecContext->channel_layout = STREAM_AUDIO_CHANNEL_LAYOUT;
				if (outContext->oformat->flags & AVFMT_GLOBALHEADER)
					audioCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

				// Open the codec
				if (avcodec_open2(audioCodecContext, audioCodec, nullptr) < 0)
					throw "avcodec_open2 failed";

				audioFrameNum = 0;
				audioTimeBase = { audioCodecContext->frame_size, STREAM_AUDIO_SAMPLE_RATE };
				avcodec_parameters_from_context(audioStream->codecpar, audioCodecContext);
			}

			// Open the output file
			if (avio_open(&outContext->pb, outFileStd.c_str(), AVIO_FLAG_WRITE) < 0)
				throw "avio_open failed";

			// Write the stream header, if any
			if (avformat_write_header(outContext, nullptr) < 0)
				throw "avformat_write_header failed";

			return 0;
		}
		catch (const char* err)
		{
			log({ err });
			WARNING(err);
			return -1;
		}
	}

	RealType lib_movie_audio_file_add(StringType)
	{
		if (Sound* sound = FindSound(global::_app->exportmovie_current_sound))
			return sound->id;

		return -1;
	}

	RealType lib_movie_audio_sound_add(RealType id, RealType time, RealType volume, RealType pitch, RealType start, RealType end)
	{
		if (!audioEnabled)
			return 0.0;

		movieSounds.append(
			new MovieSound({
				FindSound(id),
				av_rescale_q(time * 1000, { 1, 1000 }, audioTimeBase),
				av_rescale_q(start * 1000, { 1, 1000 }, audioTimeBase),
				av_rescale_q(end * 1000, { 1, 1000 }, audioTimeBase),
				volume,
				pitch
			})
		);
		return 0.0;
	}

	void Encode(AVCodecContext* context, AVFrame* frame, AVStream* stream)
	{
		AVPacket* packet = av_packet_alloc();
		if (!packet)
			throw "av_packet_alloc failed";

		// Send in a frame, a nullptr will enable flush mode
		if (avcodec_send_frame(context, frame) < 0)
			throw "avcodec_send_frame failed";

		// Encode a submitted video or audio frame to the stream
		while (true)
		{
			switch (avcodec_receive_packet(context, packet))
			{
				case 0:
				{
					av_packet_rescale_ts(packet, context->time_base, stream->time_base);
					packet->stream_index = stream->index;

					if (av_interleaved_write_frame(outContext, packet) < 0)
						throw "av_interleaved_write_frame failed";
					break;
				}
				case AVERROR(EAGAIN):
				case AVERROR_EOF:
					av_packet_free(&packet);
					return;

				default:
					throw "avcodec_receive_packet failed";
			}
		}

		av_packet_free(&packet);
	}

	RealType lib_movie_frame(StringType)
	{
		try
		{
			// Allocate and init frame
			AVFrame* videoFrame = av_frame_alloc();
			if (!videoFrame)
				throw "av_frame_alloc failed";

			videoFrame->pts = videoFrameNum++;
			videoFrame->format = videoCodecContext->pix_fmt;
			videoFrame->width = videoCodecContext->width;
			videoFrame->height = videoCodecContext->height;

			if (av_frame_get_buffer(videoFrame, 0) < 0)
				throw "av_frame_get_buffer failed";

			// Convert surface into frame
			FindSurface(global::_app->export_surface)->frameBuffer->CopyColorData(videoFrameData);
			uint8_t* inData[1] = { videoFrameData }; // RGBA have one plane
			int inLinesize[1] = { 4 * (int)videoWidth }; // RGBA stride
			sws_scale(videoSwsContext, inData, inLinesize, 0, videoHeight, videoFrame->data, videoFrame->linesize);
			
			// Submit frame and encode
			Encode(videoCodecContext, videoFrame, videoStream);

			// Cleanup video
			av_frame_free(&videoFrame);

			// Write interleaved audio
			while (audioEnabled && av_compare_ts(videoFrameNum, videoCodecContext->time_base, audioFrameNum, audioTimeBase) > 0)
			{
				// Allocate frame
				AVFrame* audioFrame = av_frame_alloc();
				if (!audioFrame)
					throw "av_frame_alloc failed";

				audioFrame->nb_samples = audioCodecContext->frame_size;
				audioFrame->format = STREAM_AUDIO_SAMPLE_FORMAT_MOVIE;
				audioFrame->channel_layout = STREAM_AUDIO_CHANNEL_LAYOUT;
				audioFrame->channels = STREAM_AUDIO_CHANNELS;
				audioFrame->sample_rate = STREAM_AUDIO_SAMPLE_RATE;

				if (av_frame_get_buffer(audioFrame, 0) < 0)
					throw "av_frame_get_buffer failed";

				if (av_frame_make_writable(audioFrame) < 0)
					throw "av_frame_make_writable failed";

				// Find sounds
				QVector<MovieSound*> frameSounds;
				for (MovieSound* sound : movieSounds)
					if (audioFrameNum >= sound->frame &&
						audioFrameNum < sound->frame + sound->sound->samples / audioCodecContext->frame_size / sound->pitch - sound->start + sound->end)
						frameSounds.append(sound);

				// Write to frame (mix sounds)
				for (IntType c = 0; c < 2; c++)
				{
					float* dstData = (float*)audioFrame->data[c];
					for (IntType s = 0; s < audioCodecContext->frame_size; s++)
					{
						float dstVal = 0.f; // 0 = silence

						for (MovieSound* sound : frameSounds)
						{
							const int16_t* srcData = (int16_t*)sound->sound->buffer.data().constData();
							IntType srcSample = ((((audioFrameNum - sound->frame + sound->start) * audioCodecContext->frame_size) % (uint64_t)(sound->sound->samples / sound->pitch)) + s) * sound->pitch;

							// Add and clamp audio if in buffer range
							if (srcSample < sound->sound->samples)
							{
								float srcVal = (float)srcData[srcSample * 2 + c] / sample_max;
								dstVal = std::clamp(dstVal + srcVal * (float)sound->volume, -1.f, 1.f);
							}
						}

						dstData[s] = dstVal;
					}
				}

				audioFrame->pts = av_rescale_q(audioFrameNum++, audioTimeBase, audioCodecContext->time_base);

				// Submit frame and encode
				Encode(audioCodecContext, audioFrame, audioStream);

				// Cleanup audio
				av_frame_free(&audioFrame);
			}

			return 0;
		}
		catch (const char* err)
		{
			log({ err });
			WARNING(err);
			return -1;
		}
	}

	RealType lib_movie_done()
	{
		try
		{
			// Flush video & audio
			Encode(videoCodecContext, nullptr, videoStream);
			if (audioEnabled)
				Encode(audioCodecContext, nullptr, audioStream);

			// Write the trailer
			if (av_write_trailer(outContext) < 0)
				throw "av_write_trailer failed";

			// Close video
			sws_freeContext(videoSwsContext);
			avcodec_close(videoCodecContext);

			// Close audio
			if (audioEnabled)
			{
				avcodec_close(audioCodecContext);

				for (MovieSound* snd : movieSounds)
					delete snd;
				movieSounds.clear();
			}

			// Close the output file.
			if (avio_close(outContext->pb) < 0)
				throw "avio_close failed";

			// Free the stream
			avformat_free_context(outContext);

			deleteArrayAndReset(videoFrameData);

			return 0;
		}
		catch (const char* err)
		{
			log({ err });
			WARNING(err);
			return -1;
		}
	}

}
