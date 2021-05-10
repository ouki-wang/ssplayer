主要功能是解码一个h264的视频, 并显示到panel/HDMI显示设备上, 支持rotate功能。
用法如下：
	旋转：    ./SsPlayer 720P25.h264 0 0 480 320 1
	正常显示： ./SsPlayer 720P25.h264 0 0 1280 720 0
注意事项：
	1.vdec输入宽高无需根据视频宽高设置, vdec输出宽高与disp输出宽高设置一致即可;
	
在原来仅播放视频的基础上增加了播放声音的功能：
输入命令如下：
    ./SsPlayer 720P25.h264 0 0 480 320 1 48k_stereo.wav 2 48000
	
默认使用panel显示, 如需enable hdmi显示, 更改Makefile, ENABLE_HDMI = 0 => ENABLE_HDMI = 1
	
注：demo中使用ffmpeg仅用于demux h264文件, 获取带00 00 00 01 NAL头的数据帧送到VDEC解码. 在Makefile中使能 USE_FFMPEG = 1